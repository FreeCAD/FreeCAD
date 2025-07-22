
#
# Build and install QuickLook for FCStd files (org.freecad.fcstd)
# This is used by Homebrew and Conda/Mamba scripts alike.
#
if(FREECAD_CREATE_MAC_APP OR (APPLE AND BUILD_WITH_CONDA))
    add_subdirectory(QuickLook)
    install(
        DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/QuickLook/QuicklookFCStd.qlgenerator"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/Library/QuickLook"
    )
    install(
        PROGRAMS "${PROJECT_BINARY_DIR}/src/MacAppBundle/QuickLook/QuicklookFCStd.framework/Versions/A/QuicklookFCStd"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/Library/QuickLook/QuicklookFCStd.qlgenerator/Contents/MacOS/"
    )
endif()


#
# Build a Bundle in Homebrew.
# This is ignored by Conda/Mamba build scripts.
#
if(FREECAD_CREATE_MAC_APP)

set(PYTHON_DIR_BASENAME python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR})

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

        string(STRIP "${ADDITIONAL_DIR}" ADDITIONAL_DIR)
        string(FIND "${ADDITIONAL_DIR}" "${HOMEBREW_PREFIX}/Cellar" POSITION)
        string(LENGTH "${ADDITIONAL_DIR}" DIR_LENGTH)
        string(SUBSTRING "${ADDITIONAL_DIR}" ${POSITION} ${DIR_LENGTH}-${POSITION} DIR_TAIL)
        string(REGEX MATCHALL "^([/A-Za-z0-9_.@-]+)" CLEAR_TAIL ${DIR_TAIL})
        string(REGEX REPLACE "^${HOMEBREW_PREFIX}/Cellar/([A-Za-z0-9_]+).*$" "\\1" LIB_NAME ${CLEAR_TAIL})
        string(REGEX REPLACE ".*libexec(.*)/site-packages" "libexec/${LIB_NAME}\\1" NEW_SITE_DIR ${CLEAR_TAIL})

        install(DIRECTORY ${CLEAR_TAIL} DESTINATION ${CMAKE_INSTALL_PREFIX}/${NEW_SITE_DIR})

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

set(QT_PLUGINS_DIR "${Qt5Core_DIR}/../../../plugins")
execute_process(COMMAND "xcode-select" "--print-path"
    OUTPUT_VARIABLE XCODE_PATH
    ERROR_QUIET
    )
string(STRIP ${XCODE_PATH} XCODE_PATH)

set(XCTEST_PATH "${XCODE_PATH}/Platforms/MacOSX.platform/Developer/Library/Frameworks/XCTest.framework/Versions/Current")

# add qt assistant to bundle
install(PROGRAMS "${Qt5Core_DIR}/../../../libexec/Assistant.app/Contents/MacOS/Assistant" DESTINATION ${CMAKE_INSTALL_PREFIX}/MacOS)


# Ensure the actual plugin files are installed instead of symlinks.
file(GLOB _subdirs RELATIVE "${QT_PLUGINS_DIR}" "${QT_PLUGINS_DIR}/*")

foreach(_subdir ${_subdirs})
    if(IS_DIRECTORY "${QT_PLUGINS_DIR}/${_subdir}")
        set(_resolved_files "")
        file(GLOB _plugin_files RELATIVE "${QT_PLUGINS_DIR}/${_subdir}" "${QT_PLUGINS_DIR}/${_subdir}/*")

        foreach(_plugin_file ${_plugin_files})
            get_filename_component(_resolved_file "${QT_PLUGINS_DIR}/${_subdir}/${_plugin_file}" REALPATH)
            list(APPEND _resolved_files ${_resolved_file})
        endforeach()

        install(FILES ${_resolved_files} DESTINATION "${CMAKE_INSTALL_LIBDIR}/qtplugins/${_subdir}")
    endif()
endforeach()

#files installed by homebrew do not have write permission for regular user
install(CODE "execute_process(COMMAND chmod -R a+w ${CMAKE_INSTALL_LIBDIR})")

get_filename_component(APP_PATH ${CMAKE_INSTALL_PREFIX} PATH)

file(GLOB CONFIG_ICU "${HOMEBREW_PREFIX}/opt/icu4c/lib")

file(GLOB CONFIG_LLVM "${HOMEBREW_PREFIX}/opt/llvm/lib/c++")

file(GLOB CONFIG_GCC "${HOMEBREW_PREFIX}/opt/gcc/lib/gcc/current")

execute_process(
    COMMAND find -L /usr/local/Cellar/nglib -name MacOS
    OUTPUT_VARIABLE CONFIG_NGLIB)

install(CODE
    "message(STATUS \"Making bundle relocatable...\")
    # The top-level CMakeLists.txt should prevent multiple package manager
    # prefixes from being set, so the lib path will resolve correctly...
    execute_process(
        COMMAND python3
        ${CMAKE_SOURCE_DIR}/src/Tools/MakeMacBundleRelocatable.py
        ${APP_PATH} ${HOMEBREW_PREFIX}${MACPORTS_PREFIX}/lib ${CONFIG_ICU} ${CONFIG_LLVM} ${CONFIG_GCC} /usr/local/opt ${CONFIG_NGLIB} ${Qt5Core_DIR}/../../.. ${XCTEST_PATH}
    )"
)

endif(FREECAD_CREATE_MAC_APP)
