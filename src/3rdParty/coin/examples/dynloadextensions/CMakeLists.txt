if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DCOIN_DEBUG=1)
else()
  add_definitions(-DCOIN_DEBUG=0)
endif()

include_directories(BEFORE ${PROJECT_SOURCE_DIR}/include)

add_library(Switchboard SHARED Switchboard.cpp)
target_compile_definitions(Switchboard PRIVATE "-DSWITCHBOARD_INTERNAL")
target_include_directories(Switchboard PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/include/Inventor/annex
	${PROJECT_BINARY_DIR}/include
	${COIN_TARGET_INCLUDE_DIRECTORIES}
)

add_library(SwitchboardOperator SHARED SwitchboardOperator.cpp)
target_compile_definitions(SwitchboardOperator PRIVATE "-DSWITCHBOARDOPERATOR_INTERNAL")
target_include_directories(SwitchboardOperator PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/include/Inventor/annex
	${PROJECT_BINARY_DIR}/include
	${COIN_TARGET_INCLUDE_DIRECTORIES}
)
target_link_libraries(SwitchboardOperator Switchboard)
