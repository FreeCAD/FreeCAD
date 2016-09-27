set(PYTHON_DIR_BASENAME python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})

if(PYTHON_LIBRARY MATCHES "(.*Python\\.framework).*")
    #framework
    set(PYTHON_DIR ${CMAKE_MATCH_1}/Versions/Current/lib/${PYTHON_DIR_BASENAME})
else()
    #unix
    get_filename_component(PYTHON_DIR ${PYTHON_LIBRARY} PATH)
    set(PYTHON_DIR ${PYTHON_DIR}/${PYTHON_DIR_BASENAME})
endif()

install(CODE "execute_process(COMMAND
    ${CMAKE_COMMAND} -E copy_directory ${PYTHON_DIR} ${CMAKE_INSTALL_LIBDIR}/${PYTHON_DIR_BASENAME}
)")

if(HOMEBREW_PREFIX)
    #Homebrew installs python dependencies to a site dir in prefix/libexec
    #and installs a .pth file containing its path to the HOMEBREW_PREFIX site dir.

    file(GLOB HOMEBREW_PTH_FILES "${PYTHON_DIR}/site-packages/homebrew*.pth")

    foreach(PTH_FILE ${HOMEBREW_PTH_FILES})
        file(READ ${PTH_FILE} ADDITIONAL_DIR)

	string(STRIP ${ADDITIONAL_DIR} ADDITIONAL_DIR)
        string(REGEX REPLACE "^${HOMEBREW_PREFIX}/Cellar/([A-Za-z0-9_]+).*$" "\\1" LIB_NAME ${ADDITIONAL_DIR})
        string(REGEX REPLACE ".*libexec(.*)/site-packages" "libexec/${LIB_NAME}\\1" NEW_SITE_DIR ${ADDITIONAL_DIR})

        install(DIRECTORY ${ADDITIONAL_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX}/${NEW_SITE_DIR})

        #update the paths of the .pth files copied into the bundle
        get_filename_component(PTH_FILENAME ${PTH_FILE} NAME)
        install(CODE
            "file(WRITE
                 ${CMAKE_INSTALL_LIBDIR}/${PYTHON_DIR_BASENAME}/site-packages/${PTH_FILENAME}
		 \"../../../${NEW_SITE_DIR}/site-packages\"
            )"
        )
    endforeach(PTH_FILE)
endif()

install(DIRECTORY ${QT_PLUGINS_DIR}/ DESTINATION ${CMAKE_INSTALL_LIBDIR}/qtplugins)

#files installed by homebrew do not have write permission for regular user
install(CODE "execute_process(COMMAND chmod -R a+w ${CMAKE_INSTALL_LIBDIR})")

get_filename_component(APP_PATH ${CMAKE_INSTALL_PREFIX} PATH)

install(CODE 
    "message(STATUS \"Making bundle relocatable...\")
    # The top-level CMakeLists.txt should prevent multiple package manager
    # prefixes from being set, so the lib path will resolve correctly...
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/src/Tools/MakeMacBundleRelocatable.py
        ${APP_PATH} ${HOMEBREW_PREFIX}${MACPORTS_PREFIX}/lib
    )"
)
