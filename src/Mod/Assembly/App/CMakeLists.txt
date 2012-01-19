if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${ODE_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}

)

set(Assembly_LIBS
    ${ODE_LIBRARIES}
    ${OCC_LIBRARIES}
    Part
    FreeCADApp
)

generate_from_xml(ItemPy)
generate_from_xml(ItemAssemblyPy)
generate_from_xml(ItemPartPy)
generate_from_xml(ConstraintGroupPy)

SET(FreeGCS3D_SRCS
    gcs3d/GCS.cpp
    gcs3d/GCS.h
    gcs3d/Util.h
    gcs3d/Geo.h
    gcs3d/Constraints.cpp
    gcs3d/Constraints.h
    gcs3d/SubSystem.cpp
    gcs3d/SubSystem.h
    gcs3d/qp_eq.cpp
    gcs3d/qp_eq.h
)
SOURCE_GROUP("FreeGCS3D" FILES ${FreeGCS3D_SRCS})

SET(Features_SRCS
    Item.cpp
    Item.h
    ItemPart.cpp
    ItemPart.h
    ItemAssembly.cpp
    ItemAssembly.h
    Constraint.cpp
    Constraint.h
    ConstraintGroup.cpp
    ConstraintGroup.h
    ConstraintAngle.cpp
    ConstraintAngle.h
    ConstraintAxis.cpp
    ConstraintAxis.h
    ConstraintContact.cpp
    ConstraintContact.h
    ConstraintFix.cpp
    ConstraintFix.h
    ConstraintOffset.cpp
    ConstraintOffset.h
)
SOURCE_GROUP("Features" FILES ${Features_SRCS})

SET(Module_SRCS
    AppAssembly.cpp
    AppAssemblyPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Module_SRCS})

SET(Python_SRCS
    ItemPy.xml
    ItemPyImp.cpp
    ItemAssemblyPy.xml
    ItemAssemblyPyImp.cpp
    ItemPartPy.xml
    ItemPartPyImp.cpp
    ConstraintGroupPy.xml
    ConstraintGroupPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(Assembly_SRCS
    ${Features_SRCS}
    ${Python_SRCS}
    ${Module_SRCS}
	${FreeGCS3D_SRCS}
)


add_library(Assembly SHARED ${Assembly_SRCS})
target_link_libraries(Assembly ${Assembly_LIBS})
fc_copy_script("Mod/Assembly" "Assembly" Init.py)

if(MSVC)
    set_target_properties(Assembly PROPERTIES SUFFIX ".pyd")
    set_target_properties(Assembly PROPERTIES DEBUG_OUTPUT_NAME "Assembly_d")
    set_target_properties(Assembly PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Assembly)
    set_target_properties(Assembly PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Assembly PROPERTIES SUFFIX ".pyd")
    set_target_properties(Assembly PROPERTIES DEBUG_OUTPUT_NAME "Assembly_d")
    set_target_properties(Assembly PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Assembly)
    set_target_properties(Assembly PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Assembly PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Assembly)
    set_target_properties(Assembly PROPERTIES PREFIX "")
    SET_TARGET_PROPERTIES(Assembly PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS Assembly DESTINATION lib)
