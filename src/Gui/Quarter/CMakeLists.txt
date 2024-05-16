#quarter sources
FILE(GLOB_RECURSE Quarter_CPP_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
FILE(GLOB_RECURSE Quarter_H_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)

SET(Quarter_MOC_HDR
    src/Quarter/SignalThread.h
    src/Quarter/InteractionMode.h
    src/Quarter/SensorManager.h
    src/Quarter/ContextMenu.h
    include/Quarter/eventhandlers/FocusHandler.h
    include/Quarter/eventhandlers/DragDropHandler.h
    include/Quarter/eventhandlers/EventFilter.h
    include/Quarter/QuarterWidget.h
)

qt_wrap_cpp(Quarter_MOC_SRCS ${Quarter_MOC_HDR})

SET(Quarter_SRCS
    ${Quarter_CPP_SRC}
    ${Quarter_H_SRC}
    ${Quarter_MOC_SRCS}
)
SOURCE_GROUP("Quarter" FILES ${Quarter_SRCS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(Spacenav_FOUND)
  add_definitions(-DHAVE_SPACENAV_LIB)
  include_directories(${Spacenav_INCLUDE_DIR})
  set(Extra_libraries ${Spacenav_LIBRARIES} ${Spacenav_LIBRARY})
endif()

if( UNIX )
    set( Quarter_BUILD_SHARED ON )
ELSEIF ( APPLE )
    set( Quarter_BUILD_SHARED ON )
ELSE()
    set( Quarter_BUILD_SHARED OFF )
ENDIF ()

if ( Quarter_BUILD_SHARED )
    message( STATUS "[Quarter] Building shared library" )
    add_library(Quarter SHARED)
else()
    message( STATUS "[Quarter] Building static library" )
    add_library(Quarter STATIC)
endif()

target_sources(Quarter PRIVATE ${Quarter_SRCS})

set_target_properties(Quarter PROPERTIES PUBLIC_HEADER "${Quarter_H_SRC}")

target_include_directories(Quarter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${COIN3D_INCLUDE_DIRS} ${QtCore_INCLUDE_DIRS} ${QtWidgets_INCLUDE_DIRS})
target_link_libraries(Quarter ${COIN3D_LIBRARIES} ${OPENGL_LIBRARIES} ${QtCore_LIBRARIES} ${QtWidgets_LIBRARIES} ${Extra_libraries})

SET_BIN_DIR(Quarter Quarter)

if(WIN32)
add_definitions(-DQUARTER_INTERNAL -DQUARTER_MAKE_DLL -DCOIN_DLL)
endif(WIN32)

install(TARGETS Quarter DESTINATION ${CMAKE_INSTALL_LIBDIR})
