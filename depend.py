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

# iterate through the files
for file in files:

    print(f"{file}:")

    # find all '#include "abcdef.h"
    with open(file, "r", encoding="utf-8") as f:
        content = f.read()
    matches = re.findall(r"\#include \".+\.h\"", content)

    # print all the findings side by side
    print("    ", end="")
    print(f"{file} " if INCLUDE_SELF else "", end="")
    for match in matches:
        header_name = re.search(r"[\w\d]+\.h", match)
        print(f"{header_name[0]} ", end="")
    print()
