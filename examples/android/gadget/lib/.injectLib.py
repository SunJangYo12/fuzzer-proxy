import lief
import os

name = "libvuln.so"

xx = lief.parse(name)
xx.add_library("libgadget.so") #download libgadget.so from frida github
xx.write(name)

os.system("readelf -d {} | grep NEEDED".format(name))
