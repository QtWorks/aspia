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

#include "router/server.h"

#include "base/logging.h"
#include "base/task_runner.h"
#include "net/channel.h"
#include "proto/router.pb.h"
#include "router/database_sqlite.h"
#include "router/session_manager.h"
#include "router/session_peer.h"
#include "router/settings.h"

namespace router {

Server::Server(std::shared_ptr<base::TaskRunner> task_runner)
    : task_runner_(std::move(task_runner))
{
    DCHECK(task_runner_);
}

Server::~Server() = default;

bool Server::start()
{
    if (server_)
        return false;

    database_ = DatabaseSqlite::open();
    if (!database_)
        return false;

    Settings settings;

    authenticator_manager_ = std::make_unique<net::ServerAuthenticatorManager>(task_runner_, this);
    authenticator_manager_->setPrivateKey(settings.privateKey());
    authenticator_manager_->setUserList(
        std::make_shared<net::ServerUserList>(database_->userList()));

    server_ = std::make_unique<net::Server>();
    server_->start(settings.port(), this);

    return true;
}

void Server::onNewConnection(std::unique_ptr<net::Channel> channel)
{
    if (authenticator_manager_)
        authenticator_manager_->addNewChannel(std::move(channel));
}

void Server::onNewSession(net::ServerAuthenticatorManager::SessionInfo&& session_info)
{
    std::unique_ptr<Session> session;

    switch (session_info.session_type)
    {
        case proto::ROUTER_SESSION_PEER:
            session = std::make_unique<SessionPeer>(std::move(session_info.channel));
            break;

        case proto::ROUTER_SESSION_MANAGER:
        {
            if (!(session_info.user_flags & net::ServerUser::MANAGER))
                return;

            session = std::make_unique<SessionManager>(std::move(session_info.channel));
        }
        break;

        default:
            break;
    }

    if (!session)
        return;

    sessions_.emplace_back(std::move(session));
    sessions_.back()->start(this);
}

void Server::onSessionFinished()
{
    for (auto it = sessions_.begin(); it != sessions_.end();)
    {
        if (it->get()->isFinished())
        {
            // Session will be destroyed after completion of the current call.
            task_runner_->deleteSoon(std::move(*it));

            // Delete a session from the list.
            it = sessions_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace router
