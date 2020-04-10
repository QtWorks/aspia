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

#include "host/input_injector_win.h"

#include "base/logging.h"
#include "common/keycode_converter.h"
#include "host/sas_injector.h"

namespace host {

namespace {

const uint32_t kUsbCodeDelete = 0x07004c;
const uint32_t kUsbCodeLeftCtrl = 0x0700e0;
const uint32_t kUsbCodeRightCtrl = 0x0700e4;
const uint32_t kUsbCodeLeftAlt = 0x0700e2;
const uint32_t kUsbCodeRightAlt = 0x0700e6;

void sendKeyboardScancode(WORD scancode, DWORD flags)
{
    INPUT input;
    memset(&input, 0, sizeof(input));

    input.type       = INPUT_KEYBOARD;
    input.ki.dwFlags = flags;
    input.ki.wScan   = scancode;

    if (!(flags & KEYEVENTF_UNICODE))
    {
        input.ki.wScan &= 0xFF;

        if ((scancode & 0xFF00) != 0x0000)
            input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    // Do the keyboard event.
    if (!SendInput(1, &input, sizeof(input)))
    {
        PLOG(LS_WARNING) << "SendInput failed";
    }
}

void sendKeyboardVirtualKey(WORD key_code, DWORD flags)
{
    INPUT input;
    memset(&input, 0, sizeof(input));

    input.type       = INPUT_KEYBOARD;
    input.ki.wVk     = key_code;
    input.ki.dwFlags = flags;
    input.ki.wScan   = static_cast<WORD>(MapVirtualKeyW(key_code, MAPVK_VK_TO_VSC));

    // Do the keyboard event.
    if (!SendInput(1, &input, sizeof(input)))
    {
        PLOG(LS_WARNING) << "SendInput failed";
    }
}

} // namespace

void InputInjectorWin::setBlockInput(bool enable)
{
    switchToInputDesktop();
    block_input_ = enable;
    BlockInput(!!enable);
}

void InputInjectorWin::injectKeyEvent(const proto::KeyEvent& event)
{
    if (event.flags() & proto::KeyEvent::PRESSED)
    {
        pressed_keys_.insert(event.usb_keycode());

        if (event.usb_keycode() == kUsbCodeDelete && isCtrlAndAltPressed())
        {
            injectSAS();
            return;
        }
    }
    else
    {
        pressed_keys_.erase(event.usb_keycode());
    }

    int scancode = common::KeycodeConverter::usbKeycodeToNativeKeycode(event.usb_keycode());
    if (scancode == common::KeycodeConverter::invalidNativeKeycode())
        return;

    switchToInputDesktop();

    bool prev_state = GetKeyState(VK_CAPITAL) != 0;
    bool curr_state = (event.flags() & proto::KeyEvent::CAPSLOCK) != 0;

    if (prev_state != curr_state)
    {
        sendKeyboardVirtualKey(VK_CAPITAL, 0);
        sendKeyboardVirtualKey(VK_CAPITAL, KEYEVENTF_KEYUP);
    }

    prev_state = GetKeyState(VK_NUMLOCK) != 0;
    curr_state = (event.flags() & proto::KeyEvent::NUMLOCK) != 0;

    if (prev_state != curr_state)
    {
        sendKeyboardVirtualKey(VK_NUMLOCK, 0);
        sendKeyboardVirtualKey(VK_NUMLOCK, KEYEVENTF_KEYUP);
    }

    DWORD flags = KEYEVENTF_SCANCODE;

    if (!(event.flags() & proto::KeyEvent::PRESSED))
        flags |= KEYEVENTF_KEYUP;

    sendKeyboardScancode(static_cast<WORD>(scancode), flags);
}

void InputInjectorWin::injectPointerEvent(const proto::PointerEvent& event)
{
    switchToInputDesktop();

    desktop::Rect screen_rect =
        desktop::Rect::makeXYWH(GetSystemMetrics(SM_XVIRTUALSCREEN),
                                GetSystemMetrics(SM_YVIRTUALSCREEN),
                                GetSystemMetrics(SM_CXVIRTUALSCREEN),
                                GetSystemMetrics(SM_CYVIRTUALSCREEN));
    if (screen_rect.isEmpty())
        return;

    // Translate the coordinates of the cursor into the coordinates of the virtual screen.
    desktop::Point pos((event.x() * 65535) / (screen_rect.width() - 1),
                       (event.y() * 65535) / (screen_rect.height() - 1));

    DWORD flags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    DWORD wheel_movement = 0;

    if (pos != prev_mouse_pos_)
    {
        flags |= MOUSEEVENTF_MOVE;
        prev_mouse_pos_ = pos;
    }

    uint32_t mask = event.mask();

    // If the host is configured to swap left & right buttons.
    bool swap_buttons = !!GetSystemMetrics(SM_SWAPBUTTON);

    bool prev = (prev_mouse_button_mask_ & proto::PointerEvent::LEFT_BUTTON) != 0;
    bool curr = (mask & proto::PointerEvent::LEFT_BUTTON) != 0;
    if (curr != prev)
    {
        if (!swap_buttons)
            flags |= (curr ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP);
        else
            flags |= (curr ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP);
    }

    prev = (prev_mouse_button_mask_ & proto::PointerEvent::MIDDLE_BUTTON) != 0;
    curr = (mask & proto::PointerEvent::MIDDLE_BUTTON) != 0;
    if (curr != prev)
    {
        flags |= (curr ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP);
    }

    prev = (prev_mouse_button_mask_ & proto::PointerEvent::RIGHT_BUTTON) != 0;
    curr = (mask & proto::PointerEvent::RIGHT_BUTTON) != 0;
    if (curr != prev)
    {
        if (!swap_buttons)
            flags |= (curr ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP);
        else
            flags |= (curr ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP);
    }

    if (mask & proto::PointerEvent::WHEEL_UP)
    {
        flags |= MOUSEEVENTF_WHEEL;
        wheel_movement = static_cast<DWORD>(WHEEL_DELTA);
    }
    else if (mask & proto::PointerEvent::WHEEL_DOWN)
    {
        flags |= MOUSEEVENTF_WHEEL;
        wheel_movement = static_cast<DWORD>(-WHEEL_DELTA);
    }

    INPUT input;
    memset(&input, 0, sizeof(input));

    input.type = INPUT_MOUSE;
    input.mi.dx = pos.x();
    input.mi.dy = pos.y();
    input.mi.mouseData = wheel_movement;
    input.mi.dwFlags = flags;

    // Do the mouse event.
    if (!SendInput(1, &input, sizeof(input)))
    {
        PLOG(LS_WARNING) << "SendInput failed";
    }

    prev_mouse_button_mask_ = mask;
}

void InputInjectorWin::switchToInputDesktop()
{
    base::Desktop input_desktop(base::Desktop::inputDesktop());

    if (input_desktop.isValid() && !desktop_.isSame(input_desktop))
        desktop_.setThreadDesktop(std::move(input_desktop));

    BlockInput(!!block_input_);

    // We send a notification to the system that it is used to prevent
    // the screen saver, going into hibernation mode, etc.
    SetThreadExecutionState(ES_SYSTEM_REQUIRED);
}

bool InputInjectorWin::isCtrlAndAltPressed()
{
    bool ctrl_pressed = false;
    bool alt_pressed = false;

    for (const auto& key : pressed_keys_)
    {
        if (key == kUsbCodeLeftCtrl || key == kUsbCodeRightCtrl)
            ctrl_pressed = true;

        if (key == kUsbCodeLeftAlt || key == kUsbCodeRightAlt)
            alt_pressed = true;
    }

    return ctrl_pressed && alt_pressed;
}

} // namespace host
