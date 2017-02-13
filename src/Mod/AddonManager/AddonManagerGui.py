# -*- coding: utf-8 -*-
# FreeCAD tools of the AddonManager workbench
# (c) 2001 Juergen Riegel
# License LGPL

import FreeCAD, FreeCADGui

class CmdAddonMgr:
    def Activated(self):
        import AddonManager
        AddonManager.launchAddonMgr()
    def IsActive(self):
        return True
    def GetResources(self):
        return {'Pixmap': 'AddonManager', 'MenuText': '&Addon manager',
                'ToolTip': 'Manage FreeCAD workbenches and macros',
                'Group': 'Tools'}
