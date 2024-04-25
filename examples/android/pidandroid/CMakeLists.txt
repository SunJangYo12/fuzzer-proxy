project(NativeFuzzme)
cmake_minimum_required(VERSION 3.8)

# Executable
# ==========

add_executable(tesPid "target.c")
set_property(TARGET tesPid APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-rpath=$ORIGIN")
