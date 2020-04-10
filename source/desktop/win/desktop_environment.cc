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

#include "desktop/win/desktop_environment.h"

#include "base/win/scoped_impersonator.h"
#include "base/win/scoped_object.h"

#include <Windows.h>
#include <wtsapi32.h>

namespace desktop {

DesktopEnvironment::DesktopEnvironment()
{
    applyNewSettings();
}

DesktopEnvironment::~DesktopEnvironment()
{
    revertAll();
}

void DesktopEnvironment::setWallpaper(bool enable)
{
    if (wallpaper_ == enable)
        return;

    wallpaper_ = enable;

    revertAll();
    applyNewSettings();
}

void DesktopEnvironment::setFontSmoothing(bool enable)
{
    if (font_smoothing_ == enable)
        return;

    font_smoothing_ = enable;

    revertAll();
    applyNewSettings();
}

void DesktopEnvironment::setEffects(bool enable)
{
    if (effects_ == enable)
        return;

    effects_ = enable;

    revertAll();
    applyNewSettings();
}

void DesktopEnvironment::disableWallpaper()
{
    SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, L"", SPIF_SENDCHANGE);
}

void DesktopEnvironment::disableFontSmoothing()
{
    SystemParametersInfoW(SPI_SETFONTSMOOTHING, FALSE, 0, SPIF_SENDCHANGE);
}

void DesktopEnvironment::disableEffects()
{
    BOOL drop_shadow = TRUE;
    if (SystemParametersInfoW(SPI_GETDROPSHADOW, 0, &drop_shadow, 0))
    {
        if (drop_shadow)
        {
            SystemParametersInfoW(SPI_SETDROPSHADOW, 0, FALSE, SPIF_SENDCHANGE);
            drop_shadow_changed_ = true;
        }
    }

    ANIMATIONINFO animation;
    animation.cbSize = sizeof(animation);
    if (SystemParametersInfoW(SPI_GETANIMATION, sizeof(animation), &animation, 0))
    {
        if (animation.iMinAnimate)
        {
            animation.iMinAnimate = FALSE;
            SystemParametersInfoW(SPI_SETANIMATION, sizeof(animation), &animation, SPIF_SENDCHANGE);
            animation_changed_ = true;
        }
    }

    SystemParametersInfoW(SPI_SETDRAGFULLWINDOWS, FALSE, 0, SPIF_SENDCHANGE);
    SystemParametersInfoW(SPI_SETUIEFFECTS, 0, FALSE, SPIF_SENDCHANGE);
    SystemParametersInfoW(SPI_SETCLIENTAREAANIMATION, 0, FALSE, SPIF_SENDCHANGE);
}

void DesktopEnvironment::revertAll()
{
    if (drop_shadow_changed_)
    {
        SystemParametersInfoW(SPI_SETDROPSHADOW, 0, reinterpret_cast<PVOID>(TRUE), SPIF_SENDCHANGE);
        drop_shadow_changed_ = false;
    }

    if (animation_changed_)
    {
        ANIMATIONINFO animation;
        animation.cbSize = sizeof(animation);
        animation.iMinAnimate = TRUE;

        SystemParametersInfoW(SPI_SETANIMATION, sizeof(animation), &animation, SPIF_SENDCHANGE);

        animation_changed_ = false;
    }

    base::win::ScopedHandle user_token;

    if (!WTSQueryUserToken(WTSGetActiveConsoleSessionId(), user_token.recieve()))
        return;

    base::win::ScopedImpersonator impersonator;

    // The process of the desktop session is running with "SYSTEM" account.
    // We need the current real user, not "SYSTEM".
    if (!impersonator.loggedOnUser(user_token))
        return;

    HMODULE module = GetModuleHandleW(L"user32.dll");
    if (module)
    {
        // The function prototype is relevant for versions starting from Windows Vista.
        // Older versions have a different prototype.
        typedef BOOL(WINAPI* UpdatePerUserSystemParametersFunc)(DWORD flags);

        UpdatePerUserSystemParametersFunc update_per_user_system_parameters =
            reinterpret_cast<UpdatePerUserSystemParametersFunc>(
                GetProcAddress(module, "UpdatePerUserSystemParameters"));
        if (update_per_user_system_parameters)
        {
            // WARNING! Undocumented function!
            // Any ideas how to update user settings without using it?
            update_per_user_system_parameters(0x06);
        }
    }
}

void DesktopEnvironment::applyNewSettings()
{
    if (!wallpaper_)
        disableWallpaper();

    if (!font_smoothing_)
        disableFontSmoothing();

    if (!effects_)
        disableEffects();
}

} // namespace desktop
