CC=/opt/cross/android-ndk-r25c/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang

$CC -fPIC -shared libvuln.c -o libvuln.so
