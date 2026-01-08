# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   © 2002 Juergen Riegel <juergen.riegel@web.de>                              #
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

'''
Gathering all the information to start FreeCAD
This is the second one of three init scripts, 
the third one runs when the gui is up.
'''


# Registered in Part's Init.py file
FreeCAD.changeImportModule("STEP with colors (*.step *.STEP *.stp *.STP)", "Import", "ImportGui")
FreeCAD.changeExportModule("STEP with colors (*.step *.stp)", "Import", "ImportGui")
FreeCAD.changeExportModule("glTF (*.gltf *.glb)", "Import", "ImportGui")

App.__unit_test__ += ["TestImportGui"]

"""
class ImportWorkbench ( Workbench ):
    "Import workbench object"
    def Activate(self):
        # load the module
        try:
            Log ('Loading ImportGui module')
            import Import
            import ImportGui
        except ImportError:
            Err('Cannot load ImportGui')
            raise
    def GetIcon(self):
        # returns an icon for the workbench
        return ["/* XPM */\n"
            "static const char *fileopen[] = {\n"
            "\"16 13 5 1\",\n"
            "\". c #040404\",\n"
            "\"# c #808304\",\n"
            "\"a c None\",\n"
            "\"b c #f3f704\",\n"
            "\"c c #f3f7f3\",\n"
            "\"aaaaaaaaa...aaaa\",\n"
            "\"aaaaaaaa.aaa.a.a\",\n"
            "\"aaaaaaaaaaaaa..a\",\n"
            "\"a...aaaaaaaa...a\",\n"
            "\".bcb.......aaaaa\",\n"
            "\".cbcbcbcbc.aaaaa\",\n"
            "\".bcbcbcbcb.aaaaa\",\n"
            "\".cbcb...........\",\n"
            "\".bcb.#########.a\",\n"
            "\".cb.#########.aa\",\n"
            "\".b.#########.aaa\",\n"
            "\"..#########.aaaa\",\n"
            "\"...........aaaaa\"};\n"]

Gui.addWorkbench("Import",ImportWorkbench())
"""
# See https://forum.freecad.org/viewtopic.php?f=3&t=26782
# import Import_rc
# FreeCADGui.addPreferencePage(":/ui/preferences-import.ui","Import-Export")
