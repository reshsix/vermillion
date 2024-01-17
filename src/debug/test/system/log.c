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
#include <general/str.h>

#include <hal/generic/stream.h>

#include <system/log.h>

#include <debug/assert.h>

static void *test_ctx = (void *)0xABCD;

static char buffer[128] = {0};
static u32 buffer_s = 0;

static void
test_log_clear(void)
{
    mem_init(buffer, 0, sizeof(buffer));
    buffer_s = 0;
}

static bool
test_log_write(void *ctx, u32 idx, void *data)
{
    bool ret = (ctx == test_ctx && idx == STREAM_COMMON);

    if (ret)
        buffer[buffer_s++] = *((u8*)(data));

    return ret;
}

extern void
test_system_log(void)
{
    drv_stream logdrv = { .write = test_log_write };
    dev_stream logdev = { .context = test_ctx, .driver = &logdrv };

    dev_stream *logger = log_output(&logdev);
    assert (logger != &logdev);

    log((char)'a');
    assert (buffer[0] == 'a');
    test_log_clear();

    log("bcd");
    assert (str_comp(buffer, "bcd", 0) == 0);
    test_log_clear();

    log((bool)true);
    assert (str_comp(buffer, "true", 0) == 0);
    test_log_clear();

    log((u64)0x2345678910);
    assert (str_comp(buffer, "0x2345678910", 0) == 0);
    test_log_clear();

    log((s64)-12345678901);
    assert (str_comp(buffer, "-12345678901", 0) == 0);

    logger = log_output(logger);
    assert (logger == &logdev);
}
