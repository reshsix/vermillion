# Vermillion
**Status: 1.1β**

## Devices
- nanopi\_neo\_defconfig (NanoPi NEO)
- orangepi\_one\_defconfig (OrangePi One)

## Assembling

| Stage       | Dependencies                   |
|-------------|--------------------------------|
| Versioning  | git                            |
| Compilation | gcc-arm-none-eabi              |
| Build       | make moreutils bc              |
| Formatting  | mtools                         |
| U-Boot      | bison flex swig u-boot-tools   |
| Debug       | qemu-system-arm gdb-multiarch  |

```sh
make nanopi_neo_defconfig
make all
make debug
dd if=build/vermillion.img of=/dev/mmcblk0
```

## Writing programs

`struct vrm` is defined in `<vermillion/vrm.h>`

```c
#include <vermillion/prog.h>

extern bool
vrm_prog(struct vrm *v, const char **args, int count)
{
    const char msg[] = "vrm_prog message\r\n";
    for (int i = 0; i < sizeof(msg) - 1; i++)
        v->comm.write0(msg[i]);

    return true;
}
```

```sh
CFLAGS="-shared -fPIE -fPIC -ffreestanding -nostdlib -Wl,-evrm_prog -Wl,-z,defs"
arm-none-eabi-gcc -Iinclude $CFLAGS prog.c -o root/init.elf
```
