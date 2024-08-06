add_subdirectory(App)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Assembly_Scripts
    Init.py
    CommandCreateAssembly.py
    CommandCreateBom.py
    CommandInsertLink.py
    CommandSolveAssembly.py
    CommandCreateJoint.py
    CommandCreateView.py
    CommandExportASMT.py
    TestAssemblyWorkbench.py
    JointObject.py
    Preferences.py
    AssemblyImport.py
    UtilsAssembly.py)

if(BUILD_GUI)
    list(APPEND Assembly_Scripts InitGui.py)
endif(BUILD_GUI)

install(FILES ${Assembly_Scripts} DESTINATION Mod/Assembly)

set(AssemblyScripts_SRCS Assembly/__init__.py)

set(AssemblyTests_SRCS AssemblyTests/__init__.py AssemblyTests/TestCore.py)

set(all_files ${AssemblyTests_SRCS} ${AssemblyScripts_SRCS})

add_custom_target(AssemblyScripts ALL SOURCES ${all_files})

set(test_files ${Assembly_Scripts} ${AssemblyTests_SRCS})

add_custom_target(AssemblyTests ALL SOURCES ${test_files})

fc_copy_sources(AssemblyScripts "${CMAKE_BINARY_DIR}/Mod/Assembly" ${all_files})
fc_copy_sources(AssemblyTests "${CMAKE_BINARY_DIR}/Mod/Assembly" ${test_files})

install(FILES ${AssemblyTests_SRCS} DESTINATION Mod/Assembly/AssemblyTests)
install(FILES ${AssemblyScripts_SRCS} DESTINATION Mod/Assembly/Assembly)
