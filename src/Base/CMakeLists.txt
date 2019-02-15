if(WIN32)
    add_definitions(-DFCBase)
    add_definitions(-DPYCXX_DLL)
    add_definitions(-DBOOST_DYN_LINK)
    add_definitions(-DZIPIOS_UTF8)
endif(WIN32)

include_directories(
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYCXX_INCLUDE_DIR}
)

if(MSVC)
    set(FreeCADBase_LIBS
        ${Boost_LIBRARIES}
        ${PYTHON_LIBRARIES}
        debug ${XercesC_DEBUG_LIBRARIES}
        optimized ${XercesC_LIBRARIES}
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
        ${PYTHON_LIBRARIES}
        ${XercesC_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ZLIB_LIBRARIES}
        Rpcrt4.lib
    )
else(MSVC)
    set(FreeCADBase_LIBS
        ${PYTHON_LIBRARIES}
        ${XercesC_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ZLIB_LIBRARIES}
    )
endif(MSVC)

if (BUILD_QT5)
    include_directories(
        ${Qt5Core_INCLUDE_DIRS}
    )
    list(APPEND FreeCADBase_LIBS ${Qt5Core_LIBRARIES})
else()
    include_directories(
        ${QT_QTCORE_INCLUDE_DIR}
    )
    list(APPEND FreeCADBase_LIBS ${QT_QTCORE_LIBRARY})
endif()

# needed for OpenSUSE
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND FreeCADBase_LIBS -lutil -ldl)
endif()

generate_from_xml(BaseClassPy)
generate_from_xml(BoundBoxPy)
generate_from_xml(CoordinateSystemPy)
generate_from_xml(PersistencePy)
generate_from_xml(VectorPy)
generate_from_xml(MatrixPy)
generate_from_xml(RotationPy)
generate_from_xml(PlacementPy)
generate_from_xml(AxisPy)
generate_from_xml(UnitPy)
generate_from_xml(QuantityPy)

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

if (EXISTS ${CMAKE_SOURCE_DIR}/src/zipios++ AND NOT FREECAD_USE_EXTERNAL_ZIPIOS)
SET(zipios_SRCS
    ../zipios++/backbuffer.h
    ../zipios++/basicentry.cpp
    ../zipios++/basicentry.h
    ../zipios++/collcoll.cpp
    ../zipios++/collcoll.h
    ../zipios++/deflateoutputstreambuf.cpp
    ../zipios++/deflateoutputstreambuf.h
    ../zipios++/dircoll.cpp
    ../zipios++/dircoll.h
    ../zipios++/directory.cpp
    ../zipios++/directory.h
    ../zipios++/fcoll.cpp
    ../zipios++/fcoll.h
    ../zipios++/fcollexceptions.cpp
    ../zipios++/fcollexceptions.h
    ../zipios++/fileentry.cpp
    ../zipios++/fileentry.h
    ../zipios++/filepath.cpp
    ../zipios++/filepath.h
    ../zipios++/filterinputstreambuf.cpp
    ../zipios++/filterinputstreambuf.h
    ../zipios++/filteroutputstreambuf.cpp
    ../zipios++/filteroutputstreambuf.h
    ../zipios++/gzipoutputstream.cpp
    ../zipios++/gzipoutputstream.h
    ../zipios++/gzipoutputstreambuf.cpp
    ../zipios++/gzipoutputstreambuf.h
    ../zipios++/inflateinputstreambuf.cpp
    ../zipios++/inflateinputstreambuf.h
    ../zipios++/meta-iostreams.h
    ../zipios++/outputstringstream.h
    ../zipios++/simplesmartptr.h
    ../zipios++/virtualseeker.h
    ../zipios++/zipfile.cpp
    ../zipios++/zipfile.h
    ../zipios++/ziphead.cpp
    ../zipios++/ziphead.h
    ../zipios++/zipheadio.cpp
    ../zipios++/zipheadio.h
    ../zipios++/zipinputstream.cpp
    ../zipios++/zipinputstream.h
    ../zipios++/zipinputstreambuf.cpp
    ../zipios++/zipinputstreambuf.h
    ../zipios++/zipios_common.h
    ../zipios++/zipios-config.h
    ../zipios++/zipios_defs.h
    ../zipios++/zipoutputstreambuf.cpp
    ../zipios++/zipoutputstreambuf.h
    ../zipios++/zipoutputstream.cpp
    ../zipios++/zipoutputstream.h
)
SOURCE_GROUP("zipios" FILES ${zipios_SRCS})
endif ()

