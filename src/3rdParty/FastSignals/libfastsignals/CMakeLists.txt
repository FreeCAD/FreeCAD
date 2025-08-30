
file(GLOB LIBFASTSIGNALS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
add_library(libfastsignals STATIC ${LIBFASTSIGNALS_SRC})
set_target_properties(libfastsignals PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(libfastsignals INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
