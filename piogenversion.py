import os

try:
    os.remove("src/VERSION.cpp")
except OSError:
    pass

ver = ""
with open("VERSION", 'r') as file:
    ver = file.read()

lines = [
    "#ifdef __cplusplus\n",
    "extern \"C\" {\n",
    "#endif\n",
    "extern const char* const RCPT_VERSION;\n",
    "extern const char* const RCPT_VERSION_END;\n",
    "const char* const RCPT_VERSION = \"" + ver + "\";\n",
    "const char* const RCPT_VERSION_END = RCPT_VERSION + " + str(len(ver)) + ";\n",
    "#ifdef __cplusplus\n",
    "}\n",
    "#endif\n"
]

with open("src/VERSION.cpp", 'w') as file:
    file.writelines(lines)