SOURCE_GROUP("pycxx" FILES ${PYCXX_SOURCES})

SET(FreeCADBase_XML_SRCS
    AxisPy.xml
    BaseClassPy.xml
    BoundBoxPy.xml
    CoordinateSystemPy.xml
    MatrixPy.xml
    PersistencePy.xml
    PlacementPy.xml
    RotationPy.xml
    VectorPy.xml
    QuantityPy.xml
    UnitPy.xml
)
SOURCE_GROUP("XML" FILES ${FreeCADBase_XML_SRCS})

set(FreeCADBase_MOC_HDRS
    Debugger.h
    FutureWatcherProgress.h
)

fc_wrap_cpp(FreeCADBase_MOC_SRCS ${FreeCADBase_MOC_HDRS})

SET(FreeCADBase_UNITAPI_SRCS
    UnitsApi.cpp
    UnitsApiPy.cpp
    UnitsApi.h
    UnitsSchema.h
    UnitsSchema.cpp
    UnitsSchemaInternal.h
    UnitsSchemaInternal.cpp
    UnitsSchemaMKS.h
    UnitsSchemaMKS.cpp
    UnitsSchemaImperial1.h
    UnitsSchemaImperial1.cpp
    UnitsSchemaCentimeters.h
    UnitsSchemaCentimeters.cpp
    UnitsSchemaMmMin.h
    UnitsSchemaMmMin.cpp
    Quantity.h
    Quantity.cpp
    QuantityPyImp.cpp
    QuantityParser.l
    QuantityParser.y
    Unit.h
    Unit.cpp
    UnitPyImp.cpp
    
)
SOURCE_GROUP("Units" FILES ${FreeCADBase_UNITAPI_SRCS})

if(PYTHON_VERSION_MAJOR LESS 3)
SET(SWIG_SRCS
    swigpyrun_1.3.25.cpp
    swigpyrun_1.3.33.cpp
    swigpyrun_1.3.36.cpp
    swigpyrun_1.3.38.cpp
    swigpyrun_1.3.40.cpp
    swigpyrun.cpp
)
else(PYTHON_VERSION_MAJOR LESS 3)
SET(SWIG_SRCS
    swigpyrun.cpp
)
endif(PYTHON_VERSION_MAJOR LESS 3)

SET(FreeCADBase_CPP_SRCS
    Axis.cpp
    AxisPyImp.cpp
    Base64.cpp
    BaseClass.cpp
    BaseClassPyImp.cpp
    BoundBoxPyImp.cpp
    Builder3D.cpp
    Console.cpp
    CoordinateSystem.cpp
    CoordinateSystemPyImp.cpp
    Debugger.cpp
    Exception.cpp
    ExceptionFactory.cpp
    Factory.cpp
    FileInfo.cpp
    FileTemplate.cpp
    FutureWatcherProgress.cpp
    gzstream.cpp
    gzstream.h
    GeometryPyCXX.cpp
    Handle.cpp
    InputSource.cpp
    Interpreter.cpp
    Matrix.cpp
    MatrixPyImp.cpp
    MemDebug.cpp
    Parameter.cpp
    ParameterPy.cpp
    Persistence.cpp
    PersistencePyImp.cpp
    Placement.cpp
    PlacementPyImp.cpp
    PyExport.cpp
    PyObjectBase.cpp
    Reader.cpp
    Rotation.cpp
    RotationPyImp.cpp
    Sequencer.cpp
    Stream.cpp
    Swap.cpp
    ${SWIG_SRCS}
    TimeInfo.cpp
    Tools.cpp
    Tools2D.cpp
    Translate.cpp
    Type.cpp
    Uuid.cpp
    Vector3D.cpp
    VectorPyImp.cpp
    ViewProj.cpp
    Writer.cpp
    XMLTools.cpp
)

