project(NativeFuzzme)
cmake_minimum_required(VERSION 3.8)


# Executable
# ==========

add_executable(afl-proxy "afl-proxy.c")

set_target_properties(
    afl-proxy PROPERTIES LINK_FLAGS
    "-static-libstdc++ -DANDROID -llog -lz -lm \
    -Wl,--gc-sections -Wl,--exclude-libs,ALL -ldl"
)

target_link_libraries(afl-proxy "${CMAKE_SOURCE_DIR}/module/.build_android/libmodule.so")
