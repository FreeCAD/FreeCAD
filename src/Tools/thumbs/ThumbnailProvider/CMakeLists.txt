project(ThumbnailProvider)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

add_definitions(-D_UNICODE -D_CRT_SECURE_NO_WARNINGS)

set(ROOT_DIR ../../../3rdParty/CxImage)

#=================================================
#                  zlib
#=================================================

set(zlib_DIR ${ROOT_DIR}/zlib)
SET(zlib_SRCS
    ${zlib_DIR}/adler32.c
    ${zlib_DIR}/compress.c
    ${zlib_DIR}/crc32.c
    ${zlib_DIR}/crc32.h
    ${zlib_DIR}/deflate.c
    ${zlib_DIR}/deflate.h
    ${zlib_DIR}/gzio.c
    ${zlib_DIR}/infback.c
    ${zlib_DIR}/inffast.c
    ${zlib_DIR}/inffast.h
    ${zlib_DIR}/inffixed.h
    ${zlib_DIR}/inflate.c
    ${zlib_DIR}/inflate.h
    ${zlib_DIR}/inftrees.c
    ${zlib_DIR}/inftrees.h
    ${zlib_DIR}/trees.c
    ${zlib_DIR}/trees.h
    ${zlib_DIR}/uncompr.c
    ${zlib_DIR}/zconf.h
    ${zlib_DIR}/zconf.in.h
    ${zlib_DIR}/zlib.h
    ${zlib_DIR}/zutil.c
    ${zlib_DIR}/zutil.h
)

set(ZLIB_INCLUDE_DIR ${zlib_DIR})
set(ZLIB_LIBRARIES zlib)

add_library(zlib STATIC ${zlib_SRCS})
target_link_libraries(zlib)


#=================================================
#                  png
#=================================================

set(png_DIR ${ROOT_DIR}/png)
SET(png_SRCS
    ${png_DIR}/png.c
    ${png_DIR}/png.h
    ${png_DIR}/pngconf.h
    ${png_DIR}/pngerror.c
    ${png_DIR}/pngget.c
    ${png_DIR}/pngmem.c
    ${png_DIR}/pngpread.c
    ${png_DIR}/pngread.c
    ${png_DIR}/pngrio.c
    ${png_DIR}/pngrtran.c
    ${png_DIR}/pngrutil.c
    ${png_DIR}/pngset.c
    ${png_DIR}/pngtrans.c
    ${png_DIR}/pngvcrd.c
    ${png_DIR}/pngwio.c
    ${png_DIR}/pngwrite.c
    ${png_DIR}/pngwtran.c
    ${png_DIR}/pngwutil.c
)

set(PNG_INCLUDE_DIRS ${png_DIR})
set(PNG_LIBRARIES png)

add_library(png STATIC ${png_SRCS})
target_link_libraries(png)


#=================================================
#                  CxImage
#=================================================

set(cximage_DIR ${ROOT_DIR}/CxImage)
SET(cximage_SRCS
    ${cximage_DIR}/ximage.cpp
    ${cximage_DIR}/ximage.h
    ${cximage_DIR}/ximapng.cpp
    ${cximage_DIR}/ximapng.h
    ${cximage_DIR}/xmemfile.cpp
    ${cximage_DIR}/xmemfile.h
    ${cximage_DIR}/ximainfo.cpp
    ${cximage_DIR}/ximaenc.cpp
    ${cximage_DIR}/ximapal.cpp
    ${cximage_DIR}/ximalpha.cpp
    ${cximage_DIR}/ximawnd.cpp
    ${cximage_DIR}/ximasel.cpp
    ${cximage_DIR}/ximatran.cpp
    ${cximage_DIR}/ximalyr.cpp
    ${cximage_DIR}/ximaint.cpp
    ${cximage_DIR}/ximath.cpp
    ${cximage_DIR}/ximadsp.cpp
)

set(CXIMAGE_INCLUDE_DIRS ${ROOT_DIR})
set(CXIMAGE_LIBRARIES CxImage)

add_library(CxImage STATIC ${cximage_SRCS})
target_link_libraries(CxImage)


#=================================================
#                  FCStdThumbnail
#=================================================


include_directories(
    .
    ../../../
    ${PNG_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${CXIMAGE_INCLUDE_DIRS}
)

set(THUMBS_LIBS
    Shlwapi.lib
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${CXIMAGE_LIBRARIES}
)

set(zipios_DIR ../../..)
SET(zipios_SRCS
    ${zipios_DIR}/zipios++/backbuffer.h
    ${zipios_DIR}/zipios++/basicentry.cpp
    ${zipios_DIR}/zipios++/basicentry.h
    ${zipios_DIR}/zipios++/collcoll.cpp
    ${zipios_DIR}/zipios++/collcoll.h
    ${zipios_DIR}/zipios++/deflateoutputstreambuf.cpp
    ${zipios_DIR}/zipios++/deflateoutputstreambuf.h
    ${zipios_DIR}/zipios++/dircoll.cpp
    ${zipios_DIR}/zipios++/dircoll.h
    ${zipios_DIR}/zipios++/directory.cpp
    ${zipios_DIR}/zipios++/directory.h
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
