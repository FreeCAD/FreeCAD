include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${QT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

set(TestGui_LIBS
    FreeCADGui
)

set(Test_MOC_HDRS
    UnitTestImp.h
)
fc_wrap_cpp(Test_MOC_SRCS ${Test_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${Test_MOC_SRCS})

set(Dialogs_UIC_SRCS
    UnitTest.ui
)
qt4_wrap_ui(Dialogs_UIC_HDRS ${Dialogs_UIC_SRCS})
SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    UnitTest.ui
    UnitTestImp.cpp
    UnitTestImp.h
    UnitTestPy.cpp
    UnitTestPy.h
)
SOURCE_GROUP("Dialogs" FILES ${Dialogs_SRCS})

qt4_add_resources(Resource_SRCS Resources/Test.qrc)
SET(Resource_SRCS
    ${Resource_SRCS}
    Resources/Test.qrc
)
SOURCE_GROUP("Resource" FILES ${Resource_SRCS})

SET(TestGui_SRCS
    ${Dialogs_SRCS}
    ${Resource_SRCS}
    AppTestGui.cpp
    PreCompiled.cpp
    PreCompiled.h
)
SET(TestGuiPy_SRCS
    qtunittest.py
)

add_library(QtUnitGui SHARED ${TestGui_SRCS})

target_link_libraries(QtUnitGui ${TestGui_LIBS})
add_dependencies(QtUnitGui Test)

fc_copy_sources(QtUnitGui "${CMAKE_BINARY_DIR}/Mod/Test" qtunittest.py)

if(MSVC)
    set_target_properties(QtUnitGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(QtUnitGui PROPERTIES DEBUG_OUTPUT_NAME "QtUnitGui_d")
    set_target_properties(QtUnitGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Test)
    set_target_properties(QtUnitGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(QtUnitGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(QtUnitGui PROPERTIES DEBUG_OUTPUT_NAME "QtUnitGui_d")
    set_target_properties(QtUnitGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Test)
    set_target_properties(QtUnitGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(QtUnitGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Test)
    set_target_properties(QtUnitGui PROPERTIES PREFIX "")
    set_target_properties(QtUnitGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS QtUnitGui DESTINATION lib)
INSTALL(FILES ${TestGuiPy_SRCS} DESTINATION Mod/Test)

