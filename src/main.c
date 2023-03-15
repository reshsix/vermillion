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

#include <interface/video.h>
#include <interface/audio.h>
#include <interface/storage.h>

extern int
kernel_main(void)
{
    struct video *v = video_new();
    struct audio *a = audio_new();
    struct storage *st = storage_new();

    video_del(v);
    audio_del(a);
    storage_del(st);

    return 0;
}
