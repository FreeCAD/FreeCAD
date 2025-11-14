# SPDX-License-Identifier: LGPL-2.1-or-later

# FreeCAD init script of the JtReader module
# (c) 2007 Juergen Riegel LGPL

# Append the open handler
FreeCAD.addImportType("JtOpen (*.jt)", "JtReader")
