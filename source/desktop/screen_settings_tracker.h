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

#ifndef DESKTOP__SCREEN_SETTINGS_TRACKER_H
#define DESKTOP__SCREEN_SETTINGS_TRACKER_H

#include "base/macros_magic.h"
#include "desktop/desktop_geometry.h"
#include "desktop/pixel_format.h"

namespace desktop {

class ScreenSettingsTracker
{
public:
    ScreenSettingsTracker() = default;

    bool isSizeChanged(const Size& screen_size);
    bool isFormatChanged(const PixelFormat& pixel_format);

    const Size& screenSize() const { return size_; }
    const PixelFormat& format() const { return pixel_format_; }

private:
    Size size_;
    PixelFormat pixel_format_;

    DISALLOW_COPY_AND_ASSIGN(ScreenSettingsTracker);
};

} // namespace desktop

#endif // DESKTOP__SCREEN_SETTINGS_TRACKER_H
