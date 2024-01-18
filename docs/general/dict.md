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

## Hash table implementation

#### #include <general/dict.h>

<details>
<summary>dict</summary>
Type that contains a hash table data

</details>

<details>
<summary>dict *dict_new(size_t type)</summary>
Creates a hash table for storing data

#### Parameters
| name | description |
|------|-------------|
| type | Size of variables to be stored |

#### Return value
| case | description |
|------|-------------|
| Success | Pointer to a dict |
| Failure | NULL |

</details>

<details>
<summary>dict *dict_del(dict *d)</summary>
Frees a hash table

#### Parameters
| name | description |
|------|-------------|
| d | dict to free or NULL |

#### Return value
| case | description |
|------|-------------|
| Always | NULL |

</details>

<details>
<summary>bool *dict_get(dict *d, const char *id, void *data)</summary>
Gets data from a hash table by id

#### Parameters
| name | description |
|------|-------------|
| d | dict to be searched |
| id | id to search |
| data | Pointer to store the data, or NULL if just checking |

#### Return value
| case | description |
|------|-------------|
| Success | true |
| Failure | false |

</details>

<details>
<summary>bool *dict_set(dict *d, const char *id, void *data)</summary>
Sets data on a hash table by id

#### Parameters
| name | description |
|------|-------------|
| d | dict to be searched |
| id | id to search |
| data | Pointer to read the data, or NULL to clear |

#### Return value
| case | description |
|------|-------------|
| Success | true |
| Failure | false |

</details>
