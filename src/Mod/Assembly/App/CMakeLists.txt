include_directories (
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/3rdParty/OndselSolver
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS})
link_directories (${OCC_LIBRARY_DIR})

set (Assembly_LIBS Part PartDesign Spreadsheet FreeCADApp OndselSolver)

generate_from_xml (AssemblyObjectPy)
generate_from_xml (BomObjectPy)
generate_from_xml (BomGroupPy)
generate_from_xml (JointGroupPy)
generate_from_xml (ViewGroupPy)

set (
    Python_SRCS
    AssemblyObjectPy.xml
    AssemblyObjectPyImp.cpp
    BomObjectPy.xml
    BomObjectPyImp.cpp
    BomGroupPy.xml
    BomGroupPyImp.cpp
    JointGroupPy.xml
    JointGroupPyImp.cpp
    ViewGroupPy.xml
    ViewGroupPyImp.cpp)
source_group ("Python" FILES ${Python_SRCS})

set (Module_SRCS AppAssembly.cpp AppAssemblyPy.cpp PreCompiled.cpp PreCompiled.h)
source_group ("Module" FILES ${Module_SRCS})

set (
    Assembly_SRCS
    AssemblyObject.cpp
    AssemblyObject.h
    BomObject.cpp
    BomObject.h
    BomGroup.cpp
    BomGroup.h
    JointGroup.cpp
    JointGroup.h
    ViewGroup.cpp
    ViewGroup.h
    ${Module_SRCS}
    ${Python_SRCS})

add_library (Assembly SHARED ${Assembly_SRCS})
target_link_libraries (Assembly ${Assembly_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error (Assembly)
endif ()

if (FREECAD_USE_PCH)
    add_definitions (-D_PreComp_)
    get_msvc_precompiled_source ("PreCompiled.cpp" PCH_SRCS ${Assembly_SRCS})
    add_msvc_precompiled_header (Assembly PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif (FREECAD_USE_PCH)

set_bin_dir (Assembly AssemblyApp /Mod/Assembly)
set_python_prefix_suffix (Assembly)

install (TARGETS Assembly DESTINATION ${CMAKE_INSTALL_LIBDIR})
