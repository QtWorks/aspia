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

#include "router/win/service_constants.h"

namespace router {

const char16_t kRouterServiceFileName[] = u"aspia_router.exe";

const char16_t kRouterServiceName[] = u"aspia-router";

const char16_t kRouterServiceDisplayName[] = u"Aspia Router Service";

const char16_t kRouterServiceDescription[] =
    u"Assigns identifiers to users and routes traffic to bypass NAT.";

} // namespace router
