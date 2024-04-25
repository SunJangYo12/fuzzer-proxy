rm -R build
mkdir build
cd build
cmake \
        -DANDROID_PLATFORM=25 \
        -DCMAKE_TOOLCHAIN_FILE=/opt/cross/android-ndk-r25c/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=arm64-v8a ..

