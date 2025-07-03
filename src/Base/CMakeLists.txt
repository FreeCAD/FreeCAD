add_library(FreeCADBase SHARED)

if(WIN32)
    add_definitions(-DFCBase)
    add_definitions(-DPYCXX_DLL)
    add_definitions(-DBOOST_DYN_LINK)
    add_definitions(-DZIPIOS_UTF8)
endif(WIN32)

if(BUILD_TRACY_FRAME_PROFILER)
    add_definitions(-DBUILD_TRACY_FRAME_PROFILER)
endif()

include_directories(
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    FreeCADBase
    SYSTEM
    PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${PYCXX_INCLUDE_DIR}
    ${Python3_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${ZIPIOS_INCLUDES}
)

if(MSVC)
    set(FreeCADBase_LIBS
        ${Boost_LIBRARIES}
        ${XercesC_LIBRARIES}
        ${ZLIB_LIBRARIES}
        debug MSVCRTD.LIB
        debug MSVCPRTD.LIB
        optimized MSVCRT.LIB
        optimized MSVCPRT.LIB
        Rpcrt4.lib
        version.lib
    )

    #Universal C runtime introduced in VS 2015 (cl version 19)
    if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19"))
        list(APPEND FreeCADBase_LIBS
            debug vcruntimed.lib
            debug ucrtd.lib
            debug concrtd.lib
            optimized vcruntime.lib
            optimized ucrt.lib
            optimized concrt.lib
        )
    endif()
elseif(MINGW)
    set(FreeCADBase_LIBS
        ${XercesC_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ZLIB_LIBRARIES}
        Rpcrt4.lib
    )
else(MSVC)
    set(FreeCADBase_LIBS
        ${XercesC_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ZLIB_LIBRARIES}
    )
endif(MSVC)

include_directories(
    ${QtCore_INCLUDE_DIRS}
)

if(BUILD_TRACY_FRAME_PROFILER)
    list(APPEND FreeCADBase_LIBS TracyClient)
endif()

list(APPEND FreeCADBase_LIBS ${QtCore_LIBRARIES})

list(APPEND FreeCADBase_LIBS fmt::fmt)

if (BUILD_DYNAMIC_LINK_PYTHON)
    list(APPEND FreeCADBase_LIBS ${Python3_LIBRARIES})
endif (BUILD_DYNAMIC_LINK_PYTHON)

# needed for OpenSUSE
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND FreeCADBase_LIBS -lutil -ldl)
endif()

generate_from_py(Type)
generate_from_py(BaseClass)
generate_from_py(BoundBox)
generate_from_py(CoordinateSystem)
generate_from_py(Persistence)
generate_from_py(Vector)
generate_from_py(Matrix)
generate_from_py(Rotation)
generate_from_py(Placement)
generate_from_py(Axis)
generate_from_py(Unit)
generate_from_py(Quantity)
generate_from_py(Precision)
generate_from_any(Parameter.xsd Parameter.inl xmlSchemeString)

if(SWIG_FOUND)
    # Create the file swigpyrun.hh and then compare with the file swigpyrun.h.
    # If the files are different or if swigpyrun.h doesn't exist then copy swigpyrun.hh.
    # This is to avoid to having to build the target each time cmake configure is executed.
    execute_process(COMMAND ${SWIG_EXECUTABLE} -python -external-runtime ${CMAKE_CURRENT_BINARY_DIR}/swigpyrun.hh)

    fc_copy_file_if_different(
        "${CMAKE_CURRENT_BINARY_DIR}/swigpyrun.hh"
        "${CMAKE_CURRENT_BINARY_DIR}/swigpyrun.h"
    )
    add_definitions(-DHAVE_SWIG=1)
endif(SWIG_FOUND)

SOURCE_GROUP("pycxx" FILES ${PYCXX_SOURCES})

# auto-generate resource file with all available translations
set (FreeCADBase_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Base_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${FreeCADBase_TR_QRC} ${QM_SRCS})
qt_add_resources(FreeCADBase_QRC_SRCS ${FreeCADBase_TR_QRC})

SET(FreeCADBase_Pyi_SRCS
    Axis.pyi
    BaseClass.pyi
    BoundBox.pyi
    CoordinateSystem.pyi
    Matrix.pyi
    Persistence.pyi
    Placement.pyi
    Rotation.pyi
    Vector.pyi
    Quantity.pyi
    Type.pyi
    Unit.pyi
    Precision.pyi
)
SOURCE_GROUP("Pyi" FILES ${FreeCADBase_Pyi_SRCS})

SET(FreeCADBase_UNITAPI_SRCS
    UnitsApi.cpp
    UnitsApiPy.cpp
    UnitsApi.h
    UnitsSchema.h
    UnitsSchema.cpp
    UnitsSchemas.cpp
    UnitsSchemas.h
    UnitsSchemasData.h
    UnitsSchemasSpecs.h
    Quantity.h
    Quantity.cpp
    QuantityPyImp.cpp
    Quantity.l
    Quantity.y
    Unit.h
    Unit.cpp
    UnitPyImp.cpp
)
SOURCE_GROUP("Units" FILES ${FreeCADBase_UNITAPI_SRCS})

