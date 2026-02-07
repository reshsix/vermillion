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

#include <hal/stream.h>

#include <system/comm.h>

static dev_stream *input0  = NULL;
static dev_stream *input1  = NULL;
static dev_stream *output0 = NULL;
static dev_stream *output1 = NULL;

/* For devtree usage */

extern void
comm_config(dev_stream *in0, dev_stream *out0,
            dev_stream *in1, dev_stream *out1)
{
    input0  = in0;
    input1  = in1;
    output0 = out0;
    output1 = out1;
}

/* For external usage */

static char
comm_read(dev_stream *in)
{
    char ret = '\0';

    if (in != NULL)
        stream_read((dev_stream *)in, STREAM_COMMON, &ret);

    return ret;
}

extern char
comm_read0(void)
{
    return comm_read(input0);
}

extern char
comm_read1(void)
{
    return comm_read(input1);
}

static void
comm_write(dev_stream *out, char c)
{
    if (out != NULL)
        stream_write((dev_stream *)out, STREAM_COMMON, &c);
}

extern void
comm_write0(char c)
{
    comm_write(output0, c);
}

extern void
comm_write1(char c)
{
    comm_write(output1, c);
}
