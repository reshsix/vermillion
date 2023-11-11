# Vermillion
A framework to develop embedded applications

## Development
The project is in alpha stage

### Supported boards
| Name |
| ---- |
| Orange Pi One |
| i686 generic |

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

#include <vermillion/drivers.h>
INCLUDE_DEVICE(gpio0)

extern int
main(void)
{
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
