//
// Aspia Project
// Copyright (C) 2020 Dmitry Chapyshev <dmitry@aspia.ru>
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

#ifndef HOST__UI__SETTINGS_UTIL_H
#define HOST__UI__SETTINGS_UTIL_H

#include "base/macros_magic.h"

#include <filesystem>

#include <QCoreApplication>

namespace host {

class SettingsUtil
{
    Q_DECLARE_TR_FUNCTIONS(SettingsUtil)

public:
    static bool importFromFile(
        const std::filesystem::path& path, bool silent, QWidget* parent = nullptr);
    static bool exportToFile(
        const std::filesystem::path& path, bool silent, QWidget* parent = nullptr);

private:
    static bool copySettings(const std::filesystem::path& source_path,
                             const std::filesystem::path& target_path,
                             bool silent,
                             QWidget* parent);

    DISALLOW_COPY_AND_ASSIGN(SettingsUtil);
};

} // namespace host

#endif // HOST__UI__SETTINGS_UTIL_H
