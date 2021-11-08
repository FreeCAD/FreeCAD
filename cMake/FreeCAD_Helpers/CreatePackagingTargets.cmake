macro(CreatePackagingTargets)
    # ================================================================================
    # == Packaging ===================================================================
    #
    #add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
    add_custom_target(dist-git
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/makedist.py
              --bindir=${CMAKE_BINARY_DIR}
              --major=${PACKAGE_VERSION_MAJOR}
              --minor=${PACKAGE_VERSION_MINOR}
              WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_custom_target(distdfsg-git
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/makedist.py
              --bindir=${CMAKE_BINARY_DIR}
              --major=${PACKAGE_VERSION_MAJOR}
              --minor=${PACKAGE_VERSION_MINOR}
              --dfsg
              WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX OR MINGW)
        add_custom_target(distcheck-git
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/makedist.py
                  --bindir=${CMAKE_BINARY_DIR}
                  --major=${PACKAGE_VERSION_MAJOR}
                  --minor=${PACKAGE_VERSION_MINOR}
                  --check
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
        add_custom_target(distcheckdfsg-git
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/makedist.py
                  --bindir=${CMAKE_BINARY_DIR}
                  --major=${PACKAGE_VERSION_MAJOR}
                  --minor=${PACKAGE_VERSION_MINOR}
                  --dfsg
                  --check
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()

endmacro(CreatePackagingTargets)
