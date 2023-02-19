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

## Libc progress
C89
- [ ] assert.h
- [ ] ctype.h
- [x] errno.h
- [x] float.h
- [x] limits.h
- [ ] locale.h
- [ ] math.h
- [ ] setjmp.h
- [x] signal.h
- [x] stdarg.h
- [x] stddef.h
- [ ] stdio.h
- [ ] stdlib.h
    - [x] Process control
    - [x] Memory management
    - [ ] Numeric conversions
    - [ ] Wide string
    - [ ] Miscellaneous
- [ ] string.h
- [ ] time.h

NA1
- [x] iso646.h
- [ ] wchar.h
- [ ] wctype.h

C99
- [ ] complex.h
- [ ] fenv.h
- [ ] inttypes.h
- [x] stdbool.h
- [x] stdint.h
- [ ] tgmath.h
