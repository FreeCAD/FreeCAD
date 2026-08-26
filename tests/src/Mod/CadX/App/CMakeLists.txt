# SPDX-License-Identifier: LGPL-2.1-or-later

set(CadX_tests_SRCS
    GraphQueryTest.cpp
    GraphStoreTest.cpp
    AssemblyGraphBuilderTest.cpp
    AssemblyObjectAdapterTest.cpp
    CadXServiceTest.cpp
    GraphEvidenceTest.cpp
    MainThreadGatewayTest.cpp
    NativeAssemblyConstraintOperationsTest.cpp
    ToolRegistryTest.cpp
    AssemblyDocumentObserverTest.cpp
)

if(BUILD_PART_DESIGN)
    list(APPEND CadX_tests_SRCS NativePrimitiveOperationsTest.cpp)
endif()

add_executable(CadX_tests_run ${CadX_tests_SRCS})

target_include_directories(CadX_tests_run PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)

if(BUILD_ASSEMBLY)
    target_compile_definitions(CadX_tests_run PRIVATE CADX_HAVE_ASSEMBLY=1)
endif()

if(BUILD_PART_DESIGN)
    target_compile_definitions(CadX_tests_run PRIVATE CADX_HAVE_PART_DESIGN=1)
endif()

target_link_libraries(CadX_tests_run PRIVATE
    GTest::gtest_main
    ${Python3_LIBRARIES}
    CadXApp
)
