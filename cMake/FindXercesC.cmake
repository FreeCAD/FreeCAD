# Locate Xerces-C include paths and libraries
# Xerces-C can be found at http://xml.apache.org/xerces-c/
# Written by Frederic Heem, frederic.heem _at_ telsey.it
# Modified by Jos van den Oever

# This module defines
# XercesC_INCLUDE_DIRS, where to find ptlib.h, etc.
# XercesC_LIBRARIES, the libraries to link against to use pwlib.
# XercesC_FOUND, If false, don't try to use pwlib.

FIND_PATH(XercesC_INCLUDE_DIRS xercesc/dom/DOM.hpp
  ${CMAKE_INCLUDE_PATH}
  "[HKEY_CURRENT_USER\\software\\xerces-c\\src]"
  "[HKEY_CURRENT_USER\\xerces-c\\src]"
  $ENV{XERCESCROOT}/src/
  /usr/local/include
  /usr/include
)

FIND_LIBRARY(XercesC_LIBRARIES
  NAMES
    xerces-c
    xerces-c_3
  PATHS
    ${CMAKE_LIBRARY_PATH}
    "[HKEY_CURRENT_USER\\software\\xerces-c\\lib]"
    "[HKEY_CURRENT_USER\\xerces-c\\lib]"
    $ENV{XERCESCROOT}/${LIB_DESTINATION}
    /usr/local/${LIB_DESTINATION}
    /usr/${LIB_DESTINATION}
)

# if the include a the library are found then we have it
IF(XercesC_INCLUDE_DIRS AND XercesC_LIBRARIES)
  SET(XercesC_FOUND "YES" )
  IF(NOT XERCESC__FIND_QUIETLY)
    MESSAGE(STATUS "Found Xerces-C: ${XercesC_LIBRARIES}")
  ENDIF(NOT XERCESC__FIND_QUIETLY)
ELSE(XercesC_INCLUDE_DIRS AND XercesC_LIBRARIES)
  IF(XERCESC_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Xerces-C was not found.")
  ENDIF(XERCESC_FIND_REQUIRED)
  IF(NOT XERCESC__FIND_QUIETLY)
    MESSAGE(STATUS "Xerces-C was not found.")
  ENDIF(NOT XERCESC__FIND_QUIETLY)
ENDIF(XercesC_INCLUDE_DIRS AND XercesC_LIBRARIES)

#MARK_AS_ADVANCED(
#  XercesC_INCLUDE_DIRS
#  XercesC_LIBRARIES
#)
