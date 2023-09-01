if(BUILD_ADDONMGR)
  add_subdirectory(AddonManager)
endif(BUILD_ADDONMGR)

if(BUILD_ARCH)
  add_subdirectory(Arch)
endif(BUILD_ARCH)

if(BUILD_ASSEMBLY)
  add_subdirectory(Assembly)
endif(BUILD_ASSEMBLY)

if(BUILD_CLOUD)
    add_subdirectory(Cloud)
endif(BUILD_CLOUD)

if(BUILD_DRAFT)
  add_subdirectory(Draft)
endif(BUILD_DRAFT)

if(BUILD_DRAWING)
  add_subdirectory(Drawing)
endif(BUILD_DRAWING)

if(BUILD_FEM)
    add_subdirectory(Fem)
endif(BUILD_FEM)

if(BUILD_IDF)
  add_subdirectory(Idf)
endif(BUILD_IDF)

if(BUILD_IMPORT)
  add_subdirectory(Import)
endif(BUILD_IMPORT)

if(BUILD_INSPECTION)
  add_subdirectory(Inspection)
endif(BUILD_INSPECTION)

if(BUILD_JTREADER)
  add_subdirectory(JtReader)
endif(BUILD_JTREADER)

if(BUILD_MATERIAL)
  add_subdirectory(Material)
endif(BUILD_MATERIAL)

if(BUILD_MESH)
  add_subdirectory(Mesh)
endif(BUILD_MESH)

if(BUILD_MESH_PART)
  add_subdirectory(MeshPart)
endif(BUILD_MESH_PART)

if(BUILD_OPENSCAD)
  add_subdirectory(OpenSCAD)
endif(BUILD_OPENSCAD)

if(BUILD_PART)
  add_subdirectory(Part)
endif(BUILD_PART)

if(BUILD_PART_DESIGN)
  add_subdirectory(PartDesign)
endif(BUILD_PART_DESIGN)

if(BUILD_PATH)
    add_subdirectory(Path)
endif(BUILD_PATH)

if(BUILD_PLOT)
  add_subdirectory(Plot)
endif(BUILD_PLOT)

if(BUILD_POINTS)
  add_subdirectory(Points)
endif(BUILD_POINTS)

if(BUILD_REVERSEENGINEERING)
  add_subdirectory(ReverseEngineering)
endif(BUILD_REVERSEENGINEERING)

if(BUILD_ROBOT)
    if(EIGEN3_FOUND)
      add_subdirectory(Robot)
    else(EIGEN3_FOUND)
      MESSAGE("Due to the missing Eigen3 library the Robot module won't be built")
    endif(EIGEN3_FOUND)
endif(BUILD_ROBOT)

if(BUILD_SANDBOX)
    add_subdirectory(Sandbox)
endif(BUILD_SANDBOX)

if(BUILD_SHOW)
    add_subdirectory(Show)
endif(BUILD_SHOW)

if(BUILD_SKETCHER)
    if(EIGEN3_FOUND)
      add_subdirectory(Sketcher)
    else(EIGEN3_FOUND)
      MESSAGE("Due to the missing Eigen3 library the Sketcher module won't be built")
    endif(EIGEN3_FOUND)
endif(BUILD_SKETCHER)

if(BUILD_SPREADSHEET)
  add_subdirectory(Spreadsheet)
endif(BUILD_SPREADSHEET)

if(BUILD_START)
  add_subdirectory(Start)
endif(BUILD_START)

if(BUILD_SURFACE)
  add_subdirectory(Surface)
endif(BUILD_SURFACE)

if(BUILD_TECHDRAW)
    add_subdirectory(Measure)
    add_subdirectory(TechDraw)
endif(BUILD_TECHDRAW)

#add_subdirectory(TemplatePyMod)

if(BUILD_TEST)
  add_subdirectory(Test)
endif(BUILD_TEST)

if(BUILD_TUX)
    add_subdirectory(Tux)
endif(BUILD_TUX)

if(BUILD_WEB)
  add_subdirectory(Web)
endif(BUILD_WEB)
