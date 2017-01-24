
#!/bin/bash
#export CONFIG_DEBUG_SECTION_MISMATCH=y
export KBUILD_BUILD_USER=oleg.svs
export GCC_VERSION="gcc version 6.2.1 20161112 (linaro) (GCC)"
export KBUILD_BUILD_HOST=SRT

echo "Make dirs >>>"

mkdir tools/tools

echo "Export toolchains >>>"

#export ARCH=arm CROSS_COMPILE=../*linaro*/bin/arm-linux-gnu-
export ARCH=arm CROSS_COMPILE=../*6*/bin/arm-linux-gnueabi-
echo "Make defconfig >>>"

make persimmon_defconfig

echo "Start build >>>"

	time make -j16

echo "======================"
