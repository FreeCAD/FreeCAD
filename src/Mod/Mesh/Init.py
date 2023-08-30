# FreeCAD init script of the Mesh module
# (c) 2004 Werner Mayer LGPL

import FreeCAD


def QT_TRANSLATE_NOOP(_1, txt):
    return txt


# Append the open handler
FreeCAD.addImportType(QT_TRANSLATE_NOOP("FileFormat", "STL Mesh (*.stl *.ast)"), "Mesh")
FreeCAD.addImportType(QT_TRANSLATE_NOOP("FileFormat", "Binary Mesh (*.bms)"), "Mesh")
FreeCAD.addImportType(QT_TRANSLATE_NOOP("FileFormat", "Alias Mesh (*.obj)"), "Mesh")
FreeCAD.addImportType(QT_TRANSLATE_NOOP("FileFormat", "Object File Format Mesh (*.off)"), "Mesh")
FreeCAD.addImportType(QT_TRANSLATE_NOOP("FileFormat", "Stanford Triangle Mesh (*.ply)"), "Mesh")
FreeCAD.addImportType(QT_TRANSLATE_NOOP("FileFormat", "Simple Model Format (*.smf)"), "Mesh")
FreeCAD.addImportType(QT_TRANSLATE_NOOP("FileFormat", "3D Manufacturing Format (*.3mf)"), "Mesh")

FreeCAD.addExportType(QT_TRANSLATE_NOOP("FileFormat", "STL Mesh (*.stl *.ast)"), "Mesh")
FreeCAD.addExportType(QT_TRANSLATE_NOOP("FileFormat", "Binary Mesh (*.bms)"), "Mesh")
FreeCAD.addExportType(QT_TRANSLATE_NOOP("FileFormat", "Alias Mesh (*.obj)"), "Mesh")
FreeCAD.addExportType(QT_TRANSLATE_NOOP("FileFormat", "Object File Format Mesh (*.off)"), "Mesh")
FreeCAD.addExportType(QT_TRANSLATE_NOOP("FileFormat", "Stanford Triangle Mesh (*.ply)"), "Mesh")
FreeCAD.addExportType(QT_TRANSLATE_NOOP("FileFormat", "Additive Manufacturing Format (*.amf)"), "Mesh")
FreeCAD.addExportType(QT_TRANSLATE_NOOP("FileFormat", "Simple Model Format (*.smf)"), "Mesh")
FreeCAD.addExportType(QT_TRANSLATE_NOOP("FileFormat", "3D Manufacturing Format (*.3mf)"), "Mesh")

FreeCAD.__unit_test__ += [ "MeshTestsApp" ]
