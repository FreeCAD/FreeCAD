
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
)

set(_TEMPLATE__LIBS
    FreeCADApp
)

SET(_TEMPLATE__SRCS
    App_TEMPLATE_.cpp
    App_TEMPLATE_Py.cpp
    PreCompiled.cpp
    PreCompiled.h
)

add_library(_TEMPLATE_ SHARED ${_TEMPLATE__SRCS})
target_link_libraries(_TEMPLATE_ ${_TEMPLATE__LIBS})
fc_copy_script("Mod/_TEMPLATE_" "_TEMPLATE_" Init.py)

if(MSVC)
    set_target_properties(_TEMPLATE_ PROPERTIES SUFFIX ".pyd")
    set_target_properties(_TEMPLATE_ PROPERTIES DEBUG_OUTPUT_NAME "_TEMPLATE__d")
    set_target_properties(_TEMPLATE_ PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/_TEMPLATE_)
    set_target_properties(_TEMPLATE_ PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(_TEMPLATE_ PROPERTIES SUFFIX ".pyd")
    set_target_properties(_TEMPLATE_ PROPERTIES DEBUG_OUTPUT_NAME "_TEMPLATE__d")
    set_target_properties(_TEMPLATE_ PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/_TEMPLATE_)
    set_target_properties(_TEMPLATE_ PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(_TEMPLATE_ PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/_TEMPLATE_)
    set_target_properties(_TEMPLATE_ PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS _TEMPLATE_ DESTINATION lib)
