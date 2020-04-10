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

#ifndef HOST__SYSTEM_SETTINGS_H
#define HOST__SYSTEM_SETTINGS_H

#include "base/macros_magic.h"
#include "base/xml_settings.h"

#include <filesystem>

namespace net {
class ServerUserList;
} // namespace net

namespace host {

class SystemSettings
{
public:
    SystemSettings();
    ~SystemSettings();

    const std::filesystem::path& filePath() const;
    bool isWritable() const;
    void sync();

    uint16_t tcpPort() const;
    void setTcpPort(uint16_t port);

    net::ServerUserList userList() const;
    void setUserList(const net::ServerUserList& user_list);

    std::string updateServer() const;
    void setUpdateServer(const std::string& server);

private:
    base::XmlSettings settings_;

    DISALLOW_COPY_AND_ASSIGN(SystemSettings);
};

} // namespace host

#endif // HOST__SYSTEM_SETTINGS_H
