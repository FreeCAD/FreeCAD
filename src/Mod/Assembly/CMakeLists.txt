
option(FREECAD_ASSEMBLY_SOLVER_LOGS "Generate extensive logs of the assembly solving process" OFF)

if(FREECAD_ASSEMBLY_SOLVER_LOGS)
  find_package(Boost COMPONENTS log REQUIRED)
  add_definitions(-DUSE_LOGGING)
endif(FREECAD_ASSEMBLY_SOLVER_LOGS)

if(MSVC)
add_definitions(/wd4503)
add_definitions(/wd4181)
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
