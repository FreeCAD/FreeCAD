# FreeCAD init script of the Mesh module
# (c) 2004 Werner Mayer LGPL

# Append the open handler
FreeCAD.addImportType("Mesh formats (*.stl *.ast *.bms *.obj *.off *.ply)","Mesh")
FreeCAD.addExportType("Mesh formats (*.stl *.ast *.bms *.obj *.off *.ply)","Mesh")
