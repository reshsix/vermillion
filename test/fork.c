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

#include <core/fork.h>

#include <debug/exit.h>
#include <debug/assert.h>

static void *result = 0;

static void
test_fork_f2(void *arg)
{
    result = arg;
}

static void
test_fork_f1(void *arg)
{
    fork *fk = fork_new(test_fork_f2, arg);
    if (fk)
        fork_run(fk);
    fork_del(fk);
}

static void
test_fork_f0(void *arg)
{
    fork *fk = fork_new(test_fork_f1, arg);
    if (fk)
        fork_run(fk);
    fork_del(fk);
}

extern void
main(void)
{
    fork *fk = fork_new(test_fork_f0, (void*)0x12345);
    fork *pfk = fk;
    assert (fk != NULL);

    fk = fork_del(fk);
    assert (fk == NULL);

    fk = fork_new(test_fork_f0, (void*)0x12345);
    assert (fk != NULL);
    assert (fk == pfk);

    fork_run(fk);
    assert (result == (void*)0x12345);
    fork_del(fk);

    exit_qemu(assert_failed);
}
