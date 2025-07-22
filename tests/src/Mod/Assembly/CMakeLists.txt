add_subdirectory(App)

if (NOT FREECAD_USE_EXTERNAL_ONDSELSOLVER)
	target_include_directories(Assembly_tests_run PUBLIC
		${CMAKE_SOURCE_DIR}/src/3rdParty/OndselSolver
	)
endif ()

target_link_libraries(Assembly_tests_run
    gtest_main
    ${Google_Tests_LIBS}
    Assembly
)
