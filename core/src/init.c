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

#include <core/mem.h>
#include <core/thread.h>
#include <core/generator.h>

extern void _devtree_init(void);
extern void _devtree_clean(void);

extern void main(void);
static thread_task(_main)
{
    main();
    thread_finish();
}

extern void
__init(void)
{
    _mem_init();
    _devtree_init();

    thread_new(_main, NULL, false, 255);
    while (_threads.cur)
    {
        thread *cur = _threads.cur;

        if (!((cur->counter * cur->priority) % 255))
        {
            if (generator_next(cur->gen))
                _threads.cur = (cur->next) ? cur->next : _threads.head;
            else
            {
                if (cur->persistent)
                    _threads.cur = (cur->next) ? cur->next : _threads.head;
                else
                    cur = thread_del(cur);
            }
        }

        if (cur)
            cur->counter++;
    }

    _devtree_clean();
    _mem_clean();

    while (true);
}
