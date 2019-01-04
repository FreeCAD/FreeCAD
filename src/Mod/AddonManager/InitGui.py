# -*- coding: utf-8 -*-
# AddonManager gui init module
# (c) 2001 Juergen Riegel
# License LGPL

import FreeCADGui
from AddonManagerGui import CmdAddonMgr

FreeCADGui.addCommand('Std_AddonMgr', CmdAddonMgr())
