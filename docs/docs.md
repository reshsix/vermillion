<!---
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
--->

## General headers

Headers that serve a general purpose

| name | description |
|------|-------------|
| [general/types.h](general/types.md) | Type definitions |
| [general/macros.h](general/macros.md) | Macro definitions |
| [general/mem.h](general/mem.md) | Memory manipulation |
| [general/str.h](general/str.md) | String manipulation |

## Environment headers

Headers that control the runtime environment

| name | description |
|------|-------------|
| [environ/state.h](environ/state.md) | Processor state control |
| [environ/fork.h](environ/fork.md) | Function stack switching |
| [environ/generator.h](environ/generator.md) | Generator implementation |

## Threading headers

Headers that control the multithreading environment

| name | description |
|------|-------------|
| [thread/thread.h](thread/thread.md) | Thread definition |
| [thread/mutex.h](thread/mutex.md) | Mutex synchronization |
| [thread/semaphore.h](thread/semaphore.md) | Semaphore synchronization |
| [thread/critical.h](thread/critical.md) | Critical keyword |
| [thread/implicit.h](thread/implicit.md) | Implicit keyword |
| [thread/channel.h](thread/channel.md) | Channel communication |
