# Try to find Coin3D Doc If found, this will define
#
# COIN3D_DOC_FOUND    - we have access to Coin3D doc, either locally or on the net
# COIN3D_DOC_TAGFILE  - full name of the tag file COIN3D_DOC_PATH     - path to html Coin3D doc

set (COIN3D_DOC_FOUND "NO")

if (COIN3D_FOUND)
    if (WIN32)
        if (CYGWIN)
            # Not yet implemented
        else (CYGWIN)
            # Not yet implemented
        endif (CYGWIN)
    else (WIN32)
        if (APPLE)
            # Not yet implemented
        else (APPLE)
            # Unix systems
            find_path (COIN3D_DOC_PATH index.html /usr/share/doc/libcoin80-doc/html
                       /usr/share/doc/coin/html /usr/share/doc/Coin/html)
            if (EXISTS ${COIN3D_DOC_PATH})
                message (STATUS "Coin3D doc is installed")
                find_file (
                    COIN3D_DOC_TAGFILE
                    NAMES coin.tag Coin.tag
                    PATHS ${COIN3D_DOC_PATH})
                if (EXISTS ${COIN3D_DOC_TAGFILE})
                    set (COIN3D_DOC_FOUND "YES")
                else (EXISTS ${COIN3D_DOC_TAGFILE})
                    find_file (
                        COIN3D_DOC_TAGFILE_GZ
                        NAMES coin.tag.gz Coin.tag.gz
                        PATHS ${COIN3D_DOC_PATH})
                    if (EXISTS ${COIN3D_DOC_TAGFILE_GZ})
                        message (STATUS "  Found ${COIN3D_DOC_TAGFILE_GZ}")
                        message (
                            STATUS
                                "  You should uncompress this file if you want to use it for source doc generation"
                        )
                    endif (EXISTS ${COIN3D_DOC_TAGFILE_GZ})

                endif (EXISTS ${COIN3D_DOC_TAGFILE})

            else (EXISTS ${COIN3D_DOC_PATH})
                # fallback: tries to use online coin doc
                message (STATUS "Coin3D doc is not installed")
                set (COIN3D_DOC_PATH http://doc.coin3d.org/Coin)
                find_file (COIN3D_DOC_TAGFILE coin.tag ${CMAKE_BINARY_DIR}/src/Doc)
                if (EXISTS ${COIN3D_DOC_TAGFILE})
                    set (COIN3D_DOC_FOUND "YES")
                endif (EXISTS ${COIN3D_DOC_TAGFILE})
            endif (EXISTS ${COIN3D_DOC_PATH})
        endif (APPLE)
    endif (WIN32)
endif (COIN3D_FOUND)

if (COIN3D_DOC_FOUND)
    message (STATUS "  Tag file: ${COIN3D_DOC_TAGFILE}")
    message (STATUS "  Location: ${COIN3D_DOC_PATH}")
endif (COIN3D_DOC_FOUND)

# export for others
set (
    COIN3D_DOC_FOUND
    "${COIN3D_DOC_FOUND}"
    CACHE BOOL "Coin3d documentation available")
