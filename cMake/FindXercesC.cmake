# Locate Xerces-C include paths and libraries
# Xerces-C can be found at http://xml.apache.org/xerces-c/
# Written by Frederic Heem, frederic.heem _at_ telsey.it
# Modified by Jos van den Oever

# This module defines
# XERCESC_INCLUDE_DIR, where to find ptlib.h, etc.
# XERCESC_LIBRARIES, the libraries to link against to use pwlib.
# XERCESC_FOUND, If false, don't try to use pwlib.

FIND_PATH(XERCESC_INCLUDE_DIR xercesc/dom/DOM.hpp
  "[HKEY_CURRENT_USER\\software\\xerces-c\\src]"
  "[HKEY_CURRENT_USER\\xerces-c\\src]"
  $ENV{XERCESCROOT}/src/
  /usr/local/include
  /usr/include
)

FIND_LIBRARY(XERCESC_LIBRARIES
  NAMES
    xerces-c
  PATHS
    "[HKEY_CURRENT_USER\\software\\xerces-c\\lib]"
    "[HKEY_CURRENT_USER\\xerces-c\\lib]"
    $ENV{XERCESCROOT}/${LIB_DESTINATION}
    /usr/local/${LIB_DESTINATION}
    /usr/${LIB_DESTINATION}
)

# if the include a the library are found then we have it
IF(XERCESC_INCLUDE_DIR AND XERCESC_LIBRARIES)
  SET(XERCESC_FOUND "YES" )
  IF(NOT XERCESC__FIND_QUIETLY)
    MESSAGE(STATUS "Found Xerces-C: ${XERCESC_LIBRARIES}")
  ENDIF(NOT XERCESC__FIND_QUIETLY)
ELSE(XERCESC_INCLUDE_DIR AND XERCESC_LIBRARIES)
  IF(XERCESC_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Xerces-C was not found.")
  ENDIF(XERCESC_FIND_REQUIRED)
  IF(NOT XERCESC__FIND_QUIETLY)
    MESSAGE(STATUS "Xerces-C was not found.")
  ENDIF(NOT XERCESC__FIND_QUIETLY)
ENDIF(XERCESC_INCLUDE_DIR AND XERCESC_LIBRARIES)

#MARK_AS_ADVANCED(
#  XERCESC_INCLUDE_DIR
#  XERCESC_LIBRARIES
#)
