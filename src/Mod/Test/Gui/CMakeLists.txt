include_directories (${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src ${Boost_INCLUDE_DIRS}
                     ${PYTHON_INCLUDE_DIRS} ${XercesC_INCLUDE_DIRS})

set (TestGui_LIBS FreeCADGui)

set (Dialogs_UIC_SRCS UnitTest.ui)

set (Dialogs_SRCS ${Dialogs_UIC_HDRS} UnitTest.ui UnitTestImp.cpp UnitTestImp.h UnitTestPy.cpp
                  UnitTestPy.h)
source_group ("Dialogs" FILES ${Dialogs_SRCS})

set (Test_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Test_translation.qrc)
qt_find_and_add_translation (QM_SRCS "Resources/translations/*_*.ts"
                             ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file (${Test_TR_QRC} ${QM_SRCS})
qt_add_resources (Resource_SRCS Resources/Test.qrc ${Test_TR_QRC})

set (Resource_SRCS ${Resource_SRCS} Resources/Test.qrc)
source_group ("Resource" FILES ${Resource_SRCS})

set (TestGui_SRCS ${Dialogs_SRCS} ${Resource_SRCS} AppTestGui.cpp PreCompiled.cpp PreCompiled.h)
set (TestGuiPy_SRCS qtunittest.py)

set (TestGuiIcon_SVG Resources/icons/TestWorkbench.svg)

if (FREECAD_USE_PCH)
    add_definitions (-D_PreComp_)
    get_msvc_precompiled_source ("PreCompiled.cpp" PCH_SRCS ${TestGui_SRCS})
    add_msvc_precompiled_header (QtUnitGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif (FREECAD_USE_PCH)

add_library (QtUnitGui SHARED ${TestGui_SRCS} ${TestGuiPy_SRCS} ${TestGuiIcon_SVG})

target_link_libraries (QtUnitGui ${TestGui_LIBS})
add_dependencies (QtUnitGui Test)
if (FREECAD_WARN_ERROR)
    target_compile_warn_error (QtUnitGui)
endif ()

fc_copy_sources (QtUnitGui "${CMAKE_BINARY_DIR}/Mod/Test" qtunittest.py)
fc_copy_sources (QtUnitGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Test"
                 ${TestGuiIcon_SVG})

set_bin_dir (QtUnitGui QtUnitGui /Mod/Test)
set_python_prefix_suffix (QtUnitGui)

install (TARGETS QtUnitGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (FILES ${TestGuiPy_SRCS} DESTINATION Mod/Test)
install (FILES ${TestGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Test/Resources/icons")
