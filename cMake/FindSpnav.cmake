IF(UNIX)

    set(TEST_SPNAV_CMAKE TRUE)

    FIND_PATH(SPNAV_INCLUDE_PATH spnav.h
    )

    FIND_LIBRARY(SPNAV_LIBRARY
        NAMES
        spnav libspnav
    )

    if(SPNAV_INCLUDE_PATH AND SPNAV_LIBRARY)
        set(SPNAV_FOUND TRUE)
        set(SPNAV_LIBRARIES ${SPNAV_LIBRARY})
        set(SPNAV_INCLUDE_DIR ${SPNAV_INCLUDE_PATH})
    endif(SPNAV_INCLUDE_PATH AND SPNAV_LIBRARY)



    if(TEST_SPNAV_CMAKE)
        if(SPNAV_INCLUDE_PATH)
            MESSAGE("found spnav include path ${SPNAV_INCLUDE_PATH}")
        else(SPNAV_INCLUDE_PATH)
            MESSAGE("didn't find spnav include path")
        endif(SPNAV_INCLUDE_PATH)

        if(SPNAV_LIBRARY)
            MESSAGE("found spnav library ${SPNAV_LIBRARY}")
        else(SPNAV_LIBRARY)
            MESSAGE("didn't find spnav library")
        endif(SPNAV_LIBRARY)
    endif(TEST_SPNAV_CMAKE)

ENDIF(UNIX)
