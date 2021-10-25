set(Library FreeCADRenderer)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB CMakeCache RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/CMakeCache.txt)

set(Renderer_Headers Renderer.h)
set(Renderer_HPP_SRCS ${Renderer_Headers})
set(Renderer_CPP_SRCS Renderer.cpp)
set(Renderer_LIBS ${Qt5Core_LIBRARIES}
                  ${Qt5Widgets_LIBRARIES}
                  ${Qt5OpenGL_LIBRARIES})

if(BUILD_BGFX)
    add_definitions(-DHAVE_BGFX)
    list(APPEND Renderer_CPP_SRCS BGFXRenderer.cpp)
    list(APPEND Renderer_HPP_SRCS BGFXRenderer.h)
    list(APPEND Renderer_LIBS bgfx bx bimg example-common)
endif(BUILD_BGFX)

set(Renderer_SRCS ${Renderer_CPP_SRCS} ${Renderer_HPP_SRCS})

# Create the library
IF(Renderer_BUILD_STATIC_LIBRARY)
    add_library(${Library} STATIC ${Renderer_SRCS})
    target_compile_definitions(${Library}
                                PUBLIC RendererExport=
                                PRIVATE RendererExport=)
ELSE(Renderer_BUILD_STATIC_LIBRARY)
    add_library(${Library} SHARED ${Renderer_SRCS})
    target_link_libraries(${Library} ${Renderer_LIBS})
    # Ensure export of DLL definitions on Windows
    if(WIN32)
        target_compile_definitions(${Library}
                                   PUBLIC RendererExport=__declspec\(dllimport\)
                                   PRIVATE RendererExport=__declspec\(dllexport\))
    else(WIN32)
        target_compile_definitions(${Library}
                                   PUBLIC RendererExport=
                                   PRIVATE RendererExport=)
    endif(WIN32)
ENDIF(Renderer_BUILD_STATIC_LIBRARY)

source_group("source" FILES ${Renderer_CPP_SRCS})
source_group("include" FILES ${Renderer_HPP_SRCS})
source_group("interface" FILES ${Renderer_Headers})

if(WIN32)
    install(TARGETS FreeCADRenderer
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
else(WIN32)
    install(TARGETS FreeCADRenderer
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(WIN32)

