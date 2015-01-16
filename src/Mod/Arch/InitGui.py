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
    Icon = """
        /* XPM */
        static char * arch_xpm[] = {
        "16 16 17 1",
        " 	c None",
        ".	c #373936",
        "+	c #464845",
        "@	c #545553",
        "#	c #626461",
        "$	c #6B6D6A",
        "%	c #727471",
        "&	c #7E807D",
        "*	c #8A8C89",
        "=	c #949693",
        "-	c #A1A3A0",
        ";	c #ADAFAC",
        ">	c #BEC1BD",
        ",	c #C9CBC8",
        "'	c #D9DCD8",
        ")	c #E4E6E3",
        "!	c #FDFFFC",
        "                ",
        "                ",
        "       &        ",
        "      >)'-%     ",
        "    #,))))),@   ",
        "   >%*-))))*#   ",
        " $')>!)**>%*%   ",
        "@=')>!!!!$==#   ",
        "=!=**;'!!&=$++  ",
        "=!!!)*@&-%#@#&-.",
        " ,!!!!#>&#=,'=%@",
        "   ;)!#!!!-*$&=@",
        "     *@!!!!!$=* ",
        "        =>!!$&  ",
        "           -+   ",
        "                "};"""

    MenuText = "Arch"
    ToolTip = "Architecture workbench"

    def Initialize(self):
        import DraftTools,DraftGui,Arch_rc,Arch,Draft_rc
        from DraftTools import translate

        # arch tools
        self.archtools = ["Arch_Wall","Arch_Structure","Arch_Rebar",
                     "Arch_Floor","Arch_Building","Arch_Site",
                     "Arch_Window","Arch_Roof","Arch_Axis",
                     "Arch_SectionPlane","Arch_Space","Arch_Stairs",
                     "Arch_Panel","Arch_Equipment",
                     "Arch_Frame","Arch_CutPlane","Arch_Add","Arch_Remove","Arch_Survey"]
        self.utilities = ["Arch_SplitMesh","Arch_MeshToShape",
                     "Arch_SelectNonSolidMeshes","Arch_RemoveShape",
                     "Arch_CloseHoles","Arch_MergeWalls","Arch_Check",
                     "Arch_IfcExplorer","Arch_ToggleIfcBrepFlag","Arch_3Views"]

        # draft tools
        self.drafttools = ["Draft_Line","Draft_Wire","Draft_Circle","Draft_Arc","Draft_Ellipse",
                        "Draft_Polygon","Draft_Rectangle", "Draft_Text",
                        "Draft_Dimension", "Draft_BSpline","Draft_Point",
                        "Draft_Facebinder","Draft_BezCurve"]
        self.draftmodtools = ["Draft_Move","Draft_Rotate","Draft_Offset",
                        "Draft_Trimex", "Draft_Upgrade", "Draft_Downgrade", "Draft_Scale",
                        "Draft_Shape2DView","Draft_Draft2Sketch","Draft_Array",
                        "Draft_Clone"]
        self.draftextratools = ["Draft_WireToBSpline","Draft_AddPoint","Draft_DelPoint","Draft_ShapeString","Draft_PathArray"]
        self.draftcontexttools = ["Draft_ApplyStyle","Draft_ToggleDisplayMode","Draft_AddToGroup",
                            "Draft_SelectGroup","Draft_SelectPlane",
                            "Draft_ShowSnapBar","Draft_ToggleGrid","Draft_UndoLine",
                            "Draft_FinishLine","Draft_CloseLine"]
        self.draftutils = ["Draft_VisGroup","Draft_Heal","Draft_FlipDimension",
                           "Draft_ToggleConstructionMode","Draft_ToggleContinueMode","Draft_Edit"]
        self.snapList = ['Draft_Snap_Lock','Draft_Snap_Midpoint','Draft_Snap_Perpendicular',
                         'Draft_Snap_Grid','Draft_Snap_Intersection','Draft_Snap_Parallel',
                         'Draft_Snap_Endpoint','Draft_Snap_Angle','Draft_Snap_Center',
                         'Draft_Snap_Extension','Draft_Snap_Near','Draft_Snap_Ortho',
                         'Draft_Snap_Dimensions','Draft_Snap_WorkingPlane']

        def QT_TRANSLATE_NOOP(scope, text): return text
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench","Arch tools"),self.archtools)
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench","Draft tools"),self.drafttools)
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench","Draft mod tools"),self.draftmodtools)
        self.appendMenu([translate("arch","&Architecture"),translate("arch","Utilities")],self.utilities)
        self.appendMenu(translate("arch","&Architecture"),self.archtools)
        self.appendMenu(translate("arch","&Draft"),self.drafttools+self.draftmodtools+self.draftextratools)
        self.appendMenu([translate("arch","&Draft"),translate("arch","Utilities")],self.draftutils+self.draftcontexttools)
        self.appendMenu([translate("arch","&Draft"),translate("arch","Snapping")],self.snapList)
        FreeCADGui.addIconPath(":/icons")
        FreeCADGui.addLanguagePath(":/translations")
        FreeCADGui.addPreferencePage(":/ui/archprefs-base.ui","Arch")
        FreeCADGui.addPreferencePage(":/ui/archprefs-defaults.ui","Arch")
        FreeCADGui.addPreferencePage(":/ui/archprefs-import.ui","Import-Export")
        if hasattr(FreeCADGui,"draftToolBar"):
            if not hasattr(FreeCADGui.draftToolBar,"loadedPreferences"):
                FreeCADGui.addPreferencePage(":/ui/userprefs-base.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/userprefs-snap.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/userprefs-visual.ui","Draft")
                FreeCADGui.addPreferencePage(":/ui/userprefs-import1.ui","Import-Export")
                FreeCADGui.addPreferencePage(":/ui/userprefs-import2.ui","Import-Export")
                FreeCADGui.draftToolBar.loadedPreferences = True
        Log ('Loading Arch module... done\n')

    def Activated(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.Activated()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.show()
        Msg("Arch workbench activated\n")
                
    def Deactivated(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.Deactivated()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.hide()
        Msg("Arch workbench deactivated\n")

    def ContextMenu(self, recipient):
        self.appendContextMenu("Utilities",self.draftcontexttools)

    def GetClassName(self): 
        return "Gui::PythonWorkbench"

FreeCADGui.addWorkbench(ArchWorkbench)

# add import/export types
FreeCAD.addImportType("Industry Foundation Classes (*.ifc)","importIFC")
FreeCAD.addExportType("Industry Foundation Classes (*.ifc)","importIFC")
FreeCAD.addExportType("Wavefront OBJ - Arch module (*.obj)","importOBJ")
FreeCAD.addExportType("WebGL file (*.html)","importWebGL")
FreeCAD.addImportType("Collada (*.dae)","importDAE")
FreeCAD.addExportType("Collada (*.dae)","importDAE")


