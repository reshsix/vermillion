/*
This file is part of vermillion.

Vermillion is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published
by the Free Software Foundation, version 3.

Vermillion is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <core/types.h>

#include <core/log.h>
#include <core/wheel.h>

#include <debug/profile.h>

extern void
profile_result(const char *file, int line, const char *func,
               u64 start, u64 end)
{
    u64 result = (end - start) * WHEEL_INNER_US;
    log (file);
    log (":");
    log (line);
    log (": ");
    log (func);
    log (": ");
    log ("Profile result : ");
    log ((s64)result);
    log (" us\r\n");
}
