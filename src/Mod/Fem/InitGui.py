# ***************************************************************************
# *   Copyright (c) 2009 Juergen Riegel <juergen.riegel@web.de>             *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""FEM module Gui init script

Gathering all the information to start FreeCAD.
This is the second one of three init scripts.
The third one runs when the gui is up.

The script is executed using exec().
This happens inside srd/Gui/FreeCADGuiInit.py
All imports made there are available here too.
Thus no need to import them here.
But the import code line is used anyway to get flake8 quired.
Since they are cached they will not be imported twice.
"""

__title__ = "FEM module Gui init script"
__author__ = "Juergen Riegel, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

# imports to get flake8 quired
import sys
import FreeCAD
import FreeCADGui
from FreeCADGui import Workbench

# needed imports
from femguiutils.migrate_gui import FemMigrateGui


# migrate old FEM Gui objects
sys.meta_path.append(FemMigrateGui())


# add FEM Gui unit tests
FreeCAD.__unit_test__ += ["TestFemGui"]


class FemWorkbench(Workbench):
    "Fem workbench object"

    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Fem/Resources/icons/FemWorkbench.svg"
        self.__class__.MenuText = "FEM"
        self.__class__.ToolTip = "FEM workbench"

    def Initialize(self):
        # load the module
        import Fem
        import FemGui
        import femcommands.commands
        # dummy usage to get flake8 and lgtm quiet
        False if Fem.__name__ else True
        False if FemGui.__name__ else True
        False if femcommands.commands.__name__ else True

    def GetClassName(self):
        # see https://forum.freecad.org/viewtopic.php?f=10&t=43300
        return "FemGui::Workbench"


FreeCADGui.addWorkbench(FemWorkbench())
