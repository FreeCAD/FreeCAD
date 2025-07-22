INCLUDE(SimQtMacros)

SET(Library Quarter)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# We R internal
ADD_DEFINITIONS(-DQUARTER_INTERNAL)

FILE(GLOB CMakeCache RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/CMakeCache.txt)

# Generate list of sources and headers for compilation
FILE(GLOB_RECURSE Sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
FILE(GLOB_RECURSE PublicHeaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)
FILE(GLOB_RECURSE PrivateHeaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ../../include/Quarter/*.h)


# Some convenient variables. The inclusion of the headers in the source variable is to force CMake to include the headers in VS project generation.
SET(AllSources ${Sources} ${PublicHeaders} ${PrivateHeaders} ${CMakeCache})


IF (Spacenav_FOUND)
  SET(Extra_libraries ${Spacenav_LIBRARIES} ${Spacenav_LIBRARY})
ENDIF(Spacenav_FOUND)


SET(AllLibs ${Coin_LIBRARY} ${QT_LIBRARIES} ${OPENGL_LIBRARY} ${Extra_libraries})



# Ensure export of DLL definitions on Windows
IF(WIN32 AND NOT Quarter_BUILD_STATIC_LIBRARY)
  ADD_DEFINITIONS(-DQUARTER_MAKE_DLL)
ENDIF(WIN32 AND NOT Quarter_BUILD_STATIC_LIBRARY)

# Add some files and filters to make the VS project more hierarchical and practical to work with
IF(MSVC_IDE)
  FILE(GLOB DeviceSources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} devices/*.cpp)
  FILE(GLOB DeviceHeaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} devices/*.h)
  FILE(GLOB PrivateDeviceHeaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ../../include/Quarter/devices/*.h)

  FILE(GLOB EventHandlerSources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} eventhandlers/*.cpp)
  FILE(GLOB EventHandlerHeaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} eventhandlers/*.h)
  FILE(GLOB PrivateEventHandlerHeaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ../../include/Quarter/eventhandlers/*.h)

  SIM_VS_GENERATE_BASIC_TREE()
  SOURCE_GROUP("Source\\Devices" FILES ${DeviceSources})
  SOURCE_GROUP("Source\\EventHandlers" FILES ${EventHandlerSources})
  SOURCE_GROUP("Headers\\Devices" FILES ${DeviceHeaders})
  SOURCE_GROUP("Headers\\Devices" FILES ${PrivateDeviceHeaders})
  SOURCE_GROUP("Headers\\EventHandlers" FILES ${EventHandlerHeaders})
  SOURCE_GROUP("Headers\\EventHandlers" FILES ${PrivateEventHandlerHeaders})
ENDIF(MSVC_IDE)

# Create the library
# FIXME: Support building as a framework on Mac OS X. kintel 20080707.
IF(Quarter_BUILD_STATIC_LIBRARY)
  ADD_LIBRARY(${Library} STATIC ${AllSources})
ELSE(Quarter_BUILD_STATIC_LIBRARY)
  ADD_LIBRARY(${Library} SHARED ${AllSources})
  TARGET_LINK_LIBRARIES(${Library} ${AllLibs})
ENDIF(Quarter_BUILD_STATIC_LIBRARY)

#  Install configuration
#
# Quarter will be installed into the directory configured by CMAKE_INSTALL_PREFIX. This
# defaults to /usr/local/ on UNIX and c:/Program Files/Quarter on Windows.
#
# On Windows, the namimg convention is as follows (ripped from the "Coin" way of doing stuff):
# Dynamic Release: Quarter.dll and Quarter.lib
# Dynamic Debug: Quarterd.dll and Quarterd.lib
# Static Release: Quarters.lib
# Static Debug: Quartersd.lib

INSTALL(TARGETS ${Library}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
