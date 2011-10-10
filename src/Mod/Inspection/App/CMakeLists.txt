if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${QT_QTCORE_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(Inspection_LIBS
    FreeCADApp
    Mesh
    Points
    Part
)

SET(Inspection_SRCS
    AppInspection.cpp
    AppInspectionPy.cpp
    InspectionFeature.cpp
    InspectionFeature.h
    PreCompiled.cpp
    PreCompiled.h
)

add_library(Inspection SHARED ${Inspection_SRCS})
target_link_libraries(Inspection ${Inspection_LIBS})
fc_copy_script("Mod/Inspection" "Inspection" Init.py)

if(MSVC)
    set_target_properties(Inspection PROPERTIES SUFFIX ".pyd")
    set_target_properties(Inspection PROPERTIES DEBUG_OUTPUT_NAME "Inspection_d")
    set_target_properties(Inspection PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Inspection)
    set_target_properties(Inspection PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Inspection PROPERTIES SUFFIX ".pyd")
    set_target_properties(Inspection PROPERTIES DEBUG_OUTPUT_NAME "Inspection_d")
    set_target_properties(Inspection PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Inspection)
    set_target_properties(Inspection PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Inspection PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Inspection)
    set_target_properties(Inspection PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS Inspection DESTINATION lib)
