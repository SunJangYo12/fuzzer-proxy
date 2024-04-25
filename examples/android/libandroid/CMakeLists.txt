project(NativeFuzzme)
cmake_minimum_required(VERSION 3.8)

link_directories(${CMAKE_SOURCE_DIR}/lib)

# Executable
# ==========

add_executable(harness "fuzz.c")
set_property(TARGET harness APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-rpath=$ORIGIN")
target_link_libraries(harness blogfuzz)