if(PYTHON_VERSION_MAJOR LESS 3)
SET(SWIG_HEADERS
    swigpyrun_1.3.25.h
    swigpyrun_1.3.33.h
    swigpyrun_1.3.36.h
    swigpyrun_1.3.38.h
    swigpyrun_1.3.40.h
    swigpyrun.inl
)
else(PYTHON_VERSION_MAJOR LESS 3)
SET(SWIG_HEADERS
    swigpyrun.inl
)
endif(PYTHON_VERSION_MAJOR LESS 3)

SET(FreeCADBase_HPP_SRCS
    Axis.h
    Base64.h
    BaseClass.h
    BoundBox.h
    Builder3D.h
    Console.h
    CoordinateSystem.h
    Debugger.h
    Exception.h
    ExceptionFactory.h
    Factory.h
    FileInfo.h
    FileTemplate.h
    FutureWatcherProgress.h
    fdstream.hpp
    gzstream.h
    GeometryPyCXX.h
    Handle.h
    InputSource.h
    Interpreter.h
    Matrix.h
    MemDebug.h
    Observer.h
    Parameter.h
    Persistence.h
    Placement.h
    PyExport.h
    PyObjectBase.h
    Reader.h
    Rotation.h
    Sequencer.h
    StdStlTools.h
    Stream.h
    Swap.h
    ${SWIG_HEADERS}
    TimeInfo.h
    Tools.h
    Tools2D.h
    Translate.h
    Type.h
    Uuid.h
    Vector3D.h
    ViewProj.h
    Writer.h
    XMLTools.h
)

SET(FreeCADBase_SRCS
    ${PYCXX_SOURCES}
    ${FreeCADBase_CPP_SRCS}
    ${FreeCADBase_HPP_SRCS}
    ${FreeCADBase_XML_SRCS}
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

# Use external zipios++ if specified.
if(FREECAD_USE_EXTERNAL_ZIPIOS)
    find_library(ZIPIOS_LIBRARY zipios)
    find_path(ZIPIOS_INCLUDES zipios++/zipios-config.h)
    if(ZIPIOS_LIBRARY)
        message(STATUS "Found zipios++: ${ZIPIOS}")
    endif()
    if(ZIPIOS_INCLUDES)
        message(STATUS "Found zipios++ headers.")
    endif()
    if(ZIPIOS_LIBRARY AND ZIPIOS_INCLUDES)
        list(APPEND FreeCADBase_LIBS ${ZIPIOS_LIBRARY})
        include_directories(${ZIPIOS_INCLUDES})
    else()
        message(FATAL_ERROR "Using external zipios++ was specified but was not found.")
    endif()
else(FREECAD_USE_EXTERNAL_ZIPIOS)
    list(APPEND FreeCADBase_SRCS ${zipios_SRCS})
endif(FREECAD_USE_EXTERNAL_ZIPIOS)


if(BUILD_USE_PCH)
    add_definitions(-D_PreComp_)
    ADD_MSVC_PRECOMPILED_HEADER(FreeCADBase PreCompiled.h PreCompiled.cpp FreeCADBase_CPP_SRCS)
endif(BUILD_USE_PCH)

add_library(FreeCADBase SHARED ${FreeCADBase_SRCS})

target_link_libraries(FreeCADBase ${FreeCADBase_LIBS})

SET_BIN_DIR(FreeCADBase FreeCADBase)

if(WIN32)
    INSTALL(TARGETS FreeCADBase
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS FreeCADBase
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(WIN32)
