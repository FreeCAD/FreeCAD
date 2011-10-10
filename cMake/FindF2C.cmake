# This module finds the f2c library.
#
# This module sets the following variables:
#  F2C_FOUND - set to true if library is found
#  F2C_DEFINITIONS - compilation options to use f2c
#  F2C_LIBRARIES - f2c library name (using full path name)

if (F2C_LIBRARIES)

  set(F2C_FOUND TRUE)

else(F2C_LIBRARIES)

  set(F2C_DEFINITIONS)

  find_library(F2C_LIBRARIES f2c
               /usr/lib
               /usr/local/lib
              )

  if(F2C_LIBRARIES)
    set(F2C_FOUND TRUE)
  else()
    set(F2C_FOUND FALSE)
  endif()

  if(NOT F2C_FIND_QUIETLY)
    if(F2C_FOUND)
      message(STATUS "f2c library found.")
    else(F2C_FOUND)
      if(F2C_FIND_REQUIRED)
        message(FATAL_ERROR "f2c library not found. Please specify library location.")
      else()
        message(STATUS "f2c library not found. Please specify library location.")
      endif()
    endif(F2C_FOUND)
  endif(NOT F2C_FIND_QUIETLY)

endif(F2C_LIBRARIES)
