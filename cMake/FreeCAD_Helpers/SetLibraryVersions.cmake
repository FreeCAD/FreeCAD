macro(SetLibraryVersions)
    # version information of libraries
    #
    if(OCC_INCLUDE_DIR AND EXISTS ${OCC_INCLUDE_DIR}/Standard_Version.hxx)
        set(HAVE_OCC_VERSION 1)
    endif(OCC_INCLUDE_DIR AND EXISTS ${OCC_INCLUDE_DIR}/Standard_Version.hxx)

    configure_file(${CMAKE_SOURCE_DIR}/src/LibraryVersions.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/LibraryVersions.h)

endmacro(SetLibraryVersions)
