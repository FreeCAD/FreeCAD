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
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Draft/Resources/icons/DraftWorkbench.svg"
        self.__class__.MenuText = "Draft"
        self.__class__.ToolTip = "The Draft module is used for basic 2D CAD Drafting"

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
                import PySide
            except ImportError:
                FreeCAD.Console.PrintWarning("Error: PySide not found, Draft workbench will be disabled.\n")
            else:
                depsOK = True
        if not depsOK:
            return

        # import Draft tools, icons
        try:
            import os,Draft_rc,DraftTools, DraftGui
            from DraftTools import translate
            FreeCADGui.addLanguagePath(":/translations")
            FreeCADGui.addIconPath(":/icons")
        except Exception as inst:
            print(inst)
            FreeCAD.Console.PrintError("Error: Initializing one or more of the Draft modules failed, Draft will not work as expected.\n")

        # setup menus
        self.cmdList = ["Draft_Line","Draft_Wire","Draft_Circle","Draft_Arc","Draft_Ellipse",
                        "Draft_Polygon","Draft_Rectangle", "Draft_Text",
                        "Draft_Dimension", "Draft_BSpline","Draft_Point",
                        "Draft_ShapeString","Draft_Facebinder","Draft_BezCurve","Draft_Label"]
        self.modList = ["Draft_Move","Draft_Rotate","Draft_Offset",
                        "Draft_Trimex", "Draft_Join", "Draft_Upgrade", "Draft_Downgrade", "Draft_Scale",
                        "Draft_Edit","Draft_WireToBSpline","Draft_AddPoint",
                        "Draft_DelPoint","Draft_Shape2DView","Draft_Draft2Sketch","Draft_Array",
                        "Draft_PathArray", "Draft_PointArray","Draft_Clone",
                        "Draft_Drawing","Draft_Mirror","Draft_Stretch"]
        self.treecmdList = ["Draft_ApplyStyle","Draft_ToggleDisplayMode","Draft_AddToGroup",
                            "Draft_SelectGroup","Draft_SelectPlane",
                            "Draft_ShowSnapBar","Draft_ToggleGrid","Draft_AutoGroup"]
        self.lineList = ["Draft_UndoLine","Draft_FinishLine","Draft_CloseLine"]
        self.utils = ["Draft_VisGroup","Draft_Heal","Draft_FlipDimension",
                      "Draft_ToggleConstructionMode","Draft_ToggleContinueMode","Draft_Edit",
                      "Draft_Slope","Draft_SetWorkingPlaneProxy","Draft_AddConstruction"]
        self.snapList = ['Draft_Snap_Lock','Draft_Snap_Midpoint','Draft_Snap_Perpendicular',
                         'Draft_Snap_Grid','Draft_Snap_Intersection','Draft_Snap_Parallel',
                         'Draft_Snap_Endpoint','Draft_Snap_Angle','Draft_Snap_Center',
                         'Draft_Snap_Extension','Draft_Snap_Near','Draft_Snap_Ortho','Draft_Snap_Special',
                         'Draft_Snap_Dimensions','Draft_Snap_WorkingPlane']
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench","Draft creation tools"),self.cmdList)
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench","Draft modification tools"),self.modList)
        self.appendMenu(QT_TRANSLATE_NOOP("draft","&Draft"),self.cmdList+self.modList)
        self.appendMenu([QT_TRANSLATE_NOOP("draft","&Draft"),QT_TRANSLATE_NOOP("Workbench","Utilities")],self.utils+self.treecmdList)
        self.appendMenu([QT_TRANSLATE_NOOP("draft","&Draft"),QT_TRANSLATE_NOOP("Workbench","Wire tools")],self.lineList)
        self.appendMenu([QT_TRANSLATE_NOOP("draft","&Draft"),QT_TRANSLATE_NOOP("Workbench","Snapping")],self.snapList)
        if hasattr(FreeCADGui,"draftToolBar"):
            if not hasattr(FreeCADGui.draftToolBar,"loadedPreferences"):
                FreeCADGui.addPreferencePage(":/ui/preferences-draft.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/preferences-draftsnap.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/preferences-draftvisual.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/preferences-drafttexts.ui","Draft")
                FreeCADGui.draftToolBar.loadedPreferences = True
        Log ('Loading Draft module...done\n')

    def Activated(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.Activated()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.show()
        Log("Draft workbench activated\n")
        
    def Deactivated(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.Deactivated()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.hide()
        Log("Draft workbench deactivated\n")

    def ContextMenu(self, recipient):
        if (recipient == "View"):
            if (FreeCAD.activeDraftCommand == None):
                if (FreeCADGui.Selection.getSelection()):
                    self.appendContextMenu("Draft",self.cmdList+self.modList)
                    self.appendContextMenu("Utilities",self.treecmdList)
                else:
                    self.appendContextMenu("Draft",self.cmdList)
            else:
                if (FreeCAD.activeDraftCommand.featureName == "Line"):
                    self.appendContextMenu("",self.lineList)
        else:
            if (FreeCADGui.Selection.getSelection()):
                self.appendContextMenu("Utilities",self.treecmdList)

    def GetClassName(self): 
        return "Gui::PythonWorkbench"

FreeCADGui.addWorkbench(DraftWorkbench)

# File format pref pages are independent and can be loaded at startup
import Draft_rc
FreeCADGui.addPreferencePage(":/ui/preferences-dxf.ui","Import-Export")
FreeCADGui.addPreferencePage(":/ui/preferences-dwg.ui","Import-Export")
FreeCADGui.addPreferencePage(":/ui/preferences-svg.ui","Import-Export")
FreeCADGui.addPreferencePage(":/ui/preferences-oca.ui","Import-Export")

FreeCAD.__unit_test__ += [ "TestDraft" ]
