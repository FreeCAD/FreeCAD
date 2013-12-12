# Try to find Coin3D Doc
# If found, this will define
#
# COIN3D_DOC_FOUND    - we have access to Coin3D doc, either locally
#                       or on the net
# COIN3D_DOC_TAGFILE  - full name of the tag file
# COIN3D_DOC_PATH     - path to html Coin3D doc

SET( COIN3D_DOC_FOUND "NO" )

IF (COIN3D_FOUND)
  IF (WIN32)
    IF (CYGWIN)
      # Not yet implemented
    ELSE (CYGWIN)
      # Not yet implemented
    ENDIF (CYGWIN)
  ELSE (WIN32)
    IF(APPLE)
      # Not yet implemented
    ELSE(APPLE)
      # Unix systems
      find_path(COIN3D_DOC_PATH index.html 
                /usr/share/doc/libcoin60-doc/html
                /usr/share/doc/coin/html
      )
      IF( EXISTS ${COIN3D_DOC_PATH})
        message(STATUS "Coin3D doc is installed")
        find_file(COIN3D_DOC_TAGFILE coin.tag 
            ${COIN3D_DOC_PATH}
        )
        IF( EXISTS ${COIN3D_DOC_TAGFILE})
          SET( COIN3D_DOC_FOUND "YES"
          )
        ELSE( EXISTS ${COIN3D_DOC_TAGFILE})
          find_file(COIN3D_DOC_TAGFILE_GZ coin.tag.gz 
              ${COIN3D_DOC_PATH}
          )
          IF( EXISTS ${COIN3D_DOC_TAGFILE_GZ})
            message(STATUS "  Found ${COIN3D_DOC_TAGFILE_GZ}")
            message(STATUS "  You should uncompress this file if you want to use it for source doc generation")
          ENDIF( EXISTS ${COIN3D_DOC_TAGFILE_GZ})

        ENDIF( EXISTS ${COIN3D_DOC_TAGFILE})

      ELSE( EXISTS ${COIN3D_DOC_PATH})
        #fallback: tries to use online coin doc
        message(STATUS "Coin3D doc is not installed")
        SET(COIN3D_DOC_PATH
            http://doc.coin3d.org/Coin
        )
        find_file(COIN3D_DOC_TAGFILE coin.tag 
            ${CMAKE_BINARY_DIR}/src/Doc
        )
        IF( EXISTS ${COIN3D_DOC_TAGFILE})
          SET( COIN3D_DOC_FOUND "YES"
          )
        #ELSE( EXISTS ${COIN3D_DOC_TAGFILE})
        #  find_program(WGET_PROG wget
        #  )
        #  IF( EXISTS ${WGET_PROG})
        #    execute_process(COMMAND ${WGET_PROG} 
        #                    -P ${CMAKE_BINARY_DIR}/src/Doc 
        #                    ${COIN3D_DOC_PATH}/coin.tag
        #    )
        #    find_file(COIN3D_DOC_TAGFILE coin.tag 
        #        ${CMAKE_BINARY_DIR}/src/Doc
        #    )
        #    IF( EXISTS ${COIN3D_DOC_TAGFILE})
        #      SET( COIN3D_DOC_FOUND "YES"
        #      )
        #    ENDIF( EXISTS ${COIN3D_DOC_TAGFILE})
        #
        #  ENDIF( EXISTS ${WGET_PROG})

        ENDIF( EXISTS ${COIN3D_DOC_TAGFILE})

      ENDIF( EXISTS ${COIN3D_DOC_PATH})
    ENDIF(APPLE)
  ENDIF(WIN32)
ENDIF(COIN3D_FOUND)

if(COIN3D_DOC_FOUND)
  message(STATUS "  Tag file: ${COIN3D_DOC_TAGFILE}")
  message(STATUS "  Location: ${COIN3D_DOC_PATH}")
endif(COIN3D_DOC_FOUND)


