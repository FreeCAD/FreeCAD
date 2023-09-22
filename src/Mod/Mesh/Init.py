# FreeCAD init script of the Mesh module
# (c) 2004 Werner Mayer LGPL

import FreeCAD

# Append the open handler
FreeCAD.addImportType("STL Mesh (*.stl *.ast)", "Mesh")
FreeCAD.addImportType("Binary Mesh (*.bms)", "Mesh")
FreeCAD.addImportType("Alias Mesh (*.obj)", "Mesh")
FreeCAD.addImportType("Object File Format Mesh (*.off)", "Mesh")
FreeCAD.addImportType("Stanford Triangle Mesh (*.ply)", "Mesh")
FreeCAD.addImportType("Simple Model Format (*.smf)", "Mesh")
FreeCAD.addImportType("3D Manufacturing Format (*.3mf)", "Mesh")

FreeCAD.addExportType("STL Mesh (*.stl *.ast)", "Mesh")
FreeCAD.addExportType("Binary Mesh (*.bms)", "Mesh")
FreeCAD.addExportType("Alias Mesh (*.obj)", "Mesh")
FreeCAD.addExportType("Object File Format Mesh (*.off)", "Mesh")
FreeCAD.addExportType("Stanford Triangle Mesh (*.ply)", "Mesh")
FreeCAD.addExportType("Additive Manufacturing Format (*.amf)", "Mesh")
FreeCAD.addExportType("Simple Model Format (*.smf)", "Mesh")
FreeCAD.addExportType("3D Manufacturing Format (*.3mf)", "Mesh")

FreeCAD.__unit_test__ += ["MeshTestsApp"]
