if(MSVC)
    add_definitions(-DFCAppImport -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)


include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(Import_LIBS
    Part
    ${OCC_OCAF_LIBRARIES}
    ${OCC_OCAF_DEBUG_LIBRARIES}
)

SET(Import_SRCS
    AppImport.cpp
    AppImportPy.cpp
    ImportOCAF.cpp
    ImportOCAF.h
    PreCompiled.cpp
    PreCompiled.h
)

add_library(Import SHARED ${Import_SRCS})
target_link_libraries(Import ${Import_LIBS})

if(MSVC)
    set_target_properties(Import PROPERTIES SUFFIX ".pyd")
    set_target_properties(Import PROPERTIES DEBUG_OUTPUT_NAME "Import_d")
    set_target_properties(Import PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Import)
    set_target_properties(Import PROPERTIES PREFIX "../")
    # Set special compiler flag to convert a SIGSEV into an exception
    set_target_properties(Import PROPERTIES COMPILE_FLAGS "/EHa")
elseif(MINGW)
    set_target_properties(Import PROPERTIES SUFFIX ".pyd")
    set_target_properties(Import PROPERTIES DEBUG_OUTPUT_NAME "Import_d")
    set_target_properties(Import PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Import)
    set_target_properties(Import PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Import PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Import)
    set_target_properties(Import PROPERTIES PREFIX "")
    set_target_properties(Import PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS Import DESTINATION lib)
