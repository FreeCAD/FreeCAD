# PartDesign gui init module
# (c) 2003 Juergen Riegel
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

#***************************************************************************
#*   (c) Juergen Riegel (juergen.riegel@web.de) 2002                       *
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
#*   Juergen Riegel 2002                                                   *
#***************************************************************************/

class PartDesignWorkbench ( Workbench ):
        "PartDesign workbench object"
        Icon = """
                /* XPM */
                static char * partdesign_xpm[] = {
                "16 16 9 1",
                " 	c None",
                ".	c #040006",
                "+	c #070F38",
                "@	c #002196",
                "#	c #0030F3",
                "$	c #5A4D20",
                "%	c #858EB2",
                "&	c #DEB715",
                "*	c #BFB99D",
                " &    ........  ",
                "&&&$..@@@@@@+...",
                "&&&&$@#####@..@.",
                "&&&&&$......@#@.",
                "&&&&&&@@@+.###@.",
                "$&&&&&&@#@.###@.",
                ".$&&&&&%#@.###@.",
                ".@*&&&*%#@.###@.",
                ".@#*&**%#@.###@.",
                ".@#@%%%.@@.###@.",
                ".@@@@@@@#@.###@.",
                ".@#######@.###@.",
                ".@#######@.##+. ",
                ".+@@@####@.@..  ",
                " ......+++..    ",
                "        ...     "};
                        """
        MenuText = "Part Design"
        ToolTip = "Part Design workbench"

        def Initialize(self):
                # load the module
                try:
                    from WizardShaft import WizardShaft
                except:
                    print "Wizard shaft not installed"
                import PartDesignGui
                import PartDesign
                import InvoluteGearFeature
        def GetClassName(self):
                return "PartDesignGui::Workbench"

Gui.addWorkbench(PartDesignWorkbench())
