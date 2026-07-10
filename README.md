# Vermillion

**Status: 1.2**

## Features
- [x] HAL
- [x] Filesystem
- [ ] Multitasking

## Example
```c
/* main.c */

#include <vermillion/devtree.h>
#include <vermillion/util/debug.h>

void main(void)
{
    if (vrm_devtree_init(VRM_PLATFORM_SUNXI_H3, VRM_BOARD_NANOPI_NEO, 0))
    {
        vrm_debug("Hello World\r\n");

        vrm_devtree_clean();
    }
}
```

## Instructions
- [Sunxi H3 boards](docs/boards/sunxi_h3.md)
