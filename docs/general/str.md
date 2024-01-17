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

## String manipulation

#### #include <general/str.h>

<details>
<summary>size_t str_length(const char *str)</summary>
Gets the length of the null-terminated string (minus terminator)

#### Parameters
| name | description |
|------|-------------|
| str | Null-terminated string |

#### Return value
| case | description |
|------|-------------|
| Always | Characters before null-terminator |

</details>

<details>
<summary>size_t str_span(const char *str, const char *chars,
                         bool complement)</summary>
Counts the span of a string which is composed solely of characters present or
not present in another string

#### Parameters
| name | description |
|------|-------------|
| str | String to be examinated |
| chars | String with the chars to examinate |
| complement | Present or not present switch |

#### Return value
| case | description |
|------|-------------|
| complement == false | Length of str composed of chars characters |
| complement == true | Length of str not composed of chars characters |

</details>

<details>
<summary>char *str_find_l(const char *str, char c)</summary>
Searches for a character in a string, starting from left

#### Parameters
| name | description |
|------|-------------|
| str | String to be examinated |
| c | Character to be searched |

#### Return value
| case | description |
|------|-------------|
| Found | &(str[location]) |
| Not found | NULL |

</details>

<details>
<summary>char *str_find_r(const char *str, char c)</summary>
Searches for a character in a string, starting from right

#### Parameters
| name | description |
|------|-------------|
| str | String to be examinated |
| c | Character to be searched |

#### Return value
| case | description |
|------|-------------|
| Found | &(str[location]) |
| Not found | NULL |

</details>

<details>
<summary>char *str_find_m(const char *str, const char *chars)</summary>
Searches for multiple character in a string, returning
when one is found

#### Parameters
| name | description |
|------|-------------|
| str | String to be examinated |
| chars | Characters to be searched |

#### Return value
| case | description |
|------|-------------|
| Found | &(str[location]) |
| Not found | NULL |

</details>

<details>
<summary>char *str_find_s(const char *str, const char *str2)</summary>
Searches for a string inside a string, returning when one match is found

#### Parameters
| name | description |
|------|-------------|
| str | String to be examinated |
| str2 | String to be searched |

#### Return value
| case | description |
|------|-------------|
| Found | &(str[location]) |
| Not found | NULL |

</details>

<details>
<summary>char *str_token(char *str, const char *chars, char **saveptr)</summary>
Splits a string based on separators in a reentrant manner

#### Parameters
| name | description |
|------|-------------|
| str | String to be split or NULL |
| chars | Characters that can serve as separators |
| saveptr | A pointer to save the state of the function |

#### Return value
| case | description |
|------|-------------|
| str != NULL | The first token |
| str == NULL | The other tokens and NULL |

</details>

<details>
<summary>void str_copy(void *dest, const void *src, size_t length)</summary>
Copies a string to another location

#### Parameters
| name | description |
|------|-------------|
| dest | Destination address |
| src | Source address |
| length | Max length (minus terminator), or 0 for unlimited |

</details>

<details>
<summary>void str_concat(void *dest, const void *src, size_t length)</summary>
Concatenates a string to another string

#### Parameters
| name | description |
|------|-------------|
| dest | Destination address |
| src | Source address |
| length | Max final length (minus terminator), or 0 for unlimited |

</details>

<details>
<summary>void *str_dupl(void *str, size_t length)</summary>
Allocates a copy of a string

#### Parameters
| name | description |
|------|-------------|
| dest | Destination address |
| length | Max final length (minus terminator), or 0 for unlimited |

</details>
