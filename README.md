# Vermillion
An embedded operating system for Orange Pi One

## Build
The building process will compile binutils, gcc and cross-compile u-boot before compiling the kernel and making a disk image for it.
```sh
sudo apt-get install gcc make git rsync g++ bison flex texinfo libncurses-dev \
                     swig python3-dev bc u-boot-tools moreutils
make

sudo apt-get install qemu-system-arm gdb-multiarch
make debug
```
