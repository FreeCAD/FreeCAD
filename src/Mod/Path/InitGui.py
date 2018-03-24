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

        # Check enablement of experimental features
        from PathScripts.PathPreferences import PathPreferences

        # load the builtin modules
        import Path
        import PathScripts
        import PathGui
        from PySide import QtGui
        FreeCADGui.addLanguagePath(":/translations")
        FreeCADGui.addIconPath(":/icons")
        # load python modules
        from PathScripts import PathArray
        from PathScripts import PathComment
        from PathScripts import PathCustom
        from PathScripts import PathDressupDogbone
        from PathScripts import PathDressupDragknife
        from PathScripts import PathDressupRampEntry
        from PathScripts import PathDressupTagGui
        from PathScripts import PathDressupLeadInOut
        from PathScripts import PathDrillingGui
        from PathScripts import PathEngraveGui
        from PathScripts import PathFixture
        from PathScripts import PathHelixGui
        from PathScripts import PathHop
        from PathScripts import PathInspect
        from PathScripts import PathJobCmd
        from PathScripts import PathMillFaceGui
        from PathScripts import PathPocketGui
        from PathScripts import PathPocketShapeGui
        from PathScripts import PathPost
        from PathScripts import PathProfileContourGui
        from PathScripts import PathProfileEdgesGui
        from PathScripts import PathProfileFacesGui
        from PathScripts import PathSanity
        from PathScripts import PathSimpleCopy
        from PathScripts import PathStop
        from PathScripts import PathSurfaceGui
        from PathScripts import PathToolController
        from PathScripts import PathToolLibraryManager
        from PathScripts import PathSimulatorGui
        import PathCommands

        # build commands list
        projcmdlist = ["Path_Job", "Path_Post"]
        toolcmdlist = ["Path_Inspect", "Path_Simulator", "Path_ToolLibraryEdit", "Path_SelectLoop"]
        prepcmdlist = ["Path_Fixture", "Path_Comment", "Path_Stop", "Path_Custom"]
        twodopcmdlist = ["Path_Contour", "Path_Profile_Faces", "Path_Profile_Edges", "Path_Pocket_Shape", "Path_Drilling", "Path_Engrave", "Path_MillFace", "Path_Helix"]
        threedopcmdlist = ["Path_Pocket_3D"]
        modcmdlist = ["Path_OperationCopy", "Path_Array", "Path_SimpleCopy" ]
        dressupcmdlist = ["Path_DressupDogbone", "Path_DressupDragKnife", "Path_DressupLeadInOut", "Path_DressupRampEntry", "Path_DressupTag"]
        extracmdlist = []
        #modcmdmore = ["Path_Hop",]
        #remotecmdlist = ["Path_Remote"]

        # Add commands to menu and toolbar
        def QT_TRANSLATE_NOOP(scope, text):
            return text

        class ThreeDCommandGroup:
            def GetCommands(self):
                return tuple(threedopcmdlist)

            def GetResources(self):
                return { 'MenuText': QT_TRANSLATE_NOOP("Path",'3D Operations'),
                         'ToolTip': QT_TRANSLATE_NOOP("Path",'3D Operations')
                       }
            def IsActive(self):
                if FreeCAD.ActiveDocument is not None:
                    for o in FreeCAD.ActiveDocument.Objects:
                        if o.Name[:3] == "Job":
                                return True
                return False

        if PathPreferences.experimentalFeaturesEnabled():
            projcmdlist.append("Path_Sanity")
            prepcmdlist.append("Path_Shape")
            threedopcmdlist.append("Path_Surface")
            extracmdlist.extend(["Path_Area", "Path_Area_Workplane"])
            FreeCADGui.addCommand('Path_3dTools', ThreeDCommandGroup())
            threedcmdgroup = ['Path_3dTools']
        else:
            threedcmdgroup = threedopcmdlist

        self.appendToolbar(QT_TRANSLATE_NOOP("Path", "Project Setup"), projcmdlist)
        self.appendToolbar(QT_TRANSLATE_NOOP("Path", "Tool Commands"), toolcmdlist)
        self.appendToolbar(QT_TRANSLATE_NOOP("Path", "New Operations"), twodopcmdlist+threedcmdgroup)
        self.appendToolbar(QT_TRANSLATE_NOOP("Path", "Path Modification"), modcmdlist)
        if extracmdlist:
            self.appendToolbar(QT_TRANSLATE_NOOP("Path", "Helpful Tools"), extracmdlist)

        self.appendMenu([QT_TRANSLATE_NOOP("Path", "&Path")], projcmdlist +["Path_ExportTemplate", "Separator"] + toolcmdlist +["Separator"] +twodopcmdlist +["Separator"] +threedopcmdlist +["Separator"])
        self.appendMenu([QT_TRANSLATE_NOOP("Path", "&Path"), QT_TRANSLATE_NOOP(
            "Path", "Path Dressup")], dressupcmdlist)
        self.appendMenu([QT_TRANSLATE_NOOP("Path", "&Path"), QT_TRANSLATE_NOOP(
            "Path", "Partial Commands")], prepcmdlist)
        self.appendMenu([QT_TRANSLATE_NOOP("Path", "&Path"), QT_TRANSLATE_NOOP(
            "Path", "Path Modification")], modcmdlist)
        if extracmdlist:
            self.appendMenu([QT_TRANSLATE_NOOP("Path", "&Path")], extracmdlist)

        self.dressupcmds = dressupcmdlist

        Path.Area.setDefaultParams(Accuracy = PathPreferences.defaultLibAreaCurveAccuracy())

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
        import PathScripts
        if len(FreeCADGui.Selection.getSelection()) == 1:
            obj = FreeCADGui.Selection.getSelection()[0]
            if obj.isDerivedFrom("Path::Feature"):
                self.appendContextMenu("", "Separator")
                self.appendContextMenu("", ["Path_Inspect"])
                selectedName = obj.Name
                if "Remote" in selectedName:
                    self.appendContextMenu("", ["Refresh_Path"])
                if "Job" in selectedName:
                    self.appendContextMenu("", ["Path_ExportTemplate"])
            if isinstance (obj.Proxy, PathScripts.PathOp.ObjectOp):
                self.appendContextMenu("", ["Path_OperationCopy"])
            if obj.isDerivedFrom("Path::Feature"):
                if "Profile" in selectedName or "Contour" in selectedName or "Dressup" in selectedName:
                    self.appendContextMenu("", "Separator")
                    #self.appendContextMenu("", ["Set_StartPoint"])
                    #self.appendContextMenu("", ["Set_EndPoint"])
                    for cmd in self.dressupcmds:
                        self.appendContextMenu("", [cmd])

Gui.addWorkbench(PathWorkbench())

FreeCAD.addImportType(
    "GCode (*.nc *.gc *.ncc *.ngc *.cnc *.tap *.gcode)", "PathGui")
FreeCAD.addExportType(
    "GCode (*.nc *.gc *.ncc *.ngc *.cnc *.tap *.gcode)", "PathGui")

