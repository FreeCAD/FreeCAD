# FreeCAD init script of the JtReader module
# (c) 2007 Juergen Riegel LGPL

import FreeCAD

# Append the open handler
FreeCAD.addImportType("JtOpen (*.jt)","JtReader")
