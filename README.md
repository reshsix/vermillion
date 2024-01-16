# Vermillion
A library operating system for unikernels

## Development status
Alpha, some core features are still not implemented.
Will support Orange Pi One (armv7) and x86-32 pcs (i686) at first.

## Dependencies
You may need the following packages, which are necessary to build the
cross-compiler, install the bootloader, go through the compilation
process and debug the resulting image
```sh
gcc make git rsync g++ bison flex texinfo libncurses-dev
kconfig-frontends moreutils swig python3-dev bc
u-boot-tools grub2-common xorriso dialog mtools
qemu-system-arm qemu-system-i386 gdb-multiarch
```

## Example
~/my\_project/main.c:
```c
#include <general/types.h>

#include <hal/base/dev.h>
#include <hal/classes/gpio.h>

#include <system/log.h>

dev_incl (gpio, gpio0)

extern void
main(void)
{
    log("Hello World!\r\n");

    gpio_config(&dev(gpio0), 13, GPIO_OUT, GPIO_PULLOFF);
    gpio_set(&dev(gpio0), 13, true);
}
```

The image will be created in build/
```sh
. export.sh

cd ~/my_project
vmake defconfig
OBJS='main.o' vmake all
vmake debug
```

For testing
```sh
cd test
./test.sh
```

## Topology
```mermaid
sequenceDiagram
    participant Hardware
    participant core/arch
    participant core/src
    participant core/devtree
    participant core/drivers
    participant libs
    participant user
    Hardware->>core/arch: Boot up
    core/arch->>core/src: Call thread_scheduler()
    activate core/src
    core/src->>core/devtree: Call _devtree_init()
    core/devtree->>core/drivers: Initialize devices
    core/src->>user: Call main()
    deactivate core/src
    activate user
    user-->>user: Call own functions
    user-->>core/drivers: Initialize extra devices
    user-->>core/devtree: Interact with devices
    user-->>core/src: Call arch-independent functions
    user-->>core/arch: Call arch-dependent functions
    user-->>libs: Call generic device libraries
    user-->>libs: Call wrapper libraries
    user-->>libs: Call useful abstractions
    user->>core/src: Return from main()
    deactivate user
    activate core/src
    core/src->>core/devtree: Call _devtree_clean()
    core/devtree->>core/drivers: Clean devices
    core/src->>core/src: Halts forever
    deactivate core/src
```
