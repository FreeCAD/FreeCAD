macro(SetupCoverage)
    if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
        # per Professional CMake 20th Edition, 33.2.1. gcov-based Coverage
        if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
            execute_process(
                    COMMAND xcrun --find gcov
                    OUTPUT_VARIABLE GCOV_EXECUTABLE
                    OUTPUT_STRIP_TRAILING_WHITESPACE
            )
        elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            find_program(LLVM_COV_EXECUTABLE llvm-cov REQUIRED)
            file(CREATE_LINK ${LLVM_COV_EXECUTABLE} ${CMAKE_BINARY_DIR}/gcov SYMBOLIC)
            set(GCOV_EXECUTABLE "${LLVM_COV_EXECUTABLE} gcov")
        else () # Assuming gcc for this example
            find_program(GCOV_EXECUTABLE gcov REQUIRED)
        endif ()

        find_program(GCOVR_EXECUTABLE gcovr REQUIRED)

        configure_file(gcovr.cfg.in gcovr.cfg @ONLY)

        add_custom_target(process_coverage
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running gcovr to process coverage results"
                COMMAND ${GCOVR_EXECUTABLE} --gcov-ignore-errors=no_working_dir_found --config gcovr.cfg .
        )
    endif()
endmacro(SetupCoverage)