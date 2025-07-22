macro(SetupZipIos)
# -------------------------------- ZipIos --------------------------------

    # Use external zipios++ if specified.
    if(FREECAD_USE_EXTERNAL_ZIPIOS)
        find_library(ZIPIOS_LIBRARY zipios)
        find_path(ZIPIOS_INCLUDES zipios++/zipios-config.h)
        if(ZIPIOS_LIBRARY)
            message(STATUS "Found zipios++: ${ZIPIOS}")
        endif()
        if(ZIPIOS_INCLUDES)
            message(STATUS "Found zipios++ headers.")
        endif()
        if(NOT ZIPIOS_LIBRARY OR NOT ZIPIOS_INCLUDES)
            message(FATAL_ERROR "Using external zipios++ was specified but was not found.")
        endif()
    else(FREECAD_USE_EXTERNAL_ZIPIOS)
        set(ZIPIOS_INCLUDES ${CMAKE_SOURCE_DIR}/src/3rdParty)
        SET(zipios_SRCS
            ${ZIPIOS_INCLUDES}/zipios++/backbuffer.h
            ${ZIPIOS_INCLUDES}/zipios++/collcoll.cpp
            ${ZIPIOS_INCLUDES}/zipios++/collcoll.h
            ${ZIPIOS_INCLUDES}/zipios++/deflateoutputstreambuf.cpp
            ${ZIPIOS_INCLUDES}/zipios++/deflateoutputstreambuf.h
            ${ZIPIOS_INCLUDES}/zipios++/fcoll.cpp
            ${ZIPIOS_INCLUDES}/zipios++/fcoll.h
            ${ZIPIOS_INCLUDES}/zipios++/fcollexceptions.cpp
            ${ZIPIOS_INCLUDES}/zipios++/fcollexceptions.h
            ${ZIPIOS_INCLUDES}/zipios++/fileentry.cpp
            ${ZIPIOS_INCLUDES}/zipios++/fileentry.h
            ${ZIPIOS_INCLUDES}/zipios++/filepath.cpp
            ${ZIPIOS_INCLUDES}/zipios++/filepath.h
            ${ZIPIOS_INCLUDES}/zipios++/filterinputstreambuf.cpp
            ${ZIPIOS_INCLUDES}/zipios++/filterinputstreambuf.h
            ${ZIPIOS_INCLUDES}/zipios++/filteroutputstreambuf.cpp
            ${ZIPIOS_INCLUDES}/zipios++/filteroutputstreambuf.h
            ${ZIPIOS_INCLUDES}/zipios++/gzipoutputstream.cpp
            ${ZIPIOS_INCLUDES}/zipios++/gzipoutputstream.h
            ${ZIPIOS_INCLUDES}/zipios++/gzipoutputstreambuf.cpp
            ${ZIPIOS_INCLUDES}/zipios++/gzipoutputstreambuf.h
            ${ZIPIOS_INCLUDES}/zipios++/inflateinputstreambuf.cpp
            ${ZIPIOS_INCLUDES}/zipios++/inflateinputstreambuf.h
            ${ZIPIOS_INCLUDES}/zipios++/meta-iostreams.h
            ${ZIPIOS_INCLUDES}/zipios++/outputstringstream.h
            ${ZIPIOS_INCLUDES}/zipios++/simplesmartptr.h
            ${ZIPIOS_INCLUDES}/zipios++/virtualseeker.h
            ${ZIPIOS_INCLUDES}/zipios++/zipfile.cpp
            ${ZIPIOS_INCLUDES}/zipios++/zipfile.h
            ${ZIPIOS_INCLUDES}/zipios++/ziphead.cpp
            ${ZIPIOS_INCLUDES}/zipios++/ziphead.h
            ${ZIPIOS_INCLUDES}/zipios++/zipheadio.cpp
            ${ZIPIOS_INCLUDES}/zipios++/zipheadio.h
            ${ZIPIOS_INCLUDES}/zipios++/zipinputstream.cpp
            ${ZIPIOS_INCLUDES}/zipios++/zipinputstream.h
            ${ZIPIOS_INCLUDES}/zipios++/zipinputstreambuf.cpp
            ${ZIPIOS_INCLUDES}/zipios++/zipinputstreambuf.h
            ${ZIPIOS_INCLUDES}/zipios++/zipios_common.h
            ${ZIPIOS_INCLUDES}/zipios++/zipios-config.h
            ${ZIPIOS_INCLUDES}/zipios++/zipios_defs.h
            ${ZIPIOS_INCLUDES}/zipios++/zipoutputstreambuf.cpp
            ${ZIPIOS_INCLUDES}/zipios++/zipoutputstreambuf.h
            ${ZIPIOS_INCLUDES}/zipios++/zipoutputstream.cpp
            ${ZIPIOS_INCLUDES}/zipios++/zipoutputstream.h
        )
    endif(FREECAD_USE_EXTERNAL_ZIPIOS)

endmacro(SetupZipIos)
