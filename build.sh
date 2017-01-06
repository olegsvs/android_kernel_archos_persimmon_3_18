
#!/bin/bash
#export CONFIG_DEBUG_SECTION_MISMATCH=y
export KBUILD_BUILD_USER=oleg.svs

export KBUILD_BUILD_HOST=SRT

echo "Make dirs >>>"

mkdir tools/tools

echo "Export toolchains >>>"

export ARCH=arm64 CROSS_COMPILE=../*linaro*/bin/aarch64-linux-gnu-

echo "Make defconfig >>>"

make dw6735_65u_s_m0_defconfig
echo "Start build >>>"

	time make -j16

echo "======================"
