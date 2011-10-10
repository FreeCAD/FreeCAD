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
    if(Subversion_WCREF_EXECUTABLE)
    EXECUTE_PROCESS(COMMAND ${Subversion_WCREF_EXECUTABLE} ${CMAKE_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/Version.h
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    else(Subversion_WCREF_EXECUTABLE)
    EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/SubWCRev.py
      --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif(Subversion_WCREF_EXECUTABLE)
else (WIN32)
    EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/SubWCRev.py
      --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_STRIP_TRAILING_WHITESPACE)
endif (WIN32)

# as fallback if Version.h isn't created for any reason
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/Version.h)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Version.h
    "/// Version Number\n"
    "#define FCVersionMajor \"0\"\n"
    "#define FCVersionMinor \"11\"\n"
    "#define FCVersionName  \"Dagoba\"\n"
    "\n"
    "#define FCRevision      \"unknown\"\n"
    "#define FCRevisionDate  \"unknown\"\n"
    "#define FCRevisionRange \"unknown\"\n"
    "#define FCRepositoryURL \"https://free-cad.svn.sourceforge.net/svnroot/free-cad/trunk\"\n"
    "#define FCCurrentDateT  \"unknown\"\n"
    )
endif (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/Version.h)
