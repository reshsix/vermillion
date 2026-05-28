# Sunxi H3

Commands prefixed with `#` instead of `$` might need root privileges

The boards supported are: `orangepi_one` and `nanopi_neo`

## Compiling u-boot
Depends on `make`, `bison`, `flex` and `swig`

Produces `u-boot/u-boot-sunxi-with-spl.bin`, replace `<BOARD>`
with the name of your board
```sh
$ git clone https://gitlab.denx.de/u-boot/u-boot.git

$ cd u-boot
$ git checkout tags/v2020.04
$ make ARCH=arm CROSS_COMPILE=arm-none-eabi- <BOARD>_defconfig
$ make ARCH=arm CROSS_COMPILE=arm-none-eabi-
$ cd ../
```

## Compiling the kernel
Depends on `make` and `gcc-arm-none-eabi`

Produces `vermillion/sunxi_h3.ld`, `vermillion/libvrm_sunxi_h3.a`,
and `CFLAGS` in the output
```sh
$ cd vermillion
$ make sunxi_h3_defconfig
$ make
$ cd ../
```

## Compiling a project
Depends on `gcc-arm-none-eabi`

Results in `kernel.elf` and `kernel.bin`

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

```sh
$ CFLAGS="-nostdlib -ffreestanding -fpic -mcpu=cortex-a7 -mfloat-abi=soft"
$ arm-none-eabi-gcc $CFLAGS -Ivermillion/include -T vermillion/sunxi_h3.ld \
                    main.c -o kernel.elf -Lvermillion -lvrm_sunxi_h3 -lgcc
$ arm-none-eabi-objcopy -O binary kernel.elf kernel.bin
```

## Creating an image (optional)
Produces `kernel.img`, useful for debugging

```sh
$ dd if=/dev/zero of=kernel.img bs=1M count=32 status=none
# losetup /dev/loop0 kernel.img
```
Use `loop0` as `<DEVICE>`, and unmount when finished with
```sh
# losetup -d /dev/loop0
```

## Preparing the device
Depends on `mtools`

Replace `<DEVICE>` with your device name, usually `mmcblk0`
```sh
# printf 'start=2048, type=0c, bootable\n' | sfdisk -q /dev/<DEVICE>
# partx -a /dev/<DEVICE>
# mkfs.vfat -F32 -s 1 /dev/<DEVICE>p1"
```

## Installing the project
Depends on `u-boot-tools`

Replace `<DEVICE>` with your device name, and `<MOUNT>`
with your mount folder path
```
# u-boot.cmd

setexpr loadaddr 0x40008000
fatload mmc 0:1 ${loadaddr} kernel.bin
go ${loadaddr}
```

```sh
# dd if=u-boot/u-boot-sunxi-with-spl.bin of=<DEVICE> bs=1024 seek=8
# mount -t vfat <DEVICE>p1 <MOUNT>
# cp kernel.bin <MOUNT>/
# mkimage -C none -A arm -T script -d u-boot.cmd <MOUNT>/boot.scr
# umount <MOUNT>
```

## Debugging on QEMU (optional)
Replace `<DEVICE>` with your device name,
or with the `kernel.img` created earlier

Depends on `qemu-system-arm` and `gdb-multiarch`

```
# debug.gdb
target remote localhost:1234

py import os
py gdb.set_convenience_variable("elf", "kernel.elf")
eval "symbol-file %s", $elf

# optional but useful layout
tui new-layout debug {-horizontal src 1 asm  1} 2 status 0 \
                     {-horizontal cmd 2 regs 1} 1
tui layout debug
tui reg all

break _start
continue
```

```sh
# qemu-system-arm -s -S -M 'orangepi-pc' -drive file=<DEVICE>,format=raw &
$ gdb-multiarch --command=debug.gdb
```
