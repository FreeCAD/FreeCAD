# Uncomment this code if you want to try Eigen's multithread
#find_package(OpenMP)
#if (OPENMP_FOUND)
#    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
#    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
#endif()

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEIGEN_NO_DEBUG") # Necessary to avoid SparseQR crash with Eigen 3.2 due to a wrong assertion

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(Text_LIBS
    ${OCC_OCAF_LIBRARIES}
    Part
    FreeCADApp
)

generate_from_xml(FontPy)
generate_from_xml(ShapeTextPy)


SET(Features_SRCS
    Font.cpp
    Font.h
    ShapeText.cpp
    ShapeText.h
)
SOURCE_GROUP("Features" FILES ${Features_SRCS})

SET(Python_SRCS
    FontPy.xml
    FontPyImp.cpp
    ShapeTextPy.xml
    ShapeTextPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})


SET(TextModule_SRCS
    AppText.cpp
    AppTextPy.cpp
    BRepFont.cpp
    BRepFont.h
    PreCompiled.cpp
    PreCompiled.h
    Measure.cpp
    Measure.h
)
SOURCE_GROUP("Module" FILES ${TextModule_SRCS})

SET(Text_SRCS
    ${Features_SRCS}
    ${TextModule_SRCS}
    ${Python_SRCS}
)

SET(Text_PCH_SRCS
    ${Features_SRCS}
    ${TextModule_SRCS}
    ${Python_SRCS}
)

add_library(Text SHARED ${Text_SRCS})
target_link_libraries(Text ${Text_LIBS})

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" Text_CPP_SRCS ${Text_PCH_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Text PreCompiled.h PreCompiled.cpp Text_CPP_SRCS)
endif(FREECAD_USE_PCH)


SET_BIN_DIR(Text Text /Mod/Text)
SET_PYTHON_PREFIX_SUFFIX(Text)

INSTALL(TARGETS Text DESTINATION ${CMAKE_INSTALL_LIBDIR})
