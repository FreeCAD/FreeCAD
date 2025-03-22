# Add include folders to the library and targets that consume it
# the SYSTEM keyword suppresses warnings for users of the library
#
# By adding this directory as an include directory the user gets a
# namespace effect.
#
# IE:
#   #include <gsl/gsl>
if(PROJECT_IS_TOP_LEVEL)
    target_include_directories(GSL INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
else()
    target_include_directories(GSL SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
endif()
