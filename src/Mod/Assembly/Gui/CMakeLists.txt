include_directories(${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${COIN3D_INCLUDE_DIRS}
                    ${OCC_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})

set(AssemblyGui_LIBS Assembly PartDesign PartGui Spreadsheet SpreadsheetGui FreeCADGui)

pyside_wrap_rc(Assembly_QRC_SRCS Resources/Assembly.qrc)

set(Assembly_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Assembly_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
                            ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Assembly_TR_QRC} ${QM_SRCS})
qt_add_resources(AssemblyResource_SRCS Resources/Assembly.qrc ${Assembly_TR_QRC})

source_group("Resources" FILES ${AssemblyResource_SRCS})

generate_from_xml(ViewProviderAssemblyPy)

set(Python_SRCS ViewProviderAssemblyPy.xml ViewProviderAssemblyPyImp.cpp)
source_group("Python" FILES ${Python_SRCS})

set(AssemblyGui_SRCS_Module
    AppAssemblyGui.cpp
    AppAssemblyGuiPy.cpp
    PreCompiled.cpp
    PreCompiled.h
    ViewProviderAssembly.cpp
    ViewProviderAssembly.h
    ViewProviderBom.cpp
    ViewProviderBom.h
    ViewProviderBomGroup.cpp
    ViewProviderBomGroup.h
    ViewProviderJointGroup.cpp
    ViewProviderJointGroup.h
    ViewProviderViewGroup.cpp
    ViewProviderViewGroup.h
    ${Assembly_QRC_SRCS})

source_group("Module" FILES ${AssemblyGui_SRCS_Module})

set(AssemblyGui_SRCS ${AssemblyResource_SRCS} ${AssemblyGui_UIC_HDRS} ${AssemblyGui_SRCS_Module}
                     ${Python_SRCS})

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    get_msvc_precompiled_source("PreCompiled.cpp" PCH_SRCS ${AssemblyGui_SRCS})
    add_msvc_precompiled_header(PathGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

set(AssemblyGuiIcon_SVG Resources/icons/AssemblyWorkbench.svg)

add_library(AssemblyGui SHARED ${AssemblyGui_SRCS} ${AssemblyGuiIcon_SVG})

if(WIN32)
    target_compile_definitions(AssemblyGui PRIVATE _USE_MATH_DEFINES)
endif(WIN32)

target_link_libraries(AssemblyGui ${AssemblyGui_LIBS})
if(FREECAD_WARN_ERROR)
    target_compile_warn_error(AssemblyGui)
endif()

set_bin_dir(AssemblyGui AssemblyGui /Mod/Assembly)
set_python_prefix_suffix(AssemblyGui)

fc_copy_sources(AssemblyGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Assembly"
                ${AssemblyGuiIcon_SVG})
fc_target_copy_resource(AssemblyGui ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR}/Mod/Assembly
                        Assembly_rc.py)

install(TARGETS AssemblyGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${AssemblyGuiIcon_SVG}
        DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Assembly/Resources/icons")
install(FILES ${Assembly_QRC_SRCS} DESTINATION Mod/Assembly)
