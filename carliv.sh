if [  -f "arch/arm/boot/zImage-dtb" ]
then
    cp -rf arch/arm/boot/zImage-dtb ../../CarlivImageKitchen32/boot_snowmm/
    cd
    cd CarlivImageKitchen32/boot_snowmm/
    rm boot.img-kernel
    mv zImage-dtb boot.img-kernel
    cd ..
    ./carliv
fi
