//
// Aspia Project
// Copyright (C) 2019 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "console/computer_dialog.h"
#include "client/config_factory.h"
#include "console/computer_dialog_desktop.h"
#include "console/computer_dialog_general.h"
#include "console/computer_dialog_parent.h"
#include "console/computer_factory.h"
#include "crypto/secure_memory.h"

#include <QAbstractButton>
#include <QDateTime>

namespace console {

namespace {

enum ItemType
{
    ITEM_TYPE_PARENT,
    ITEM_TYPE_GENERAL,
    ITEM_TYPE_DESKTOP_MANAGE,
    ITEM_TYPE_DESKTOP_VIEW
};

} // namespace

ComputerDialog::ComputerDialog(QWidget* parent,
                               const QString& parent_name,
                               const proto::address_book::Computer& computer)
    : QDialog(parent),
      mode_(Mode::MODIFY_COMPUTER),
      computer_(computer)
{
    client::ConfigFactory::fixupDesktopConfig(
        computer_.mutable_session_config()->mutable_desktop_manage());
    client::ConfigFactory::fixupDesktopConfig(
        computer_.mutable_session_config()->mutable_desktop_view());

    init(parent_name);
}

ComputerDialog::ComputerDialog(QWidget* parent, const QString& parent_name)
    : QDialog(parent),
      mode_(Mode::CREATE_COMPUTER),
      computer_(ComputerFactory::defaultComputer())
{
    init(parent_name);
}

ComputerDialog::~ComputerDialog()
{
    crypto::memZero(computer_.mutable_name());
    crypto::memZero(computer_.mutable_address());
    crypto::memZero(computer_.mutable_username());
    crypto::memZero(computer_.mutable_password());
    crypto::memZero(computer_.mutable_comment());
}

void ComputerDialog::closeEvent(QCloseEvent* event)
{
    settings_.setComputerDialogGeometry(saveGeometry());
    settings_.setComputerDialogState(ui.splitter->saveState());
    QDialog::closeEvent(event);
}

bool ComputerDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui.widget && event->type() == QEvent::Resize)
    {
        for (auto tab : tabs_)
            tab->resize(ui.widget->size());
    }

    return QDialog::eventFilter(watched, event);
}

void ComputerDialog::onTabChanged(QTreeWidgetItem* current)
{
    if (current)
        showTab(current->type());
}

void ComputerDialog::buttonBoxClicked(QAbstractButton* button)
{
    if (ui.button_box->standardButton(button) == QDialogButtonBox::Ok)
    {
        for (auto tab : tabs_)
        {
            int type = static_cast<ComputerDialogTab*>(tab)->type();

            if (type == ITEM_TYPE_GENERAL)
            {
                ComputerDialogGeneral* general_tab =
                    static_cast<ComputerDialogGeneral*>(tab);

                if (!general_tab->saveSettings(&computer_))
                    return;
            }
            else if (type == ITEM_TYPE_DESKTOP_MANAGE)
            {
                ComputerDialogDesktop* desktop_tab =
                    static_cast<ComputerDialogDesktop*>(tab);

                desktop_tab->saveSettings(
                    computer_.mutable_session_config()->mutable_desktop_manage());
            }
            else if (type == ITEM_TYPE_DESKTOP_VIEW)
            {
                ComputerDialogDesktop* desktop_tab =
                    static_cast<ComputerDialogDesktop*>(tab);

                desktop_tab->saveSettings(
                    computer_.mutable_session_config()->mutable_desktop_view());
            }
        }

        int64_t current_time = QDateTime::currentSecsSinceEpoch();

        if (mode_ == Mode::CREATE_COMPUTER)
            computer_.set_create_time(current_time);

        computer_.set_modify_time(current_time);

        accept();
    }
    else
    {
        reject();
    }

    close();
}

void ComputerDialog::init(const QString& parent_name)
{
    ui.setupUi(this);

    restoreGeometry(settings_.computerDialogGeometry());
    ui.splitter->restoreState(settings_.computerDialogState());

    connect(ui.tree, &QTreeWidget::currentItemChanged, this, &ComputerDialog::onTabChanged);
    connect(ui.button_box, &QDialogButtonBox::clicked, this, &ComputerDialog::buttonBoxClicked);

    QTreeWidgetItem* general_item = new QTreeWidgetItem(ITEM_TYPE_GENERAL);
    general_item->setIcon(0, QIcon(QStringLiteral(":/img/computer.png")));
    general_item->setText(0, tr("General"));

    QTreeWidgetItem* sessions_item = new QTreeWidgetItem(ITEM_TYPE_PARENT);
    sessions_item->setIcon(0, QIcon(QStringLiteral(":/img/settings.png")));
    sessions_item->setText(0, tr("Sessions"));

    ui.tree->addTopLevelItem(general_item);
    ui.tree->addTopLevelItem(sessions_item);

    QTreeWidgetItem* desktop_manage_item = new QTreeWidgetItem(ITEM_TYPE_DESKTOP_MANAGE);
    desktop_manage_item->setIcon(0, QIcon(QStringLiteral(":/img/monitor-keyboard.png")));
    desktop_manage_item->setText(0, tr("Manage"));

    QTreeWidgetItem* desktop_view_item = new QTreeWidgetItem(ITEM_TYPE_DESKTOP_VIEW);
    desktop_view_item->setIcon(0, QIcon(QStringLiteral(":/img/monitor.png")));
    desktop_view_item->setText(0, tr("View"));

    sessions_item->addChild(desktop_manage_item);
    sessions_item->addChild(desktop_view_item);

    ComputerDialogParent* parent_tab =
        new ComputerDialogParent(ITEM_TYPE_PARENT, ui.widget);
    ComputerDialogGeneral* general_tab =
        new ComputerDialogGeneral(ITEM_TYPE_GENERAL, ui.widget);
    ComputerDialogDesktop* desktop_manage_tab =
        new ComputerDialogDesktop(ITEM_TYPE_DESKTOP_MANAGE, ui.widget);
    ComputerDialogDesktop* desktop_view_tab =
        new ComputerDialogDesktop(ITEM_TYPE_DESKTOP_VIEW, ui.widget);

    general_tab->restoreSettings(parent_name, computer_);
    desktop_manage_tab->restoreSettings(
        proto::SESSION_TYPE_DESKTOP_MANAGE, computer_.session_config().desktop_manage());
    desktop_view_tab->restoreSettings(
        proto::SESSION_TYPE_DESKTOP_VIEW, computer_.session_config().desktop_view());

    tabs_.append(general_tab);
    tabs_.append(desktop_manage_tab);
    tabs_.append(desktop_view_tab);
    tabs_.append(parent_tab);

    QSize min_size;

    for (auto tab : tabs_)
    {
        min_size.setWidth(std::max(tab->sizeHint().width(), min_size.width()));
        min_size.setHeight(std::max(tab->minimumSizeHint().height(), min_size.height()));
    }

    ui.widget->setMinimumSize(min_size);
    ui.widget->installEventFilter(this);

    ui.tree->setCurrentItem(general_item);
    ui.tree->expandAll();
}

void ComputerDialog::showTab(int type)
{
    for (auto tab : tabs_)
    {
        if (static_cast<ComputerDialogTab*>(tab)->type() == type)
            tab->show();
        else
            tab->hide();
    }
}

} // namespace console
