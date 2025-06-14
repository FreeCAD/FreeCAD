
file(GLOB LIBFASTSIGNALS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
add_library(libfastsignals ${LIBFASTSIGNALS_SRC})
custom_enable_cxx17(libfastsignals)
target_include_directories(libfastsignals INTERFACE "${CMAKE_SOURCE_DIR}")
