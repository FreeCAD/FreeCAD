add_library(Sketcher SHARED)

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
)

target_include_directories(
    Sketcher
    SYSTEM
    PUBLIC
    ${EIGEN3_INCLUDE_DIR}
)

set(Sketcher_LIBS
    Part
    FreeCADApp
)

generate_from_py(SketchObjectSF)
generate_from_py(SketchObject)
generate_from_py(SketchGeometryExtension)
generate_from_py(ExternalGeometryExtension)
generate_from_py(GeometryFacade)
generate_from_py(ExternalGeometryFacade)
generate_from_py(Constraint)
generate_from_py(Sketch)


SET(Properties_SRCS
    PropertyConstraintList.cpp
    PropertyConstraintList.h
)
SOURCE_GROUP("Properties" FILES ${Properties_SRCS})


SET(Features_SRCS
    SketchObjectSF.cpp
    SketchObjectSF.h
    SketchGeometryExtension.cpp
    SketchGeometryExtension.h
    ExternalGeometryExtension.cpp
    ExternalGeometryExtension.h
    SolverGeometryExtension.cpp
    SolverGeometryExtension.h
    SketchObject.cpp
    SketchObject.h
    SketchAnalysis.h
    SketchAnalysis.cpp
    Analyse.h
    PythonConverter.cpp
    PythonConverter.h
)
SOURCE_GROUP("Features" FILES ${Features_SRCS})

SET(Datatypes_SRCS
    Constraint.cpp
    Constraint.h
    Sketch.cpp
    Sketch.h
    GeoList.h
    GeoList.cpp
    GeometryFacade.cpp
    GeometryFacade.h
    ExternalGeometryFacade.cpp
    ExternalGeometryFacade.h
    GeoEnum.cpp
    GeoEnum.h
)
SOURCE_GROUP("Datatypes" FILES ${Datatypes_SRCS})

SET(Python_SRCS
    SketchObjectSF.pyi
    SketchObjectSFPyImp.cpp
    SketchObject.pyi
    SketchObjectPyImp.cpp
    SketchGeometryExtension.pyi
    SketchGeometryExtensionPyImp.cpp
    ExternalGeometryExtension.pyi
    ExternalGeometryExtensionPyImp.cpp
    GeometryFacade.pyi
    GeometryFacadePyImp.cpp
    ExternalGeometryFacade.pyi
    ExternalGeometryFacadePyImp.cpp
    ConstraintPyImp.cpp
    Constraint.pyi
    Sketch.pyi
    SketchPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})


SET(PlaneGCS_SRCS
    planegcs/GCS.cpp
    planegcs/GCS.h
    planegcs/Util.h
    planegcs/Geo.cpp
    planegcs/Geo.h
    planegcs/Constraints.cpp
    planegcs/Constraints.h
    planegcs/SubSystem.cpp
    planegcs/SubSystem.h
    planegcs/qp_eq.cpp
    planegcs/qp_eq.h
)
SOURCE_GROUP("PlaneGCS" FILES ${PlaneGCS_SRCS})

SET(SketchModule_SRCS
    AppSketcher.cpp
    AppSketcherPy.cpp
    PreCompiled.cpp
    PreCompiled.h
    Measure.cpp
    Measure.h
)
SOURCE_GROUP("Module" FILES ${SketchModule_SRCS})

SET(Sketcher_SRCS
    ${Features_SRCS}
    ${PlaneGCS_SRCS}
    ${SketchModule_SRCS}
    ${Python_SRCS}
    ${Properties_SRCS}
    ${Datatypes_SRCS}
)

SET(Sketcher_PCH_SRCS
    ${Features_SRCS}
    ${SketchModule_SRCS}
    ${Python_SRCS}
    ${Properties_SRCS}
    ${Datatypes_SRCS}
)

target_sources(Sketcher PRIVATE ${Sketcher_SRCS})
target_link_libraries(Sketcher ${Sketcher_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Sketcher)
endif()

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" Sketcher_CPP_SRCS ${Sketcher_PCH_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Sketcher PreCompiled.h PreCompiled.cpp Sketcher_CPP_SRCS)
endif(FREECAD_USE_PCH)

# Suppress some very long Eigen3 warnings of older versions
if (EIGEN3_NO_DEPRECATED_COPY)
    set_source_files_properties(
        planegcs/GCS.cpp
        planegcs/SubSystem.cpp
        planegcs/qp_eq.cpp
        PROPERTIES COMPILE_FLAGS ${EIGEN3_NO_DEPRECATED_COPY})
endif ()


SET_BIN_DIR(Sketcher Sketcher /Mod/Sketcher)
SET_PYTHON_PREFIX_SUFFIX(Sketcher)

INSTALL(TARGETS Sketcher DESTINATION ${CMAKE_INSTALL_LIBDIR})
