
if(BUILD_POINTS)
  add_subdirectory(Points)
endif(BUILD_POINTS)

if(BUILD_COMPLETE)
  add_subdirectory(Complete)
endif(BUILD_COMPLETE)

if(BUILD_TEST)
  add_subdirectory(Test)
endif(BUILD_TEST)

#add_subdirectory(TemplatePyMod)

if(BUILD_IMAGE)
  add_subdirectory(Image)
endif(BUILD_IMAGE)

if(BUILD_MESH)
  add_subdirectory(Mesh)
endif(BUILD_MESH)

if(BUILD_PART)
  add_subdirectory(Part)
endif(BUILD_PART)

if(BUILD_MATERIAL)
  add_subdirectory(Material)
endif(BUILD_MATERIAL)

if(BUILD_PART_DESIGN)
  add_subdirectory(PartDesign)
endif(BUILD_PART_DESIGN)

if(BUILD_RAYTRACING)
  add_subdirectory(Raytracing)
endif(BUILD_RAYTRACING)

if(BUILD_DRAWING)
  add_subdirectory(Drawing)
endif(BUILD_DRAWING)

IF(EIGEN3_FOUND)
    if(BUILD_SKETCHER)
      add_subdirectory(Sketcher)
    endif(BUILD_SKETCHER)
    
    if(BUILD_ROBOT)
      add_subdirectory(Robot)
    endif(BUILD_ROBOT)
ELSE(EIGEN3_FOUND)
    MESSAGE("Due to the missing Eigen3 library the Sketcher module won't be built")
    MESSAGE("Due to the missing Eigen3 library the Robot module won't be built")
ENDIF(EIGEN3_FOUND)

if(BUILD_REVERSEENGINEERING)
  add_subdirectory(ReverseEngineering)
endif(BUILD_REVERSEENGINEERING)

if(BUILD_MESH_PART)
  add_subdirectory(MeshPart)
endif(BUILD_MESH_PART)

if(BUILD_DRAFT)
  add_subdirectory(Draft)
endif(BUILD_DRAFT)

if(BUILD_WEB)
  add_subdirectory(Web)
endif(BUILD_WEB)

if(BUILD_START)
  add_subdirectory(Start)
endif(BUILD_START)

if(BUILD_IDF)
  add_subdirectory(Idf)
endif(BUILD_IDF)

if(BUILD_IMPORT)
  add_subdirectory(Import)
endif(BUILD_IMPORT)

if(BUILD_INSPECTION)
  add_subdirectory(Inspection)
endif(BUILD_INSPECTION)

if(BUILD_ARCH)
  add_subdirectory(Arch)
endif(BUILD_ARCH)

if(BUILD_ASSEMBLY)
  add_subdirectory(Assembly)
endif(BUILD_ASSEMBLY)

if(BUILD_CAM)
    add_subdirectory(Cam)
endif(BUILD_CAM)

if(BUILD_FEM)
    add_subdirectory(Fem)
endif(BUILD_FEM)

if(BUILD_SANDBOX)
    add_subdirectory(Sandbox)
endif(BUILD_SANDBOX)

if(BUILD_SHIP)
  add_subdirectory(Ship)
endif(BUILD_SHIP)

if(BUILD_OPENSCAD)
  add_subdirectory(OpenSCAD)
endif(BUILD_OPENSCAD)

if(BUILD_PLOT)
  add_subdirectory(Plot)
endif(BUILD_PLOT)

if(BUILD_SPREADSHEET)
  add_subdirectory(Spreadsheet)
endif(BUILD_SPREADSHEET)

if(BUILD_JTREADER)
  add_subdirectory(JtReader)
endif(BUILD_JTREADER)

if(BUILD_PATH)
    add_subdirectory(Path)
endif(BUILD_PATH)
