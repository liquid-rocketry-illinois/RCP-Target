#!/bin/bash
vstr=$(cat ./VERSION)
echo "" > src/VERSION.cpp
echo -e "#ifdef __cplusplus" >> src/VERSION.cpp
echo -e "extern \"C\" {" >> src/VERSION.cpp
echo -e "#endif" >> src/VERSION.cpp
echo -e "extern const char* const RCPT_VERSION;" >> src/VERSION.cpp
echo -e "extern const char* const RCPT_VERSION_END;" >> src/VERSION.cpp
echo -e "const char* const RCPT_VERSION = \"$vstr\";" >> src/VERSION.cpp
echo -e "const char* const RCPT_VERSION_END = RCPT_VERSION + ${#vstr};" >> src/VERSION.cpp
echo -e "#ifdef __cplusplus" >> src/VERSION.cpp
echo -e "}" >> src/VERSION.cpp
echo -e "#endif" >> src/VERSION.cpp
