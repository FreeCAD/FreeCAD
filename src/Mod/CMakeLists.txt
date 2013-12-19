#inter-module dependencies

#took these from Mod/Complete/App/AppComplete.cpp. appears to be working.
if(FREECAD_BUILD_COMPLETE)
  set(FREECAD_BUILD_MESH ON)
  set(FREECAD_BUILD_POINTS ON)
  set(FREECAD_BUILD_DRAWING ON)
  set(FREECAD_BUILD_RAYTRACING ON)
  set(FREECAD_BUILD_PART_DESIGN ON)
  set(FREECAD_BUILD_IMAGE ON)
  set(FREECAD_BUILD_DRAFT ON)
endif(FREECAD_BUILD_COMPLETE)

#inferred from .py files. appears to be working.
if(FREECAD_BUILD_ARCH)
  set(FREECAD_BUILD_PART ON)
  set(FREECAD_BUILD_MESH ON)
  set(FREECAD_BUILD_DRAFT ON)
endif(FREECAD_BUILD_ARCH)

#inferred from .py files. appears to be working.
if(FREECAD_BUILD_OPENSCAD)
  set(FREECAD_BUILD_MESH_PART ON)
  set(FREECAD_BUILD_DRAFT ON)
endif(FREECAD_BUILD_OPENSCAD)

#inferred from cmakelists.txt. appears to be working.
if(FREECAD_BUILD_INSPECTION)
  set(FREECAD_BUILD_MESH ON)
  set(FREECAD_BUILD_POINTS ON)
  set(FREECAD_BUILD_PART ON)
endif(FREECAD_BUILD_INSPECTION)

#inferred from .py files. appears to be working.
if(FREECAD_BUILD_SHIP)
  set(FREECAD_BUILD_PART ON)
  set(FREECAD_BUILD_PLOT ON)
  set(FREECAD_BUILD_IMAGE ON)
endif(FREECAD_BUILD_SHIP)

#inferred from .py files. appears to be working.
if(FREECAD_BUILD_SPREADSHEET)
  set(FREECAD_BUILD_DRAFT ON)
endif(FREECAD_BUILD_SPREADSHEET)

#this has been tested. appears to be working.
if(FREECAD_BUILD_DRAFT)
  set(FREECAD_BUILD_SKETCHER ON)
endif(FREECAD_BUILD_DRAFT)

#inferred from cmakelists.txt. appears to be working.
if(FREECAD_BUILD_PART_DESIGN)
  set(FREECAD_BUILD_SKETCHER ON)
endif(FREECAD_BUILD_PART_DESIGN)

#inferred from cmakelists.txt. appears to be working.
if(FREECAD_BUILD_REVERSEENGINEERING)
  set(FREECAD_BUILD_PART ON)
  set(FREECAD_BUILD_MESH ON)
endif(FREECAD_BUILD_REVERSEENGINEERING)

#inferred from cmakelists.txt. appears to be working.
if(FREECAD_BUILD_MESH_PART)
  set(FREECAD_BUILD_PART ON)
  set(FREECAD_BUILD_MESH ON)
endif(FREECAD_BUILD_MESH_PART)

#inferred from cmakelists.txt. build errors. obsolete?
if(FREECAD_BUILD_CAM)
  set(FREECAD_BUILD_PART ON)
  set(FREECAD_BUILD_MESH ON)
endif(FREECAD_BUILD_CAM)

#inferred from cmakelists.txt. appears to be working.
if(FREECAD_BUILD_RAYTRACING)
  set(FREECAD_BUILD_PART ON)
endif(FREECAD_BUILD_RAYTRACING)

#inferred from cmakelists.txt. appears to be working.
if(FREECAD_BUILD_DRAWING)
  set(FREECAD_BUILD_PART ON)
endif(FREECAD_BUILD_DRAWING)

#inferred from cmakelists.txt. appears to be working.
if(FREECAD_BUILD_ROBOT)
  set(FREECAD_BUILD_PART ON)
endif(FREECAD_BUILD_ROBOT)

#inferred from cmakelists.txt. appears to be working.
if(FREECAD_BUILD_SKETCHER)
  set(FREECAD_BUILD_PART ON)
endif(FREECAD_BUILD_SKETCHER)

#inferred from .py files. built, but not sure how to test.
if(FREECAD_BUILD_IDF)
  set(FREECAD_BUILD_PART ON)
endif(FREECAD_BUILD_IDF)

#inferred from cmakelists.txt. built, but not sure how to test.
if(FREECAD_BUILD_IMPORT)
  set(FREECAD_BUILD_PART ON)
endif(FREECAD_BUILD_IMPORT)

#inferred from cmakelists.txt. appears to be working.
if(FREECAD_BUILD_FEM)
  set(FREECAD_BUILD_PART ON)
endif(FREECAD_BUILD_FEM)

#inferred from cmakelists.txt. appears to be working.
if(FREECAD_BUILD_SANDBOX)
  set(FREECAD_BUILD_PART ON)
  set(FREECAD_BUILD_MESH ON)
endif(FREECAD_BUILD_SANDBOX)

