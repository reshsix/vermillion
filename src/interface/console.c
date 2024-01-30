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

#include <hal/classes/fs.h>

#include <system/log.h>
#include <system/wheel.h>
#include <system/display.h>

static u8 bar[] = {
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0xe0, 0xff, 0x00, 0x00, 0xe0, 0xff, 0x00, 0x00, 0xe0, 0xff,
  0x00, 0x00, 0xe0, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0xe0, 0xff,
  0x00, 0x00, 0xe0, 0xff, 0x00, 0x00, 0xe0, 0xff, 0x00, 0x00, 0xe0, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff
};

static bool loading = false, switching = false;

static void
show_bars(void *arg)
{
    u8 count = (u32)arg;
    if (loading)
    {
        u16 wp = 2570, hp = 2570;

        for (u8 i = 0; i < 26; i++)
        {
            if (!switching)
            {
                display_show(((count - i) % 26) == 0, wp * i, 0,
                             wp, hp, DISPLAY_START);
                display_show(((count + i) % 26) == 0, wp * i, 65535 - hp,
                             wp, hp, DISPLAY_START);
            }
        }

        wheel_schedule(WHEEL_OUTER, show_bars, (void*)(count - 1), 2);
    }
}

struct console
{
    bool enabled;
    void *bg, *font;

    u16 rows, cols;
    u16 wp, hp, rwp, rhp;

    u8 row, col;
};
struct console con = {0};

bool console_input(u8);

static bool cursor = false;
static bool inputting = false, blinking = false;
static void
blink_cursor(void *arg)
{
    (void)arg;
    if (!inputting)
    {
        blinking = true;

        if (cursor)
            console_input('\b');
        else
            console_input('_');
        cursor = !cursor;

        blinking = false;
    }
    wheel_schedule(WHEEL_OUTER, blink_cursor, NULL, 50);
}

extern bool
console_init(dev_fs *fs, u16 width, u16 height)
{
    bool ret = !(con.enabled);

    if (ret)
    {
        loading = true;
        ret = display_config(NULL, 0, 0, 0, bar, 2, 2, 8, 8) &&
              wheel_schedule(WHEEL_OUTER, show_bars, NULL, 2);
    }

    void *splash = NULL;
    if (ret)
    {
        #define SPLASH_W 256
        #define SPLASH_H 256

        u32 splash_s = SPLASH_W * SPLASH_H * 4;
        splash = mem_new(splash_s);

        struct fs_file *f = fs_open(fs, "console/splash.bgra");
        ret = (splash && f && fs_read(f, splash, splash_s));

        if (ret)
        {
            switching = true;
            ret = display_config(NULL, 0, 0, 0, splash, 1, 1,
                                 SPLASH_W, SPLASH_H) &&
                  display_show(0, 32768 - 5000, 32768 - 5000,
                               10000, 10000, DISPLAY_MIDDLE) &&
                  display_config(NULL, 0, 0, 0, bar, 2, 2, 8, 8);
            switching = false;
        }
        fs_close(f);
    }

    #define BG_W 1920
    #define BG_H 1080
    if (ret)
    {
        u32 bg_s = BG_W * BG_H * 4;
        con.bg = mem_new(bg_s);

        struct fs_file *f = fs_open(fs, "console/bg.bgra");
        ret = (con.bg && f && fs_read(f, con.bg, bg_s));
        fs_close(f);
    }

    #define FONT_W 128
    #define FONT_H 256
    if (ret)
    {
        u32 font_s = FONT_W * FONT_H * 4;
        con.font = mem_new(font_s);

        struct fs_file *f = fs_open(fs, "console/font.bgra");
        ret = (con.font && f && fs_read(f, con.font, font_s));
        fs_close(f);
    }

    if (ret)
    {
        #define BG_OP ((255 * 25) / 100)
        #define FONT_HC 16
        #define FONT_VC 16

        loading = false;
        ret = display_config(con.bg, BG_W, BG_H, BG_OP,
                             con.font, FONT_HC, FONT_VC, FONT_W, FONT_H) &&
              display_clear(0, 0, 65535, 65535);
    }

    if (ret)
    {
        con.enabled = true;

        con.rows = width / 8;
        con.cols = height / 16;

        con.wp = 65535 / con.rows;
        con.hp = 65535 / con.cols;
        if (width > height)
            con.wp = con.wp * width / height;
        else if (height > width)
            con.hp = con.hp * height / width;

        con.rwp = con.wp;
        con.rhp = con.hp;
        if (width > height)
            con.rwp = con.rwp * height / width;
        else if (height > width)
            con.rhp = con.rhp * width / height;

        wheel_schedule(WHEEL_OUTER, blink_cursor, NULL, 50);
    }
    else
    {
        con.bg = mem_del(con.bg);
        con.font = mem_del(con.font);
    }
    mem_del(splash);

    return ret;
}

extern bool
console_clean(void)
{
    con.enabled = false;
    mem_del(con.bg);
    mem_del(con.font);

    return true;
}

extern bool
console_input(u8 c)
{
    bool ret = (con.enabled);

    inputting = true;
    if (ret)
    {
        for (u8 i = 0; i < ((c == '\b') + (cursor && !blinking)); i++)
        {
            if (con.row != 0)
                con.row--;
            else if (con.col != 0)
            {
                con.row = con.rows - 1;
                con.col--;
            }

            u16 xp = con.rwp * con.row;
            u16 yp = con.rhp * con.col;
            ret = display_clear(xp, yp, con.wp, con.hp);
        }
    }

    if (ret)
    {
        switch (c)
        {
            case '\b':
                break;
            case '\t':
                con.row = ((con.row / 4) + 1) * 4;
                if (con.row >= con.rows)
                {
                    con.row = 0;
                    con.col++;
                }
                break;
            case '\n':
                con.row = 0;
                con.col++;
                break;
            case '\r':
                con.row = 0;
                break;
            default:
                if (c >= 32 && c != 255)
                {
                    u16 xp = con.rwp * con.row++;
                    u16 yp = con.rhp * con.col;
                    ret = display_show(c, xp, yp, con.wp, con.hp,
                                       DISPLAY_START);
                }
                break;
        }
    }

    if (ret)
    {
        if (con.row >= con.rows)
        {
            con.row %= con.rows;
            con.col++;
        }

        if (con.col >= con.cols)
        {
            u8 cols = (con.col % con.cols) + 1;
            ret = display_vscroll(con.hp * cols, true);
            con.col = con.cols - cols;
        }
    }
    inputting = false;

    return ret;
}
