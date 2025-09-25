macro(SetupEigen)
# -------------------------------- Eigen --------------------------------

    # necessary for Sketcher module
    # necessary for Robot module

    find_package(Eigen3)
    if(NOT Eigen3_FOUND)
        message("=================\n"
                "Eigen3 not found.\n"
                "=================\n")
    endif(NOT Eigen3_FOUND)

    if (Eigen3_FOUND AND ${Eigen3_VERSION} VERSION_LESS "3.4.0")
        message(WARNING "Disable module flatmesh because it requires "
                        "minimum Eigen3 version 3.4.0 but version ${Eigen3_VERSION} was found")
        set (BUILD_FLAT_MESH OFF)
    endif()

if(NOT Eigen3_INCLUDE_DIR)
find_path(Eigen3_INCLUDE_DIR NAMES signature_of_eigen3_matrix_library
     PATHS
     ${CMAKE_INSTALL_PREFIX}/include
     PATH_SUFFIXES eigen3 eigen
)
endif(NOT Eigen3_INCLUDE_DIR)

endmacro(SetupEigen)
