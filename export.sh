# This file is part of vermillion.

# Vermillion is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 3.

# Vermillion is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with vermillion. If not, see <https://www.gnu.org/licenses/>.

case "$(pwd)" in
    *\ *)
        echo "Headers don't work properly in directories with spaces"
        exit 1
    ;;
esac

BOARD="$(dialog --stdout --menu 'Choose the target board' 0 0 0 \
         orangepi_one '' \
         i686         '' )"
[ -z "$BOARD" ] && exit 1
export BOARD
clear

[ -z "$VERMILLION" ] && PATH_EXPORT=1
VERMILLION="$(pwd)"
export VERMILLION

vmake()
(
    set -e

    ORG="$(pwd)"
    EXTERN=''

    if [ "$NDEBUG" = 1 ]; then
        CFLAGS="-O2 -DNDEBUG=1 $CFLAGS"
    else
        CFLAGS="-O0 -ggdb3 $CFLAGS"
    fi
    export NDEBUG

    cd "$VERMILLION/user"
    echo "[user]"
    ORG="$ORG" OBJS="$OBJS" BUILD="$ORG/build" make $@

    for x in $(find "$ORG/build/user/"* -iname '*.o' 2>/dev/null); do
        EXTERN="$EXTERN $(realpath "$x")"
    done

    cd "$VERMILLION/libs"
    for x in $LIBS; do
        cd "$x"
        echo "[libs/$x]"
        BUILD="$ORG/build" make $@
        cd ..
    done

    for x in $(find "$ORG/build/libs/"* -iname '*.o' 2>/dev/null); do
        EXTERN="$EXTERN $(realpath "$x")"
    done

    CFLAGS="$CFLAGS" EXTERN="$EXTERN" CONFIG="$ORG/.config" BUILD="$ORG/build"
    export CFLAGS EXTERN CONFIG BUILD

    cd "$VERMILLION/core"
    echo "[core]"
    make $@
)

cd core
cp "config/${BOARD}_defconfig" .config
kconfig-conf --olddefconfig Kconfig > /dev/null

CC="$( (cat .config; echo 'echo $CONFIG_TARGET') | sh)-gcc"
export CC

rm -f .config
cd ..

CFLAGS="-Wall -Wextra -Wno-attributes"
CFLAGS="$CFLAGS -c -std=gnu99 -nostdlib -ffreestanding"
CFLAGS="$CFLAGS -I$VERMILLION/core/include"

for x in $(find "$VERMILLION/libs"/*/* -prune -type d -iname 'include'); do
    CFLAGS="$CFLAGS -I$x"
done
export CFLAGS

if [ "$PATH_EXPORT" = 1 ]; then
    PATH="$PATH:$VERMILLION:$VERMILLION/core/deps/tools/bin"
    export PATH
fi
unset PATH_EXPORT

echo ""
echo "[Variables exported]"
echo "CC=$CC"
echo "CFLAGS=$CFLAGS"
echo "PATH=$PATH"
echo ""
echo "*** Use vmake with OBJS and LIBS to compile a project (space-separated)"
