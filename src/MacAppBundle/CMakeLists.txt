# SPDX-License-Identifier: LGPL-2.1-or-later

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
    set(PYTHON_DIR ${CMAKE_MATCH_1}/Versions/${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/lib/${PYTHON_DIR_BASENAME})
else()
    #unix
    get_filename_component(PYTHON_DIR ${PYTHON_LIBRARY} PATH)
    set(PYTHON_DIR ${PYTHON_DIR}/${PYTHON_DIR_BASENAME})
endif()

message(" PYTHON_DIR is ${PYTHON_DIR} --------------------ipatch--")
message(" PYTHON_DIR_BASENAME is ${PYTHON_DIR_BASENAME} --------------------ipatch--")

install(CODE "execute_process(COMMAND
${CMAKE_COMMAND} -E copy_directory ${PYTHON_DIR} ${CMAKE_INSTALL_LIBDIR}/${PYTHON_DIR_BASENAME}
)")

if(HOMEBREW_PREFIX)

  set(MACOS_BUNDLE_CONTENTS_DIR "Contents")
  set(MACOS_BUNDLE_RESOURCES_DIR "${MACOS_BUNDLE_CONTENTS_DIR}/Resources")
  set(MACOS_BUNDLE_EXECUTABLES_DIR "${MACOS_BUNDLE_CONTENTS_DIR}/MacOS")
  set(MACOS_BUNDLE_FRAMEWORKS_DIR "${MACOS_BUNDLE_CONTENTS_DIR}/Frameworks")
  set(MACOS_BUNDLE_LIB_DIR "${MACOS_BUNDLE_CONTENTS_DIR}/lib")

  message(" PYTHON_DIR is ${PYTHON_DIR} --------------------ipatch--")

  file(GLOB HOMEBREW_PTH_FILES "${HOMEBREW_PREFIX}/lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages/freecad*.pth")

  message(STATUS "HOMEBREW_PTH_FILES are ${HOMEBREW_PTH_FILES} -----------------------ipatch--")

  # process each .pth file found
  foreach(PTH_FILE ${HOMEBREW_PTH_FILES})
    # read the .pth file into a single string
    file(READ ${PTH_FILE} FILE_CONTENT)

    # split the content by newlines into a list of lines
    string(REPLACE "\n" ";" LINES "${FILE_CONTENT}")

    # Process each line from the .pth file.
    foreach(LINE ${LINES})
      # Remove leading/trailing whitespace.
      string(STRIP "${LINE}" CLEAN_LINE)

      # skip empty lines or comments.
      if(CLEAN_LINE MATCHES "^#|^$")
        continue()
      endif()

      # handle both site.addsitedir() calls and direct paths
      if(CLEAN_LINE MATCHES "site\\.addsitedir\\('([^']+)'\\)")
        # Extract the path from site.addsitedir() function call
        string(REGEX MATCH "'([^']+)'" PATH_MATCH "${CLEAN_LINE}")
        string(REPLACE "'" "" ADDITIONAL_DIR "${PATH_MATCH}")
      elseif(NOT CLEAN_LINE MATCHES "import site")
        # Direct path (like the last line in your example)
        set(ADDITIONAL_DIR "${CLEAN_LINE}")
      else()
        # Skip import statements
        continue()
      endif()

      # check if extracted path is valid
      if(ADDITIONAL_DIR AND EXISTS "${ADDITIONAL_DIR}")
        message(STATUS "Processing directory: ${ADDITIONAL_DIR}")

        # Check if the path matches the Homebrew Cellar format or other paths
        if(ADDITIONAL_DIR MATCHES "${HOMEBREW_PREFIX}/Cellar" OR ADDITIONAL_DIR MATCHES "/usr/local/Cellar")

          # Get the package name from the path for organizing in site-packages
          # This handles paths like: /usr/local/Cellar/coin3d@4.0.3_py312/4.0.3_1/lib/python3.12/site-packages/
          # need to extract "coin3d@4.0.3_py312" from the path
          string(REGEX MATCH "/Cellar/([^/]+)/" PACKAGE_MATCH "${ADDITIONAL_DIR}")
          if(PACKAGE_MATCH)
            string(REGEX REPLACE "/Cellar/([^/]+)/" "\\1" PACKAGE_NAME "${PACKAGE_MATCH}")
          else()
            # Fallback - just use the directory name before version
            get_filename_component(PARENT_DIR "${ADDITIONAL_DIR}" DIRECTORY)
            get_filename_component(GRANDPARENT_DIR "${PARENT_DIR}" DIRECTORY)
            get_filename_component(PACKAGE_NAME "${GRANDPARENT_DIR}" NAME)
          endif()

          message(STATUS "Package name: ${PACKAGE_NAME}")

          # Define the destination in the bundle's site-packages
          set(BUNDLE_SITE_PACKAGES_DIR "${CMAKE_INSTALL_PREFIX}/lib/python3.12/site-packages")

          # Copy all contents from the source site-packages to bundle site-packages
          # Using glob to get all directories and files in the source
          file(GLOB PACKAGE_CONTENTS "${ADDITIONAL_DIR}/*")

          foreach(CONTENT_ITEM ${PACKAGE_CONTENTS})
            get_filename_component(ITEM_NAME "${CONTENT_ITEM}" NAME)

            if(IS_DIRECTORY "${CONTENT_ITEM}")
              # Install directory
              install(DIRECTORY "${CONTENT_ITEM}/" 
                DESTINATION "${BUNDLE_SITE_PACKAGES_DIR}/${ITEM_NAME}"
                USE_SOURCE_PERMISSIONS)
              message(STATUS "  Installing directory: ${ITEM_NAME}")
            else()
              # Install file
              install(FILES "${CONTENT_ITEM}" 
                DESTINATION "${BUNDLE_SITE_PACKAGES_DIR}")
              message(STATUS "  Installing file: ${ITEM_NAME}")
            endif()
          endforeach()

          message(STATUS "Copied package contents from: ${ADDITIONAL_DIR}")
          message(STATUS "  To: ${BUNDLE_SITE_PACKAGES_DIR}")

        else()
          message(STATUS "Skipping non-Homebrew path: ${ADDITIONAL_DIR}")
        endif()
      else()
        message(STATUS "Path does not exist or is invalid: ${ADDITIONAL_DIR}")
      endif()

      # Reset for next iteration
      set(ADDITIONAL_DIR "")

    endforeach(LINE)
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

install(CODE
  "message(STATUS \"Making bundle relocatable...\")
  # The top-level CMakeLists.txt should prevent multiple package manager
  # prefixes from being set, so the lib path will resolve correctly...
  execute_process(
    COMMAND python3
    ${CMAKE_SOURCE_DIR}/src/Tools/MakeMacBundleRelocatable.py
    ${APP_PATH}
    ${HOMEBREW_PREFIX}${MACPORTS_PREFIX}/lib
    ${HOMEBREW_PREFIX}/opt
    ${HOMEBREW_PREFIX}/opt/*/lib
    ${Qt5Core_DIR}/../../..
    ${XCTEST_PATH}
    )"
  )

endif(FREECAD_CREATE_MAC_APP)
