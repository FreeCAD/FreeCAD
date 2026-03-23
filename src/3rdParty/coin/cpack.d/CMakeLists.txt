if (POLICY CMP0070)
  cmake_policy(SET CMP0070 NEW)
endif()

# Dumps all the variables listed in PACKAGE_OPTIONS in a text file named
# <lower case build type>-options.txt
#
# The report is installed in 
# "${CMAKE_INSTALL_INFODIR}/${PROJECT_NAME}${PROJECT_VERSION_MAJOR}"
# and packaged with the runtime component.
#
# The following helper function can be used in the client projects to populate
# the PACKAGE_OPTIONS list.
#
# function(report_prepare)
#   set(multiValueArgs IF_APPLE IF_WIN32)
#   cmake_parse_arguments(REPORT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
#   if (REPORT_IF_APPLE AND APPLE) 
#     list(APPEND res ${REPORT_IF_APPLE})
#   endif()
#   if (REPORT_IF_WIN32 AND WIN32) 
#     list(APPEND res ${REPORT_IF_WIN32})
#   endif()
#   list(APPEND res ${REPORT_UNPARSED_ARGUMENTS})
#   list(APPEND PACKAGE_OPTIONS ${res} )
#   set(PACKAGE_OPTIONS "${PACKAGE_OPTIONS}" PARENT_SCOPE)
# endfunction(report_prepare)
# 
# Invokation example:
#
# report_prepare(
#   SIMAGE_BUILD_SHARED_LIBS
#   SIMAGE_BUILD_DOCUMENTATION
#   SIMAGE_USE_QIMAGE
#   SIMAGE_USE_QT5
#   SIMAGE_LIBSNDFILE_SUPPORT
#   SIMAGE_OGGVORBIS_SUPPORT
#   SIMAGE_QIMAGE_SUPPORT
#   SIMAGE_EPS_SUPPORT
#   SIMAGE_MPEG2ENC_SUPPORT
#   SIMAGE_PIC_SUPPORT
#   SIMAGE_RGB_SUPPORT
#   SIMAGE_TGA_SUPPORT
#   SIMAGE_XWD_SUPPORT
#   IF_WIN32 
#     SIMAGE_USE_AVIENC SIMAGE_USE_GDIPLUS SIMAGE_AVIENC_SUPPORT SIMAGE_GDIPLUS_SUPPORT
#   IF_APPLE 
#     SIMAGE_USE_CGIMAGE SIMAGE_USE_QUICKTIME SIMAGE_CGIMAGE_SUPPORT SIMAGE_QUICKTIME_SUPPORT
# )
function(report_write)
  list(APPEND res "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
  list(APPEND res "OPTION_PKG_DEBUGINFO=${OPTION_PKG_DEBUGINFO}")
  foreach(O IN LISTS PACKAGE_OPTIONS)
    list(APPEND res "${O}=${${O}}")
  endforeach()
  list(SORT res)
  set(report_name "build-options.txt")
  file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${report_name} CONTENT "$<JOIN:${res},\n>")
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${report_name} DESTINATION "${CMAKE_INSTALL_INFODIR}/${PROJECT_NAME}${PROJECT_VERSION_MAJOR}" COMPONENT runtime)
endfunction(report_write)

# retrieves git project infos setting the following variables:
#
# PACKAGE_BUILD_NUMBER number of commits since last tag
function(git_info)
  # just in case git package under msys2 is a python2 script.
  # As execute_process on MSYS2/MinGW uses CMD.exe as command interpreter
  # this cannot be started.
  set(GIT_COMMAND ${GIT_EXECUTABLE})
  if (MSYS OR MINGW)
    execute_process(
      COMMAND file ${GIT_EXECUTABLE}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      OUTPUT_STRIP_TRAILING_WHITESPACE
      OUTPUT_VARIABLE git_file_type
    )
    if ("${git_file_type}" MATCHES "Python script")
      set(GIT_COMMAND python2 ${GIT_EXECUTABLE})
    endif()
  endif()
  execute_process(
    COMMAND ${GIT_COMMAND} describe --tags --long
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE VERSION_TAG
  )
  string(REGEX REPLACE "^[a-zA-Z]*-[0-9]*.[0-9]*.[0-9]*-" "" PACKAGE_BUILD_NUMBER "${VERSION_TAG}")
  string(REGEX REPLACE "-.*$" "" PACKAGE_BUILD_NUMBER "${PACKAGE_BUILD_NUMBER}")
  set(PACKAGE_BUILD_NUMBER "${PACKAGE_BUILD_NUMBER}" PARENT_SCOPE)
endfunction(git_info)

option(OPTION_PKG_DEBUGINFO "Build additional debug information package[s]")
if (OPTION_PKG_DEBUGINFO AND NOT CMAKE_BUILD_TYPE STREQUAL Debug AND NOT CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
  message(WARNING "OPTION_PKG_DEBUGINFO is ON but required debug info could be missing in '${CMAKE_BUILD_TYPE}' builds")
endif()

# get all found components
get_cmake_property(PACKAGE_COMPONENTS_ALL COMPONENTS)

find_program(GIT_EXECUTABLE NAMES git)
if (GIT_EXECUTABLE)
  git_info()
  message(STATUS "Git found at ${GIT_EXECUTABLE}, PACKAGE_BUILD_NUMBER set to ${PACKAGE_BUILD_NUMBER}")
else()
  set(PACKAGE_BUILD_NUMBER 1)
  message(WARNING "Git not found, PACKAGE_BUILD_NUMBER set to ${PACKAGE_BUILD_NUMBER}")
endif()
  
file(GLOB CONFIGS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cmake.in)

if(APPLE)
  configure_file(DMGSetup.scpt.in DMGSetup.scpt @ONLY)
  configure_file("${CMAKE_SOURCE_DIR}/COPYING" "${CMAKE_BINARY_DIR}/COPYING.txt" COPYONLY)
  configure_file("${CMAKE_SOURCE_DIR}/README"  "${CMAKE_BINARY_DIR}/README.txt"  COPYONLY)
  configure_file("${CMAKE_SOURCE_DIR}/packaging/macosx/Welcome.rtf"  "${CMAKE_BINARY_DIR}/WELCOME.rtf"  COPYONLY)
endif()

foreach(FILE ${CONFIGS})
  string(REGEX REPLACE ".cmake.in$" ".cmake" CFG ${FILE})
  configure_file(${FILE} ${CFG} @ONLY)
endforeach()

report_write()
