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

#pragma once

#include <general/types.h>

#include <hal/stream.h>

/* For devtree usage */

void comm_config(dev_stream *in0, dev_stream *out0,
                 dev_stream *in1, dev_stream *out1);

/* For external usage */

char comm_read0(void);
char comm_read1(void);
void comm_write0(char c);
void comm_write1(char c);
