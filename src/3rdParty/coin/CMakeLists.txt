cmake_minimum_required(VERSION 3.0...3.31)

set(COIN_MAJOR_VERSION 4)
set(COIN_MINOR_VERSION 0)
set(COIN_MICRO_VERSION 7)
set(COIN_BETA_VERSION)
set(COIN_VERSION ${COIN_MAJOR_VERSION}.${COIN_MINOR_VERSION}.${COIN_MICRO_VERSION}${COIN_BETA_VERSION})

project(Coin VERSION ${COIN_MAJOR_VERSION}.${COIN_MINOR_VERSION}.${COIN_MICRO_VERSION})
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

string(TIMESTAMP COIN_BUILD_YEAR "%Y")
math(EXPR COIN_SO_VERSION ${PROJECT_VERSION_MAJOR}*20)

# ############################################################################
# these will be removed after upgrading CMake minimum version
set(PROJECT_DESCRIPTION "A high-level 3D visualization library with Open Inventor 2.1 API")
# ############################################################################

if(POLICY CMP0054)
  # CMake version 3.14.5 warns when the policy is not set and uses OLD behaviour
  cmake_policy(SET CMP0054 NEW)
endif()

if(POLICY CMP0072)
  # get rid of OpenGL GLVND warning from CMake 3.11
  cmake_policy(SET CMP0072 NEW)
endif()

if(POLICY CMP0075)
  # get rid of CMAKE_REQUIRED_LIBRARIES warning from CMake 3.12.2
  cmake_policy(SET CMP0075 NEW)
endif()

# ############################################################################
# Prevent in-source builds, as they often cause severe build problems
# ############################################################################

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "${CMAKE_PROJECT_NAME} requires an out of source build. Please create a separate build directory and run 'cmake <path_to_${CMAKE_PROJECT_NAME}> [options]' from there.")
endif()

# ############################################################################
# Set minimum C++ standard needed to compile the Coin code
# ############################################################################
set( CMAKE_CXX_STANDARD 11 CACHE STRING "C++ standard used for the build" )
message( STATUS "Setting C++ standard: '${CMAKE_CXX_STANDARD}', which is the minimum needed to compile Coin.")

# ############################################################################
# Include necessary submodules
# ############################################################################

# Add path for Coin specific utility scripts
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

include(CheckCSourceRuns)
include(CheckCXXSourceCompiles)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckTypeSize)
include(CMakeDependentOption)
include(GNUInstallDirs)
include(FeatureSummary)
include(CoinCMakeUtilities)

# ############################################################################
# Provide options to customise the build
# ############################################################################

option(COIN_BUILD_SHARED_LIBS "Build shared library when ON (default), static when OFF." ON)
option(COIN_BUILD_TESTS "Build unit tests when ON (default), skips them when OFF." ON)
option(COIN_BUILD_DOCUMENTATION "Build and install API documentation (requires Doxygen)." OFF)
option(COIN_BUILD_AWESOME_DOCUMENTATION "Build and install API documentation in new modern style (requires Doxygen)." OFF)
option(COIN_BUILD_EXAMPLES "Build examples when ON (default), skips them when OFF." OFF)
cmake_dependent_option(COIN_BUILD_INTERNAL_DOCUMENTATION "Document internal code not part of the API." OFF "COIN_BUILD_DOCUMENTATION" OFF)
cmake_dependent_option(COIN_BUILD_DOCUMENTATION_MAN "Build Coin man pages." OFF "COIN_BUILD_DOCUMENTATION" OFF)
cmake_dependent_option(COIN_BUILD_DOCUMENTATION_QTHELP "Build QtHelp documentation." OFF "COIN_BUILD_DOCUMENTATION" OFF)
cmake_dependent_option(COIN_BUILD_DOCUMENTATION_CHM "Build compressed HTML help manual (requires HTML help compiler)" OFF "COIN_BUILD_DOCUMENTATION" OFF)

option(COIN_THREADSAFE "Enable thread safe render traversals." OFF)
option(HAVE_VRML97 "Enable VRML97 support." ON)
cmake_dependent_option(COIN_HAVE_JAVASCRIPT "Enable JavaScript capabilities for VRML97." ON "HAVE_VRML97" OFF)
option(HAVE_NODEKITS "Enable NodeKit support." ON)
cmake_dependent_option(HAVE_DRAGGERS "Enable Dragger support." ON "HAVE_NODEKITS" OFF)
cmake_dependent_option(HAVE_MANIPULATORS "Enable Manipulator support." ON "HAVE_NODEKITS" OFF)
option(HAVE_SOUND "Enable OpenAL sound support." ON)
option(HAVE_3DS_IMPORT_CAPABILITIES "Enable 3ds import capabilities." ON)
option(USE_EXTERNAL_EXPAT "Use system install of Expat." OFF)
option(USE_EXCEPTIONS "Compile with exceptions (g++ only)." ON)
option(USE_SUPERGLU "Use superglu library when ON, GLU implementation of platform when OFF (default)." OFF)

option(FONTCONFIG_RUNTIME_LINKING   "Enable FontConfig runtime linking when ON (default), disable when OFF." ON)
option(FREETYPE_RUNTIME_LINKING     "Enable FreeType runtime linking when ON (default), disable when OFF." ON)
option(LIBBZIP2_RUNTIME_LINKING     "Enable libBZip2 runtime linking when ON (default), disable when OFF." ON)
option(OPENAL_RUNTIME_LINKING       "Enable OpenAL runtime linking when ON (default), disable when OFF." ON)
option(SIMAGE_RUNTIME_LINKING       "Enable simage runtime linking when ON (default), disable when OFF" ON)
option(ZLIB_RUNTIME_LINKING         "Enable ZLib runtime linking when ON (default), disable when OFF." ON)
option(GLU_RUNTIME_LINKING          "Enable GLU runtime linking when ON (default), disable when OFF." ON)
option(SPIDERMONKEY_RUNTIME_LINKING "Enable SpiderMonkey runtime linking when ON (default), disable when OFF." ON)

option(COIN_VERBOSE "Add verbose debugging information during the configure process." OFF)
option(HAVE_MULTIPLE_VERSION "Forces versioned paths for includes and documentation when ON, usual behaviour otherwise." OFF)
option(COIN_USE_CPACK "If enabled the cpack subrepo is mandatory" OFF)

