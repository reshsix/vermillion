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

## Semaphore synchronization

#### #include <thread/semaphore.h>

<details>
<summary>semaphore (count) {}</summary>
Defines a semaphore block

#### Parameters
| name | description |
|------|-------------|
| count | Number of slots |

</details>

<details>
<summary>void semaphore_wait(int *s)</summary>
Semaphore wait function

#### Parameters
| name | description |
|------|-------------|
| s | Pointer to an int counter |

</details>

<details>
<summary>void semaphore_signal(int *s)</summary>
Semaphore signal function

#### Parameters
| name | description |
|------|-------------|
| s | Pointer to an int counter |

</details>
