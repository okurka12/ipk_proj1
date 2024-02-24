#
# dont submit this
#
# generates list of header files that each .c file is dependent on
#
# for each file, prints filename of the file itself (optional) and all the
# header files
#
import re
import os

# should we print the filename of the file itself?
INCLUDE_SELF = True

# get .c files
files = os.listdir(".")
files = [file for file in files if ".c" in file]

dependecies = {}

# iterate through the .c files and find dependencies
for file in files:

    dependecies[file] = []

    # find all '#include "abcdef.h"'
    with open(file, "r", encoding="utf-8") as f:
        content = f.read()
    matches = re.findall(r"\#include \".+\.h\"", content)
    dependecies[file].extend([re.search(r"[\w\d]+\.h", m)[0] for m in matches])


# print the dependencies for all the files
for file in dependecies:
    print(f"{file}:")

    # print all the findings side by side
    print("    ", end="")
    print(f"{file} " if INCLUDE_SELF else "", end="")
    for dependency in dependecies[file]:
        print(f"{dependency}", end=" ")
    print()

# read makefile
with open("Makefile", "r", encoding="utf-8") as f:
    makefile = f.read()

# check if all the header files are listed in makefile
print()
modules = re.findall(r"\S+\.o:\s*(?:(?:\w+\.?\w?+\s*)*\\?\s*)*", makefile)
for module in modules:

    objfile = module.split(":")[0]
    file = module.split(":")[1].split()[0]

    for dependency in dependecies[file]:
        if dependency not in module:
            print(f"Warning: {dependency} not listed as "
                  f"a dependency of {objfile}")


# the above regexp can be tested with this
r"""
udpcl.o: udpcl.c \
udpcl.h ipk24chat.h utils.h rwmsgid.h
	$(CC) $(CFLAGS) -c -o $@ $<

rwmsgid.o: rwmsgid.c file rwmsgid.h
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: main.c \
ipk24chat.h \
\
udpcl.h \
utils.h argparse.h
	$(CC) $(CFLAGS) -c -o $@ $<

argparse.o: argparse.c argparse.h ipk24chat.h utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

gexit.o: gexit.c ipk24chat.h gexit.h utils.h
	$(CC) $(CFLAGS) -c -o $@ $<
"""

