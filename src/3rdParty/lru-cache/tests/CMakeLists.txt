########################################
# CONFIG
########################################

add_compile_options(-g -Werror -DDEBUG)

########################################
# DEPENDENCIES
########################################

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/googletest)

set(GTEST_INCLUDE_DIRS
  ${gtest_SOURCE_DIR}/include
  ${gtest_SOURCE_DIR})

########################################
# INCLUDES
########################################

include_directories(${GTEST_INCLUDE_DIRS})

########################################
# SOURCES
########################################

set(TEST_LRU_CACHE_SOURCES
  move-awareness-test.cpp
  last-accessed-test.cpp
  iterator-test.cpp
  cache-test.cpp
  timed-cache-test.cpp
  statistics-test.cpp
  wrap-test.cpp
  callback-test.cpp
)

###########################################################
## BINARIES
###########################################################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

########################################
# TARGET
########################################

add_executable(lru-cache-test ${TEST_LRU_CACHE_SOURCES})

target_link_libraries(lru-cache-test gtest gtest_main)

add_test(
  NAME lru-cache-test
  COMMAND lru-cache-test
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
