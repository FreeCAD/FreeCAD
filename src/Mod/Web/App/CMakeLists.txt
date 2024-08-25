include_directories (${CMAKE_CURRENT_BINARY_DIR} ${Boost_INCLUDE_DIRS} ${OCC_INCLUDE_DIR}
                     ${PYTHON_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR})

include_directories (${QtNetwork_INCLUDE_DIRS})
set (Web_LIBS FreeCADApp ${QtNetwork_LIBRARIES})

set (Web_SRCS AppWeb.cpp PreCompiled.cpp PreCompiled.h Server.cpp Server.h)

if (FREECAD_USE_PCH)
    add_definitions (-D_PreComp_)
    get_msvc_precompiled_source ("PreCompiled.cpp" PCH_SRCS ${Web_SRCS})
    add_msvc_precompiled_header (Web PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif (FREECAD_USE_PCH)

add_library (Web SHARED ${Web_SRCS})
target_link_libraries (Web ${Web_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error (Web)
endif ()

set_bin_dir (Web Web /Mod/Web)
set_python_prefix_suffix (Web)

install (TARGETS Web DESTINATION ${CMAKE_INSTALL_LIBDIR})
