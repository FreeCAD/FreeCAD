
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)

check_include_file(GL/gl.h HAVE_GL_GL_H)

# i/o streams
check_include_file_cxx(istream HAVE_ISTREAM)
check_include_file_cxx(ostream HAVE_OSTREAM)
check_include_file_cxx(fstream HAVE_FSTREAM)
check_include_file_cxx(sstream HAVE_SSTREAM)
check_include_file_cxx(ios HAVE_IOS)
check_include_file_cxx(iostream HAVE_IOSTREAM)
check_include_file_cxx(iomanip HAVE_IOMANIP)

include(TestForANSIStreamHeaders)
IF(NOT CMAKE_NO_ANSI_STREAM_HEADERS)
	SET(HAVE_STD_IOSTREAM 1)
	SET(USE_STD_IOSTREAM 1)
ENDIF(NOT CMAKE_NO_ANSI_STREAM_HEADERS)

include(TestForSTDNamespace)
IF(NOT CMAKE_NO_ANSI_STRING_STREAM)
	SET(HAVE_NAMESPACES 1)
ENDIF(NOT CMAKE_NO_ANSI_STRING_STREAM)

SET(HAVE_QGLFORMAT_EQ_OP 1)
SET(HAVE_QGLFORMAT_SETOVERLAY 1)
SET(HAVE_QGLWIDGET_SETAUTOBUFFERSWAP 1)
SET(HAVE_QT_KEYPAD_DEFINE 1)
SET(HAVE_QWIDGET_SHOWFULLSCREEN 1)


file(WRITE ${CMAKE_BINARY_DIR}/backtrace.cpp
         "#include <cstddef>\n"
         "#include <execinfo.h>\n\n"
         "int main() {\n"
         "    void *callstack[128];\n"
         "    size_t nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);\n"
         "    size_t nFrames = backtrace(callstack, nMaxFrames);\n"
         "    char **symbols = backtrace_symbols(callstack, nFrames);\n"
         "    return 0;\n"
         "}"
)

try_compile(
  RESULT_VAR
    ${CMAKE_BINARY_DIR}
  SOURCES
    ${CMAKE_BINARY_DIR}/backtrace.cpp
)

SET(HAVE_BACKTRACE_SYMBOLS ${RESULT_VAR})
