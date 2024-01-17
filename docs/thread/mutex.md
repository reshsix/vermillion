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

## Mutex synchronization

#### #include <thread/mutex.h>

<details>
<summary>mutex (...) {}</summary>
Defines a mutex synchronized block

#### Parameters
| name | description |
|------|-------------|
| ... | empty: blocks other threads, otherwise: blocks when same parameter |

</details>

<details>
<summary>void mutex_lock(void **m, void *param)</summary>
Mutex lock function

##### Parameters
| name | description |
|------|-------------|
| m | Pointer to a void * which will hold the locking parameter |
| param | NULL if the lock is for all threads, or a parameter

</details>

<details>
<summary>void mutex_unlock(void **m, void *param)</summary>
Mutex unlock function

#### Parameters
| name | description |
|------|-------------|
| m | Pointer to a void * which will hold the locking parameter |
| param | NULL if the lock is for all threads, or a parameter

</details>
