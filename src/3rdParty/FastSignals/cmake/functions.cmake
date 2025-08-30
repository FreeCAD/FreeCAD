# Function to add a library target.
function(custom_add_library_from_dir TARGET)
    # Gather files from the current directory
    file(GLOB TARGET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
    add_library(${TARGET} ${TARGET_SRC})
endfunction()

# Function to add an executable target.
function(custom_add_executable_from_dir TARGET)
    # Gather files from the current directory
    file(GLOB TARGET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
    add_executable(${TARGET} ${TARGET_SRC})
endfunction()

# Function to add an executable target containing tests for a library.
function(custom_add_test_from_dir TARGET LIBRARY)
    custom_add_executable_from_dir(${TARGET})
    # Add path to Catch framework header
    target_include_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/libs/catch")
    # Link with the library being tested
    target_link_libraries(${TARGET} ${LIBRARY})
    # Register the executable with CMake as a test set
    add_test(${TARGET} ${TARGET})
endfunction()
