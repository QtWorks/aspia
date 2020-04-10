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

#include "net/ip_util.h"

#include "base/strings/unicode.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <ws2tcpip.h>
#else
#error Platform support not implemented
#endif // defined(OS_*)

namespace net {

bool isValidIpV4Address(std::u16string_view address)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, base::local8BitFromUtf16(address).c_str(), &(sa.sin_addr)) != 0;
}

bool isValidIpV6Address(std::u16string_view address)
{
    struct sockaddr_in6 sa;
    return inet_pton(AF_INET6, base::local8BitFromUtf16(address).c_str(), &(sa.sin6_addr)) != 0;
}

} // namespace net
