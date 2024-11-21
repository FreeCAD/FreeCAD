find_package( Git QUIET )

if ( GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git" )
    # Update submodules as needed
    option( E57_GIT_SUBMODULE_UPDATE "Check submodules and update during build" ON )

    if ( E57_GIT_SUBMODULE_UPDATE )
        message( STATUS "Submodule update using git (${GIT_EXECUTABLE})" )
        message( STATUS "Submodule update directory: ${CMAKE_CURRENT_SOURCE_DIR}" )

        execute_process( COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                            RESULT_VARIABLE GIT_SUBMOD_RESULT
                            ERROR_VARIABLE GIT_SUBMOD_RESULT)

        if ( GIT_SUBMOD_RESULT EQUAL "0" )
            message( STATUS "Submodule update complete" )
        else()
            message( FATAL_ERROR "Submodule update failed with ${GIT_SUBMOD_RESULT}, please checkout submodules" )
        endif()
    endif()
endif()
