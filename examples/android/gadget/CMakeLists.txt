project(NativeFuzzme)
cmake_minimum_required(VERSION 3.8)

link_directories(${CMAKE_SOURCE_DIR}/lib)

# Executable
# ==========

add_executable(targetFuzz "target.c")
set_property(TARGET targetFuzz APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-rpath=$ORIGIN")
target_link_libraries(targetFuzz vuln)
