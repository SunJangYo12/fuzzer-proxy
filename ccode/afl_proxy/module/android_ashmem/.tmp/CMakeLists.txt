project(NativeFuzzme)
cmake_minimum_required(VERSION 3.8)


# Library
# ==========


add_library(android-shmem
   SHARED
   "shmem.c"
)

set_target_properties(
    android-shmem PROPERTIES LINK_FLAGS
    "-std=c99 -static-libstdc++ -DANDROID -llog -lz -lm -lc \
    -Wl,--gc-sections -Wl,--exclude-libs,ALL -ldl"
)

