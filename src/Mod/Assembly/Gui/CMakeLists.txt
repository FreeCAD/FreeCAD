include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(AssemblyGui_LIBS
    FreeCADGui
)

PYSIDE_WRAP_RC(Assembly_QRC_SRCS Resources/Assembly.qrc)

set (Assembly_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Assembly_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Assembly_TR_QRC} ${QM_SRCS})
qt_add_resources(AssemblyResource_SRCS Resources/Assembly.qrc ${Assembly_TR_QRC})

SOURCE_GROUP("Resources" FILES ${AssemblyResource_SRCS})


SET(AssemblyGui_SRCS_Module
    ${Assembly_QRC_SRCS}
)

SOURCE_GROUP("Module" FILES ${AssemblyGui_SRCS_Module})


SET(AssemblyGui_SRCS
    ${AssemblyResource_SRCS}
    ${AssemblyGui_UIC_HDRS}
    ${AssemblyGui_SRCS_Module}
)


SET(AssemblyGuiIcon_SVG
    Resources/icons/AssemblyWorkbench.svg
)

add_library(AssemblyGui SHARED ${AssemblyGui_SRCS} ${AssemblyGuiIcon_SVG})
target_link_libraries(AssemblyGui ${AssemblyGui_LIBS})

SET_BIN_DIR(AssemblyGui AssemblyGui /Mod/Assembly)
SET_PYTHON_PREFIX_SUFFIX(AssemblyGui)

fc_copy_sources(AssemblyGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Assembly" ${AssemblyGuiIcon_SVG})
fc_target_copy_resource(AssemblyGui
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/Assembly
        Assembly_rc.py)

INSTALL(TARGETS AssemblyGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${AssemblyGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Assembly/Resources/icons")
