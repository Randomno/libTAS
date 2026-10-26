/*
    Copyright 2015-2026 Clément Gallet <clement.gallet@ens-lyon.org>

    This file is part of libTAS.

    libTAS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libTAS.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Hooks of the unix side of wine `win32u` syscalls, which are exported by
 * `win32u.so` (wine >= 7). Windows-side functions such as GetAsyncKeyState()
 * from user32.dll end up calling these. Unix-side functions use the native
 * calling convention, even on i386. */

#include "win32u.h"

#include "hookpatch.h"
#include "logging.h"
#include "inputs/winekeyboardlayout.h"
#include "inputs/inputs.h"
#include "xlib/XlibGameWindow.h"
#include "../shared/inputs/SingleInput.h"

#include <X11/keysym.h>
#include <cstdint>

namespace libtas {
#if defined(__i386__) || defined(__x86_64__)

typedef struct tagPOINT {
    int32_t x;
    int32_t y;
} POINT;

namespace orig {

static short (*NtUserGetAsyncKeyState)(int key);
static short (*NtUserGetKeyState)(int vkey);
static int (*NtUserGetKeyboardState)(unsigned char *state);
static int (*NtUserGetCursorPos)(POINT *pt);

}

static bool isKeysymPressed(KeySym ks)
{
    if (ks == NoSymbol)
        return false;

    for (int i=0; i<AllInputs::MAXKEYS; i++) {
        if (Inputs::game_ai.keyboard[i] == 0) {
            break;
        }
        if (Inputs::game_ai.keyboard[i] == ks) {
            return true;
        }
    }
    return false;
}

static bool isVKeyPressed(int vkey)
{
    switch (vkey) {
        /* Mouse buttons. libTAS uses Xlib numbering, where button 2 is the
         * middle button and button 3 is the right button. */
        case 0x01: // VK_LBUTTON
            return Inputs::game_ai.pointer.mask & (1 << SingleInput::POINTER_B1);
        case 0x02: // VK_RBUTTON
            return Inputs::game_ai.pointer.mask & (1 << SingleInput::POINTER_B3);
        case 0x04: // VK_MBUTTON
            return Inputs::game_ai.pointer.mask & (1 << SingleInput::POINTER_B2);
        case 0x05: // VK_XBUTTON1
        case 0x06: // VK_XBUTTON2
            return false;

        /* Generic modifier keys match both left and right keys */
        case 0x10: // VK_SHIFT
            return isKeysymPressed(XK_Shift_L) || isKeysymPressed(XK_Shift_R);
        case 0x11: // VK_CONTROL
            return isKeysymPressed(XK_Control_L) || isKeysymPressed(XK_Control_R);
        case 0x12: // VK_MENU
            return isKeysymPressed(XK_Alt_L) || isKeysymPressed(XK_Alt_R);

        default:
            if ((vkey < 0) || (vkey > 0xff))
                return false;
            return isKeysymPressed(VKeyToXKeysym(vkey));
    }
}

short NtUserGetAsyncKeyState(int key)
{
    LOGTRACE(LCF_WINE | LCF_KEYBOARD, "%s called with key %d", __func__, key);
    return isVKeyPressed(key) ? static_cast<short>(0x8000) : 0;
}

short NtUserGetKeyState(int vkey)
{
    LOGTRACE(LCF_WINE | LCF_KEYBOARD, "%s called with key %d", __func__, vkey);
    /* Low-order bit is the toggle state, which we don't track */
    return isVKeyPressed(vkey) ? static_cast<short>(0x8000) : 0;
}

int NtUserGetKeyboardState(unsigned char *state)
{
    LOGTRACE_SIMPLE(LCF_WINE | LCF_KEYBOARD);
    if (!state)
        return 0;

    for (int vkey=0; vkey<256; vkey++) {
        state[vkey] = isVKeyPressed(vkey) ? 0x80 : 0;
    }
    return 1;
}

int NtUserGetCursorPos(POINT *pt)
{
    LOGTRACE_SIMPLE(LCF_WINE | LCF_MOUSE);
    if (!pt)
        return 0;

    /* libTAS pointer coordinates are relative to the game window, while wine
     * uses screen coordinates. Use the window position that we report to
     * wine, so that ScreenToClient() gives back our pointer coordinates. */
    int x, y;
    XlibGameWindow::getCoords(XlibGameWindow::get(), &x, &y);
    pt->x = Inputs::game_ai.pointer.x + x;
    pt->y = Inputs::game_ai.pointer.y + y;
    LOG(LL_DEBUG, LCF_WINE | LCF_MOUSE, "   returning (%d,%d)", pt->x, pt->y);
    return 1;
}

void hook_win32u()
{
    /* Patching twice would make the trampoline call our own function */
    static bool hooked = false;
    if (hooked)
        return;
    hooked = true;

    HOOK_PATCH_ORIG(NtUserGetAsyncKeyState, "win32u.so");
    HOOK_PATCH_ORIG(NtUserGetKeyState, "win32u.so");
    HOOK_PATCH_ORIG(NtUserGetKeyboardState, "win32u.so");
    HOOK_PATCH_ORIG(NtUserGetCursorPos, "win32u.so");
}

#else
void hook_win32u()
{
}
#endif
}
