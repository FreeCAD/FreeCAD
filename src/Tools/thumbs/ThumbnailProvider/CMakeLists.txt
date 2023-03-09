project(ThumbnailProvider)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

add_definitions(-D_UNICODE -D_CRT_SECURE_NO_WARNINGS)

#=================================================
#                  zlib
#=================================================

find_package(ZLIB REQUIRED)

#=================================================
#                  FCStdThumbnail
#=================================================


include_directories(
    .
    ../../../
    ${ZLIB_INCLUDE_DIR}
)

set(THUMBS_LIBS
    Shlwapi.lib
    ${ZLIB_LIBRARIES}
)

set(zipios_DIR ../../..)
SET(zipios_SRCS
    ${zipios_DIR}/zipios++/backbuffer.h
    #${zipios_DIR}/zipios++/basicentry.cpp
    #${zipios_DIR}/zipios++/basicentry.h
    ${zipios_DIR}/zipios++/collcoll.cpp
    ${zipios_DIR}/zipios++/collcoll.h
    ${zipios_DIR}/zipios++/deflateoutputstreambuf.cpp
    ${zipios_DIR}/zipios++/deflateoutputstreambuf.h
    #${zipios_DIR}/zipios++/dircoll.cpp
    #${zipios_DIR}/zipios++/dircoll.h
    #${zipios_DIR}/zipios++/directory.cpp
    #${zipios_DIR}/zipios++/directory.h
    ${zipios_DIR}/zipios++/fcoll.cpp
    ${zipios_DIR}/zipios++/fcoll.h
    ${zipios_DIR}/zipios++/fcollexceptions.cpp
    ${zipios_DIR}/zipios++/fcollexceptions.h
    ${zipios_DIR}/zipios++/fileentry.cpp
    ${zipios_DIR}/zipios++/fileentry.h
    ${zipios_DIR}/zipios++/filepath.cpp
    ${zipios_DIR}/zipios++/filepath.h
    ${zipios_DIR}/zipios++/filterinputstreambuf.cpp
    ${zipios_DIR}/zipios++/filterinputstreambuf.h
    ${zipios_DIR}/zipios++/filteroutputstreambuf.cpp
    ${zipios_DIR}/zipios++/filteroutputstreambuf.h
    ${zipios_DIR}/zipios++/gzipoutputstream.cpp
    ${zipios_DIR}/zipios++/gzipoutputstream.h
    ${zipios_DIR}/zipios++/gzipoutputstreambuf.cpp
    ${zipios_DIR}/zipios++/gzipoutputstreambuf.h
    ${zipios_DIR}/zipios++/inflateinputstreambuf.cpp
    ${zipios_DIR}/zipios++/inflateinputstreambuf.h
    ${zipios_DIR}/zipios++/meta-iostreams.h
    ${zipios_DIR}/zipios++/outputstringstream.h
    ${zipios_DIR}/zipios++/simplesmartptr.h
    ${zipios_DIR}/zipios++/virtualseeker.h
    ${zipios_DIR}/zipios++/zipfile.cpp
    ${zipios_DIR}/zipios++/zipfile.h
    ${zipios_DIR}/zipios++/ziphead.cpp
    ${zipios_DIR}/zipios++/ziphead.h
    ${zipios_DIR}/zipios++/zipheadio.cpp
    ${zipios_DIR}/zipios++/zipheadio.h
    ${zipios_DIR}/zipios++/zipinputstream.cpp
    ${zipios_DIR}/zipios++/zipinputstream.h
    ${zipios_DIR}/zipios++/zipinputstreambuf.cpp
    ${zipios_DIR}/zipios++/zipinputstreambuf.h
    ${zipios_DIR}/zipios++/zipios_common.h
    ${zipios_DIR}/zipios++/zipios-config.h
    ${zipios_DIR}/zipios++/zipios_defs.h
    ${zipios_DIR}/zipios++/zipoutputstreambuf.cpp
    ${zipios_DIR}/zipios++/zipoutputstreambuf.h
    ${zipios_DIR}/zipios++/zipoutputstream.cpp
    ${zipios_DIR}/zipios++/zipoutputstream.h
)
SOURCE_GROUP("zipios++" FILES ${zipios_SRCS})

SET(THUMBS_SRCS
    ${zipios_SRCS}
    ClassFactory.cpp
    ClassFactory.h
    Common.h
    FCConfig.h
    Main.cpp
    ThumbnailProvider.cpp
    ThumbnailProvider.def
    ThumbnailProvider.h
)

add_library(FCStdThumbnail SHARED ${THUMBS_SRCS})
target_link_libraries(FCStdThumbnail ${THUMBS_LIBS})
