add_subdirectory(Points)
add_subdirectory(Complete)
add_subdirectory(Test)
#add_subdirectory(TemplatePyMod)

add_subdirectory(Image)

add_subdirectory(Mesh)

add_subdirectory(Part)
add_subdirectory(Material)
add_subdirectory(PartDesign)
add_subdirectory(Raytracing)
add_subdirectory(Drawing)
IF(EIGEN3_FOUND)
    add_subdirectory(Sketcher)
    add_subdirectory(Robot)
ELSE(EIGEN3_FOUND)
    MESSAGE("Due to the missing Eigen3 library the Sketcher module won't be built")
    MESSAGE("Due to the missing Eigen3 library the Robot module won't be built")
ENDIF(EIGEN3_FOUND)

add_subdirectory(ReverseEngineering)
add_subdirectory(MeshPart)
add_subdirectory(Draft)

add_subdirectory(Web)
add_subdirectory(Start)
add_subdirectory(Idf)
add_subdirectory(Import)
add_subdirectory(Inspection)

add_subdirectory(Arch)

add_subdirectory(Assembly)

if(FREECAD_BUILD_CAM)
    add_subdirectory(Cam)
endif(FREECAD_BUILD_CAM)
#if(FREECAD_BUILD_FEM)
    add_subdirectory(Fem)
#	MESSAGE("Build FEM")
#endif(FREECAD_BUILD_FEM)
if(FREECAD_BUILD_SANDBOX)
    add_subdirectory(Sandbox)
endif(FREECAD_BUILD_SANDBOX)

add_subdirectory(Ship)
add_subdirectory(OpenSCAD)

add_subdirectory(Plot)

add_subdirectory(Spreadsheet)



