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
    TKXCAF
    TKLCAF
    TKXDESTEP
    TKXDEIGES
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
fc_copy_script("Mod/Import" "ImportGui" Init.py)
fc_copy_script("Mod/Import" "ImportGui" InitGui.py)

if(MSVC)
    set_target_properties(ImportGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(ImportGui PROPERTIES DEBUG_OUTPUT_NAME "ImportGui_d")
    set_target_properties(ImportGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Import)
    set_target_properties(ImportGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(ImportGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(ImportGui PROPERTIES DEBUG_OUTPUT_NAME "ImportGui_d")
    set_target_properties(ImportGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Import)
    set_target_properties(ImportGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(ImportGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Import)
    set_target_properties(ImportGui PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS ImportGui DESTINATION lib)
