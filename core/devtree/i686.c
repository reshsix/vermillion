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

#include <core/dev.h>
#include <core/drv.h>
#include <core/log.h>
#include <core/mem.h>
#include <core/wheel.h>

#include <core/pic.h>
#include <core/timer.h>
#include <core/video.h>
#include <core/stream.h>
#include <core/serial.h>

drv_incl (serial, i686_com)
dev_decl (serial, i686_com, tty0)
dev_decl (serial, i686_com, tty1)
dev_decl (serial, i686_com, tty2)
dev_decl (serial, i686_com, tty3)

drv_incl (pic, i686_pic)
dev_decl (pic, i686_pic, pic)

drv_incl (timer, i686_timer)
dev_decl (timer, i686_timer, timer0)

drv_incl (video, i686_fb)
dev_decl (video, i686_fb, video0)

extern u32 *multiboot_addr;
static void *multiboot_info = NULL;

extern void
_devtree_init(void)
{
    if (multiboot_addr)
        multiboot_info = mem_new(multiboot_addr[0]);
    if (multiboot_info)
        mem_copy(multiboot_info, multiboot_addr, multiboot_addr[0]);

    dev_init (tty0, 0x3F8)
    dev_config (tty0, .serial.baud   = 115200,
                      .serial.bits   = DRIVER_SERIAL_CHAR_8B,
                      .serial.parity = DRIVER_SERIAL_PARITY_NONE,
                      .serial.stop   = DRIVER_SERIAL_STOP_1B);
    log_set_dev((dev_stream *)&dev(tty0));

    dev_init (tty1, 0x2F8)
    dev_init (tty2, 0x3E8)
    dev_init (tty3, 0x2E8)

    dev_init (pic)
    dev_init (timer0, &dev(pic), 0);

    dev_init (video0, multiboot_info);

    pic_state(&dev(pic), true);
    wheel_timer(&dev(timer0));
}

extern void
_devtree_clean(void)
{
    dev_clean (tty1)
    dev_clean (tty3)
    dev_clean (tty2)

    dev_clean (tty0)
    log_set_dev(NULL);
}
