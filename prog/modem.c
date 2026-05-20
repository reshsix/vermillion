/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <vermillion/lib.h>
#include <vermillion/vrm.h>
#include <vermillion/prog.h>

#include <vermillion/hal/uart.h>
#include <vermillion/hal/timer.h>
static struct vrm_uart_v1  *uart  = NULL;
static struct vrm_timer_v1 *timer = NULL;

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

static uint8_t buffer[1024] = {0};
static char name[256] = {0};
static char size[128] = {0};
static size_t bytes = 0;

static vrm_disk_f *current = NULL;
static const char *error   = NULL;

static bool
soh(struct vrm *v, uint8_t *block, bool *started, bool *finished)
{
    bool ret = true;

    uint8_t blk, nblk, crc0, crc1;
    ret = uart->read(0, &blk,  0) &&
          uart->read(0, &nblk, 0);
    for (size_t i = 0; ret && i < 128; i++)
        ret = uart->read(0, &(buffer[i]), 0);
    if (ret)
        ret = uart->read(0, &crc0, 0) &&
              uart->read(0, &crc1, 0);

    if (ret)
    {
        nblk = ~nblk;
        if ((blk == *block) && (nblk == *block))
        {
            size_t length = v->str.length(buffer);
            if (length)
            {
                if (*started)
                    ret = v->fs.write(current, buffer, 128);
                else
                {
                    v->str.copy(name, "/recv/", 0);
                    v->str.concat(name, buffer, 0);
                    v->str.copy(size, &(buffer[length]), 0);

                    bytes = 0;
                    for (size_t i = 0; i < v->str.length(size); i++)
                    {
                        if (size[i] < '0' || size[i] > '9')
                            continue;

                        bytes *= 10;
                        bytes += size[i] - '0';
                    }

                    if (v->fs.create(0, name, false))
                    {
                        if (current)
                            v->fs.close(current);

                        current = v->fs.open(0, name);
                        if (!current)
                        {
                            error = "Failed to open file\r\n";
                            ret = false;
                        }
                    }
                    else
                    {
                        error = "File creation failed\r\n";
                        ret = false;
                    }

                    *started = true;
                }
            }
            else
                *finished = true;

            (*block)++;
        }
        else
        {
            error = "Broken SOH block received\r\n";
            ret = false;
        }

        if (ret)
            ret = uart->write(0, ACK, 0);
        else
            uart->write(0, NAK, 0);
    }

    uart->write(0, 'C', 0);

    return ret;
}

static bool
stx(struct vrm *v, uint8_t *block, bool *started)
{
    bool ret = true;

    uint8_t blk, nblk, crc0, crc1;
    ret = uart->read(0, &blk,  0) &&
          uart->read(0, &nblk, 0);
    for (size_t i = 0; i < 1024; i++)
        ret = uart->read(0, &(buffer[i]), 0);
    if (ret)
        ret = uart->read(0, &crc0, 0) &&
              uart->read(0, &crc1, 0);

    if (ret)
    {
        nblk = ~nblk;
        if ((blk == *block) && (nblk == *block))
        {
            ret = v->fs.write(current, buffer, 1024);
            (*block)++;
        }
        else
        {
            error = "Broken STX block received\r\n";
            ret = false;
        }

        if (ret)
            ret = uart->write(0, ACK, 0);
        else
            uart->write(0, NAK, 0);
    }

    return ret;
}

static bool
eot(struct vrm *v, uint8_t *block, bool *started, bool *finished)
{
    bool ret = uart->write(0, NAK, 0);

    uint8_t id = 0;
    if (ret)
        ret = uart->read(0, &id, 0);
    if (ret && id == EOT)
        ret = uart->write(0, ACK, 0);
    if (ret)
    {
        uart->write(0, 'C', 0);

        *block = 0;
        ret = soh(v, block, started, finished);
    }

    return ret;
}

extern bool
vrm_prog(struct vrm *v, const char **args, int count)
{
    bool ret = true;

    uart  = v->driver(VRM_UART,  VRM_UART_V1);
    timer = v->driver(VRM_TIMER, VRM_TIMER_V1);

    v->fs.create(0, "/recv", true);
    bool handshake = false;
    bool started   = false;
    bool finished  = false;

    uint8_t block = 0;
    while (ret)
    {
        uint8_t id = 0;

        if (!handshake)
        {
            for (size_t i = 0; i < 30; i++)
            {
                uart->write(0, 'C', 0);
                ret = uart->read(0, &id, VRM_UART_NOWAIT);
                if (ret)
                {
                    handshake = true;
                    break;
                }
                timer->alarm(0, 1000000, false, NULL, NULL);
            }
        }
        else
            ret = uart->read(0, &id, 0);

        if (ret)
        {
            switch (id)
            {
                case SOH:
                    ret = soh(v, &block, &started, &finished);
                    break;
                case STX:
                    ret = stx(v, &block, &started);
                    break;
                case EOT:
                    ret = eot(v, &block, &started, &finished);
                    break;
                case CAN:
                    ret = false;
                    break;
                default:
                    break;
            }

            if (!ret)
            {
                for (size_t i = 0; i < 10; i++)
                    uart->write(0, CAN, 0);
                if (error)
                {
                    v->syslog.string("\r\n");
                    v->syslog.string(error);
                }
            }
        }

        if (finished)
            break;
    }

    if (current)
        v->fs.close(current);

    return ret;
}
