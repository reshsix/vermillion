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

#include <general/types.h>

#include <debug/assert.h>

#include <system/log.h>

bool assert_failed = false;

extern void
assert_fail(const char *file, int line, const char *func, const char *text)
{
    assert_failed = true;
    log (file);
    log (":");
    log (line);
    log (": ");
    log (func);
    log (": ");
    log ("Assertion '");
    log (text);
    log ("' failed\n");
}
