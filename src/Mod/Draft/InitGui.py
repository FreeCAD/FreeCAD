#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              * 
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

__title__="FreeCAD Draft Workbench - Init file"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = ["http://free-cad.sourceforge.net"]

class DraftWorkbench (Workbench):
    "the Draft Workbench"
    Icon = """
        /* XPM */
        static char * draft_xpm[] = {
        "14 16 96 2",
        "  	c None",
        ". 	c #584605",
        "+ 	c #513E03",
        "@ 	c #E6B50D",
        "# 	c #C29F0E",
        "$ 	c #6E5004",
        "% 	c #F7BD0B",
        "& 	c #8F7008",
        "* 	c #F3C711",
        "= 	c #B1950F",
        "- 	c #785402",
        "; 	c #946C05",
        "> 	c #FABF0B",
        ", 	c #F7C20E",
        "' 	c #8D740A",
        ") 	c #F8D115",
        "! 	c #9F8A0F",
        "~ 	c #593D00",
        "{ 	c #FEB304",
        "] 	c #F3B208",
        "^ 	c #987407",
        "/ 	c #FDC70E",
        "( 	c #EFC311",
        "_ 	c #8F790C",
        ": 	c #FBDA18",
        "< 	c #8B7C0F",
        "[ 	c #B88203",
        "} 	c #FEBA08",
        "| 	c #E7B00A",
        "1 	c #A17E09",
        "2 	c #FCCE12",
        "3 	c #E6C213",
        "4 	c #96830E",
        "5 	c #FBE11C",
        "6 	c #786F0F",
        "7 	c #CA9406",
        "8 	c #FDC10B",
        "9 	c #D8AA0C",
        "0 	c #AE8E0C",
        "a 	c #FCD415",
        "b 	c #DBBF15",
        "c 	c #A09012",
        "d 	c #F9E61F",
        "e 	c #69650E",
        "f 	c #4B3702",
        "g 	c #DAA609",
        "h 	c #CAA50E",
        "i 	c #BB9D10",
        "j 	c #FCDB18",
        "k 	c #CEB817",
        "l 	c #AB9E15",
        "m 	c #F2E821",
        "n 	c #5E5C0E",
        "o 	c #503D03",
        "p 	c #E8B60D",
        "q 	c #CAAF13",
        "r 	c #C1B218",
        "s 	c #B6AE19",
        "t 	c #EAE625",
        "u 	c #575723",
        "v 	c #594605",
        "w 	c #F1C511",
        "x 	c #AB9510",
        "y 	c #D7C018",
        "z 	c #FBE81F",
        "A 	c #B3AC18",
        "B 	c #BCB81D",
        "C 	c #7F8051",
        "D 	c #645207",
        "E 	c #9D8C11",
        "F 	c #E4D31C",
        "G 	c #BEB62F",
        "H 	c #6C6A3F",
        "I 	c #E1E1E1",
        "J 	c #73610A",
        "K 	c #7C720F",
        "L 	c #A1A084",
        "M 	c #FFFFFF",
        "N 	c #565656",
        "O 	c #887921",
        "P 	c #988F44",
        "Q 	c #BFBEB7",
        "R 	c #EEEEEC",
        "S 	c #C0C0C0",
        "T 	c #323232",
        "U 	c #4D4B39",
        "V 	c #C7C7C7",
        "W 	c #FBFBFB",
        "X 	c #BFBFBF",
        "Y 	c #141414",
        "Z 	c #222222",
        "` 	c #303030",
        " .	c #313131",
        "..	c #282828",
        "+.	c #121212",
        "@.	c #000000",
        "        .                   ",
        "      + @ #                 ",
        "    $ % & * =               ",
        "  - ; > , ' ) !             ",
        "~ { ] ^ / ( _ : <           ",
        "  [ } | 1 2 3 4 5 6         ",
        "    7 8 9 0 a b c d e       ",
        "    f g / h i j k l m n     ",
        "      o p 2 i q 5 r s t u   ",
        "        v w a x y z A B C   ",
        "          D ) j E F G H I   ",
        "            J : 5 K L M M N ",
        "              O P Q R M S T ",
        "                U V W X Y Z ",
        "                    `  ...+.",
        "    @.@.@.@.@.@.@.@.        "};
        """

    MenuText = "Draft"
    ToolTip = "The Draft module is used for basic 2D CAD Drafting"

    def Initialize(self):
        # run self-tests
        depsOK = False
        try:
            from pivy import coin
            if FreeCADGui.getSoDBVersion() != coin.SoDB.getVersion():
                raise AssertionError("FreeCAD and Pivy use different versions of Coin. This will lead to unexpected behaviour.")
        except AssertionError:
            FreeCAD.Console.PrintWarning("Error: FreeCAD and Pivy use different versions of Coin. This will lead to unexpected behaviour.\n")
        except ImportError:
            FreeCAD.Console.PrintWarning("Error: Pivy not found, Draft workbench will be disabled.\n")
        except:
            FreeCAD.Console.PrintWarning("Error: Unknown error while trying to load Pivy\n")
        else:
            try:
                import PyQt4
            except ImportError:
                FreeCAD.Console.PrintWarning("Error: PyQt4 not found, Draft workbench will be disabled.\n")
            else:
                depsOK = True
        if not depsOK:
            return
    
        try:
            import os,macros,DraftTools,DraftGui,Draft_rc
            FreeCADGui.addLanguagePath(":/translations")
            FreeCADGui.addIconPath(":/icons")
            if not hasattr(FreeCADGui.draftToolBar,"loadedPreferences"):
                FreeCADGui.addPreferencePage(":/ui/userprefs-base.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/userprefs-import.ui","Draft")
                FreeCADGui.draftToolBar.loadedPreferences = True
            self.appendMenu(["&Macro",str(DraftTools.translate("draft","Installed Macros"))],macros.macrosList)
            Log ('Loading Draft module...done\n')
        except:
            pass
        self.cmdList = ["Draft_Line","Draft_Wire","Draft_Circle","Draft_Arc",
                        "Draft_Polygon","Draft_Rectangle", "Draft_Text",
                        "Draft_Dimension", "Draft_BSpline","Draft_Point"]
        self.modList = ["Draft_Move","Draft_Rotate","Draft_Offset",
                        "Draft_Trimex", "Draft_Upgrade", "Draft_Downgrade", "Draft_Scale",
                        "Draft_Drawing","Draft_Edit","Draft_WireToBSpline","Draft_AddPoint",
                        "Draft_DelPoint","Draft_Shape2DView","Draft_Draft2Sketch","Draft_Array",
                        "Draft_Clone"]
        self.treecmdList = ["Draft_ApplyStyle","Draft_ToggleDisplayMode","Draft_AddToGroup",
                            "Draft_SelectGroup","Draft_SelectPlane","Draft_ToggleSnap",
                            "Draft_ShowSnapBar","Draft_ToggleGrid"]
        self.lineList = ["Draft_UndoLine","Draft_FinishLine","Draft_CloseLine"]
        self.appendToolbar(str(DraftTools.translate("draft","Draft creation tools")),self.cmdList)
        self.appendToolbar(str(DraftTools.translate("draft","Draft modification tools")),self.modList)
        self.appendMenu(str(DraftTools.translate("draft","&Draft")),self.cmdList+self.modList)
        self.appendMenu([str(DraftTools.translate("draft","&Draft")),str(DraftTools.translate("draft","Context tools"))],self.treecmdList)
        self.appendMenu([str(DraftTools.translate("draft","&Draft")),str(DraftTools.translate("draft","Wire tools"))],self.lineList)
                                        
    def Activated(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.Activated()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.show()
        Msg("Draft workbench activated\n")
        
    def Deactivated(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.Deactivated()
        Msg("Draft workbench deactivated\n")

    def ContextMenu(self, recipient):
        if (recipient == "View"):
            if (FreeCAD.activeDraftCommand == None):
                if (FreeCADGui.Selection.getSelection()):
                    self.appendContextMenu("Draft",self.cmdList+self.modList)
                    self.appendContextMenu("Draft context tools",self.treecmdList)
                else:
                    self.appendContextMenu("Draft",self.cmdList)
            else:
                if (FreeCAD.activeDraftCommand.featureName == "Line"):
                    self.appendContextMenu("",self.lineList)
        else:
            if (FreeCADGui.Selection.getSelection()):
                self.appendContextMenu("Draft context tools",self.treecmdList)

    def GetClassName(self): 
        return "Gui::PythonWorkbench"

# ability to turn off the Draft workbench (since it is also all included in Arch)
if not FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("hideDraftWorkbench"):
    FreeCADGui.addWorkbench(DraftWorkbench)
App.addImportType("Autodesk DXF (*.dxf)","importDXF") 
App.addImportType("SVG as geometry (*.svg)","importSVG")
App.addImportType("Open CAD Format (*.oca *.gcad)","importOCA")
App.addImportType("Common airfoil data (*.dat)","importAirfoilDAT")
App.addExportType("Autodesk DXF (*.dxf)","importDXF")
App.addExportType("Flattened SVG (*.svg)","importSVG")
App.addExportType("Open CAD Format (*.oca)","importOCA")

