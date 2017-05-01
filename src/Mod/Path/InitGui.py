# ***************************************************************************
# *   (c) Yorik van Havre (yorik@uncreated.net) 2014                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/

class PathWorkbench (Workbench):
    "Path workbench"

    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Path/Resources/icons/PathWorkbench.svg"
        self.__class__.MenuText = "Path"
        self.__class__.ToolTip = "Path workbench"

    def Initialize(self):
        # Add preferences pages - before loading PathGui to properly order pages of Path group
        from PathScripts import PathPreferencesPathJob, PathPreferencesPathDressup
        FreeCADGui.addPreferencePage(PathPreferencesPathJob.JobPreferencesPage, "Path")
        FreeCADGui.addPreferencePage(PathPreferencesPathDressup.DressupPreferencesPage, "Path")

        # load the builtin modules
        import Path
        import PathGui
        from PySide import QtGui
        FreeCADGui.addLanguagePath(":/translations")
        FreeCADGui.addIconPath(":/icons")
        # load python modules
        from PathScripts import PathArray
        from PathScripts import PathComment
        from PathScripts import PathCompoundExtended
        from PathScripts import PathContour
        from PathScripts import PathCopy
        from PathScripts import PathCustom
        from PathScripts import PathDressup
        from PathScripts import PathDressupDogbone
        from PathScripts import PathDressupDragknife
        from PathScripts import PathDressupHoldingTags
        from PathScripts import PathDrilling
        from PathScripts import PathEngrave
        from PathScripts import PathFacePocket
        from PathScripts import PathFaceProfile
        from PathScripts import PathFixture
        from PathScripts import PathFromShape
        from PathScripts import PathHelix
        from PathScripts import PathHop
        from PathScripts import PathInspect
        from PathScripts import PathJob
        from PathScripts import PathLoadTool
        from PathScripts import PathMillFace
        from PathScripts import PathPlane
        from PathScripts import PathPocket
        from PathScripts import PathPost
        from PathScripts import PathProfile
        from PathScripts import PathProfileEdges
        from PathScripts import PathSanity
        from PathScripts import PathSimpleCopy
        from PathScripts import PathStock
        from PathScripts import PathStop
        from PathScripts import PathSurface
        from PathScripts import PathToolLenOffset
        from PathScripts import PathToolLibraryManager
        import PathCommands

        # build commands list
        projcmdlist = ["Path_Job", "Path_Post", "Path_Inspect", "Path_Sanity"]
        toolcmdlist = ["Path_ToolLibraryEdit"]
        prepcmdlist = ["Path_Plane", "Path_Fixture", "Path_ToolLenOffset", "Path_Comment", "Path_Stop", "Path_FaceProfile", "Path_FacePocket", "Path_Custom", "Path_Shape"]
        twodopcmdlist = ["Path_Contour", "Path_Profile", "Path_Profile_Edges", "Path_Pocket", "Path_Drilling", "Path_Engrave", "Path_MillFace", "Path_Helix"]
        threedopcmdlist = ["Path_Surfacing"]
        modcmdlist = ["Path_Copy", "Path_CompoundExtended", "Path_Array", "Path_SimpleCopy" ]
        dressupcmdlist = ["PathDressup_Dogbone", "PathDressup_DragKnife", "PathDressup_HoldingTags"]
        extracmdlist = ["Path_SelectLoop", "Path_Shape", "Path_Area", "Path_Area_Workplane", "Path_Stock"]
        #modcmdmore = ["Path_Hop",]
        #remotecmdlist = ["Path_Remote"]

        # Add commands to menu and toolbar
        def QT_TRANSLATE_NOOP(scope, text):
            return text

        def translate(context, text):
            return QtGui.QApplication.translate(context, text, None, QtGui.QApplication.UnicodeUTF8).encode("utf8")
        self.appendToolbar(QT_TRANSLATE_NOOP("Path", "Project Setup"), projcmdlist)
        self.appendToolbar(QT_TRANSLATE_NOOP("Path", "Tool Commands"), toolcmdlist)
        #self.appendToolbar(QT_TRANSLATE_NOOP("Path", "Partial Commands"), prepcmdlist)
        self.appendToolbar(QT_TRANSLATE_NOOP("Path", "New Operations"), twodopcmdlist+threedopcmdlist)
        self.appendToolbar(QT_TRANSLATE_NOOP("Path", "Path Modification"), modcmdlist)
        self.appendToolbar(QT_TRANSLATE_NOOP("Path", "Helpful Tools"), extracmdlist)

        self.appendMenu([QT_TRANSLATE_NOOP("Path", "&Path")], projcmdlist +["Separator"] + toolcmdlist +["Separator"] +twodopcmdlist +["Separator"] +threedopcmdlist +["Separator"])
        #self.appendMenu([QT_TRANSLATE_NOOP("Path", "Path"), QT_TRANSLATE_NOOP(
        #    "Path", "Tools")], toolcmdlist)
        self.appendMenu([QT_TRANSLATE_NOOP("Path", "&Path"), QT_TRANSLATE_NOOP(
            "Path", "Path Dressup")], dressupcmdlist)
        self.appendMenu([QT_TRANSLATE_NOOP("Path", "&Path"), QT_TRANSLATE_NOOP(
            "Path", "Partial Commands")], prepcmdlist)
        #self.appendMenu([QT_TRANSLATE_NOOP("Path", "Path"), QT_TRANSLATE_NOOP(
        #    "Path", "New Operations")], opcmdlist)
        self.appendMenu([QT_TRANSLATE_NOOP("Path", "&Path"), QT_TRANSLATE_NOOP(
            "Path", "Path Modification")], modcmdlist)
        #self.appendMenu([QT_TRANSLATE_NOOP("Path", "Path"), QT_TRANSLATE_NOOP(
        #    "Path", "Path Modification")], modcmdmore)
        # self.appendMenu([QT_TRANSLATE_NOOP("Path", "Path"), QT_TRANSLATE_NOOP(
        #     "Path", "Remote Operations")], remotecmdlist)
        self.appendMenu([QT_TRANSLATE_NOOP("Path", "&Path")], extracmdlist)

        Log('Loading Path workbench... done\n')

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
                self.appendContextMenu("", ["Path_Inspect"])
                if "Profile" or "Contour" in FreeCADGui.Selection.getSelection()[0].Name:
                    #self.appendContextMenu("", ["Add_Tag"])
                    self.appendContextMenu("", ["Set_StartPoint"])
                    self.appendContextMenu("", ["Set_EndPoint"])
                if "Remote" in FreeCADGui.Selection.getSelection()[0].Name:
                    self.appendContextMenu("", ["Refresh_Path"])

Gui.addWorkbench(PathWorkbench())

FreeCAD.addImportType(
    "GCode (*.nc *.gc *.ncc *.ngc *.cnc *.tap *.gcode)", "PathGui")
FreeCAD.addExportType(
    "GCode (*.nc *.gc *.ncc *.ngc *.cnc *.tap *.gcode)", "PathGui")
