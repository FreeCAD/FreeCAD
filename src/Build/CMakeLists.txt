#FIND_PACKAGE(Subversion)

#IF(Subversion_FOUND)
#    Subversion_WC_INFO(${CMAKE_SOURCE_DIR} Project)
#    MESSAGE(STATUS "Current revision is ${Project_WC_REVISION}")
#ENDIF(Subversion_FOUND)


#add_custom_target(Version ALL
#        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/SubWCRev.py
#          --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR}
#          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#    DEPENDS Version.h.in
#    COMMENT Building Version.h out of Version.h.in
#)


if (WIN32)
    # TortoiseSVN
    FIND_PROGRAM(Subversion_WCREF_EXECUTABLE SubWCRev)
    SET (RESULT 1)
    if(Subversion_WCREF_EXECUTABLE)
    EXECUTE_PROCESS(COMMAND ${Subversion_WCREF_EXECUTABLE} ${CMAKE_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/Version.h
      RESULT_VARIABLE RESULT
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif(Subversion_WCREF_EXECUTABLE)
    # No SVN repo
    if (NOT RESULT EQUAL 0)
    EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/SubWCRev.py
      --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif(NOT RESULT EQUAL 0)
else (WIN32)
    EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/SubWCRev.py
      --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_STRIP_TRAILING_WHITESPACE)
endif (WIN32)

# as fallback if Version.h isn't created for any reason
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/Version.h)
    file(READ  ${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in VERSION_INFO)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Version.h  ${VERSION_INFO})
endif (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/Version.h)
