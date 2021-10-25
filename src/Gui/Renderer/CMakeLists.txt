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

    file(GLOB_RECURSE Assets RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} bgfx/assets/*)
    list(APPEND Renderer_Assets ${Assets})

endif(BUILD_BGFX)

if(BUILD_DILIGENT)
    add_definitions(-DHAVE_DILIGENT)

    list(APPEND Renderer_CPP_SRCS DiligentRenderer.cpp
                                  Diligent/TexturedCube.cpp)
    list(APPEND Renderer_HPP_SRCS DiligentRenderer.h
                                  Diligent/TexturedCube.hpp)
    list(APPEND Renderer_LIBS
        Diligent-Common
        Diligent-GraphicsTools
        Diligent-TextureLoader
        Diligent-GraphicsEngineOpenGL-static
        Diligent-GraphicsEngineVk-static)
    # if (APPLE)
    #     list(APPEND Renderer_LIBS
    #         Diligent-GraphicsEngineMetal-static)
    # endif()
    # if (MSVC)
    #     list(APPEND Renderer_LIBS
    #         Diligent-GraphicsEngineD3D11-static
    #         Diligent-GraphicsEngineD3D12-static)
    # endif()

    file(GLOB_RECURSE Assets RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Diligent/assets/*)
    list(APPEND Renderer_Assets ${Assets})

endif(BUILD_DILIGENT)

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

install(FILES ${Renderer_Assets} DESTINATION ${CMAKE_INSTALL_DATADIR}/Renderer)

ADD_CUSTOM_TARGET(Renderer_assets ALL SOURCES ${Assets})
fc_copy_sources(Renderer_assets
                "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Renderer"
                ${Renderer_Assets})

