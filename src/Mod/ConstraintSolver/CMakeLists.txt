
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(ConstraintSolver_Scripts
    Init.py
)

if(BUILD_GUI)
    list (APPEND ConstraintSolver_Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(ConstraintSolverScripts ALL
    SOURCES ${ConstraintSolver_Scripts}
)

fc_target_copy_resource_flat(ConstraintSolverScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/ConstraintSolver
    ${ConstraintSolver_Scripts}
)

install(
    FILES
        ${ConstraintSolver_Scripts}
    DESTINATION
        Mod/ConstraintSolver
)
