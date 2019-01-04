# -*- coding: utf-8 -*-
# _TEMPLATEPY_ gui init module
# (c) 2001 Juergen Riegel
# License LGPL

import FreeCAD


class _TEMPLATEPY_Workbench ( Workbench ):
    "_TEMPLATEPY_ workbench object"
    Icon = FreeCAD.getHomePath() + "Mod/_TEMPLATEPY_/Resources/icons/_TEMPLATEPY_Workbench.svg"
    MenuText = "_TEMPLATEPY_"
    ToolTip = "_TEMPLATEPY_ workbench"
    
    def Initialize(self):
        # load the module
        import _TEMPLATEPY_Gui
        self.appendToolbar('_TEMPLATEPY_',['_TEMPLATEPY__HelloWorld'])
        self.appendMenu('_TEMPLATEPY_',['_TEMPLATEPY__HelloWorld'])
    
    def GetClassName(self):
        return "Gui::PythonWorkbench"

Gui.addWorkbench(_TEMPLATEPY_Workbench())
