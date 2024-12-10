if(EXISTS "D:/FreeCAD/fork/FreeCAD/build-debug-msvc/tests/Points_tests_run.exe")
  if(NOT EXISTS "D:/FreeCAD/fork/FreeCAD/build-debug-msvc/tests/Points_tests_run[1]_tests.cmake" OR
     NOT "D:/FreeCAD/fork/FreeCAD/build-debug-msvc/tests/Points_tests_run[1]_tests.cmake" IS_NEWER_THAN "D:/FreeCAD/fork/FreeCAD/build-debug-msvc/tests/Points_tests_run.exe" OR
     NOT "D:/FreeCAD/fork/FreeCAD/build-debug-msvc/tests/Points_tests_run[1]_tests.cmake" IS_NEWER_THAN "${CMAKE_CURRENT_LIST_FILE}")
    include("D:/Qt/Tools/CMake_64/share/cmake-3.29/Modules/GoogleTestAddTests.cmake")
    gtest_discover_tests_impl(
      TEST_EXECUTABLE [==[D:/FreeCAD/fork/FreeCAD/build-debug-msvc/tests/Points_tests_run.exe]==]
      TEST_EXECUTOR [==[]==]
      TEST_WORKING_DIR [==[D:/FreeCAD/fork/FreeCAD/build-debug-msvc/tests]==]
      TEST_EXTRA_ARGS [==[]==]
      TEST_PROPERTIES [==[]==]
      TEST_PREFIX [==[]==]
      TEST_SUFFIX [==[]==]
      TEST_FILTER [==[]==]
      NO_PRETTY_TYPES [==[FALSE]==]
      NO_PRETTY_VALUES [==[FALSE]==]
      TEST_LIST [==[Points_tests_run_TESTS]==]
      CTEST_FILE [==[D:/FreeCAD/fork/FreeCAD/build-debug-msvc/tests/Points_tests_run[1]_tests.cmake]==]
      TEST_DISCOVERY_TIMEOUT [==[5]==]
      TEST_XML_OUTPUT_DIR [==[]==]
    )
  endif()
  include("D:/FreeCAD/fork/FreeCAD/build-debug-msvc/tests/Points_tests_run[1]_tests.cmake")
else()
  add_test(Points_tests_run_NOT_BUILT Points_tests_run_NOT_BUILT)
endif()
