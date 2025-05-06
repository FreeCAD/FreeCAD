find_package (SWIG REQUIRED)
include (${SWIG_USE_FILE})

find_package (PythonLibs)
include_directories (${PYTHON_INCLUDE_PATH})

include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/..)

# Build the _kdtree python module
set_source_files_properties (py-kdtree.i PROPERTIES CPLUSPLUS ON)
swig_add_module (kdtree python py-kdtree.i)
swig_link_libraries (kdtree ${PYTHON_LIBRARIES})

# Copy the test file into the build dir
install (FILES py-kdtree_test.py DESTINATION ${CMAKE_INSTALL_PREFIX}/python)
install (FILES ${CMAKE_BINARY_DIR}/python-bindings/kdtree.py DESTINATION ${CMAKE_INSTALL_PREFIX}/python)
install (FILES ${CMAKE_BINARY_DIR}/python-bindings/_kdtree.so DESTINATION ${CMAKE_INSTALL_PREFIX}/python)

