if(MSVC)
    add_definitions(-DFCAppPart -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)


include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
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


fc_target_copy_resource(ImportGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Import
    ${CMAKE_BINARY_DIR}/Mod/Import
    Init.py InitGui.py)

if(MSVC)
    set_target_properties(ImportGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(ImportGui PROPERTIES DEBUG_OUTPUT_NAME "ImportGui_d")
    set_target_properties(ImportGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Import)
    set_target_properties(ImportGui PROPERTIES PREFIX "../")
    # Set special compiler flag to convert a SIGSEV into an exception
    set_target_properties(ImportGui PROPERTIES COMPILE_FLAGS "/EHa")
elseif(MINGW)
    set_target_properties(ImportGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(ImportGui PROPERTIES DEBUG_OUTPUT_NAME "ImportGui_d")
    set_target_properties(ImportGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Import)
    set_target_properties(ImportGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(ImportGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Import)
    set_target_properties(ImportGui PROPERTIES PREFIX "")
    set_target_properties(ImportGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS ImportGui DESTINATION lib)
