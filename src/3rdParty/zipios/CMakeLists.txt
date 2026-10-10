# SPDX-License-Identifier: LGPL-2.1-or-later

# Zipios 2.3.4 from https://github.com/Zipios/Zipios, the zipios/ and src/ folders unchanged

set(ZIPIOS_VERSION_MAJOR 2)
set(ZIPIOS_VERSION_MINOR 3)
set(ZIPIOS_VERSION_PATCH 4)
set(ZIPIOS_VERSION 2.3.4 CACHE INTERNAL "The version of the bundled Zipios")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/zipios/zipios-config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/zipios/zipios-config.hpp
)

add_library(zipios STATIC
    src/backbuffer.cpp
    src/collectioncollection.cpp
    src/deflateoutputstreambuf.cpp
    src/directorycollection.cpp
    src/directoryentry.cpp
    src/dosdatetime.cpp
    src/filecollection.cpp
    src/fileentry.cpp
    src/filepath.cpp
    src/filterinputstreambuf.cpp
    src/filteroutputstreambuf.cpp
    src/gzipoutputstream.cpp
    src/gzipoutputstreambuf.cpp
    src/inflateinputstreambuf.cpp
    src/streamentry.cpp
    src/virtualseeker.cpp
    src/zipcentraldirectoryentry.cpp
    src/zipendofcentraldirectory.cpp
    src/zipfile.cpp
    src/zipinputstream.cpp
    src/zipinputstreambuf.cpp
    src/zipios_common.cpp
    src/ziplocalentry.cpp
    src/zipoutputstream.cpp
    src/zipoutputstreambuf.cpp
)

set_target_properties(zipios PROPERTIES POSITION_INDEPENDENT_CODE ON)

# upstream code: no warnings of its own, and gzipoutputstream.hpp needs <fstream> without including it
if(MSVC)
    # as in upstream's CMake file, for ssize_t and localtime_s
    target_compile_definitions(zipios PRIVATE ZIPIOS_WINDOWS NOMINMAX)
    target_compile_options(zipios PRIVATE /w)
else()
    target_compile_options(zipios PRIVATE -w)
endif()
target_precompile_headers(zipios PRIVATE <fstream>)

# src/ for the stream classes that upstream doesn't install, but FreeCAD uses
target_include_directories(zipios SYSTEM
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
    PRIVATE
        ${ZLIB_INCLUDE_DIR}
)

target_link_libraries(zipios PRIVATE ${ZLIB_LIBRARIES})
