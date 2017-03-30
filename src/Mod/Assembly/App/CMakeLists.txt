if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

add_definitions(-DBOOST_${Boost_VERSION})

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Mod/Assembly/App
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
    #${ODE_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}

)

link_directories(${OCC_LIBRARY_DIR})

set(Assembly_LIBS
    #${ODE_LIBRARIES}
    ${OCC_LIBRARIES}
    ${Boost_LIBRARIES}
    ${Boost_LOG_LIBRARY_RELEASE}
    Part
    FreeCADApp
)

generate_from_xml(ItemPy)
generate_from_xml(ProductRefPy)
#generate_from_xml(PartRefPy)
generate_from_xml(ConstraintPy)
generate_from_xml(ConstraintGroupPy)

SET(Features_SRCS
    Item.cpp
    Item.h
    #PartRef.cpp
    #PartRef.h
    Product.cpp
    Product.h
    ProductRef.cpp
    ProductRef.h
    Constraint.cpp
    Constraint.h
    ConstraintGroup.cpp
    ConstraintGroup.h
)
SOURCE_GROUP("Features" FILES ${Features_SRCS})

SET(Module_SRCS
    AppAssembly.cpp
    AppAssemblyPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Module_SRCS})

#externalization is not possible for msvc as a stupid bug prevents the function definition resolving
if(MSVC)
	set(Solver_SRC )
else(MSVC)
	set(Solver_SRC  Solver/solver_3d_ext1.cpp	
			Solver/solver_3d_ext2.cpp
			Solver/solver_3d_ext3.cpp
	)
	
	if(FREECAD_ASSEMBLY_DEBUG_FACILITIES)
	set(Solver_SRC  ${Solver_SRC}	
			Solver/solver_state_ext1.cpp
			Solver/solver_state_ext2.cpp
	)
	endif(FREECAD_ASSEMBLY_DEBUG_FACILITIES)
endif(MSVC) 

SOURCE_GROUP("Solver" FILES ${Solver_SRC})

SET(Python_SRCS
    ItemPy.xml
    ItemPyImp.cpp
    ProductRefPy.xml
    ProductRefPyImp.cpp
    #PartRefPy.xml
    #PartRefPyImp.cpp
    ConstraintPy.xml
    ConstraintPyImp.cpp
    ConstraintGroupPy.xml
    ConstraintGroupPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(Assembly_SRCS
    ${Features_SRCS}
    ${Python_SRCS}
    ${Module_SRCS}
    ${Solver_SRC}
)

add_library(Assembly SHARED ${Assembly_SRCS})
target_link_libraries(Assembly ${Assembly_LIBS} ${log_LIB})


fc_target_copy_resource(Assembly 
    ${CMAKE_SOURCE_DIR}/src/Mod/Assembly
    ${CMAKE_BINARY_DIR}/Mod/Assembly
    Init.py
	AssemblyLib.py)

SET_BIN_DIR(Assembly Assembly /Mod/Assembly)
SET_PYTHON_PREFIX_SUFFIX(Assembly)

INSTALL(TARGETS Assembly DESTINATION ${CMAKE_INSTALL_LIBDIR})
