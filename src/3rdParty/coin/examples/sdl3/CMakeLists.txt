if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DCOIN_DEBUG=1)
else()
  add_definitions(-DCOIN_DEBUG=0)
endif()

find_package(SDL3 REQUIRED)

add_executable(sdl3-example sdl3.cpp)

target_include_directories(sdl3-example PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/include/Inventor/annex
	${PROJECT_BINARY_DIR}/include
	${COIN_TARGET_INCLUDE_DIRECTORIES}
)

target_link_libraries(sdl3-example SDL3 Coin GL)
target_link_libraries(sdl3-example ${COIN_TARGET_LINK_LIBRARIES_GL})
