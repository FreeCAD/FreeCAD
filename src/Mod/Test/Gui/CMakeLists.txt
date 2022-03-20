include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
)

set(TestGui_LIBS
    FreeCADGui
)

set(Dialogs_UIC_SRCS
    UnitTest.ui
)

if(BUILD_QT5)
    qt5_wrap_ui(Dialogs_UIC_HDRS ${Dialogs_UIC_SRCS})
else()
    qt4_wrap_ui(Dialogs_UIC_HDRS ${Dialogs_UIC_SRCS})
endif()

SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    UnitTest.ui
    UnitTestImp.cpp
    UnitTestImp.h
    UnitTestPy.cpp
    UnitTestPy.h
)
SOURCE_GROUP("Dialogs" FILES ${Dialogs_SRCS})

if(BUILD_QT5)
    qt5_add_resources(Resource_SRCS Resources/Test.qrc)
else()
    qt4_add_resources(Resource_SRCS Resources/Test.qrc)
endif()

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

SET(TestGuiIcon_SVG
    Resources/icons/TestWorkbench.svg
)

add_library(QtUnitGui SHARED ${TestGui_SRCS} ${TestGuiPy_SRCS} ${TestGuiIcon_SVG})

target_link_libraries(QtUnitGui ${TestGui_LIBS})
add_dependencies(QtUnitGui Test)

fc_copy_sources(QtUnitGui "${CMAKE_BINARY_DIR}/Mod/Test" qtunittest.py)
fc_copy_sources(QtUnitGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Test" ${TestGuiIcon_SVG})

SET_BIN_DIR(QtUnitGui QtUnitGui /Mod/Test)
SET_PYTHON_PREFIX_SUFFIX(QtUnitGui)

INSTALL(TARGETS QtUnitGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${TestGuiPy_SRCS} DESTINATION Mod/Test)
INSTALL(FILES ${TestGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Test/Resources/icons")
