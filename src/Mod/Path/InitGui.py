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
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Path/Resources/icons/PathWorkbench.svg"
        self.__class__.MenuText = "Path"
        self.__class__.ToolTip = "Path workbench"

    def Initialize(self):
        # load the builtin modules
        import Path
        import PathGui
        from PySide import QtGui
        FreeCADGui.addLanguagePath(":/translations")
        FreeCADGui.addIconPath(":/icons")
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
        def QT_TRANSLATE_NOOP(scope, text): 
            return text
        def translate(context,text):
            return QtGui.QApplication.translate(context, text, None, QtGui.QApplication.UnicodeUTF8).encode("utf8")
        self.appendToolbar(translate("Path","Project Setup"),projcmdlist)
        self.appendToolbar(translate("Path","Partial Commands"),prepcmdlist)
        self.appendToolbar(translate("Path","New Operations"),opcmdlist)
        self.appendToolbar(translate("Path","Path Modification"),modcmdlist)

        self.appendMenu([translate("Path","Path"),translate("Path","Project Setup")],projcmdlist)
        self.appendMenu([translate("Path","Path"),translate("Path","Partial Commands")],prepcmdlist)
        self.appendMenu([translate("Path","Path"),translate("Path","New Operations")],opcmdlist)
        self.appendMenu([translate("Path","Path"),translate("Path","Path Modification")],modcmdlist)
        
        # Add preferences pages
        import os
        FreeCADGui.addPreferencePage(FreeCAD.getHomePath()+os.sep+"Mod"+os.sep+"Path"+os.sep+"PathScripts"+os.sep+"DlgSettingsPath.ui","Path")
        
        Log ('Loading Path workbench... done\n')

    def GetClassName(self):
        return "Gui::PythonWorkbench"
        
    def Activated(self):
        # update the translation engine
        FreeCADGui.updateLocale()
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
