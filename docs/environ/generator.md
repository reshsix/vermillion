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

## Generator implementation

#### #include <environ/generator.h>

<details>
<summary>generator</summary>
Type containing the data of a generator

</details>

<details>
<summary>generator *generator_new(void (*f)(generator *),
                                  void *arg)</summary>
Allocates a generator variable and fills it with relevant data

#### Parameters
| name | description |
|------|-------------|
| f | Generator function to be executed |
| arg | Argument to be passed to the generator (through generator_arg) |

</details>

<details>
<summary>generator *generator_del(generator *g)</summary>
Frees a generator variable

#### Parameters
| name | description |
|------|-------------|
| g | Allocated generator or NULL |

#### Return value
| case | description |
|------|-------------|
| Always | NULL |

</details>

<details>
<summary>bool generator_next(generator *g)</summary>
Runs a generator until a yield

#### Parameters
| name | description |
|------|-------------|
| g | Generator to be executed |

#### Return value
| case | description |
|------|-------------|
| Reached a yield point | true |
| Reached a finish point | false |

</details>

<details>
<summary>void generator_rewind(generator *g)</summary>
Rewinds a generator back to the start

#### Parameters
| name | description |
|------|-------------|
| g | Generator to be rewinded |

</details>

<details>
<summary>void *generator_arg(generator *g)</summary>
Returns the argument passed to the generator

#### Parameters
| name | description |
|------|-------------|
| g | Current generator or a generator to be inspected |

</details>

<details>
<summary>void generator_yield(generator *g)</summary>
Defines a yield point on the generator function

#### Parameters
| name | description |
|------|-------------|
| g | Current generator |

</details>

<details>
<summary>[[noreturn]] generator_finish(generator *g)</summary>
Defines a finish point on the generator function (obligatory)

#### Parameters
| name | description |
|------|-------------|
| g | Current generator |

</details>
