project (libkdtree CXX)
cmake_minimum_required (VERSION 2.6.0)

option (BUILD_PYTHON_BINDINGS "Build Python bindings (requires SWIG)")

if (WIN32)

   # Maximum warning level
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

   # Be strict about warnings... make them errors
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")

   # Detect 64-bit portability issues
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wp64")

else (WIN32)

   # Maximum warning level
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

   # turn on debugging
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

endif (WIN32)

if (BUILD_PYTHON_BINDINGS)
   add_subdirectory (python-bindings)
endif (BUILD_PYTHON_BINDINGS)

include_directories (${PROJECT_SOURCE_DIR})

add_subdirectory(examples)

file (GLOB KDTREE_HEADERS kdtree++/*.hpp)
install (FILES ${KDTREE_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

