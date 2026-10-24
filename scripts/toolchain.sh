#!/usr/bin/env bash
#
# Builds the i686-elf cross compiler (binutils + gcc, C only) into ./toolchain.
# Follows https://wiki.osdev.org/GCC_Cross-Compiler
#
#   ./scripts/toolchain.sh
#
set -euo pipefail

BINUTILS_VERSION=2.42
GCC_VERSION=13.2.0
TARGET=i686-elf

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PREFIX="${PREFIX:-$ROOT/toolchain}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
GNU_MIRROR="${GNU_MIRROR:-https://ftp.gnu.org/gnu}"
WORK="$PREFIX/.build"

if [ -x "$PREFIX/bin/$TARGET-gcc" ]; then
    echo "$TARGET toolchain already installed in $PREFIX"
    exit 0
fi

for tool in make gcc g++ bison flex makeinfo wget tar; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "error: '$tool' is required (see the header of this script)" >&2
        exit 1
    }
done

mkdir -p "$WORK"
cd "$WORK"

fetch() {   
    local url=$1 file; file=$(basename "$url")
    [ -f "$file" ] || wget -q --show-progress "$url"
    [ -d "${file%.tar.xz}" ] || tar xf "$file"
}

echo "==> downloading sources"
fetch "$GNU_MIRROR/binutils/binutils-$BINUTILS_VERSION.tar.xz"
fetch "$GNU_MIRROR/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.xz"
(cd "gcc-$GCC_VERSION" && ./contrib/download_prerequisites)

export PATH="$PREFIX/bin:$PATH"

echo "==> building binutils $BINUTILS_VERSION"
mkdir -p build-binutils && cd build-binutils
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" \
    --with-sysroot --disable-nls --disable-werror >/dev/null
make -j"$JOBS" >/dev/null
make install >/dev/null
cd ..

echo "==> building gcc $GCC_VERSION (this takes a while)"
mkdir -p build-gcc && cd build-gcc
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" \
    --disable-nls --enable-languages=c --without-headers >/dev/null
make -j"$JOBS" all-gcc >/dev/null
make -j"$JOBS" all-target-libgcc >/dev/null
make install-gcc >/dev/null
make install-target-libgcc >/dev/null
cd ..

rm -rf "$WORK"
echo "==> done: $("$PREFIX/bin/$TARGET-gcc" --version | head -1)"
echo "    next steps:"
echo "      make                                         # build the kernel"
echo "      qemu-system-i386 -kernel kernel.elf -m 512M  # run it"
echo "      ./scripts/test.sh                            # run the tests"