cmake_dependent_option(COIN_BUILD_MAC_FRAMEWORK "Build framework instead of dylib on Mac OS X when ON. Only valid if COIN_BUILD_SHARED_LIBS is ON." OFF "APPLE;NOT IOS;COIN_BUILD_SHARED_LIBS" OFF)
cmake_dependent_option(COIN_BUILD_MAC_X11 "Build for X11 on Mac OS X when ON. Default is OFF." OFF "APPLE" OFF)
cmake_dependent_option(COIN_BUILD_MAC_AGL "Build for AGL on Mac OS X when ON. Default is OFF." OFF "APPLE" OFF)

option(COIN_BUILD_GLX "Build for GLX on Linux when ON. Default is ON." ON)
option(COIN_BUILD_EGL "Build for EGL on Linux when ON. Default is ON." ON)

check_include_file_cxx(thread HAVE_STD_THREAD)
cmake_dependent_option(COIN_DEBUG_CHECK_THREAD "Enable thread check in several central Coin functions." OFF "HAVE_STD_THREAD" OFF)

report_prepare(
  COIN_BUILD_SHARED_LIBS
  COIN_BUILD_TESTS
  COIN_BUILD_DOCUMENTATION
  COIN_BUILD_INTERNAL_DOCUMENTATION
  COIN_BUILD_DOCUMENTATION_MAN
  COIN_BUILD_DOCUMENTATION_QTHELP
  COIN_BUILD_DOCUMENTATION_CHM
  COIN_BUILD_EGL
  COIN_BUILD_AWESOME_DOCUMENTATION
  COIN_THREADSAFE
  HAVE_VRML97
  COIN_HAVE_JAVASCRIPT
  HAVE_NODEKITS
  HAVE_DRAGGERS
  HAVE_MANIPULATORS
  HAVE_SOUND
  HAVE_3DS_IMPORT_CAPABILITIES
  USE_EXTERNAL_EXPAT
  USE_EXCEPTIONS
  USE_SUPERGLU
  FONTCONFIG_RUNTIME_LINKING
  FREETYPE_RUNTIME_LINKING
  LIBBZIP2_RUNTIME_LINKING
  OPENAL_RUNTIME_LINKING
  SIMAGE_RUNTIME_LINKING
  ZLIB_RUNTIME_LINKING
  GLU_RUNTIME_LINKING
  SPIDERMONKEY_RUNTIME_LINKING
  HAVE_MULTIPLE_VERSION
  IF_APPLE
    COIN_BUILD_MAC_FRAMEWORK
    COIN_BUILD_MAC_X11
    COIN_BUILD_MAC_AGL
  IF_MSVC
    COIN_BUILD_MSVC_STATIC_RUNTIME
    COIN_BUILD_SINGLE_LIB
    MSVC_VERSION
    COIN_BUILD_MSVC_MP
)

if(HAVE_MULTIPLE_VERSION)
  # Modifies the installation path for distributed includes, docs and datadir
  versionize(INCLUDEDIR DOCDIR DATADIR)
  dump_variable(
    CMAKE_INSTALL_DATADIR
    CMAKE_INSTALL_FULL_DATADIR
    CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_FULL_INCLUDEDIR
    CMAKE_INSTALL_DOCDIR
    CMAKE_INSTALL_FULL_DOCDIR
  )
