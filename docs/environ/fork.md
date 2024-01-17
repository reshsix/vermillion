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

## Function stack switching

#### #include <environ/fork.h>

<details>
<summary>fork</summary>
Type containing architecture-dependent data to
execute a function in a new stack

</details>

<details>
<summary>fork *fork_new(void (*f)(void *), void *arg)</summary>
Allocates a fork variable and fills it with relevant data

#### Return value
| case | description |
|------|-------------|
| f | Function to be executed in a new stack |
| arg | Argument to be passed to the function |

</details>

<details>
<summary>fork *fork_del(fork *fk)</summary>
Frees a fork variable

#### Parameters
| name | description |
|------|-------------|
| fk | Allocated fork or NULL |

#### Return value
| case | description |
|------|-------------|
| Always | NULL |

</details>

<details>
<summary>void fork_run(fork *fk)</summary>
Runs a function in another stack, swapping before and restoring after

#### Parameters
| name | description |
|------|-------------|
| fk | Allocated fork |

</details>
