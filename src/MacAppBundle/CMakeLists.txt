# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

#
# Build and install QuickLook for FCStd files (org.freecad.fcstd)
# This is used by Homebrew and Conda/Mamba scripts alike.
#
if(FREECAD_CREATE_MAC_APP OR (APPLE AND BUILD_WITH_CONDA))
    add_subdirectory(QuickLook)
    # Installation handled by QuickLook/CMakeLists.txt
endif()


#
# Build a Bundle in Homebrew.
# This is ignored by Conda/Mamba build scripts.
#
if(FREECAD_CREATE_MAC_APP)

set(PYTHON_DIR_BASENAME python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR})

if(Python3_LIBRARY_DIRS)
    # Get first library directory
    list(GET Python3_LIBRARY_DIRS 0 FIRST_PYTHON_LIB_DIR)
    if(FIRST_PYTHON_LIB_DIR MATCHES "(.*Python\\.framework).*")
        #framework
        set(PYTHON_DIR ${CMAKE_MATCH_1}/Versions/${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/lib/${PYTHON_DIR_BASENAME})
    else()
        #unix
        set(PYTHON_DIR ${FIRST_PYTHON_LIB_DIR}/${PYTHON_DIR_BASENAME})
    endif()
else()
    # Fallback: derive from Python3_EXECUTABLE
    get_filename_component(PYTHON_BIN_DIR ${Python3_EXECUTABLE} DIRECTORY)
    get_filename_component(PYTHON_PREFIX ${PYTHON_BIN_DIR} DIRECTORY)
    set(PYTHON_DIR ${PYTHON_PREFIX}/lib/${PYTHON_DIR_BASENAME})
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

  file(GLOB HOMEBREW_PTH_FILES "${HOMEBREW_PREFIX}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages/freecad*.pth")

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

set(QT_PLUGINS_DIR "${Qt6Core_DIR}/../../../plugins")
# Fallback for Homebrew Qt6 layout
if(NOT EXISTS "${QT_PLUGINS_DIR}")
    set(QT_PLUGINS_DIR "/opt/homebrew/share/qt/plugins")
endif()
set(QT_ASSISTANT_PATH "${Qt6Core_DIR}/../../../libexec/Assistant.app/Contents/MacOS/Assistant")
set(QT_PLUGIN_SUBDIRS "platforms;imageformats;styles;iconengines")


execute_process(COMMAND "xcode-select" "--print-path"
  OUTPUT_VARIABLE XCODE_PATH
  ERROR_QUIET
)
string(STRIP ${XCODE_PATH} XCODE_PATH)

set(XCTEST_PATH "${XCODE_PATH}/Platforms/MacOSX.platform/Developer/Library/Frameworks/XCTest.framework/Versions/Current")

# add qt assistant to bundle
if(EXISTS "${QT_ASSISTANT_PATH}")
    install(PROGRAMS "${QT_ASSISTANT_PATH}" DESTINATION ${CMAKE_INSTALL_PREFIX}/MacOS)
else()
    message(WARNING "Qt Assistant not found at ${QT_ASSISTANT_PATH}, skipping installation")
endif()

# Install specific Qt plugin subdirectories that FreeCAD needs
foreach(_subdir ${QT_PLUGIN_SUBDIRS})
    if(IS_DIRECTORY "${QT_PLUGINS_DIR}/${_subdir}")
        set(_resolved_files "")
        file(GLOB _plugin_files "${QT_PLUGINS_DIR}/${_subdir}/*.dylib")

        foreach(_plugin_file ${_plugin_files})
            if(EXISTS "${_plugin_file}" AND NOT IS_DIRECTORY "${_plugin_file}")
                get_filename_component(_resolved_file "${_plugin_file}" REALPATH)
                list(APPEND _resolved_files ${_resolved_file})
            endif()
        endforeach()

        if(_resolved_files)
            install(FILES ${_resolved_files} DESTINATION "${CMAKE_INSTALL_LIBDIR}/qtplugins/${_subdir}")

            # Also install platform plugins to standard Qt location for runtime loading
            if("${_subdir}" STREQUAL "platforms")
                install(FILES ${_resolved_files} DESTINATION "${CMAKE_INSTALL_PREFIX}/PlugIns/platforms")
            endif()
        endif()
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
    ${Qt${FREECAD_QT_MAJOR_VERSION}Core_DIR}/../../..
    ${XCTEST_PATH}
    )"
  )

# Fix Qt platform plugin dependencies after bundle relocation
# This addresses the SIGKILL issue where Qt plugins reference framework paths
# but the Qt libraries are installed as flat dylibs
install(CODE "
  message(STATUS \"Fixing Qt platform plugin dependencies...\")

  # Find platform plugins
  file(GLOB_RECURSE PLATFORM_PLUGINS \"${CMAKE_INSTALL_PREFIX}/PlugIns/platforms/*.dylib\")

  foreach(PLUGIN \${PLATFORM_PLUGINS})
    message(STATUS \"Processing plugin: \${PLUGIN}\")

    # Get current dependencies
    execute_process(
      COMMAND otool -L \"\${PLUGIN}\"
      OUTPUT_VARIABLE PLUGIN_DEPS
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Fix framework-style Qt dependencies to use flat dylib names
    if(FREECAD_QT_MAJOR_VERSION STREQUAL \"6\")
      set(QT_FRAMEWORKS \"QtCore;QtGui;QtWidgets;QtOpenGL;QtPrintSupport;QtSvg;QtNetwork\")
    else()
      set(QT_FRAMEWORKS \"QtCore;QtGui;QtWidgets;QtOpenGL;QtPrintSupport;QtSvg;QtNetwork\")
    endif()

    foreach(FRAMEWORK \${QT_FRAMEWORKS})
      # Change @rpath/QtXXX.framework/Versions/A/QtXXX to @rpath/QtXXX
      execute_process(
        COMMAND install_name_tool -change
          \"@rpath/\${FRAMEWORK}.framework/Versions/A/\${FRAMEWORK}\"
          \"@rpath/\${FRAMEWORK}\"
          \"\${PLUGIN}\"
        OUTPUT_QUIET
        ERROR_QUIET
      )

      # Also handle versioned framework paths for Qt5
      if(FREECAD_QT_MAJOR_VERSION STREQUAL \"5\")
        execute_process(
          COMMAND install_name_tool -change
            \"@rpath/\${FRAMEWORK}.framework/Versions/5/\${FRAMEWORK}\"
            \"@rpath/\${FRAMEWORK}\"
            \"\${PLUGIN}\"
          OUTPUT_QUIET
          ERROR_QUIET
        )
      endif()
    endforeach()

    message(STATUS \"Fixed dependencies for: \${PLUGIN}\")
  endforeach()

  message(STATUS \"Qt platform plugin dependency fixing complete.\")
")

endif(FREECAD_CREATE_MAC_APP)
