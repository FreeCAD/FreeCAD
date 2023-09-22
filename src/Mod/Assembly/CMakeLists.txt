add_subdirectory(App)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Assembly_Scripts
    Init.py
    CommandCreateAssembly.py
    CommandInsertLink.py
    CommandCreateJoint.py
    TestAssemblyWorkbench.py
    JointObject.py
    Preferences.py
    AssemblyImport.py
    UtilsAssembly.py
)

if(BUILD_GUI)
    list (APPEND Assembly_Scripts InitGui.py)
endif(BUILD_GUI)

INSTALL(
    FILES
    ${Assembly_Scripts}
    DESTINATION
    Mod/Assembly
)

SET(AssemblyScripts_SRCS
    Assembly/__init__.py
)


SET(AssemblyTests_SRCS
    AssemblyTests/__init__.py
    AssemblyTests/TestCore.py
)


SET(all_files
    ${AssemblyTests_SRCS}
    ${AssemblyScripts_SRCS}
)

ADD_CUSTOM_TARGET(AssemblyScripts ALL
    SOURCES ${all_files}
)

SET(test_files
    ${Assembly_Scripts}
    ${AssemblyTests_SRCS}
)

ADD_CUSTOM_TARGET(AssemblyTests ALL
    SOURCES ${test_files}
)

fc_copy_sources(AssemblyScripts "${CMAKE_BINARY_DIR}/Mod/Assembly" ${all_files})
fc_copy_sources(AssemblyTests "${CMAKE_BINARY_DIR}/Mod/Assembly" ${test_files})

INSTALL(
    FILES
        ${AssemblyTests_SRCS}
    DESTINATION
        Mod/Assembly/AssemblyTests
)