SET(SWIG_SRCS
    swigpyrun.cpp
)

SET(FreeCADBase_CPP_SRCS
    Axis.cpp
    AxisPyImp.cpp
    Base64.cpp
    BaseClass.cpp
    BaseClassPyImp.cpp
    BindingManager.cpp
    BoundBoxPyImp.cpp
    Builder3D.cpp
    Console.cpp
    ConsoleObserver.cpp
    Color.cpp
    CoordinateSystem.cpp
    CoordinateSystemPyImp.cpp
    Debugger.cpp
    DualQuaternion.cpp
    Exception.cpp
    ExceptionFactory.cpp
    Factory.cpp
    FileInfo.cpp
    FutureWatcherProgress.cpp
    GeometryPyCXX.cpp
    Handle.cpp
    InputSource.cpp
    Interpreter.cpp
    Matrix.cpp
    MatrixPyImp.cpp
    Observer.cpp
    Parameter.xsd
    Parameter.cpp
    ParameterPy.cpp
    Persistence.cpp
    PersistencePyImp.cpp
    Placement.cpp
    PlacementPyImp.cpp
    PrecisionPyImp.cpp
    ProgressIndicator.cpp
    ProgressIndicatorPy.cpp
    PyExport.cpp
    PyObjectBase.cpp
    PythonTypeExt.cpp
    Reader.cpp
    Rotation.cpp
    RotationPyImp.cpp
    Sequencer.cpp
    ServiceProvider.cpp
    SmartPtrPy.cpp
    Stream.cpp
    Swap.cpp
    ${SWIG_SRCS}
    Tools.cpp
    Tools2D.cpp
    Tools3D.cpp
    Translate.cpp
    Type.cpp
    TypePyImp.cpp
    UniqueNameManager.cpp
    Uuid.cpp
    Vector3D.cpp
    VectorPyImp.cpp
    ViewProj.cpp
    Writer.cpp
    XMLTools.cpp
    ZipHeader.cpp
)

SET(SWIG_HEADERS
    swigpyrun.inl
)

SET(FreeCADBase_HPP_SRCS
    Axis.h
    Base64.h
    Base64Filter.h
    BaseClass.h
    BindingManager.h
    Bitmask.h
    BoundBox.h
    Builder3D.h
    Console.h
    ConsoleObserver.h
    Converter.h
    Color.h
    CoordinateSystem.h
    Debugger.h
    DualNumber.h
    DualQuaternion.h
    Exception.h
    ExceptionFactory.h
    Factory.h
    FileInfo.h
    FutureWatcherProgress.h
    GeometryPyCXX.h
    Handle.h
    InputSource.h
    Interpreter.h
    Matrix.h
    Observer.h
    Parameter.h
    Persistence.h
    Placement.h
    Precision.h
    ProgressIndicatorPy.h
    ProgressIndicator.h
    PyExport.h
    PyObjectBase.h
    PyWrapParseTupleAndKeywords.h
    PythonTypeExt.h
    Reader.h
    Rotation.h
    ServiceProvider.h
    Sequencer.h
    SmartPtrPy.h
    Stream.h
    Swap.h
    ${SWIG_HEADERS}
    TimeInfo.h
    Tools.h
    Tools2D.h
    Tools3D.h
    Translate.h
    Type.h
    UnlimitedUnsigned.h
    UniqueNameManager.h
    Uuid.h
    Vector3D.h
    ViewProj.h
    Writer.h
    XMLTools.h
    ZipHeader.h
)

SET(FreeCADBase_SRCS
    ${PYCXX_SOURCES}
    ${FreeCADBase_QRC_SRCS}
    ${FreeCADBase_CPP_SRCS}
    ${FreeCADBase_HPP_SRCS}
    ${FreeCADBase_Pyi_SRCS}
    ${FreeCADBase_UNITAPI_SRCS}
    PyTools.c
    PyTools.h
    PreCompiled.cpp
    PreCompiled.h
)

IF (MSVC)
    SET(FreeCADBase_SRCS
        ${FreeCADBase_SRCS}
        StackWalker.cpp
        StackWalker.h
    )
ENDIF(MSVC)

if(FREECAD_USE_EXTERNAL_ZIPIOS)
    list(APPEND FreeCADBase_LIBS ${ZIPIOS_LIBRARY})
else()
    list(APPEND FreeCADBase_SRCS ${zipios_SRCS})
    SOURCE_GROUP("zipios" FILES ${zipios_SRCS})
endif()

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    ADD_MSVC_PRECOMPILED_HEADER(FreeCADBase PreCompiled.h PreCompiled.cpp FreeCADBase_CPP_SRCS)
endif(FREECAD_USE_PCH)

target_sources(FreeCADBase PRIVATE ${FreeCADBase_SRCS})
target_link_libraries(FreeCADBase ${FreeCADBase_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(FreeCADBase)
endif()

SET_BIN_DIR(FreeCADBase FreeCADBase)

if(WIN32)
    INSTALL(TARGETS FreeCADBase
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS FreeCADBase
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(WIN32)
