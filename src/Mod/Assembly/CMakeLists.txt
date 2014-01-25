
option(FREECAD_ASSEMBLY_DEBUG_FACILITIES "Tune solver settings manual and generate extensive logs of the assembly solving process" OFF)

if(FREECAD_ASSEMBLY_DEBUG_FACILITIES)
  find_package(Boost COMPONENTS log REQUIRED)
  add_definitions(-DUSE_LOGGING)
  add_definitions(-DASSEMBLY_DEBUG_FACILITIES)
endif(FREECAD_ASSEMBLY_DEBUG_FACILITIES)

if(MSVC)
add_definitions(/wd4503)
add_definitions(/wd4181)
else(MSVC)
add_definitions(-DDCM_EXTERNAL_CORE)
add_definitions(-DDCM_EXTERNAL_3D)
add_definitions(-DDCM_EXTERNAL_STATE)
endif(MSVC)

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
    DESTINATION
        Mod/Assembly
)
