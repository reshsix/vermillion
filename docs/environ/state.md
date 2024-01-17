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

## Processor state control

#### #include <environ/state.h>

<details>
<summary>state</summary>
Type containing architecture-dependent processor state

</details>

<details>
<summary>void *state_save(state *st)</summary>
Saves the current state into a state variable

#### Parameters
| name | description |
|------|-------------|
| st | Destination state |

#### Return value
| case | description |
|------|-------------|
| First execution | NULL |
| Restored state | Value from state_load |

</details>

<details>
<summary>[[noreturn]] void state_load(state *st, void *ret)</summary>
Loads a past state from a state variable

#### Parameters
| name | description |
|------|-------------|
| st | Destination state |
| ret | The return value of state_save |

</details>
