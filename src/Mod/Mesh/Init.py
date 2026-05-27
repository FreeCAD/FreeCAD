# SPDX-License-Identifier: LGPL-2.1-or-later

# FreeCAD init script of the Mesh module
# (c) 2004 Werner Mayer LGPL

import FreeCAD

translate = FreeCAD.Qt.translate

# Append the open handler
FreeCAD.addImportType("STL Mesh (*.stl *.STL *.ast *.AST)", "Mesh")
FreeCAD.addImportType("Binary Mesh (*.bms *.BMS)", "Mesh")
FreeCAD.addImportType("Alias Mesh (*.obj *.OBJ)", "Mesh")
FreeCAD.addImportType("Object File Format Mesh (*.off *.OFF)", "Mesh")
FreeCAD.addImportType("Stanford Triangle Mesh (*.ply *.PLY)", "Mesh")
FreeCAD.addImportType("Simple Model Format (*.smf *.SMF)", "Mesh")
FreeCAD.addImportType("3D Manufacturing Format (*.3mf *.3MF)", "Mesh")

FreeCAD.addTranslatableExportType(translate("FileFormat", "STL Mesh"), ["stl", "ast"], "Mesh")
FreeCAD.addTranslatableExportType(translate("FileFormat", "Binary Mesh"), ["bms"], "Mesh")

#: Translation note: "Alias" in this case is a product/format name and should not be translated
FreeCAD.addTranslatableExportType(translate("FileFormat", "Alias Mesh"), ["obj"], "Mesh")

#: Translation note: "Object File Format" is the official name and should not be translated
FreeCAD.addTranslatableExportType(
    translate("FileFormat", "Object File Format Mesh"), ["off"], "Mesh"
)

FreeCAD.addExportType("Stanford Triangle Mesh (*.ply)", "Mesh")
FreeCAD.addExportType("Additive Manufacturing Format (*.amf)", "Mesh")
FreeCAD.addExportType("Simple Model Format (*.smf)", "Mesh")
FreeCAD.addExportType("3D Manufacturing Format (*.3mf)", "Mesh")

FreeCAD.__unit_test__ += ["MeshTestsApp"]
