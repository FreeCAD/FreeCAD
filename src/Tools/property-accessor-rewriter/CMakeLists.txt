cmake_minimum_required(VERSION 3.16)

project(property_accessor_rewriter LANGUAGES CXX)

find_package(Clang REQUIRED CONFIG) # On some systems you may need a custom FindClang

add_executable(property-accessor-rewriter
    main.cpp
)

target_link_libraries(property-accessor-rewriter
    PRIVATE
        clang-cpp
        LLVM
)

target_compile_features(property-accessor-rewriter PRIVATE cxx_std_20)

target_include_directories(property-accessor-rewriter
    PRIVATE
        ${CLANG_INCLUDE_DIRS}
)
