# Vermillion
**Status: 1.0 γ**

## Depends on
```sh
gcc make git rsync g++ bison flex texinfo libncurses-dev
kconfig-frontends moreutils swig python3-dev bc
u-boot-tools dialog mtools qemu-system-arm gdb-multiarch
```

## Configuration
- nanopi\_neo\_defconfig (NanoPi NEO)
- orangepi\_one\_defconfig (OrangePi One)

## Compilation

Created in `build`
```sh
make -f core.mk nanopi_neo_defconfig

make -f deps.mk all
make -f core.mk all

make -f core.mk debug
```

## Writing programs

```c
#include <vermillion/entry.h>

extern bool
vrm_entry(struct vrm *v, const char **args, int count)
{
    const char msg[] = "vrm_entry message\r\n";
    for (int i = 0; i < sizeof(msg) - 1; i++)
        v->comm.write0(msg[i]);

    return true;
}
```

```sh
PATH=$PATH:$(realpath deps/tools/bin)
CFLAGS="-shared -fPIE -fPIC -ffreestanding -nostdlib -Wl,-evrm_entry -Wl,-z,defs"
arm-none-eabi-gcc -Iinclude $CFLAGS prog.c -o root/init.elf
```
