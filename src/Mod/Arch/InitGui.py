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
        "16 16 9 1",
        " 	c None",
        ".	c #543016",
        "+	c #6D2F08",
        "@	c #954109",
        "#	c #874C24",
        "$	c #AE6331",
        "%	c #C86423",
        "&	c #FD7C26",
        "*	c #F5924F",
        "                ",
        "                ",
        "       #        ",
        "      ***$#     ",
        "    .*******.   ",
        "   *##$****#+   ",
        " #**%&&##$#@@   ",
        ".$**%&&&&+@@+   ",
        "@&@#$$%&&@@+..  ",
        "@&&&%#.#$#+..#$.",
        " %&&&&+%#.$**$@+",
        "   @%&+&&&$##@@+",
        "     @.&&&&&@@@ ",
        "        @%&&@@  ",
        "           @+   ",
        "                "};
        """
    MenuText = "Arch"
    ToolTip = "Architecture workbench"
	
    def Initialize(self):
        import DraftTools,DraftGui,Arch_rc,Arch,Draft_rc

        # arch tools
        self.archtools = ["Arch_Wall","Arch_Structure",
                     "Arch_Floor","Arch_Building","Arch_Site",
                     "Arch_Window","Arch_Roof","Arch_Axis",
                     "Arch_SectionPlane","Arch_Add","Arch_Remove"]
        self.meshtools = ["Arch_SplitMesh","Arch_MeshToShape",
                     "Arch_SelectNonSolidMeshes","Arch_RemoveShape",
                     "Arch_CloseHoles"]
        self.calctools = ["Arch_Check"]

        # draft tools
        self.drafttools = ["Draft_Line","Draft_Wire","Draft_Circle","Draft_Arc",
                        "Draft_Polygon","Draft_Rectangle", "Draft_Text",
                        "Draft_Dimension", "Draft_BSpline","Draft_Point"]
        self.draftmodtools = ["Draft_Move","Draft_Rotate","Draft_Offset",
                        "Draft_Trimex", "Draft_Upgrade", "Draft_Downgrade", "Draft_Scale",
                        "Draft_Drawing","Draft_Edit","Draft_WireToBSpline","Draft_AddPoint",
                        "Draft_DelPoint","Draft_Shape2DView","Draft_Draft2Sketch","Draft_Array",
                        "Draft_Clone"]
        self.draftcontexttools = ["Draft_ApplyStyle","Draft_ToggleDisplayMode","Draft_AddToGroup",
                            "Draft_SelectGroup","Draft_SelectPlane","Draft_ToggleSnap",
                            "Draft_ShowSnapBar","Draft_ToggleGrid","Draft_UndoLine",
                            "Draft_FinishLine","Draft_CloseLine"]

        self.appendToolbar(str(DraftTools.translate("arch","Arch tools")),self.archtools)
        self.appendToolbar(str(DraftTools.translate("arch","Draft tools")),self.drafttools)
        self.appendToolbar(str(DraftTools.translate("arch","Draft mod tools")),self.draftmodtools)
        self.appendMenu([str(DraftTools.translate("arch","&Architecture")),str(DraftTools.translate("arch","Conversion Tools"))],self.meshtools)
        self.appendMenu([str(DraftTools.translate("arch","&Architecture")),str(DraftTools.translate("arch","Calculation Tools"))],self.calctools)
        self.appendMenu(str(DraftTools.translate("arch","&Architecture")),self.archtools)
        self.appendMenu(str(DraftTools.translate("arch","&Draft")),self.drafttools+self.draftmodtools)
        self.appendMenu([str(DraftTools.translate("arch","&Draft")),str(DraftTools.translate("arch","Context Tools"))],self.draftcontexttools)
        FreeCADGui.addIconPath(":/icons")
        FreeCADGui.addLanguagePath(":/translations")
        FreeCADGui.addPreferencePage(":/ui/archprefs-base.ui","Arch")
        if not hasattr(FreeCADGui.draftToolBar,"loadedPreferences"):
            FreeCADGui.addPreferencePage(":/ui/userprefs-base.ui","Draft")
            FreeCADGui.addPreferencePage(":/ui/userprefs-import.ui","Draft")
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
        Msg("Arch workbench deactivated\n")
                
    def ContextMenu(self, recipient):
        self.appendContextMenu("Draft context tools",self.draftcontexttools)

    def GetClassName(self): 
        return "Gui::PythonWorkbench"

FreeCADGui.addWorkbench(ArchWorkbench)
FreeCAD.addImportType("Industry Foundation Classes (*.ifc)","importIFC")
FreeCAD.addExportType("Wavefront OBJ - Arch module (*.obj)","importOBJ")
# check for pycollada
try:
    import collada
except:
    from DraftTools import translate
    FreeCAD.Console.PrintMessage(str(translate("arch","pycollada not found, collada support will be disabled.\n")))
else:
    FreeCAD.addImportType("Collada (*.dae)","importDAE")
    FreeCAD.addExportType("Collada (*.dae)","importDAE")

