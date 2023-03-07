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

## Libc missing features
General
- locale.h
- setjmp.h
- stdio.h
- stdlib.h's atoX strtoX qsort bsearch
- time.h
- fenv.h (C99)
- inttypes.h (C99)

Wide strings
- stdlib.h mbX wcX
- wchar.h (NA1)
- wctype.h (NA1)

Math
- math.h
- complex.h (C99)
- tgmath.h (C99)

## Extra headers
### bitbang.h
Implementation of protocols for unimplemented/damaged controllers
### diagnostic.h
Functions to check the board health/state
### generator.h
Implementation of generators to make coroutines
### types.h
All generic types used in other headers
### utils.h
Tools necessary for other headers that are not categorized separatedly
