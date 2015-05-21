#! python
# -*- coding: utf-8 -*-

import sys
from PySide import QtGui ,QtCore
import FreeCADGui
import CEM

#app = QtGui.qApp
#mw = FreeCADGui.getMainWindow()

#QtGui.QMessageBox.information(None,"Apollo program","Houston, we have a problem")

class CEMGui_create_PLM:
    def __init__(self):
        pass

    def __del__(self):
        FreeCAD.Console.PrintError('CEMGui_create_PLM was destroyed\n')

    def Activated(self):
        print "create PML"
        if FreeCADGui.ActiveDocument != None:
            pass
        else:
            FreeCAD.Console.PrintWarning('A 3d view must be created\n')

    def GetResources(self):
        return {'Pixmap' : 'python',
                'MenuText': 'Create PML...',
                'ToolTip': 'Create a perfectly matched layer.'}

class CEMGui_create_lattice:
    def __init__(self):
        pass

    def __del__(self):
        FreeCAD.Console.PrintError('CEMGui_create_lattice was destroyed\n')

    def Activated(self):
        print "Create lattice"
        if FreeCADGui.ActiveDocument != None:
            import lattice
            # Check so that there is not already a lattice object, since there can only be one:
#            if FreeCAD.ActiveDocument
            lattice.makeLattice()
        else:
            FreeCAD.Console.PrintWarning('A 3d view must be created\n')

    def GetResources(self):
        return {'Pixmap' : 'python',
                'MenuText': 'Create lattice...',
                'ToolTip': 'Create the boundary for the simulation. In MEEP this is set via the geometry-lattice variable, hence the name.'}


FreeCADGui.addCommand('CEMGui_create_PLM',CEMGui_create_PLM())
FreeCADGui.addCommand('CEMGui_create_lattice',CEMGui_create_lattice())

