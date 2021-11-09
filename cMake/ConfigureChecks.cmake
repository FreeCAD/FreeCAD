
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)

#check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(GL/gl.h HAVE_GL_GL_H)

#check_include_file(iomanip.h HAVE_IOMANIP_H)
#check_include_file(limits.h HAVE_LIMITS_H)
#check_include_file(values.h HAVE_VALUES_H)
#check_include_file(float.h HAVE_FLOAT_H)
#check_include_file(inttypes.h HAVE_INTTYPES_H)

#check_include_file(libc.h HAVE_LIBC_H)
#check_include_file(memory.h HAVE_MEMORY_H)
#check_include_file(stdint.h HAVE_STDINT_H)
#check_include_file(stdlib.h HAVE_STDLIB_H)
#check_include_file(unistd.h HAVE_UNISTD_H)
#check_include_file(strings.h HAVE_STRINGS_H)
#check_include_file(string.h HAVE_STRING_H)

#check_include_file(bstring.h HAVE_BSTRING_H)
#check_include_file(siginfo.h HAVE_SIGINFO_H)
#check_include_file(bits/sigset.h HAVE_BITS_SIGSET_H)
#check_include_file(sys/dir.h HAVE_SYS_DIR_H)
#check_include_file(sys/filio.h HAVE_SYS_FILIO_H)
#check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
#check_include_file(sys/select.h HAVE_SYS_SELECT_H)
#check_include_file(sys/stat.h HAVE_SYS_STAT_H)
#check_include_file(sys/types.h HAVE_SYS_TYPES_H)

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
