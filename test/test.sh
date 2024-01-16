#!/bin/bash

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

run_test()
{
    ARG="${1%.c}.o"
    OBJS="$ARG" vmake all && vmake test
    RET="$?"

    rm "build/user/$ARG"
    return "$RET"
}

cd ..
. export.sh

cd test
vmake defconfig > /dev/null || exit 1

failure=0

clear
find * -name '*.c' | while IFS= read -r line; do
    echo "[${line%.c}.h]"
    result="$(true | run_test "$line")"
    if [[ "$?" != 0 ]]; then
        asserts="$(echo "$result" | grep "^$(pwd)/$x")"
        if [[ -n "$asserts" ]]; then
            echo "$asserts"
        else
            echo "$result" | cat -v
        fi
        failure=1
    fi
done

exit "$failure"
