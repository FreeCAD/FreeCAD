# -*- coding: utf-8 -*-
# AddonManager gui init module
# (c) 2001 Juergen Riegel
# License LGPL

import AddonManager

FreeCADGui.addLanguagePath(":/translations")
FreeCADGui.addCommand("Std_AddonMgr", AddonManager.CommandAddonManager())

import FreeCAD

FreeCAD.__unit_test__ += ["TestAddonManagerGui"]
