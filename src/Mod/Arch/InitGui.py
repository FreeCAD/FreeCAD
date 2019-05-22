#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
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

class ArchWorkbench(Workbench):
    "Arch workbench object"
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Arch/Resources/icons/ArchWorkbench.svg"
        self.__class__.MenuText = "Arch"
        self.__class__.ToolTip = "Architecture workbench"

    def Initialize(self):
        import DraftTools,DraftGui,Arch_rc,Arch,Draft_rc
        from DraftTools import translate

        # arch tools
        self.archtools = ["Arch_Wall","Arch_Structure","Arch_Rebar","Arch_BuildingPart",
                     "Arch_Floor","Arch_Building","Arch_Site","Arch_Reference",
                     "Arch_Window","Arch_Roof","Arch_AxisTools",
                     "Arch_SectionPlane","Arch_Space","Arch_Stairs",
                     "Arch_PanelTools","Arch_Equipment",
                     "Arch_Frame", "Arch_Fence", "Arch_MaterialTools","Arch_Schedule","Arch_PipeTools",
                     "Arch_CutPlane","Arch_Add","Arch_Remove","Arch_Survey"]
        self.utilities = ["Arch_Component","Arch_CloneComponent","Arch_SplitMesh","Arch_MeshToShape",
                     "Arch_SelectNonSolidMeshes","Arch_RemoveShape",
                     "Arch_CloseHoles","Arch_MergeWalls","Arch_Check",
                     "Arch_ToggleIfcBrepFlag","Arch_3Views",
                     "Arch_IfcSpreadsheet","Arch_ToggleSubs"]
                     
        # try to locate the Rebar addon
        try:
            import RebarTools
        except:
            pass
        else:
            class RebarGroupCommand:
                def GetCommands(self):
                    return tuple(RebarTools.RebarCommands+["Arch_Rebar"])
                def GetResources(self):
                    return { 'MenuText': 'Rebar tools',
                             'ToolTip': 'Rebar tools'
                           }
                def IsActive(self):
                    return not FreeCAD.ActiveDocument is None
            FreeCADGui.addCommand('Arch_RebarTools', RebarGroupCommand())
            self.archtools[2] = "Arch_RebarTools"

        # draft tools
        self.drafttools = ["Draft_Line","Draft_Wire","Draft_Circle","Draft_Arc","Draft_Ellipse",
                        "Draft_Polygon","Draft_Rectangle", "Draft_Text",
                        "Draft_Dimension", "Draft_BSpline","Draft_Point",
                        "Draft_Facebinder","Draft_BezCurve","Draft_Label"]
        self.draftmodtools = ["Draft_Move","Draft_Rotate","Draft_Offset",
                        "Draft_Trimex", "Draft_Upgrade", "Draft_Downgrade", "Draft_Scale",
                        "Draft_Shape2DView","Draft_Draft2Sketch","Draft_Array",
                        "Draft_Clone"]
        self.draftextratools = ["Draft_WireToBSpline","Draft_AddPoint","Draft_DelPoint","Draft_ShapeString",
                                "Draft_PathArray","Draft_Mirror","Draft_Stretch"]
        self.draftcontexttools = ["Draft_ApplyStyle","Draft_ToggleDisplayMode","Draft_AddToGroup","Draft_AutoGroup",
                            "Draft_SelectGroup","Draft_SelectPlane",
                            "Draft_ShowSnapBar","Draft_ToggleGrid","Draft_UndoLine",
                            "Draft_FinishLine","Draft_CloseLine"]
        self.draftutils = ["Draft_VisGroup","Draft_Heal","Draft_FlipDimension",
                           "Draft_ToggleConstructionMode","Draft_ToggleContinueMode","Draft_Edit",
                           "Draft_Slope","Draft_SetWorkingPlaneProxy","Draft_AddConstruction"]
        self.snapList = ['Draft_Snap_Lock','Draft_Snap_Midpoint','Draft_Snap_Perpendicular',
                         'Draft_Snap_Grid','Draft_Snap_Intersection','Draft_Snap_Parallel',
                         'Draft_Snap_Endpoint','Draft_Snap_Angle','Draft_Snap_Center',
                         'Draft_Snap_Extension','Draft_Snap_Near','Draft_Snap_Ortho','Draft_Snap_Special',
                         'Draft_Snap_Dimensions','Draft_Snap_WorkingPlane']

        def QT_TRANSLATE_NOOP(scope, text): return text
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench","Arch tools"),self.archtools)
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench","Draft tools"),self.drafttools)
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench","Draft mod tools"),self.draftmodtools)
        self.appendMenu([QT_TRANSLATE_NOOP("arch","&Arch"),QT_TRANSLATE_NOOP("arch","Utilities")],self.utilities)
        self.appendMenu(QT_TRANSLATE_NOOP("arch","&Arch"),self.archtools)
        self.appendMenu(QT_TRANSLATE_NOOP("arch","&Draft"),self.drafttools+self.draftmodtools+self.draftextratools)
        self.appendMenu([QT_TRANSLATE_NOOP("arch","&Draft"),QT_TRANSLATE_NOOP("arch","Utilities")],self.draftutils+self.draftcontexttools)
        self.appendMenu([QT_TRANSLATE_NOOP("arch","&Draft"),QT_TRANSLATE_NOOP("arch","Snapping")],self.snapList)
        FreeCADGui.addIconPath(":/icons")
        FreeCADGui.addLanguagePath(":/translations")
        if hasattr(FreeCADGui,"draftToolBar"):
            if not hasattr(FreeCADGui.draftToolBar,"loadedArchPreferences"):
                FreeCADGui.addPreferencePage(":/ui/preferences-arch.ui","Arch")
                FreeCADGui.addPreferencePage(":/ui/preferences-archdefaults.ui","Arch")
                FreeCADGui.draftToolBar.loadedArchPreferences = True
            if not hasattr(FreeCADGui.draftToolBar,"loadedPreferences"):
                FreeCADGui.addPreferencePage(":/ui/preferences-draft.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/preferences-draftsnap.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/preferences-draftvisual.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/preferences-drafttexts.ui","Draft")
                FreeCADGui.draftToolBar.loadedPreferences = True
        Log ('Loading Arch module... done\n')

    def Activated(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.Activated()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.show()
        Log("Arch workbench activated\n")
                
    def Deactivated(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.Deactivated()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.hide()
        Log("Arch workbench deactivated\n")

    def ContextMenu(self, recipient):
        self.appendContextMenu("Utilities",self.draftcontexttools)

    def GetClassName(self): 
        return "Gui::PythonWorkbench"

FreeCADGui.addWorkbench(ArchWorkbench)

# File format pref pages are independent and can be loaded at startup
import Arch_rc
FreeCADGui.addPreferencePage(":/ui/preferences-ifc.ui","Import-Export")
FreeCADGui.addPreferencePage(":/ui/preferences-dae.ui","Import-Export")

FreeCAD.__unit_test__ += [ "TestArch" ]


