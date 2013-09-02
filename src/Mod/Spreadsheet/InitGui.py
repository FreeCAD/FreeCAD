#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013 - Yorik van Havre <yorik@uncreated.net>            *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

class SpreadsheetWorkbench(Workbench):
    "Spreadsheet workbench object"
    Icon = """
        /* XPM */
        static char * Spreadsheet_xpm[] = {
        "16 16 5 1",
        "   c None",
        ".  c #151614",
        "+  c #575956",
        "@  c #969895",
        "#  c #F7F9F6",
        "                ",
        "                ",
        " ...............",
        ".@##@+########@.",
        ".+@@+.@@@@@@@@+.",
        "..+++.+++++++++.",
        ".@##@+########@.",
        ".+@@+.@@@@@@@@+.",
        "..+++.+++++++++.",
        ".@##@+########@.",
        ".+@@+.@@@@@@@@+.",
        "..+++.+++++++++.",
        ".@##@+########@.",
        "..+++.+++++++++.",
        "                ",
        "                "};"""

    MenuText = "Spreadsheet"
    ToolTip = "Spreadsheet workbench"

    def Initialize(self):
        import Spreadsheet,Spreadsheet_rc
        from DraftTools import translate
        commands = ["Spreadsheet_Create","Spreadsheet_Controller"]
        self.appendToolbar(str(translate("Spreadsheet","Spreadsheet tools")),commands)
        self.appendMenu(str(translate("Spreadsheet","&Spreadsheet")),commands)
        FreeCADGui.addIconPath(":/icons")
        FreeCADGui.addLanguagePath(":/translations")
        Log ('Loading Spreadsheet module... done\n')

    def Activated(self):
        Msg("Spreadsheet workbench activated\n")
                
    def Deactivated(self):
        Msg("Spreadsheet workbench deactivated\n")

    def GetClassName(self): 
        return "Gui::PythonWorkbench"

FreeCADGui.addWorkbench(SpreadsheetWorkbench)


