cmake_minimum_required(VERSION 3.16)

project(OndselSolverLibrary VERSION 1.0.1 DESCRIPTION "Assembly Constraints and Multibody Dynamics code")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -gdwarf-4")

include(GNUInstallDirs)

file(GLOB ONDSELSOLVER_SOURCES "*.cpp")
file(GLOB ONDSELSOLVER_HEADERS "*.h")
add_library(OndselSolver STATIC ${ONDSELSOLVER_SOURCES} ${ONDSELSOLVER_HEADERS})
target_include_directories(OndselSolver PUBLIC ${CMAKE_SOURCE_DIR})

#target_include_directories(OndselSolver PUBLIC ${CMAKE_SOURCE_DIR}/OndselSolver)

#configure_file(OndselSolver.pc.in ${CMAKE_BINARY_DIR}/OndselSolver.pc @ONLY)
#install(TARGETS OndselSolver
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OndselSolver)
#install(FILES ${CMAKE_BINARY_DIR}/OndselSolver.pc
#        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
