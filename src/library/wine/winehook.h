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

#ifndef LIBTAS_WINEHOOK_H_INCLUDED
#define LIBTAS_WINEHOOK_H_INCLUDED

namespace libtas {

/* Replace the entry of a wine syscall in the syscall table of a wine unix
 * library. Contrary to patching the function itself, only the calls coming
 * from the Windows side are redirected, while wine internal calls keep using
 * the original function. */
void hook_wine_syscall(const char* library, const char* name, void** orig_function, void* my_function);

/* Returns if libtas must be initialized in this process. When launching a
 * wine game, only the game process is hooked, and not the other processes
 * that wine starts, nor the 64-bit process that restarts a 32-bit game. */
bool wine_should_init(int argc, char** argv);

/* Store the address range of wine unix ntdll library */
void register_wine_ntdll(const char* path);

/* Returns if the address belongs to wine unix ntdll library. Wine computes
 * timeouts from the real time, so its own time calls must not be faked. */
bool isWineNtdllAddress(const void* addr);

}

#endif
