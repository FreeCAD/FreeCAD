
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Surface_Scripts
    Init.py
    TestSurfaceApp.py
)

set(Surface_TestScripts
    SurfaceTests/__init__.py
    SurfaceTests/TestBlendCurve.py
)

if(BUILD_GUI)
    list (APPEND Surface_Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(SurfaceScripts ALL
    SOURCES ${Surface_Scripts} ${Surface_TestScripts}
)

fc_target_copy_resource(SurfaceScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Surface
    ${Surface_Scripts}
    ${Surface_TestScripts}
)

install(
    FILES
        ${Surface_Scripts}
    DESTINATION
        Mod/Surface
)

install(
    FILES
        ${Surface_TestScripts}
    DESTINATION
        Mod/Surface/SurfaceTests
)
