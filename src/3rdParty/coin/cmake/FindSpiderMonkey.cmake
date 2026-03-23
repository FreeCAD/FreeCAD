# - Try to find the SpiderMonkey library
# Once done this will define
#  SPIDERMONKEY_FOUND - True if SpiderMonkey found.
#  SPIDERMONKEY_INCLUDE_DIRS - The SpiderMonkey include directories.
#  SPIDERMONKEY_LIBRARIES - The libraries needed to use SpiderMonkey
#  SPIDERMONKEY_DEFINITIONS - Compiler switches required for using SpiderMonkey
#  SPIDERMONKEY_VERSION - Version of the SpiderMonkey library

if(SPIDERMONKEY_INCLUDE_DIRS AND SPIDERMONKEY_LIBRARIES)
  set(SpiderMonkey_FIND_QUIETLY TRUE)
endif()

if(SPIDERMONKEY_ROOT)
  set(include_PATHS PATHS "${SPIDERMONKEY_ROOT}/include" NO_DEFAULT_PATH)
  set(library_PATHS PATHS "${SPIDERMONKEY_ROOT}/lib" NO_DEFAULT_PATH)
else()
  set(include_PATHS PATHS /usr/include /usr/local/include)
endif()

find_path(
  SPIDERMONKEY_INCLUDE_DIR
  NAMES jsapi.h
  ${include_PATHS}
  PATH_SUFFIXES
    js-1.7.0 js
)

string(REGEX REPLACE "^.*js-([0-9]+[.]?[0-9]?[.]?[0-9]?).*" "\\1" SPIDERMONKEY_VERSION ${SPIDERMONKEY_INCLUDE_DIR})
# dedicated workaround to find version string when not embedded in the includedir (<= 1.8.5)
if(${SPIDERMONKEY_VERSION} STREQUAL ${SPIDERMONKEY_INCLUDE_DIR})
  if(EXISTS ${SPIDERMONKEY_INCLUDE_DIR}/jsversion.h)
    set(VERS_FILE "${SPIDERMONKEY_INCLUDE_DIR}/jsversion.h")
  elseif(EXISTS ${SPIDERMONKEY_INCLUDE_DIR}/jsconfig.h)
    set(VERS_FILE "${SPIDERMONKEY_INCLUDE_DIR}/jsconfig.h")
  else()
    message(ERROR "unknown location of the header with the version string")
  endif()
  file(STRINGS ${VERS_FILE} VERS_STRING REGEX "#define JS_VERSION .*")
  string(REGEX REPLACE "#define JS_VERSION \(.*\)" "\\1" SPIDERMONKEY_VERSION ${VERS_STRING})
endif()

find_library(
  SPIDERMONKEY_LIBRARY
  NAMES mozjs-${SPIDERMONKEY_VERSION} mozjs${SPIDERMONKEY_VERSION} mozjs js js${SPIDERMONKEY_VERSION}
  ${library_PATHS}
)


include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set SPIDERMONKEY_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
  SpiderMonkey
  DEFAULT_MSG
  SPIDERMONKEY_LIBRARY
  SPIDERMONKEY_INCLUDE_DIR)

mark_as_advanced(SPIDERMONKEY_INCLUDE_DIR SPIDERMONKEY_LIBRARY)

set(SPIDERMONKEY_LIBRARIES ${SPIDERMONKEY_LIBRARY})
set(SPIDERMONKEY_INCLUDE_DIRS ${SPIDERMONKEY_INCLUDE_DIR})

