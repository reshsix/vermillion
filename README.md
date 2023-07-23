# Vermillion
A framework to develop embedded applications

## Development
The project is in alpha stage, lacking some libc functions and drivers

### Libc missing features
- stdlib.h's atoX strtoX qsort bsearch mbX wcX
- locale.h, setjmp.h, stdio.h, math.h, time.h
- wchar.h, wctype.h (NA1)
- fenv.h, inttypes.h, complex.h, tgmath.h (C99)

### Supported boards
| Name | Configuration |
| ---- | ------------- |
| Orange Pi One | orangepi\_one\_defconfig |
| x86 generic | i686\_defconfig |

## Dependencies
You may need the following packages, which are necessary to build the
cross-compiler, install the bootloader, go through the compilation
process and debug the resulting image
```sh
gcc make git rsync g++ bison flex texinfo libncurses-dev
kconfig-frontends moreutils swig python3-dev bc
u-boot-tools grub2-common xorriso dialog
qemu-system-arm qemu-system-i386 gdb-multiarch
```

## Example
~/my\_project/main.c:
```c
#include <easy/io.h>

extern int
main(void)
{
    io_chip("gpio0");
    io_config(10, OUTPUT);
    io_write(10, HIGH);

    return 0x0;
}
```

The image will be created in build/
```sh
. export.sh
vmake tools

cd ~/my_project
vmake defconfig
OBJS='main.o' LIBS='easy' vmake all
vmake debug
```
