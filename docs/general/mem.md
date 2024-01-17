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

## Memory manipulation

#### #include <general/mem.h>

<details>
<summary>void *mem_new(size_t size)</summary>
Allocates and initializes memory to zero

#### Parameters
| name | description |
|------|-------------|
| size | Minimum size |

#### Return value
| case | description |
|------|-------------|
| Success | Block of at least size bytes |
| Failure | NULL |

</details>

<details>
<summary>void *mem_renew(void *mem, size_t size)</summary>
Reallocates and initializes memory to zero

#### Parameters
| name | description |
|------|-------------|
| mem | Allocated pointer or NULL |
| size | Minimum size |

#### Return value
| case | description |
|------|-------------|
| Success | Block of at least size bytes |
| Failure | NULL |

</details>

<details>
<summary>void *mem_del(void *mem)</summary>
Frees an allocated pointer

#### Parameters
| name | description |
|------|-------------|
| mem | Allocated pointer or NULL |

#### Return value
| case | description |
|------|-------------|
| Always | NULL |

</details>

<details>
<summary>int *mem_comp(const void *mem, const void *mem2,
                       size_t length)</summary>
Compares two memory blocks

#### Parameters
| name | description |
|------|-------------|
| mem | First block |
| mem2 | Second block |
| length | Bytes to compare |

#### Return value
| case | description |
|------|-------------|
| mem == mem2 | 0 |
| mem != mem2 | mem[unmatched] - mem2[unmatched] |

</details>

<details>
<summary>void *mem_find(const void *mem, u8 c, size_t length)</summary>
Finds a character in a memory block

#### Parameters
| name | description |
|------|-------------|
| mem | Memory block |
| c | Character to find |
| length | Bytes to compare |

#### Return value
| case | description |
|------|-------------|
| Found | &(mem[location]) |
| Not found | NULL |

</details>


<details>
<summary>void mem_init(const void *mem, u8 c, size_t length)</summary>
Initializes a memory block to a certain value

#### Parameters
| name | description |
|------|-------------|
| mem | Memory block |
| c | Value to use |
| length | Bytes to change |

</details>

<details>
<summary>void mem_copy(void *dest, const void *src, size_t length)</summary>
Copies a memory block to another location, which can overlap source

#### Parameters
| name | description |
|------|-------------|
| dest | Destination address |
| src | Source address |
| length | Bytes to copy |

</details>
