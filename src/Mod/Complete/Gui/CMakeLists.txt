include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

set(CompleteGui_LIBS
    Complete
    FreeCADGui
)

qt4_add_resources(Complete_QRC_SRCS Resources/Complete.qrc)

SET(CompleteGui_SRCS
    ${Complete_QRC_SRCS}
    AppCompleteGui.cpp
    AppCompleteGuiPy.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(CompleteGui SHARED ${CompleteGui_SRCS})
target_link_libraries(CompleteGui ${CompleteGui_LIBS})
fc_copy_script("Mod/Complete" "CompleteGui" InitGui.py)

if(MSVC)
    set_target_properties(CompleteGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(CompleteGui PROPERTIES DEBUG_OUTPUT_NAME "CompleteGui_d")
    set_target_properties(CompleteGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Complete)
    set_target_properties(CompleteGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(CompleteGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(CompleteGui PROPERTIES DEBUG_OUTPUT_NAME "CompleteGui_d")
    set_target_properties(CompleteGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Complete)
    set_target_properties(CompleteGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(CompleteGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Complete)
    set_target_properties(CompleteGui PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS CompleteGui DESTINATION lib)
