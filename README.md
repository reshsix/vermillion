# Vermillion

**Status: 1.2γ**

## Features
- [x] HAL
- [x] Filesystem
- [ ] Multitasking

## Libraries
[Antimony](https://github.com/reshsix/antimony) - Shell tools
(Incompatible with current version)

## Example
```c
/* main.c */

#include <vermillion/devtree.h>
#include <vermillion/util/debug.h>

void main(void)
{
    if (vrm_devtree_init(VRM_PLATFORM_SUNXI_H3, VRM_BOARD_NANOPI_NEO, 0))
    {
        vrm_debug_string("Hello World\r\n");

        vrm_devtree_clean();
    }
}
```

## Instructions
- [Sunxi H3 boards](docs/boards/sunxi_h3.md)
