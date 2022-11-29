#***************************************************************************
#*   Copyright (c) 2002,2003 Juergen Riegel <juergen.riegel@web.de>        *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/

# PartDesign gui init module
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

class PartDesignWorkbench ( Workbench ):
    "PartDesign workbench object"
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/PartDesign/Resources/icons/PartDesignWorkbench.svg"
        self.__class__.MenuText = "Part Design"
        self.__class__.ToolTip = "Part Design workbench"

    def Initialize(self):
        # load the module
        try:
            import traceback
            from PartDesign.WizardShaft import WizardShaft
        except RuntimeError:
            print ("{}".format(traceback.format_exc()))
        except ImportError:
            print("Wizard shaft module cannot be loaded")
            try:
                from FeatureHole import HoleGui
            except Exception:
                pass

        import PartDesignGui
        import PartDesign
        try:
            from PartDesign import InvoluteGearFeature
            from PartDesign import SprocketFeature
        except ImportError:
            print("Involute gear module cannot be loaded")
            #try:
            #    from FeatureHole import HoleGui
            #except:
            #    pass

    def GetClassName(self):
        return "PartDesignGui::Workbench"

Gui.addWorkbench(PartDesignWorkbench())

FreeCAD.__unit_test__ += [ "TestPartDesignGui" ]
