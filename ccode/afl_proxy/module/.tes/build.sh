export CC=/opt/cross/android-ndk-r25c/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang
export LIBKU=/home/gumi/LabC/fridaLab/project_1/ccode/afl_proxy/module/build_android/libmodule.so

#$CC ../android_ashmem/shmem.c shmatku.c -llog

$CC portable_shmatku.c $LIBKU
