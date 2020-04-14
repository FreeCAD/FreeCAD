macro(SetupEigen)
# -------------------------------- Eigen --------------------------------

    # necessary for Sketcher module
    # necessary for Robot module

    find_package(Eigen3)
    if(NOT EIGEN3_FOUND)
        message("=================\n"
                "Eigen3 not found.\n"
                "=================\n")
    endif(NOT EIGEN3_FOUND)

    if (${EIGEN3_VERSION} VERSION_LESS "3.3.1")
        message(WARNING "Disable module flatmesh because it requires "
                        "minimum Eigen3 version 3.3.1 but version ${EIGEN3_VERSION} was found")
        set (BUILD_FLAT_MESH OFF)
    endif()

endmacro(SetupEigen)