else()
  set(CMAKE_INSTALL_DATADIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
endif()

# ############################################################################
# Find all necessary and optional Coin3D dependencies
# ############################################################################

# Fail early if one of the required packages cannot be found

# As not all CMake FindXXX modules and config packages provide target support,
# we should explicitly check for them on.

if(COIN_BUILD_MAC_X11)
  find_package(X11 REQUIRED)
  # On Mac OS X, GLX is provided as a separate OpenGL implementation, different
  # from the standard OpenGL framework which provides support for GLUT and native
  # Mac OS X applications.
  if(X11_FOUND)
    find_path(X11_GL_INCLUDE_PATH GL/glx.h ${X11_INC_SEARCH_PATH})
    if(NOT X11_GL_INCLUDE_PATH)
      message(FATAL_ERROR "Could not find GL/glx.h")
    endif()

    find_library(X11_GL_LIB GL ${X11_LIB_SEARCH_PATH})
    if(NOT X11_GL_LIB)
      message(FATAL_ERROR "Could not find libGL.dylib")
    endif()

    find_library(X11_GLU_LIB GLU ${X11_LIB_SEARCH_PATH})
    if(NOT X11_GLU_LIB)
      message(FATAL_ERROR "Could not find libGLU.dylib")
    endif()
    list(APPEND COIN_TARGET_INCLUDE_DIRECTORIES ${X11_INCLUDE_DIR} ${X11_GL_INCLUDE_PATH})
    list(APPEND COIN_TARGET_LINK_LIBRARIES ${X11_LIBRARIES} ${X11_GL_LIB} ${X11_GLU_LIB})
  endif()
else()
  if(UNIX AND NOT APPLE AND COIN_BUILD_GLX)
    find_package(X11 REQUIRED)
    list(APPEND COIN_TARGET_INCLUDE_DIRECTORIES ${X11_INCLUDE_DIR})
    list(APPEND COIN_TARGET_LINK_LIBRARIES ${X11_LIBRARIES})
  endif()
  find_package(OpenGL REQUIRED)
  if(OPENGL_FOUND)
    set(HAVE_OPENGL 1)
    set(OPENGL_SYSTEM_LIBRARY_NAME "${OPENGL_gl_LIBRARY}${CMAKE_SHARED_LIBRARY_SUFFIX}")
    if(COIN_BUILD_EGL)
      if (OpenGL_EGL_FOUND)
        set(HAVE_EGL 1)
        if (NOT TARGET OpenGL::EGL)
          list(APPEND COIN_TARGET_INCLUDE_DIRECTORIES ${OPENGL_EGL_INCLUDE_DIR})
          list(APPEND COIN_TARGET_LINK_LIBRARIES_GL ${OPENGL_egl_LIBRARY})
        else()
          list(APPEND COIN_TARGET_LINK_LIBRARIES_GL OpenGL::EGL)
        endif()
      endif()
    endif ()
    if(APPLE)
      set(GLU_IS_PART_OF_GL 1)
    endif()
    #include_directories("${OPENGL_INCLUDE_DIR}")
    if (NOT TARGET OpenGL::GL)
      list(APPEND COIN_TARGET_INCLUDE_DIRECTORIES ${OPENGL_INCLUDE_DIR})
      list(APPEND COIN_TARGET_LINK_LIBRARIES_GL ${OPENGL_gl_LIBRARY})
    else()
      list(APPEND COIN_TARGET_LINK_LIBRARIES_GL OpenGL::GL)
    endif()
    if(OpenGL_GLU_FOUND AND NOT COIN_BUILD_EGL)
      set(HAVE_GLU 1)
      if (NOT TARGET OpenGL::GLU)
        list(APPEND COIN_TARGET_INCLUDE_DIRECTORIES ${OPENGL_INCLUDE_DIR})
        list(APPEND COIN_TARGET_LINK_LIBRARIES_GL ${OPENGL_glu_LIBRARY})
      else()
        list(APPEND COIN_TARGET_LINK_LIBRARIES_GL OpenGL::GLU)
      endif()
    endif()
  endif()
endif()

list(APPEND COIN_TARGET_LINK_LIBRARIES ${COIN_TARGET_LINK_LIBRARIES_GL})

set(CMAKE_THREAD_PREFER_PTHREAD 1)
set(THREADS_PREFER_PTHREAD_FLAG 1)
find_package(Threads)
if(Threads_FOUND)
  set(HAVE_THREADS 1)
  if(CMAKE_USE_WIN32_THREADS_INIT)
    set(USE_W32THREAD 1)
    set(COIN_THREADID_TYPE DWORD)
  elseif(CMAKE_USE_PTHREADS_INIT)
    set(USE_PTHREAD 1)
    set(COIN_THREADID_TYPE pthread_t)
  endif()
  list(APPEND COIN_TARGET_LINK_LIBRARIES Threads::Threads)
  #pthreads does not provide pkg-config file, therefore not added here
endif()

if(USE_EXTERNAL_EXPAT)
  find_package(EXPAT)
  if(EXPAT_FOUND)
    set(HAVE_EXPAT 1)
    if(NOT TARGET EXPAT::EXPAT)
      add_library(EXPAT::EXPAT UNKNOWN IMPORTED)
      set_target_properties(EXPAT::EXPAT PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${EXPAT_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${EXPAT_INCLUDE_DIRS}")
    endif()
    list(APPEND COIN_TARGET_LINK_LIBRARIES EXPAT::EXPAT)
  endif()
else()
  if(NOT WIN32)
    set(XML_DEV_URANDOM 1)
  endif()
endif()

if(USE_SUPERGLU)
  find_package(superglu)
  if(superglu_FOUND)
    set(HAVE_SUPERGLU 1)
    set(GLU_RUNTIME_LINKING OFF)
    list(REMOVE_ITEM COIN_TARGET_LINK_LIBRARIES ${OPENGL_glu_LIBRARY})
    if(TARGET OpenGL::GLU)
      list(REMOVE_ITEM COIN_TARGET_LINK_LIBRARIES OpenGL::GLU)
    endif()
    list(APPEND COIN_TARGET_LINK_LIBRARIES superglu::GLU)
  endif()
endif()

#find_package(FLEX)
#if(FLEX_FOUND)
# set(HAVE_FLEX 1)
# #include_directories(${FLEX_INCLUDE_DIRS})
# #list(APPEND COIN_TARGET_INCLUDE_DIRECTORIES ${FLEX_INCLUDE_DIRS})
#endif()
#
#find_package(BISON)
#if(BISON_FOUND)
# set(HAVE_BISON 1)
# #include_directories(${BISON_INCLUDE_DIRS})
# #list(APPEND COIN_TARGET_INCLUDE_DIRECTORIES ${BISON_INCLUDE_DIRS})
#endif()

if(HAVE_SOUND)
  if(NOT OPENAL_RUNTIME_LINKING)
    find_package(OpenAL)
    if(OpenAL_FOUND)
      set(HAVE_OPENAL 1)
      # Checks specific OpenAL configurations
      set(CMAKE_REQUIRED_INCLUDES ${OPENGL_INCLUDE_DIR})
      check_include_file(AL/al.h HAVE_AL_AL_H)
      check_include_file(OpenAL/al.h HAVE_OPENAL_AL_H)
      set(CMAKE_REQUIRED_INCLUDES)
      if(NOT TARGET OpenAL::OpenAL)
        add_library(OpenAL::OpenAL UNKNOWN IMPORTED)
        set_target_properties(OpenAL::OpenAL PROPERTIES
          IMPORTED_LINK_INTERFACE_LANGUAGES "C"
          IMPORTED_LOCATION "${OPENAL_LIBRARY}"
          INTERFACE_INCLUDE_DIRECTORIES "${OPENAL_INCLUDE_DIR}")
      endif()
      list(APPEND COIN_TARGET_LINK_LIBRARIES OpenAL::OpenAL)
    endif()
  endif()
endif()

if(NOT ZLIB_RUNTIME_LINKING)
  find_package(ZLIB)
  if(ZLIB_FOUND)
    set(HAVE_ZLIB 1)
    if(NOT TARGET ZLIB::ZLIB)
      add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
      set_target_properties(ZLIB::ZLIB PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIRS}")

      if(ZLIB_LIBRARY_RELEASE)
        set_property(TARGET ZLIB::ZLIB APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(ZLIB::ZLIB PROPERTIES
          IMPORTED_LOCATION_RELEASE "${ZLIB_LIBRARY_RELEASE}")
      endif()

      if(ZLIB_LIBRARY_DEBUG)
        set_property(TARGET ZLIB::ZLIB APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(ZLIB::ZLIB PROPERTIES
          IMPORTED_LOCATION_DEBUG "${ZLIB_LIBRARY_DEBUG}")
      endif()

      if(NOT ZLIB_LIBRARY_RELEASE AND NOT ZLIB_LIBRARY_DEBUG)
        set_property(TARGET ZLIB::ZLIB APPEND PROPERTY
          IMPORTED_LOCATION "${ZLIB_LIBRARY}")
      endif()
    endif()
    list(APPEND COIN_TARGET_LINK_LIBRARIES ZLIB::ZLIB)
  endif()
endif()

if(NOT LIBBZIP2_RUNTIME_LINKING)
  find_package(BZip2)
  if(BZip2_FOUND)
    set(HAVE_BZIP2 1)
    if(NOT TARGET BZip2::BZip2)
      add_library(BZip2::BZip2 UNKNOWN IMPORTED)
      set_target_properties(BZip2::BZip2 PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        INTERFACE_INCLUDE_DIRECTORIES "${BZIP2_INCLUDE_DIRS}")

      if(BZIP2_LIBRARY_RELEASE)
        set_property(TARGET BZip2::BZip2 APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(BZip2::BZip2 PROPERTIES
          IMPORTED_LOCATION_RELEASE "${BZIP2_LIBRARY_RELEASE}")
      endif()

      if(BZIP2_LIBRARY_DEBUG)
        set_property(TARGET BZip2::BZip2 APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(BZip2::BZip2 PROPERTIES
          IMPORTED_LOCATION_DEBUG "${BZIP2_LIBRARY_DEBUG}")
      endif()

      if(NOT BZIP2_LIBRARY_RELEASE AND NOT BZIP2_LIBRARY_DEBUG)
        set_property(TARGET BZip2::BZip2 APPEND PROPERTY
          IMPORTED_LOCATION "${BZIP2_LIBRARY}")
      endif()
    endif()
    list(APPEND COIN_TARGET_LINK_LIBRARIES BZip2::BZip2)
  endif()
endif()

if(NOT FREETYPE_RUNTIME_LINKING)
  find_package(Freetype)
  if(Freetype_FOUND)
    set(HAVE_FREETYPE 1)
    if(NOT TARGET Freetype::Freetype)
      add_library(Freetype::Freetype UNKNOWN IMPORTED)
      set_target_properties(Freetype::Freetype PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIRS}")

      if(FREETYPE_LIBRARY_RELEASE)
        set_property(TARGET Freetype::Freetype APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(Freetype::Freetype PROPERTIES
          IMPORTED_LOCATION_RELEASE "${FREETYPE_LIBRARY_RELEASE}")
      endif()

      if(FREETYPE_LIBRARY_DEBUG)
        set_property(TARGET Freetype::Freetype APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(Freetype::Freetype PROPERTIES
          IMPORTED_LOCATION_DEBUG "${FREETYPE_LIBRARY_DEBUG}")
      endif()

      if(NOT FREETYPE_LIBRARY_RELEASE AND NOT FREETYPE_LIBRARY_DEBUG)
        set_target_properties(Freetype::Freetype PROPERTIES
          IMPORTED_LOCATION "${FREETYPE_LIBRARY}")
      endif()
    endif()
    set(HAVE_FREETYPE_H 1)
    list(APPEND COIN_TARGET_LINK_LIBRARIES Freetype::Freetype)
  endif()
endif()

if(NOT FONTCONFIG_RUNTIME_LINKING)
  find_package(Fontconfig)
  if(Fontconfig_FOUND)
    set(HAVE_FONTCONFIG 1)
    if(NOT TARGET Fontconfig::Fontconfig)
      add_library(Fontconfig::Fontconfig UNKNOWN IMPORTED)
      set_target_properties(Fontconfig::Fontconfig PROPERTIES
        IMPORTED_LOCATION "${FONTCONFIG_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${FONTCONFIG_COMPILE_OPTIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${FONTCONFIG_INCLUDE_DIR}"
      )
    endif()
    list(APPEND COIN_TARGET_LINK_LIBRARIES Fontconfig::Fontconfig)
  endif()
endif()

if(HAVE_VRML97 AND COIN_HAVE_JAVASCRIPT)
  if(NOT SPIDERMONKEY_RUNTIME_LINKING)
    find_package(SpiderMonkey)
    if(SPIDERMONKEY_FOUND)
      set(HAVE_SPIDERMONKEY_VIA_LINKTIME_LINKING 1)
      set(HAVE_SPIDERMONKEY_H 1)
      if(NOT TARGET SpiderMonkey::SpiderMonkey)
        add_library(SpiderMonkey::SpiderMonkey UNKNOWN IMPORTED)
        set_target_properties(SpiderMonkey::SpiderMonkey PROPERTIES
          IMPORTED_LOCATION "${SPIDERMONKEY_LIBRARIES}"
          INTERFACE_INCLUDE_DIRECTORIES "${SPIDERMONKEY_INCLUDE_DIRS}"
        )
      endif()
      list(APPEND COIN_TARGET_LINK_LIBRARIES SpiderMonkey::SpiderMonkey)
    endif()
  endif()
endif()

if(NOT SIMAGE_RUNTIME_LINKING)
  find_package(simage)
  if(simage_FOUND)
    set(HAVE_LIBSIMAGE 1)
    list(APPEND COIN_TARGET_LINK_LIBRARIES simage::simage)
  endif()
endif()

message(STATUS "${COIN_TARGET_INCLUDE_DIRECTORIES}")
message(STATUS "${COIN_TARGET_LINK_LIBRARIES}")

# ##########################################################################
# Setup build environment
# ##########################################################################

if(NOT CMAKE_BUILD_TYPE)
  # Has no effect for multi configuration generators (VisualStudio, Xcode).
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose type of build, options are Debug, Release, RelWithDebInfo, MinSizeRel." FORCE)
endif()
# Set common output directories for all targets built.
# First for the generic no-config case (e.g. with mingw)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# Second, for multi-config builds (e.g. msvc)
foreach (_config ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${_config} _config)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_config} "${CMAKE_BINARY_DIR}/lib")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_config} "${CMAKE_BINARY_DIR}/lib")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_config} "${CMAKE_BINARY_DIR}/bin")
endforeach()

# Checks for existence of specified types and set variable to the first type found
coin_find_int_type_with_size(COIN_INT8_T 1 "int8_t" "char")
coin_find_int_type_with_size(COIN_UINT8_T 1 "uint8_t" "u_int8_t" "unsigned char")
coin_find_int_type_with_size(COIN_INT16_T 2 "int16_t" "short" "int")
coin_find_int_type_with_size(COIN_UINT16_T 2 "uint16_t" "u_int16_t" "unsigned short" "unsigned int")
coin_find_int_type_with_size(COIN_INT32_T 4 "int32_t" "int" "long")
coin_find_int_type_with_size(COIN_UINT32_T 4 "uint32_t" "u_int32_t" "unsigned int" "unsigned long")
coin_find_int_type_with_size(COIN_INT64_T 8 "int64_t" "long" "int" "long long" "__int64")
coin_find_int_type_with_size(COIN_UINT64_T 8 "uint64_t" "u_int64_t" "unsigned long" "unsigned int" "unsigned long long" "unsigned __int64")
coin_find_int_type_with_size(COIN_INTPTR_T ${CMAKE_SIZEOF_VOID_P} "intptr_t" "int" "long" "long long" "__int64")
coin_find_int_type_with_size(COIN_UINTPTR_T ${CMAKE_SIZEOF_VOID_P} "uintptr_t" "u_intptr_t" "_W64 unsigned int" "unsigned int" "unsigned long" "u_int64_t" "unsigned long long" "unsigned __int64")

if(MSVC)
  option(COIN_BUILD_MSVC_STATIC_RUNTIME "Build against the static Microsoft Visual C runtime library." OFF)
  option(COIN_BUILD_SINGLE_LIB "Build only one library when ON, multiple when OFF." ON)
  if (MSVC_VERSION GREATER 1500 OR MSVC_VERSION EQUAL 1500)
    option(COIN_BUILD_MSVC_MP "Enable build with multiple processes in Visual Studio" ON)
  else()
    set(COIN_BUILD_MSVC_MP OFF CACHE BOOL "Compiler option /MP requires at least Visual Studio 2008 (VS9) or newer" FORCE)
  endif()
  if(COIN_BUILD_MSVC_MP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  endif ()

  # Enable C++ exception handling
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

  coin_msvc_link_static_crt(${COIN_BUILD_MSVC_STATIC_RUNTIME})
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES)
else()
  if(APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
  endif()
  option(COIN_BUILD_SINGLE_LIB "Build only one library when ON, multiple when OFF." OFF)
endif()

set(HAVE_GZDOPEN 1)
if(WIN32)
  set(HAVE_WIN32_API 1)
  #set(HAVE_WGL 1)
  if(MINGW)
    set(COIN_DEFAULT_SHARED_POSTFIX "")
    set(COIN_DEFAULT_STATIC_POSTFIX "")
  else()
    # On Windows the major version number is part of the library name
    set(COIN_DEFAULT_SHARED_POSTFIX ${PROJECT_VERSION_MAJOR})
    set(COIN_DEFAULT_STATIC_POSTFIX ${PROJECT_VERSION_MAJOR}s)
  endif()
  if(COIN_BUILD_SHARED_LIBS)
    set(COIN_DEFAULT_POSTFIX ${COIN_DEFAULT_SHARED_POSTFIX})
    set(COIN_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
    add_definitions(-DCOIN_MAKE_DLL)
  else()
    set(COIN_DEFAULT_POSTFIX ${COIN_DEFAULT_STATIC_POSTFIX})
    set(COIN_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
  endif()
  set(CMAKE_RELEASE_POSTFIX "${COIN_DEFAULT_POSTFIX}"
    CACHE STRING "Default filename postfix for libraries under configuration Release")
  set(CMAKE_MINSIZEREL_POSTFIX "${COIN_DEFAULT_POSTFIX}"
    CACHE STRING "Default filename postfix for libraries under configuration MinSizeRel")
  set(CMAKE_RELWITHDEBINFO_POSTFIX "${COIN_DEFAULT_POSTFIX}"
    CACHE STRING "Default filename postfix for libraries under configuration RelWithDebInfo")
  set(CMAKE_DEBUG_POSTFIX "${COIN_DEFAULT_POSTFIX}d"
    CACHE STRING "Default filename postfix for libraries under configuration Debug")

  set(COIN_RELEASE_SYSTEM_LIBRARY_NAME Coin${CMAKE_RELEASE_POSTFIX}${COIN_LIBRARY_SUFFIX})
  set(COIN_DEBUG_SYSTEM_LIBRARY_NAME Coin${CMAKE_DEBUG_POSTFIX}${COIN_LIBRARY_SUFFIX})
elseif(APPLE)
  if(POLICY CMP0042)
    # get rid of MACOSX_RPATH warning on Mac OS X from CMake 3.12.2
    cmake_policy(SET CMP0042 NEW)
  endif()

  if(POLICY CMP0068)
    # get rid of BUILD_WITH_INSTALL_RPATH warning on Mac OS X from CMake 3.12.2
    cmake_policy(SET CMP0068 NEW)
  endif()

  configure_file(Info.plist.in Info.plist)
  configure_file(version.plist.in version.plist)
  set(HAVE_GZDOPEN)

  list(APPEND COIN_TARGET_LINK_LIBRARIES "-framework CoreFoundation" "-framework CoreGraphics")

  set(COIN_MAC_FRAMEWORK ${COIN_BUILD_MAC_FRAMEWORK})
  set(COIN_MAC_FRAMEWORK_NAME "Inventor")
  set(COIN_MAC_FRAMEWORK_PREFIX "${CMAKE_INSTALL_PREFIX}")
  set(COIN_MAC_FRAMEWORK_VERSION "D") # Coin 4 will be "D", to allow parallel installations of Coin3 (if that compiles!)
  set(PACKAGING_FRAMEWORK_NAME ${COIN_MAC_FRAMEWORK_NAME})

  # package config *.pc entries
  if(COIN_BUILD_MAC_FRAMEWORK)
    set(COIN_PKGCFG_FRAMEWORK_DIR "${COIN_MAC_FRAMEWORK_PREFIX}/${COIN_MAC_FRAMEWORK_NAME}.framework")
    set(COIN_PKGCFG_INCLUDE_DIR "\${frameworkdir}/Versions/${COIN_MAC_FRAMEWORK_VERSION}/Resources/include")
    set(COIN_EXTRA_CPPFLAGS "-F${COIN_MAC_FRAMEWORK_PREFIX}")
    set(COIN_EXTRA_FP_LDFLAGS "-Wl,-F${COIN_MAC_FRAMEWORK_PREFIX}")
    set(COIN_EXTRA_LDFLAGS "${COIN_EXTRA_FP_LDFLAGS} -Wl,-framework,${COIN_MAC_FRAMEWORK_NAME} -Wl,-framework,OpenGL")
  else()
    set(COIN_EXTRA_LDFLAGS " -lCoin -Wl,-framework,OpenGL")
  endif()

  # Let's enable all OS X specific code
  set(COIN_MACOSX_FRAMEWORK ${COIN_BUILD_MAC_FRAMEWORK})
  set(COIN_MACOS_10 1)
  set(COIN_MACOS_10_3 1)

  set(COIN_RELEASE_SYSTEM_LIBRARY_NAME "$<TARGET_FILE_NAME:Coin>")
  set(COIN_DEBUG_SYSTEM_LIBRARY_NAME "$<TARGET_FILE_NAME:Coin>")
else()
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  set(COIN_RELEASE_SYSTEM_LIBRARY_NAME "$<TARGET_FILE_NAME:Coin>")
  set(COIN_DEBUG_SYSTEM_LIBRARY_NAME "$<TARGET_FILE_NAME:Coin>")
endif()

check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stddef.h HAVE_STDDEF_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(io.h HAVE_IO_H)
check_include_file(ieeefp.h HAVE_IEEEFP_H)
check_include_file(time.h HAVE_TIME_H)
check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)

check_include_file(windows.h HAVE_WINDOWS_H)
if(HAVE_WINDOWS_H)
  check_include_file(direct.h HAVE_DIRECT_H)
  check_include_files("windows.h;tlhelp32.h" HAVE_TLHELP32_H)
  check_symbol_exists(_fpclass float.h HAVE__FPCLASS)
  check_symbol_exists(_splitpath stdlib.h HAVE__SPLITPATH)
  check_symbol_exists(LoadLibrary windows.h HAVE_WINDLL_RUNTIME_BINDING)
  check_symbol_exists(GetEnvironmentVariable windows.h HAVE_GETENVIRONMENTVARIABLE)
  check_symbol_exists(QueryPerformanceCounter windows.h HAVE_QUERYPERFORMANCECOUNTER)
else()
  check_include_file(libgen.h HAVE_LIBGEN_H)
  check_symbol_exists(gettimeofday sys/time.h HAVE_GETTIMEOFDAY)
  check_include_file(mach-o/dyld.h HAVE_MACH_O_DYLD_H)
  if(HAVE_MACH_O_DYLD_H)
    check_symbol_exists(NSLookupAndBindSymbol mach-o/dyld.h HAVE_DYLD_RUNTIME_BINDING)
  endif()
  # On FreeBSD, NetBSD and OpenBSD there is no libdl, as the required
  # functionality is already built into libc. So use ${CMAKE_DL_LIBS} instead.
  check_include_file(dlfcn.h HAVE_DLFCN_H)
  if(HAVE_DLFCN_H)
    set(HAVE_DL_LIB 1)
    list(APPEND COIN_TARGET_LINK_LIBRARIES ${CMAKE_DL_LIBS})
  endif()
  check_library_exists(dld shl_load "" HAVE_DLD_LIB)
endif()

set(CMAKE_REQUIRED_INCLUDES ${COIN_TARGET_INCLUDE_DIRECTORIES})
if(WIN32)
  set(CMAKE_REQUIRED_LIBRARIES ${COIN_TARGET_LINK_LIBRARIES} gdi32)
  check_cxx_source_compiles("
    #include <windows.h>
    #include <GL/gl.h>
    int main() { (void)wglCreateContext(0L); return 0; }
  " HAVE_WGL)
elseif(APPLE)
  set(CMAKE_REQUIRED_LIBRARIES ${COIN_TARGET_LINK_LIBRARIES})
  if(COIN_BUILD_MAC_X11)
    check_cxx_source_compiles("
      #include <GL/gl.h>
      #include <GL/glx.h>
      int main() { (void)glXChooseVisual(0L, 0, 0L); glEnd(); return 0; }
    " HAVE_GLX)
  else()
    if(COIN_BUILD_MAC_AGL)
      set(CMAKE_REQUIRED_LIBRARIES ${COIN_TARGET_LINK_LIBRARIES} "-Wl,-framework,ApplicationServices" "-Wl,-framework,AGL" "-Wl,-framework,Carbon")
      check_cxx_source_compiles("
        #include <AGL/agl.h>
        #define __CARBONSOUND__
        #include <Carbon/Carbon.h>
        int main() { AGLContext ctx; WindowRef wref = aglGetWindowRef(ctx); HIViewRef href = HIViewGetRoot(wref); return 0; }
      " HAVE_AGL)
      check_cxx_source_compiles("
        #include <AGL/agl.h>
        int main() { AGLPBuffer pbuffer; return 0; }
      " HAVE_AGL_PBUFFER)
    endif()
    set(CMAKE_REQUIRED_LIBRARIES ${COIN_TARGET_LINK_LIBRARIES})
    check_cxx_source_compiles("
      #include <OpenGL/OpenGL.h>
      int main() { CGLGetCurrentContext(); return 0; }
    " HAVE_CGL)
    check_cxx_source_compiles("
      #include <OpenGL/OpenGL.h>
      int main() { CGLPBufferObj pbuffer; return 0; }
    " HAVE_CGL_PBUFFER)
  endif()
endif()

if(UNIX AND NOT APPLE AND COIN_BUILD_GLX)
  if (NOT TARGET OpenGL::GLX)
    if (OPENGL_glx_LIBRARY)
      list(APPEND COIN_TARGET_LINK_LIBRARIES ${OPENGL_glx_LIBRARY})
    endif()
  else()
    list(APPEND COIN_TARGET_LINK_LIBRARIES OpenGL::GLX)
  endif()
  set(CMAKE_REQUIRED_LIBRARIES ${COIN_TARGET_LINK_LIBRARIES})
  check_cxx_source_compiles("
    #include <GL/gl.h>
    #include <GL/glx.h>
    int main() { (void)glXChooseVisual(0L, 0, 0L); glEnd(); return 0; }
  " HAVE_GLX)
endif()

# Checks specific OpenGL configurations
if(HAVE_WINDOWS_H)
  check_include_files("windows.h;GL/gl.h" HAVE_GL_GL_H)
  check_include_files("windows.h;GL/glu.h" HAVE_GL_GLU_H)
  check_include_files("windows.h;GL/gl.h;GL/glext.h" HAVE_GL_GLEXT_H)
elseif(APPLE AND NOT COIN_BUILD_MAC_X11)
  check_include_file(OpenGL/gl.h HAVE_OPENGL_GL_H)
  check_include_file(OpenGL/glu.h HAVE_OPENGL_GLU_H)
  check_include_file(OpenGL/glext.h HAVE_OPENGL_GLEXT_H)
  check_include_file(OpenGL/CGLCurrent.h HAVE_OPENGL_CGLCURRENT_H)
else()
  check_include_files("GL/gl.h" HAVE_GL_GL_H)
  check_include_files("GL/glu.h" HAVE_GL_GLU_H)
  check_include_files("GL/gl.h;GL/glext.h" HAVE_GL_GLEXT_H)
endif()

set(CMAKE_REQUIRED_INCLUDES)
set(CMAKE_REQUIRED_LIBRARIES)

if(HAVE_WINDOWS_H)
  set(SIM_INCLUDE_WINDOWS_H "#include <windows.h>")
else()
  set(SIM_INCLUDE_WINDOWS_H "/* #include <windows.h> - not needed on system */")
endif()
if(HAVE_GL_GL_H)
  set(SIM_INCLUDE_GL_H "#include <GL/gl.h>")
elseif(HAVE_OPENGL_GL_H)
  set(SIM_INCLUDE_GL_H "#include <OpenGL/gl.h>")
else()
  set(SIM_INCLUDE_GL_H "#error \"don't know how to include gl.h header\"")
endif()
if(HAVE_SUPERGLU)
  set(SIM_INCLUDE_GLU_H "/* #include <GL/glu.h> - not used, Coin linked with embedded SuperGLU */")
elseif(HAVE_GL_GLU_H)
  set(SIM_INCLUDE_GLU_H "#include <GL/glu.h>")
elseif(HAVE_OPENGL_GLU_H)
  set(SIM_INCLUDE_GLU_H "#include <OpenGL/glu.h>")
else()
  set(SIM_INCLUDE_GLU_H "/* #include <GL/glu.h> - not found on system */")
endif()
if(HAVE_GL_GLEXT_H)
  set(SIM_INCLUDE_GLEXT_H "#include <GL/glext.h>")
elseif(HAVE_OPENGL_GLEXT_H)
  set(SIM_INCLUDE_GLEXT_H "#include <OpenGL/glext.h>")
else()
  set(SIM_INCLUDE_GLEXT_H "/* #include <GL/glext.h> - not found on system */")
endif()

check_symbol_exists(va_copy stdarg.h HAVE_VA_COPY_MACRO)
check_symbol_exists(strncasecmp string.h HAVE_STRNCASECMP)
check_symbol_exists(memmove string.h HAVE_MEMMOVE)
check_symbol_exists(bcopy strings.h HAVE_BCOPY)
check_symbol_exists(fstat "sys/stat.h;sys/types.h" HAVE_FSTAT)
check_symbol_exists(localtime_s time.h HAVE_LOCALTIME_S)
check_symbol_exists(localtime_r time.h HAVE_LOCALTIME_R)
if(NOT HAVE_FSTAT)
  check_symbol_exists(_fstat "sys/stat.h;sys/types.h" HAVE__FSTAT)
endif()
check_symbol_exists(ftime "sys/types.h;sys/timeb.h" HAVE_FTIME)
if(NOT HAVE_FTIME)
  check_symbol_exists(_ftime "sys/types.h;sys/timeb.h" HAVE__FTIME)
endif()
check_symbol_exists(getcwd unistd.h HAVE_GETCWD)
if(NOT HAVE_GETCWD)
  check_symbol_exists(_getcwd direct.h HAVE__GETCWD)
endif()
check_symbol_exists(isinf math.h HAVE_ISINF)
check_symbol_exists(isnan math.h HAVE_ISNAN)
if(NOT HAVE_ISNAN)
  check_symbol_exists(_isnan float.h HAVE__ISNAN)
endif()
check_symbol_exists(finite math.h HAVE_FINITE)
if(NOT HAVE_FINITE)
  check_symbol_exists(_finite float.h HAVE__FINITE)
endif()
check_symbol_exists(ilogb math.h HAVE_ILOGB)
if(NOT HAVE_ILOGB)
  check_symbol_exists(_logb float.h HAVE__LOGB)
endif()
check_symbol_exists(vsnprintf "stdio.h;stdarg.h" HAVE_VSNPRINTF)
if(NOT HAVE_VSNPRINTF)
  check_symbol_exists(_vsnprintf "stdio.h;stdarg.h" HAVE__VSNPRINTF)
endif()
check_symbol_exists(__builtin_expect assert.h HAVE___BUILTIN_EXPECT)
set(HAVE_ASSERT_WITH_BUILTIN_EXPECT ${HAVE___BUILTIN_EXPECT})

check_symbol_exists(__func__ "" FUNC)
check_symbol_exists(__PRETTY_FUNCTION__ "" PRETTY_FUNCTION)
check_symbol_exists(__FUNCTION__ "" FUNCTION)
if(FUNC)
  set(HAVE_C_COMPILER_FUNCTION_NAME_VAR __func__)
  set(HAVE_CPP_COMPILER_FUNCTION_NAME_VAR __func__)
elseif(PRETTY_FUNCTION)
  set(HAVE_C_COMPILER_FUNCTION_NAME_VAR __PRETTY_FUNCTION__)
  set(HAVE_CPP_COMPILER_FUNCTION_NAME_VAR __PRETTY_FUNCTION__)
elseif(FUNCTION)
  set(HAVE_C_COMPILER_FUNCTION_NAME_VAR __FUNCTION__)
  set(HAVE_CPP_COMPILER_FUNCTION_NAME_VAR __FUNCTION__)
endif()

if(NOT HAVE_UNISTD_H)
  add_definitions(-DYY_NO_UNISTD_H)
endif()

check_struct_has_member("struct timespec" tv_nsec pthread.h HAVE_PTHREAD_TIMESPEC_NSEC)

# Setting SIM_TIMEVAL_TV_SEC_T & SIM_TIMEVAL_TV_USEC_T for determining correct variable size
if(HAVE_TIME_H)
  set(CMAKE_EXTRA_INCLUDE_FILES time.h)
endif()
if(WIN32)
  set(CMAKE_EXTRA_INCLUDE_FILES "${CMAKE_EXTRA_INCLUDE_FILES};WinSock2.h")
else()
  set(CMAKE_EXTRA_INCLUDE_FILES "${CMAKE_EXTRA_INCLUDE_FILES};sys/time.h")
endif()

check_type_size("((struct timeval*)0)->tv_sec" TIMEVAL_TV_SEC)
if(HAVE_TIMEVAL_TV_SEC)
  coin_find_int_type_with_size(SIM_TIMEVAL_TV_SEC_T ${TIMEVAL_TV_SEC} "time_t" "long")
  if(NOT SIM_TIMEVAL_TV_SEC_T)
    message(FATAL_ERROR "TIMEVAL_TV_SEC: Neither of checked types (time_t, long) were of requested size ${TIMEVAL_TV_SEC}")
  endif()
endif()

check_type_size("((struct timeval*)0)->tv_usec" TIMEVAL_TV_USEC)
if(HAVE_TIMEVAL_TV_USEC)
  coin_find_int_type_with_size(SIM_TIMEVAL_TV_USEC_T ${TIMEVAL_TV_USEC} "suseconds_t" "long" "long long")
  if(NOT SIM_TIMEVAL_TV_USEC_T)
    message(FATAL_ERROR "TIMEVAL_TV_USEC: Neither of checked types (suseconds_t, long, long long) were of requested size ${TIMEVAL_TV_USEC}")
  endif()
endif()
set(CMAKE_EXTRA_INCLUDE_FILES)

if(HAVE_WINDLL_RUNTIME_BINDING OR HAVE_DYLD_RUNTIME_BINDING OR HAVE_DLD_LIB OR HAVE_DL_LIB)
  set(HAVE_DYNAMIC_LINKING 1)
endif()

check_cxx_source_compiles("
  #define TEST_QUOTE(str) #str
  int main(void) { char xx[] = TEST_QUOTE(str) \"xxx\"; return 0; }
" HAVE_HASH_QUOTING)

if(NOT HAVE_HASH_QUOTING)
  check_cxx_source_compiles("
    #define TEST_QUOTE(str) \"str\"
    int main(void) { char xx[] = (TEST_QUOTE(\"str\") \"xxx\"; return 0; }
  " HAVE_APOSTROPHES_QUOTING)
endif()

if(HAVE_3DS_IMPORT_CAPABILITIES)
  set(FEAT_HAVE_3DS 1)
else()
  set(FEAT_HAVE_3DS 0)
endif()
if(HAVE_VRML97)
  set(FEAT_HAVE_VRML97 1)
else()
  set(FEAT_HAVE_VRML97 0)
endif()
if(HAVE_SOUND)
  set(FEAT_HAVE_SOUND 1)
else()
  set(FEAT_HAVE_SOUND 0)
endif()
if(HAVE_SUPERGLU)
  set(FEAT_HAVE_SUPERGLU 1)
else()
  set(FEAT_HAVE_SUPERGLU 0)
endif()
if(HAVE_THREADS)
  set(FEAT_HAVE_THREADS 1)
else()
  set(FEAT_HAVE_THREADS 0)
endif()
if(COIN_THREADSAFE)
  set(FEAT_HAVE_SAFETHREAD 1)
else()
  set(FEAT_HAVE_SAFETHREAD 0)
endif()

# ############################################################################
# Setup targets in subdirectories
# ############################################################################

# COIN_DOCUMENTATION_FILES is filled with all source files.
unset(COIN_DOCUMENTATION_FILES CACHE)
# COIN_INTERNAL_DOCUMENTATION_FILES is filled with source files that are not part of the public API.
# If COIN_BUILD_INTERNAL_DOCUMENTATION is YES then these files are added to DOXYGEN_INPUT, otherwise
# they are put in DOXYGEN_EXCLUDE.
unset(COIN_INTERNAL_DOCUMENTATION_FILES CACHE)

add_subdirectory(include)
add_subdirectory(data)
add_subdirectory(src)

if (COIN_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(COIN_BUILD_TESTS)
  enable_testing()
  add_subdirectory(testsuite)
endif()

# add_feature_info(ThreadSafe COIN_THREADSAFE "Thread safe render traversals.")
# add_feature_info(VRML97 HAVE_VRML97 "VRML97 support.")
# add_feature_info(JavaScript COIN_HAVE_JAVASCRIPT "JavaScript capabilities.")
# add_feature_info(NodeKits HAVE_NODEKITS "NodeKit support.")
# add_feature_info(Draggers HAVE_DRAGGERS "Dragger support.")
# add_feature_info(Manipulators HAVE_MANIPULATORS "Manipulator support.")
# add_feature_info(Sound HAVE_SOUND "Sound support.")
# add_feature_info(3dsImport HAVE_3DS_IMPORT_CAPABILITIES "3ds import capabilities.")
# add_feature_info(Expat USE_EXTERNAL_EXPAT "Use system install of Expat.")
# add_feature_info(Exceptions USE_EXCEPTIONS "Compile with exceptions (g++ only).")
# add_feature_info(SuperGLU USE_SUPERGLU "Use superglu library.")
# feature_summary(WHAT ALL
#                 INCLUDE_QUIET_PACKAGES
#                 DESCRIPTION "Enabled Features:"
#                 VAR enabledFeaturesText)
# message(STATUS "${enabledFeaturesText}")

# ############################################################################
# New CPACK section, please see the README file inside cpack.d directory.
if (COIN_USE_CPACK)
  add_subdirectory(cpack.d)
endif()
