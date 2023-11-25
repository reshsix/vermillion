# Vermillion
A library operating system for unikernels

## Supported boards
Currently focused on Orange Pi One (armv7), supports i686
for arch dependency tests

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
#include <easy/io.h>

#include <vermillion/utils.h>
#include <vermillion/drivers.h>
INCLUDE_DEVICE(gpio0)

extern int
main(void)
{
    log_s("Hello World!\r\n");

    io_chip(&DEVICE(gpio0));
    io_config(13, OUTPUT);
    io_write(13, HIGH);

    return 0x0;
}
```

The image will be created in build/
```sh
. export.sh

cd ~/my_project
vmake defconfig
OBJS='main.o' LIBS='easy' vmake all
vmake debug
```

For testing
```sh
. export.sh

cd test
vmake defconfig
OBJS='utils.o' vmake all
vmake debug
```

## Missing features before first release
- core/utils.o
    - Software interrupt handling
- libs (Generic device libraries)
    - Framebuffer (video drivers)
    - Sound (audio drivers)
    - Storage (fs drivers)
- libs (Wrappers libraries)
    - Libc wrapper
- libs (Useful abstractions)
    - Threads or Coroutines

## Topology
```mermaid
sequenceDiagram
    participant Hardware
    participant core/arch
    participant core/init.o
    participant core/utils.o
    participant core/devtree
    participant core/drivers
    participant libs
    participant user
    Hardware->>core/arch: Boot up
    core/arch->>core/init.o: Call __init()
    activate core/init.o
    core/init.o->>core/utils.o: Call _utils_init()
    core/init.o->>core/devtree: Call _devtree_init()
    core/devtree->>core/drivers: Initialize devices
    core/init.o->>user: Call main()
    deactivate core/init.o
    activate user
    user-->>user: Call own functions
    user-->>core/drivers: Initialize extra devices
    user-->>core/devtree: Interact with devices
    user-->>core/utils.o: Call arch-independent functions
    user-->>core/arch: Call arch-dependent functions
    user-->>libs: Call generic device libraries
    user-->>libs: Call wrapper libraries
    user-->>libs: Call useful abstractions
    user->>core/init.o: Return from main()
    deactivate user
    activate core/init.o
    core/init.o->>core/utils.o: Call _utils_clean()
    core/init.o->>core/devtree: Call _devtree_clean()
    core/devtree->>core/drivers: Clean devices
    core/init.o->>core/init.o: Halts forever
    deactivate core/init.o
```
