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
#include <signal.h>
#include <stdlib.h>
#include <_types.h>
#include <_utils.h>

static void (*handlers[SIGLAST + 1])(int) = {NULL};

static void
signal_default(int n)
{
    bool stop = true;

    print("\r\n");
    switch (n)
    {
        case SIGABRT:
            print("Aborted");
            break;
        case SIGFPE:
            print("Floating point exception");
            break;
        case SIGILL:
            print("Illegal instruction");
            break;
        case SIGINT:
            print("Interrupted");
            break;
        case SIGSEGV:
            print("Segmentation Fault");
            break;
        case SIGTERM:
            print("Terminated");
            break;
        case SIGTRAP:
            print("Press enter to continue...");
            while (fgetc(stdin) != '\r');
            stop = false;
            break;
    }

    if (stop)
        exit(EXIT_FAILURE);
}

static void
signal_ignore(int n)
{
    (void)n;
    return;
}

extern void *
signal(int n, void (*f)(int))
{
    void *ret = f;

    if (n <= SIGLAST)
    {
        if (f == SIG_IGN)
            f = signal_ignore;

        handlers[n] = f;
    }
    else
        ret = NULL;

    return ret;
}

extern int
raise(int n)
{
    int ret = 0;

    if (n <= SIGLAST)
    {
        if (handlers[n] != NULL)
            handlers[n](n);
        else
            signal_default(n);
    }
    else
        ret = SIG_ERR;

    return ret;
}
