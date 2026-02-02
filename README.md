# Vermillion
**Status: 1.0 α**

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
