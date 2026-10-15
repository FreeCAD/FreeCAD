macro(SetupZipIos)
# -------------------------------- ZipIos --------------------------------

    # Use external Zipios if specified.
    if(FREECAD_USE_EXTERNAL_ZIPIOS)
        find_library(ZIPIOS_LIBRARY zipios)
        find_path(ZIPIOS_INCLUDES zipios/zipfile.hpp)
        # FreeCAD also uses the stream classes, which Zipios 2.3.4 doesn't install
        find_path(ZIPIOS_STREAM_INCLUDES zipinputstream.hpp PATH_SUFFIXES zipios)
        if(ZIPIOS_LIBRARY)
            message(STATUS "Found Zipios: ${ZIPIOS_LIBRARY}")
        endif()
        if(ZIPIOS_INCLUDES)
            message(STATUS "Found Zipios headers.")
        endif()
        if(NOT ZIPIOS_LIBRARY OR NOT ZIPIOS_INCLUDES)
            message(FATAL_ERROR "Using external Zipios was specified but was not found.")
        endif()
        if(NOT ZIPIOS_STREAM_INCLUDES
           OR NOT EXISTS ${ZIPIOS_STREAM_INCLUDES}/zipoutputstream.hpp
           OR NOT EXISTS ${ZIPIOS_STREAM_INCLUDES}/zipcentraldirectoryentry.hpp)
            message(FATAL_ERROR "The external Zipios lacks the headers zipinputstream.hpp, "
                "zipoutputstream.hpp and zipcentraldirectoryentry.hpp that FreeCAD uses.")
        endif()

        if(EXISTS ${ZIPIOS_INCLUDES}/zipios/zipios-config.hpp)
            file(STRINGS ${ZIPIOS_INCLUDES}/zipios/zipios-config.hpp ZIPIOS_VERSION
                 REGEX "#define[ \t]+ZIPIOS_VERSION_STRING")
            string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" ZIPIOS_VERSION "${ZIPIOS_VERSION}")
        endif()

        add_library(zipios UNKNOWN IMPORTED)
        set_target_properties(zipios PROPERTIES
            IMPORTED_LOCATION ${ZIPIOS_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES "${ZIPIOS_INCLUDES};${ZIPIOS_STREAM_INCLUDES}"
        )
    endif(FREECAD_USE_EXTERNAL_ZIPIOS)

endmacro(SetupZipIos)
