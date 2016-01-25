#***************************************************************************
#*   (c) Yorik van Havre (yorik@uncreated.net) 2014                        *
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


class PathWorkbench ( Workbench ):
    "Path workbench"
    Icon = """
            /* XPM */
            static char * Path_xpm[] = {
            "16 16 9 1",
            "   c None",
            ".  c #262623",
            "+  c #452F16",
            "@  c #525451",
            "#  c #7E5629",
            "$  c #838582",
            "%  c #BE823B",
            "&  c #989A97",
            "*  c #CFD1CE",
            "  .@@@@@@@@@@.  ",
            "  $**********$  ",
            "  @$$$&&&&$$$@  ",
            "    .$&&&&$.    ",
            "    @******.    ",
            "    @******.    ",
            "    ...@@...    ",
            "     .&&@.      ",
            "     .@. .      ",
            "       .&&.     ",
            "     .$*$.      ",
            "     .$. .      ",
            "+###+  .@&.+###+",
            "+%%%+ .$$. +%%%+",
            "+%%%%#.. .#%%%%+",
            ".++++++..++++++."};
            """
    MenuText = "Path"
    ToolTip = "Path workbench"

    def Initialize(self):
        # load the builtin modules
        import Path
        import PathGui
        # load python modules
        from PathScripts import PathProfile
        from PathScripts import PathPocket
        from PathScripts import PathDrilling
        from PathScripts import PathDressup
        from PathScripts import PathHop
        from PathScripts import PathCopy
        from PathScripts import PathFixture
        from PathScripts import PathCompoundExtended
        from PathScripts import PathProject
        from PathScripts import PathToolTableEdit
        from PathScripts import PathStock
        from PathScripts import PathPlane
        from PathScripts import PathPost
        from PathScripts import PathToolLenOffset
        from PathScripts import PathLoadTool
        from PathScripts import PathComment
        from PathScripts import PathStop
        from PathScripts import PathMachine
        from PathScripts import PathFromShape
        from PathScripts import PathKurve
        from PathScripts import PathArray
        from PathScripts import PathFaceProfile
        from PathScripts import PathFacePocket
        from PathScripts import PathCustom
        from PathScripts import PathInspect
        from PathScripts import PathSimpleCopy

        # build commands list
        projcmdlist = ["Path_Project", "Path_ToolTableEdit","Path_Post","Path_Inspect"]
        prepcmdlist = ["Path_Plane","Path_Fixture","Path_LoadTool","Path_ToolLenOffset","Path_Comment","Path_Stop",
                       "Path_FaceProfile","Path_FacePocket","Path_Custom","Path_FromShape"]
        opcmdlist = ["Path_Profile","Path_Kurve","Path_Pocket","Path_Drilling"]
        modcmdlist = ["Path_Copy","Path_CompoundExtended","Path_Dressup","Path_Hop","Path_Array","Path_SimpleCopy"]


        # Add commands to menu and toolbar
        def QT_TRANSLATE_NOOP(scope, text): return text
        self.appendToolbar(QT_TRANSLATE_NOOP("PathWorkbench","Commands for setting up Project"),projcmdlist)
        self.appendToolbar(QT_TRANSLATE_NOOP("PathWorkbench","Partial Commands"),prepcmdlist)
        self.appendToolbar(QT_TRANSLATE_NOOP("PathWorkbench","Operations"),opcmdlist)
        self.appendToolbar(QT_TRANSLATE_NOOP("PathWorkbench","Commands for grouping,copying, and organizing operations"),modcmdlist)

        self.appendMenu([QT_TRANSLATE_NOOP("PathWorkbench","Path"),QT_TRANSLATE_NOOP("Path","Project Setup")],projcmdlist)
        self.appendMenu([QT_TRANSLATE_NOOP("PathWorkbench","Path"),QT_TRANSLATE_NOOP("Path","Partial Commands")],prepcmdlist)
        self.appendMenu([QT_TRANSLATE_NOOP("PathWorkbench","Path"),QT_TRANSLATE_NOOP("Path","New Operations")],opcmdlist)
        self.appendMenu([QT_TRANSLATE_NOOP("PathWorkbench","Path"),QT_TRANSLATE_NOOP("Path","Path Modification")],modcmdlist)
        
        # Add preferences pages
        import os
        FreeCADGui.addPreferencePage(FreeCAD.getHomePath()+os.sep+"Mod"+os.sep+"Path"+os.sep+"PathScripts"+os.sep+"DlgSettingsPath.ui","Path")
        
        Log ('Loading Path workbench... done\n')

    def GetClassName(self):
        return "Gui::PythonWorkbench"
        
    def Activated(self):
        Msg("Path workbench activated\n")
                
    def Deactivated(self):
        Msg("Path workbench deactivated\n")
        
    def ContextMenu(self, recipient):
        if len(FreeCADGui.Selection.getSelection()) == 1:
            if FreeCADGui.Selection.getSelection()[0].isDerivedFrom("Path::Feature"):
                self.appendContextMenu("",["Path_Inspect"])


Gui.addWorkbench(PathWorkbench())

FreeCAD.addImportType("GCode (*.nc *.gc *.ncc *.ngc *.cnc *.tap *.gcode)","PathGui")
FreeCAD.addExportType("GCode (*.nc *.gc *.ncc *.ngc *.cnc *.tap *.gcode)","PathGui")
