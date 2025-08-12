file(STRINGS ${SOURCE}/VERSION vstr)

if(${BTYPE} STREQUAL "Debug")
    string(APPEND vstr "-DEBUG")
endif()

set(CODESTR "#ifdef __cplusplus\nextern \"C\" {\n#endif\nextern const char* const RCPT_VERSION\;\nextern const char* const RCPT_VERSION_END\;\nconst char* const RCPT_VERSION = \"")
string(APPEND CODESTR ${vstr})
string(APPEND CODESTR "\"\; const char* const RCPT_VERSION_END = RCPT_VERSION + ")
string(LENGTH ${vstr} vstrlen)
string(APPEND CODESTR ${vstrlen})
string(APPEND CODESTR "\;\n#ifdef __cplusplus\n}\n#endif")
file(WRITE ${BIN}/VERSION.cpp ${CODESTR})
