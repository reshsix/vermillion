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

#include <thread/thread.h>

#include <system/log.h>
#include <system/wheel.h>
#include <system/display.h>

#include <debug/test.h>

#include <interface/console.h>

thread_decl (extern, main)
{
    test_all();

    log("Hello World!\r\n");

    const char msg[] = "Vermillion 1.0a: Hello World\r\n\r\n";
    const char msg2[] = "Input not implemented";
    for (u8 i = 0; i < sizeof(msg); i++)
        console_input(msg[i]);
    for (u8 i = 0; i < sizeof(msg2); i++)
        console_input(msg2[i]);

    for (;;)
        wheel_sleep(WHEEL_OUTER, 255);

    thread_finish();
}
