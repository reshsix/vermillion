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
#include <general/mem.h>

#include <hal/base/dev.h>
#include <hal/base/drv.h>
#include <hal/generic/block.h>
#include <hal/generic/stream.h>
#include <hal/classes/pic.h>
#include <hal/classes/uart.h>
#include <hal/classes/timer.h>
#include <hal/classes/video.h>

#include <system/log.h>
#include <system/wheel.h>

drv_incl (uart, i686_com);
dev_decl (uart, i686_com, tty0);
dev_decl (uart, i686_com, tty1);
dev_decl (uart, i686_com, tty2);
dev_decl (uart, i686_com, tty3);

drv_incl (pic, i686_pic);
dev_decl (pic, i686_pic, pic);

drv_incl (timer, i686_timer);
dev_decl (timer, i686_timer, timer0);

drv_incl (video, i686_fb);
dev_decl (video, i686_fb, video0);

drv_incl (block, memory);
dev_decl (block, memory, ram);

extern u32 *multiboot_addr;
static void *multiboot_info = NULL;

extern void
devtree_init(void)
{
    if (multiboot_addr)
        multiboot_info = mem_new(multiboot_addr[0]);
    if (multiboot_info)
        mem_copy(multiboot_info, multiboot_addr, multiboot_addr[0]);

    dev_init (ram, 0x0, 0x200, CONFIG_RAM_SIZE / 0x200);

    dev_init (tty0, 0x3F8);
    uart_config(&dev(tty0), 115200, UART_8B, UART_NOPARITY, UART_1S);
    log_output((dev_stream *)&dev(tty0));

    dev_init (tty1, 0x2F8);
    dev_init (tty2, 0x3E8);
    dev_init (tty3, 0x2E8);

    dev_init (pic);
    dev_init (timer0, &dev(pic), 0);

    dev_init (video0, multiboot_info);

    pic_state(&dev(pic), true);
    wheel_timer(&dev(timer0));
}

extern void
devtree_clean(void)
{
    dev_clean (tty1);
    dev_clean (tty3);
    dev_clean (tty2);

    dev_clean (tty0);
    log_output(NULL);
}
