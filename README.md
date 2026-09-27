# Vermillion

**Status: 1.3.0γ**

## Features
- [x] HAL
- [x] Filesystem
- [x] Multitasking

## Example
```c
/* main.c */

#include <vermillion/task.h>
#include <vermillion/devtree.h>

#include <vermillion/util/debug.h>

static struct vrm_task_mut mut = {.owner = NULL};

static void
task(void *arg)
{
    while (true)
    {
        VRM_TASK_MUTEX(&mut, 1000)
        {
            vrm_debug(arg);
        }
        vrm_task_delay(1000);
    }
}

extern void
main(void)
{
    if (vrm_devtree_init(VRM_PLATFORM_SUNXI_H3, VRM_BOARD_NANOPI_NEO, VRM_NONE))
    {
        vrm_task_create(task, "Task C running", 29);
        vrm_task_create(task, "Task B running", 30);
        vrm_task_create(task, "Task A running", 31);
        vrm_task_scheduler(0, 1000, VRM_NONE);

        vrm_devtree_clean();
    }
}
```

## Instructions
- [Sunxi H3 boards](docs/boards/sunxi_h3.md)
