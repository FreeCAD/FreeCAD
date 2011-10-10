
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
 #   ${ODE_INCLUDE_DIRS}
)

set(Assembly_LIBS
#    ${ODE_LIBRARIES}
    FreeCADApp
)

SET(Assembly_SRCS
    AppAssembly.cpp
    AppAssemblyPy.cpp
    PreCompiled.cpp
    PreCompiled.h
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
endif(MSVC)

install(TARGETS Assembly DESTINATION lib)