#inferred from header includes. not tested.
if(FREECAD_BUILD_JTREADER)
  set(FREECAD_BUILD_MESH ON)
endif(FREECAD_BUILD_JTREADER)

#leaving test module for someone else.
#image has none. as far as I can tell.
#mesh has none. as far as I can tell.
#part has none. as far as I can tell.
#material has none. as far as I can tell.
#web has none. as far as I can tell.
#start has none. as far as I can tell.
#assembly has none. as far as I can tell.
#plot has none. as far as I can tell.
#points has none. as far as I can tell

#End inter-module dependencies

if(FREECAD_BUILD_POINTS)
  add_subdirectory(Points)
endif(FREECAD_BUILD_POINTS)

if(FREECAD_BUILD_COMPLETE)
  add_subdirectory(Complete)
endif(FREECAD_BUILD_COMPLETE)

if(FREECAD_BUILD_TEST)
  add_subdirectory(Test)
endif(FREECAD_BUILD_TEST)

#add_subdirectory(TemplatePyMod)

if(FREECAD_BUILD_IMAGE)
  add_subdirectory(Image)
endif(FREECAD_BUILD_IMAGE)

if(FREECAD_BUILD_MESH)
  add_subdirectory(Mesh)
endif(FREECAD_BUILD_MESH)

if(FREECAD_BUILD_PART)
  add_subdirectory(Part)
endif(FREECAD_BUILD_PART)

if(FREECAD_BUILD_MATERIAL)
  add_subdirectory(Material)
endif(FREECAD_BUILD_MATERIAL)

if(FREECAD_BUILD_PART_DESIGN)
  add_subdirectory(PartDesign)
endif(FREECAD_BUILD_PART_DESIGN)

if(FREECAD_BUILD_RAYTRACING)
  add_subdirectory(Raytracing)
endif(FREECAD_BUILD_RAYTRACING)

if(FREECAD_BUILD_DRAWING)
  add_subdirectory(Drawing)
endif(FREECAD_BUILD_DRAWING)

IF(EIGEN3_FOUND)
    if(FREECAD_BUILD_SKETCHER)
      add_subdirectory(Sketcher)
    endif(FREECAD_BUILD_SKETCHER)
    
    if(FREECAD_BUILD_ROBOT)
      add_subdirectory(Robot)
    endif(FREECAD_BUILD_ROBOT)
ELSE(EIGEN3_FOUND)
    MESSAGE("Due to the missing Eigen3 library the Sketcher module won't be built")
    MESSAGE("Due to the missing Eigen3 library the Robot module won't be built")
ENDIF(EIGEN3_FOUND)

if(FREECAD_BUILD_REVERSEENGINEERING)
  add_subdirectory(ReverseEngineering)
endif(FREECAD_BUILD_REVERSEENGINEERING)

if(FREECAD_BUILD_MESH_PART)
  add_subdirectory(MeshPart)
endif(FREECAD_BUILD_MESH_PART)

if(FREECAD_BUILD_DRAFT)
  add_subdirectory(Draft)
endif(FREECAD_BUILD_DRAFT)

if(FREECAD_BUILD_WEB)
  add_subdirectory(Web)
endif(FREECAD_BUILD_WEB)

if(FREECAD_BUILD_START)
  add_subdirectory(Start)
endif(FREECAD_BUILD_START)

if(FREECAD_BUILD_IDF)
  add_subdirectory(Idf)
endif(FREECAD_BUILD_IDF)

if(FREECAD_BUILD_IMPORT)
  add_subdirectory(Import)
endif(FREECAD_BUILD_IMPORT)

if(FREECAD_BUILD_INSPECTION)
  add_subdirectory(Inspection)
endif(FREECAD_BUILD_INSPECTION)

if(FREECAD_BUILD_ARCH)
  add_subdirectory(Arch)
endif(FREECAD_BUILD_ARCH)

if(FREECAD_BUILD_ASSEMBLY)
  add_subdirectory(Assembly)
endif(FREECAD_BUILD_ASSEMBLY)

if(FREECAD_BUILD_CAM)
    add_subdirectory(Cam)
endif(FREECAD_BUILD_CAM)

if(FREECAD_BUILD_FEM)
    add_subdirectory(Fem)
endif(FREECAD_BUILD_FEM)

if(FREECAD_BUILD_SANDBOX)
    add_subdirectory(Sandbox)
endif(FREECAD_BUILD_SANDBOX)

if(FREECAD_BUILD_SHIP)
  add_subdirectory(Ship)
endif(FREECAD_BUILD_SHIP)

if(FREECAD_BUILD_OPENSCAD)
  add_subdirectory(OpenSCAD)
endif(FREECAD_BUILD_OPENSCAD)

if(FREECAD_BUILD_PLOT)
  add_subdirectory(Plot)
endif(FREECAD_BUILD_PLOT)

if(FREECAD_BUILD_SPREADSHEET)
  add_subdirectory(Spreadsheet)
endif(FREECAD_BUILD_SPREADSHEET)

if(FREECAD_BUILD_JTREADER)
  add_subdirectory(JtReader)
endif(FREECAD_BUILD_JTREADER)
