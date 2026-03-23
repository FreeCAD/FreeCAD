# Add a target to generate API documentation with Doxygen
if(COIN_BUILD_DOCUMENTATION)
  find_package(Doxygen)
  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is needed to build the documentation.")
  endif()

# ############################################################################
# Setup documentation options
# ############################################################################
  set(DOXYGEN_GENERATE_TODOLIST NO)
  set(DOXYGEN_GENERATE_DEPRECATEDLIST NO)
  set(GENERATE_HTMLHELP NO)
  set(DOXYGEN_GENERATE_MAN NO)
  set(GENERATE_QHP NO)
  set(GENERATE_TREEVIEW YES)
  set(HHC_PROGRAM)
  if(WIN32)
    if(COIN_BUILD_DOCUMENTATION_CHM)
      find_program(HHC_PROGRAM NAMES hhc.exe PATHS "C:/Program Files/HTML Help Workshop" "C:/Program Files (x86)/HTML Help Workshop" DOC "HTML Help Compiler program")
      if(NOT HHC_PROGRAM)
        message(FATAL_ERROR "Missing program HTML Help Compiler")
      else()
        set(GENERATE_HTMLHELP YES)
        set(GENERATE_TREEVIEW NO)
        mark_as_advanced(HHC_PROGRAM)
      endif()
    endif()
  else()
    if(COIN_BUILD_DOCUMENTATION_MAN)
      set(DOXYGEN_GENERATE_MAN YES)
    endif()
  endif()
  if(COIN_BUILD_DOCUMENTATION_QTHELP)
    find_program(QHG_LOCATION NAMES qhelpgenerator qhelpgenerator-qt5 DOC "Qt qhelpgenerator")
    if(NOT QHG_LOCATION)
      message(FATAL_ERROR "Missing program Qt qhelpgenerator")
    else()
      set(GENERATE_QHP YES)
      mark_as_advanced(QHG_LOCATION)
    endif()
  endif()

  # Add default files
  file(GLOB_RECURSE DOX *.dox*)
  set(COIN_DOCUMENTATION_FILES
    ${COIN_DOCUMENTATION_FILES}
    "${PROJECT_SOURCE_DIR}/docs/releases.dox"
    ${DOX}
    CACHE INTERNAL "COIN_DOCUMENTATION_FILES"
  )

  set(DOXYGEN_INTERNAL_DOCS NO)
  set(DOXYGEN_EXTRACT_PRIVATE NO)
  set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
  set(DOXYGEN_EXCLUDE)
  if(COIN_BUILD_INTERNAL_DOCUMENTATION)
    set(DOXYGEN_INTERNAL_DOCS YES)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
    set(COIN_DOCUMENTATION_FILES
      ${COIN_DOCUMENTATION_FILES}
      ${COIN_INTERNAL_DOCUMENTATION_FILES}
      CACHE INTERNAL "COIN_DOCUMENTATION_FILES"
    )
  else()
    set(COIN_DOCUMENTATION_EXCLUDE_SYMBOLS SoProfilerP)
    foreach(XF ${COIN_INTERNAL_DOCUMENTATION_FILES})
      set(DOXYGEN_EXCLUDE "${DOXYGEN_EXCLUDE}                         ${XF} \\\n")
    endforeach()
  endif()
  set(DOXYGEN_INPUT)
  list(REMOVE_DUPLICATES COIN_DOCUMENTATION_FILES)
  foreach(XF ${COIN_DOCUMENTATION_FILES})
    set(DOXYGEN_INPUT "${DOXYGEN_INPUT}                         ${XF} \\\n")
  endforeach()
  set(DOXYGEN_INPUT "${DOXYGEN_INPUT}                         ${PROJECT_SOURCE_DIR}/src/tidbits.cpp")
  set(DOXYGEN_EXCLUDE_SYMBOLS)
  foreach(XF ${COIN_DOCUMENTATION_EXCLUDE_SYMBOLS})
    set(DOXYGEN_EXCLUDE_SYMBOLS "${DOXYGEN_EXCLUDE_SYMBOLS}                         ${XF} \\\n")
  endforeach()

  set(DOXYFILE "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
  set(DOXYGEN_OUTPUT "${CMAKE_BINARY_DIR}/html/index.html")
  configure_file("${PROJECT_SOURCE_DIR}/docs/coin.doxygen.cmake.in" ${DOXYFILE} @ONLY)

# ############################################################################
# Setup documentation targets
# ############################################################################
  add_custom_command(
    OUTPUT ${DOXYGEN_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E echo_append "Generating API documentation with Doxygen "
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
    COMMAND ${CMAKE_COMMAND} -E echo "done."
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS ${DOXYFILE}
  )
  add_custom_target(documentation ALL DEPENDS ${DOXYGEN_OUTPUT})

# ############################################################################
# Install built documentation files
# ############################################################################
  if(NOT COIN_BUILD_MAC_FRAMEWORK)
    install(DIRECTORY "${CMAKE_BINARY_DIR}/html" DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT documentation REGEX ".*\\.(chm|qch)" EXCLUDE)
    if(COIN_BUILD_DOCUMENTATION_CHM)
      install(FILES "${CMAKE_BINARY_DIR}/html/${PROJECT_NAME}.chm" DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT documentation)
    endif()
    if(COIN_BUILD_DOCUMENTATION_QTHELP)
      install(FILES "${CMAKE_BINARY_DIR}/html/${PROJECT_NAME}.qch" DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT documentation)
    endif()
    if(COIN_BUILD_DOCUMENTATION_MAN)
      install(DIRECTORY "${CMAKE_BINARY_DIR}/man/man3" DESTINATION ${CMAKE_INSTALL_MANDIR} COMPONENT documentation)
    endif()
  endif()
endif()



# ############################################################################
# Add a target to generate new modern API documentation with Doxygen
# ############################################################################

if(COIN_BUILD_AWESOME_DOCUMENTATION)
  find_package(Doxygen)
  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is needed to build the documentation.")
  endif()

  find_package(Git QUIET)
  if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
  # Update submodules as needed
      option(GIT_SUBMODULE "Check submodules during build" ON)
      if(GIT_SUBMODULE)
          message(STATUS "Submodule update")
          execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                          RESULT_VARIABLE GIT_SUBMOD_RESULT)
          if(NOT GIT_SUBMOD_RESULT EQUAL "0")
              message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
          endif()
      endif()
  endif()

# ############################################################################
# Setup documentation options
# ############################################################################
  set(DOXYGEN_GENERATE_TODOLIST NO)
  set(DOXYGEN_GENERATE_DEPRECATEDLIST NO)
  set(GENERATE_HTMLHELP NO)
  set(DOXYGEN_GENERATE_MAN NO)
  set(GENERATE_QHP NO)
  set(GENERATE_TREEVIEW YES)

  # Add default files
  file(GLOB_RECURSE DOX *.dox*)
  set(COIN_DOCUMENTATION_FILES
    ${COIN_DOCUMENTATION_FILES}
    "${PROJECT_SOURCE_DIR}/docs/releases.dox"
    ${DOX}
    CACHE INTERNAL "COIN_DOCUMENTATION_FILES"
  )

  set(DOXYGEN_INTERNAL_DOCS NO)
  set(DOXYGEN_EXTRACT_PRIVATE NO)
  set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
  set(DOXYGEN_EXCLUDE)
  if(COIN_BUILD_INTERNAL_DOCUMENTATION)
    set(DOXYGEN_INTERNAL_DOCS YES)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
    set(COIN_DOCUMENTATION_FILES
      ${COIN_DOCUMENTATION_FILES}
      ${COIN_INTERNAL_DOCUMENTATION_FILES}
      CACHE INTERNAL "COIN_DOCUMENTATION_FILES"
    )
  else()
    set(COIN_DOCUMENTATION_EXCLUDE_SYMBOLS SoProfilerP)
    foreach(XF ${COIN_INTERNAL_DOCUMENTATION_FILES})
      set(DOXYGEN_EXCLUDE "${DOXYGEN_EXCLUDE}                         ${XF} \\\n")
    endforeach()
  endif()
  set(DOXYGEN_INPUT)
  list(REMOVE_DUPLICATES COIN_DOCUMENTATION_FILES)
  foreach(XF ${COIN_DOCUMENTATION_FILES})
    set(DOXYGEN_INPUT "${DOXYGEN_INPUT}                         ${XF} \\\n")
  endforeach()
  set(DOXYGEN_INPUT "${DOXYGEN_INPUT}                         ${PROJECT_SOURCE_DIR}/src/tidbits.cpp")
  set(DOXYGEN_EXCLUDE_SYMBOLS)
  foreach(XF ${COIN_DOCUMENTATION_EXCLUDE_SYMBOLS})
    set(DOXYGEN_EXCLUDE_SYMBOLS "${DOXYGEN_EXCLUDE_SYMBOLS}                         ${XF} \\\n")
  endforeach()

  set(GITHUB_LINK "https://github.com/coin3d/coin")
  set(DOXYFILE_AWESOME "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile_awesome")
  set(HTML_HEADER_AWESOME "${CMAKE_CURRENT_BINARY_DIR}/header_awesome.html")
  set(DOXYGEN_OUTPUT_AWESOME "${CMAKE_BINARY_DIR}/html_awesome/index.html")
  configure_file("${PROJECT_SOURCE_DIR}/docs/coin.doxygen.awesome.cmake.in" ${DOXYFILE_AWESOME} @ONLY)
  configure_file("${PROJECT_SOURCE_DIR}/docs/doxygen-awesome/doxygen-custom/header.html.cmake.in" ${HTML_HEADER_AWESOME} @ONLY)

# ############################################################################
# Setup documentation targets
# ############################################################################
  add_custom_command(
    OUTPUT ${DOXYGEN_OUTPUT_AWESOME}
    COMMAND ${CMAKE_COMMAND} -E echo_append "Generating modern API documentation with Doxygen "
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_AWESOME}
    COMMAND ${CMAKE_COMMAND} -E echo "done."
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS ${DOXYFILE_AWESOME}
  )
  add_custom_target(documentation_awesome ALL DEPENDS ${DOXYGEN_OUTPUT_AWESOME})

# ############################################################################
# Install built documentation files
# ############################################################################
  if(NOT COIN_BUILD_MAC_FRAMEWORK)
    install(DIRECTORY "${CMAKE_BINARY_DIR}/html_awesome" DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT documentation REGEX ".*\\.(chm|qch)" EXCLUDE)
  endif()
endif()
