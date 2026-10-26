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

#ifndef LIBTAS_WINENTDLL_H_INCLUDED
#define LIBTAS_WINENTDLL_H_INCLUDED

namespace libtas {

void hook_ntdll(const char* path);

/* Replace the wine shared user data page, which contains time values that
 * Windows functions read directly, such as GetTickCount(). Does nothing if
 * the page is not mapped yet. */
void takeover_wine_shared_data();

/* Write the current deterministic time into our shared user data page */
void update_wine_shared_data();

}

#endif
