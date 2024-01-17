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

## Channel communication

#### #include <thread/channel.h>

<details>
<summary>channel</summary>
Type that contains the data of a channel

</details>

<details>
<summary>channel *channel_new(size_t type, size_t size)</summary>
Creates a channel for inter-thread communication

#### Parameters
| name | description |
|------|-------------|
| type | Size of each variable to be passed |
| size | Amount of data it can hold before blocking |

#### Return value
| case | description |
|------|-------------|
| Success | Pointer to a channel |
| Failure | NULL |

</details>

<details>
<summary>channel *channel_del(channel *ch)</summary>
Frees a channel

#### Parameters
| name | description |
|------|-------------|
| t | Channel to free or NULL |

#### Return value
| case | description |
|------|-------------|
| Always | NULL |

</details>

<details>
<summary>bool *channel_empty(channel *ch)</summary>
Checks if a channel is empty

#### Parameters
| name | description |
|------|-------------|
| ch | Channel to be inspected |

#### Return value
| case | description |
|------|-------------|
| Channel is empty | true |
| Channel is not empty | false |

</details>

<details>
<summary>bool *channel_full(channel *ch)</summary>
Checks if a channel is full

#### Parameters
| name | description |
|------|-------------|
| ch | Channel to be inspected |

#### Return value
| case | description |
|------|-------------|
| Channel is full | true |
| Channel is not full | false |

</details>

<details>
<summary>size_t *channel_stat(channel *ch)</summary>
Checks how many writes can be done before blocking

#### Parameters
| name | description |
|------|-------------|
| ch | Channel to be inspected |

#### Return value
| case | description |
|------|-------------|
| Always | Writes before blocking  |

</details>

<details>
<summary>size_t *channel_read(channel *ch, void *data)</summary>
Reads from inter-thread channel

#### Threading
| notes |
|-------|
| Counts as one yield point |
| Has implicit yields |
| Doesn't work on critical |

#### Parameters
| name | description |
|------|-------------|
| ch | Channel to be read from |
| data | Pointer to where data will be written

</details>

<details>
<summary>size_t *channel_write(channel *ch, void *data)</summary>
Writes to inter-thread channel

#### Threading
| notes |
|-------|
| Counts as one yield point |
| Has implicit yields |
| Doesn't work on critical |

#### Parameters
| name | description |
|------|-------------|
| ch | Channel to be written |
| data | Pointer to where data will be read

</details>
