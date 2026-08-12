#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ROOTFS=$1
SYSROOT=$2
SRC=$3
CC=$4
fetch(){ url=$1;file=$2;[ -f "$file" ]||curl -fL --retry 3 --output "$file" "$url"; }
unpack(){ archive=$1;dir=$2;[ -d "$dir" ]||tar -xf "$archive" -C "$SRC"; }
mkdir -p "$SRC" "$ROOTFS/usr/bin"
fetch https://ftp.gnu.org/gnu/binutils/binutils-2.43.1.tar.xz "$SRC/binutils.tar.xz"
fetch https://ftp.gnu.org/gnu/gcc/gcc-14.2.0/gcc-14.2.0.tar.xz "$SRC/gcc.tar.xz"
fetch https://ftp.gnu.org/gnu/gmp/gmp-6.3.0/gmp-6.3.0.tar.xz "$SRC/gmp.tar.xz"
fetch https://ftp.gnu.org/gnu/mpfr/mpfr-4.2.1.tar.xz "$SRC/mpfr.tar.xz"
fetch https://ftp.gnu.org/gnu/mpc/mpc-1.3.1.tar.gz "$SRC/mpc.tar.gz"
unpack "$SRC/binutils.tar.xz" "$SRC/binutils-2.43.1"
unpack "$SRC/gcc.tar.xz" "$SRC/gcc-14.2.0"
unpack "$SRC/gmp.tar.xz" "$SRC/gmp-6.3.0"
unpack "$SRC/mpfr.tar.xz" "$SRC/mpfr-4.2.1"
unpack "$SRC/mpc.tar.gz" "$SRC/mpc-1.3.1"
ln -sfn "$SRC/gmp-6.3.0" "$SRC/gcc-14.2.0/gmp"
ln -sfn "$SRC/mpfr-4.2.1" "$SRC/gcc-14.2.0/mpfr"
ln -sfn "$SRC/mpc-1.3.1" "$SRC/gcc-14.2.0/mpc"
mkdir -p "$SRC/binutils-build"
(cd "$SRC/binutils-build" && "$SRC/binutils-2.43.1/configure" --host=x86_64-linux-musl --target=x86_64-linux-musl --prefix=/usr --disable-nls --disable-werror --disable-gdb && make -j"$(nproc)" && make DESTDIR="$ROOTFS" install)
mkdir -p "$SRC/gcc-build"
(cd "$SRC/gcc-build" && "$SRC/gcc-14.2.0/configure" --host=x86_64-linux-musl --target=x86_64-linux-musl --build=x86_64-linux-gnu --prefix=/usr --with-sysroot=/ --with-native-system-header-dir=/include --enable-languages=c,c++ --disable-multilib --disable-nls --disable-shared --disable-libssp --disable-libquadmath --disable-libgomp --disable-libatomic --disable-libvtv && make CC="$CC" CXX="$CC" all-gcc -j"$(nproc)" && make DESTDIR="$ROOTFS" install-gcc)
if [ -x "$ROOTFS/usr/bin/x86_64-linux-musl-gcc" ]; then ln -sf x86_64-linux-musl-gcc "$ROOTFS/usr/bin/gcc"; fi
if [ -x "$ROOTFS/usr/bin/x86_64-linux-musl-g++" ]; then ln -sf x86_64-linux-musl-g++ "$ROOTFS/usr/bin/g++"; fi
fetch https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5.tar.gz "$SRC/cmake.tar.gz"
unpack "$SRC/cmake.tar.gz" "$SRC/cmake-3.30.5"
(cd "$SRC/cmake-3.30.5" && CC="$ROOTFS/usr/bin/x86_64-linux-musl-gcc" CXX="$ROOTFS/usr/bin/x86_64-linux-musl-g++" ./bootstrap --prefix=/usr -- -DCMAKE_USE_OPENSSL=OFF && make -j"$(nproc)" && make DESTDIR="$ROOTFS" install)
fetch https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.0/llvm-project-19.1.0.src.tar.xz "$SRC/llvm.tar.xz"
unpack "$SRC/llvm.tar.xz" "$SRC/llvm-project-19.1.0.src"
mkdir -p "$SRC/llvm-build"
cmake -S "$SRC/llvm-project-19.1.0.src/llvm" -B "$SRC/llvm-build" -G Ninja -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CC" -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS=-static
cmake --build "$SRC/llvm-build" -j"$(nproc)"
DESTDIR="$ROOTFS" cmake --install "$SRC/llvm-build"
