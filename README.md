# Vermillion
A framework to develop embedded applications

## Development
The project is in alpha stage, lacking some libc functions and drivers

### Libc missing features
- stdlib.h's atoX strtoX qsort bsearch mbX wcX
- stdio.h's buffering (fflush, setbuf/setvbuf)
- stdio.h's scanf/printf family
- stdio.h's freopen, fwrite\_fs, remove, rename, tmpfile, tmpnam
- setjmp.h, time.h, inttypes.h
- Wide character extensions: wchar.h, wctype.h  (NA1)
- Mathematical functions: math.h (C89), fenv.h, complex.h, tgmath.h (C99)

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
u-boot-tools grub2-common xorriso dialog
qemu-system-arm qemu-system-i386 gdb-multiarch
```

## Example
~/my\_project/main.c:
```c
#include <stdio.h>
#include <easy/io.h>

extern int
main(void)
{
    puts("Hello World!\r\n");

    io_chip("gpio0");
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
