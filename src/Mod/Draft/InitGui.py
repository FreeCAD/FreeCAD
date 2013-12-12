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
__url__ = ["http://www.freecadweb.org"]

class DraftWorkbench (Workbench):
    "the Draft Workbench"
    Icon = """
        /* XPM */
        static char * draft_xpm[] = {
        "16 16 17 1",
        " 	c None",
        ".	c #5F4A1C",
        "+	c #5A4E36",
        "@	c #8A4D00",
        "#	c #835A04",
        "$	c #7E711F",
        "%	c #847954",
        "&	c #C27400",
        "*	c #817D74",
        "=	c #E79300",
        "-	c #BFAB0C",
        ";	c #ADA791",
        ">	c #B3AE87",
        ",	c #B0B2AE",
        "'	c #ECD200",
        ")	c #D6D8D5",
        "!	c #FCFEFA",
        "   ,!!)!!!!!!!!!",
        "   ,!!>;!!!!!!!!",
        "   ,!!>-,!!!!!!!",
        "   ,!!>'$)!!!!!!",
        "   ,!!>-'%!!!!!!",
        "   ,!!>-$-;!!!!!",
        "   ,!!>-*-$)!!!!",
        " @&+!!>-*;-%!!!!",
        "@&=+)!;'-''-*!!!",
        ".@@.;;%%....+;;!",
        ".&&===========$,",
        ".&&=====&&####.,",
        ".&&.++***,,)))!!",
        "#==+)!!!!!!!!!!!",
        " ##+)!!!!!!!!!!!",
        "   *,,,,,,,,,,,,"};"""

    MenuText = "Draft"
    ToolTip = "The Draft module is used for basic 2D CAD Drafting"

    def Initialize(self):
        def QT_TRANSLATE_NOOP(scope, text):
            return text

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

        # import Draft tools, icons and macros menu
        try:
            import os,macros,Draft_rc,DraftTools, DraftGui
            from DraftTools import translate
            FreeCADGui.addLanguagePath(":/translations")
            FreeCADGui.addIconPath(":/icons")
            self.appendMenu(["&Macro",str(translate("draft","Installed Macros"))],macros.macrosList)
        except:
            pass

        # setup menus
        self.cmdList = ["Draft_Line","Draft_Wire","Draft_Circle","Draft_Arc","Draft_Ellipse",
                        "Draft_Polygon","Draft_Rectangle", "Draft_Text",
                        "Draft_Dimension", "Draft_BSpline","Draft_Point",
                        "Draft_ShapeString","Draft_Facebinder"]
        self.modList = ["Draft_Move","Draft_Rotate","Draft_Offset",
                        "Draft_Trimex", "Draft_Upgrade", "Draft_Downgrade", "Draft_Scale",
                        "Draft_Drawing","Draft_Edit","Draft_WireToBSpline","Draft_AddPoint",
                        "Draft_DelPoint","Draft_Shape2DView","Draft_Draft2Sketch","Draft_Array",
                        "Draft_PathArray","Draft_Clone"]
        self.treecmdList = ["Draft_ApplyStyle","Draft_ToggleDisplayMode","Draft_AddToGroup",
                            "Draft_SelectGroup","Draft_SelectPlane",
                            "Draft_ShowSnapBar","Draft_ToggleGrid"]
        self.lineList = ["Draft_UndoLine","Draft_FinishLine","Draft_CloseLine"]
        self.utils = ["Draft_Heal","Draft_FlipDimension"]
        self.snapList = ['Draft_Snap_Lock','Draft_Snap_Midpoint','Draft_Snap_Perpendicular',
                         'Draft_Snap_Grid','Draft_Snap_Intersection','Draft_Snap_Parallel',
                         'Draft_Snap_Endpoint','Draft_Snap_Angle','Draft_Snap_Center',
                         'Draft_Snap_Extension','Draft_Snap_Near','Draft_Snap_Ortho',
                         'Draft_Snap_Dimensions']
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench","Draft creation tools"),self.cmdList)
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench","Draft modification tools"),self.modList)
        self.appendMenu(str(translate("draft","&Draft")),self.cmdList+self.modList)
        self.appendMenu([str(translate("draft","&Draft")),str(translate("draft","Context tools"))],self.treecmdList)
        self.appendMenu([str(translate("draft","&Draft")),str(translate("draft","Utilities"))],self.utils)
        self.appendMenu([str(translate("draft","&Draft")),str(translate("draft","Wire tools"))],self.lineList)
        self.appendMenu([str(translate("draft","&Draft")),str(translate("draft","Snapping"))],self.snapList)
        if hasattr(FreeCADGui,"draftToolBar"):
            if not hasattr(FreeCADGui.draftToolBar,"loadedPreferences"):
                FreeCADGui.addPreferencePage(":/ui/userprefs-base.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/userprefs-visual.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/userprefs-import.ui","Draft")
                FreeCADGui.draftToolBar.loadedPreferences = True
        Log ('Loading Draft module...done\n')

    def Activated(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.Activated()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.show()
        Msg("Draft workbench activated\n")
        
    def Deactivated(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.Deactivated()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.hide()
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

# add Import/Export types
App.addImportType("Autodesk DXF (*.dxf)","importDXF") 
App.addImportType("SVG as geometry (*.svg)","importSVG")
App.addImportType("Open CAD Format (*.oca *.gcad)","importOCA")
App.addImportType("Common airfoil data (*.dat)","importAirfoilDAT")
App.addExportType("Autodesk DXF (*.dxf)","importDXF")
App.addExportType("Flattened SVG (*.svg)","importSVG")
App.addExportType("Open CAD Format (*.oca)","importOCA")
App.addImportType("Autodesk DWG (*.dwg)","importDWG") 
App.addExportType("Autodesk DWG (*.dwg)","importDWG")
