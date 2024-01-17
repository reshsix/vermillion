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

## Thread definition

#### #include <thread/thread.h>

<details>
<summary>thread</summary>
Type that contains the data of a thread

</details>

<details>
<summary>thread_decl (storage, id){}</summary>
Defines a new thread task, the body is a function

#### Parameters
| name | description |
|------|-------------|
| storage | extern or static |
| id | Name of the function |

</details>

<details>
<summary>thread_incl (id){}</summary>
Includes an extern thread task declared somewhere else

#### Parameters
| name | description |
|------|-------------|
| id | Name of the function |

</details>

<details>
<summary>thread_task</summary>
Pointer type to a function declared as thread task

</details>

<details>
<summary>thread *thread_new(thread_task(f), void *arg, bool persistent,
                            u8 priority)</summary>
Defines a new thread and schedules it for execution

#### Parameters
| name | description |
|------|-------------|
| f | Thread function to be executed |
| arg | Argument to be passed to the thread (through thread_arg) |
| persistent | Not managed or freed by the scheduler (false/true) |
| priority | Defines thread execution to (priority/255) of the time |

#### Return value
| case | description |
|------|-------------|
| Success, persistent == true | Pointer to a thread |
| Success, persistent == false | 0x1 |
| Failure | NULL |

</details>

<details>
<summary>thread *thread_del(thread *t)</summary>
Frees a persistent thread

#### Parameters
| name | description |
|------|-------------|
| t | Persistent thread to free or NULL |

#### Return value
| case | description |
|------|-------------|
| Always | NULL |

</details>

<details>
<summary>size_t thread_sync(thread *t, size_t step)</summary>
Wait until reaching a certain yield point

#### Parameters
| name | description |
|------|-------------|
| t | Persistent thread |
| step | Index of yield point (starting at zero) |

#### Return value
| case | description |
|------|-------------|
| Always | Step after sync |

</details>

<details>
<summary>size_t thread_wait(thread *t)</summary>
Wait until reaching the finish point

#### Parameters
| name | description |
|------|-------------|
| t | Persistent thread |

#### Return value
| case | description |
|------|-------------|
| Always | Step after wait |

</details>

<details>
<summary>bool thread_rewind(thread *t)</summary>
Rewinds a persistent thread

#### Parameters
| name | description |
|------|-------------|
| t | Persistent thread |

#### Return value
| case | description |
|------|-------------|
| Success | true |
| Failure | false |

</details>

<details>
<summary>void *thread_arg(void)</summary>
Returns the argument passed to the current thread

#### Return value
| case | description |
|------|-------------|
| Always | Argument passed on thread_new |

</details>

<details>
<summary>void thread_yield(void)</summary>
Defines a yield point on the current thread

</details>

<details>
<summary>[[noreturn]] thread_loop(void)</summary>
Defines a looping point on the current thread

</details>

<details>
<summary>[[noreturn]] noreturn thread_finish(void)</summary>
Defines a finish point on the current thread (obligatory)

</details>
