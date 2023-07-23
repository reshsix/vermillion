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

#include <stdio.h>
#include <_utils.h>
#include <stdlib.h>
#include <string.h>
#include <vermillion/drivers.h>

struct devnode _devtree_head = {0};
struct devnode *_devtree_tail = &_devtree_head;

extern struct driver *
driver_find(const char *name)
{
    struct driver *ret = NULL;

    for (u32 i = 0; i < _drivers_c; i++)
    {
        if (!(strcmp(_drivers[i]->name, name)))
        {
            ret = _drivers[i];
            break;
        }
    }

    return ret;
}

extern struct device *
device_find(const char *name)
{
    struct device *ret = NULL;

    for (struct devnode *dev = &_devtree_head; dev && dev != _devtree_tail;
         dev = dev->next)
    {
        if (!(strcmp(dev->cur.name, name)))
        {
            ret = &(dev->cur);
            break;
        }
    }

    return ret;
}

struct device *_devtree_logger = NULL;
extern void
_devtree_log(const char *s)
{
    if (_devtree_logger)
    {
        for (; s[0] != '\0'; s = &(s[1]))
        {
            if (s[0] != '\0')
                _devtree_logger->driver->interface.stream.write
                    (_devtree_logger->context, s[0]);
        }
    }
}
