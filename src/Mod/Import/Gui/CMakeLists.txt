if(MSVC)
    add_definitions(-DFCAppPart -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)


include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
)

link_directories(${OCC_LIBRARY_DIR})

set(ImportGui_LIBS
    FreeCADGui
    PartGui
    Import
    ${OCC_OCAF_LIBRARIES}
    ${OCC_OCAF_DEBUG_LIBRARIES}
)

SET(ImportGui_SRCS
    AppImportGui.cpp
    AppImportGuiPy.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(ImportGui SHARED ${ImportGui_SRCS})
target_link_libraries(ImportGui ${ImportGui_LIBS})


SET_BIN_DIR(ImportGui ImportGui /Mod/Import)
SET_PYTHON_PREFIX_SUFFIX(ImportGui)

INSTALL(TARGETS ImportGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
