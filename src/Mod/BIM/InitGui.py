# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""The BIM workbench"""

import os
import FreeCAD
import FreeCADGui
import Arch_rc

class BIMWorkbench(Workbench):

    def __init__(self):

        def QT_TRANSLATE_NOOP(context, text):
            return text

        bdir = os.path.join(FreeCAD.getResourceDir(), "Mod", "BIM")
        tt = QT_TRANSLATE_NOOP("BIM","The BIM workbench is used to model buildings")
        self.__class__.MenuText = QT_TRANSLATE_NOOP("BIM", "BIM")
        self.__class__.ToolTip =  tt
        self.__class__.Icon = os.path.join(bdir,"Resources", "icons",
                                                "BIMWorkbench.svg")

    def Initialize(self):

        # add translations and icon paths
        FreeCADGui.addIconPath(":/icons")
        FreeCADGui.addLanguagePath(":/translations")

        # Create menus and toolbars
        self.createTools()

        # Load Arch & Draft preference pages
        self.loadPreferences()

        Log("Loading BIM module... done\n")
        FreeCADGui.updateLocale()


    def createTools(self):

        "Create tolbars and menus"

        def QT_TRANSLATE_NOOP(context, text):
            return text

        # Import Draft & BIM commands
        import DraftTools
        import bimcommands
        from nativeifc import ifc_commands

        # build menus and toolbars
        self.draftingtools = [
            "BIM_Sketch",
            "Draft_Line",
            "Draft_Wire",
            "Draft_Circle",
            "Draft_Arc",
            "Draft_Arc_3Points",
            "Draft_Fillet",
            "Draft_Ellipse",
            "Draft_Polygon",
            "Draft_Rectangle",
            "Draft_BSpline",
            "Draft_BezCurve",
            "Draft_CubicBezCurve",
            "Draft_Point",
        ]

        self.annotationtools = [
            "BIM_Text",
            "Draft_ShapeString",
            "BIM_DimensionAligned",
            "BIM_DimensionHorizontal",
            "BIM_DimensionVertical",
            "BIM_Leader",
            "Draft_Label",
            "Arch_Axis",
            "Arch_AxisSystem",
            "Arch_Grid",
            "Arch_SectionPlane",
            "Draft_Hatch",
            "BIM_TDPage",
            "BIM_TDView",
            "BIM_Shape2DView",
        ]

        self.bimtools = [
            "BIM_Project",
            "Arch_Site",
            "Arch_Building",
            "Arch_Level",
            "Arch_Space",
            "Separator",
            "Arch_Wall",
            "Arch_CurtainWall",
            "BIM_Column",
            "BIM_Beam",
            "BIM_Slab",
            "BIM_Door",
            "Arch_Window",
            "Arch_Pipe",
            "Arch_PipeConnector",
            "Arch_Stairs",
            "Arch_Roof",
            "Arch_Panel",
            "Arch_Frame",
            "Arch_Fence",
            "Arch_Truss",
            "Arch_Equipment",
            "Arch_Rebar",
        ]

        self.generictools = [
            "Arch_Profile",
            "BIM_Box",
            "BIM_Builder",
            "Draft_Facebinder",
            "BIM_Library",
            "Arch_Component",
            "Arch_Reference",
        ]

        self.modify_gen = [
            "Draft_Move",
            "BIM_Copy",
            "Draft_Rotate",
            "BIM_Clone",
            "BIM_SimpleCopy",
            "BIM_Compound",
        ]
        self.modify_2d = [
            "Draft_Offset",
            "BIM_Offset2D",
            "Draft_Trimex",
            "Draft_Join",
            "Draft_Split",
            "Draft_Scale",
            "Draft_Stretch",
            "Draft_Draft2Sketch",
        ]
        self.modify_obj = [
            "Draft_Upgrade",
            "Draft_Downgrade",
            "Arch_Add",
            "Arch_Remove",
        ]
        self.modify_3d = [
            "Draft_OrthoArray",
            "Draft_PathArray",
            "Draft_PolarArray",
            "Draft_PointArray",
            "Arch_CutPlane",
            "Draft_Mirror",
            "BIM_Extrude",
            "BIM_Cut",
            "BIM_Fuse",
            "BIM_Common",
        ]

        sep = ["Separator"]
        self.modify = self.modify_gen + sep + self.modify_2d + sep + self.modify_obj + sep + self.modify_3d

        self.manage = [
            "BIM_Setup",
            "BIM_Views",
            "BIM_ProjectManager",
            "BIM_Windows",
            "BIM_IfcElements",
            "BIM_IfcQuantities",
            "BIM_IfcProperties",
            "BIM_Classification",
            "BIM_Layers",
            "BIM_Material",
            "Arch_Schedule",
            "BIM_Preflight",
            "Draft_AnnotationStyleEditor",
        ]

        self.utils = [
            "BIM_TogglePanels",
            "BIM_Trash",
            "BIM_WPView",
            "Draft_SelectGroup",
            "Draft_Slope",
            "Draft_WorkingPlaneProxy",
            "Draft_AddConstruction",
            "Arch_SplitMesh",
            "Arch_MeshToShape",
            "Arch_SelectNonSolidMeshes",
            "Arch_RemoveShape",
            "Arch_CloseHoles",
            "Arch_MergeWalls",
            "Arch_Check",
            "Arch_ToggleIfcBrepFlag",
            "Arch_ToggleSubs",
            "Arch_Survey",
            "BIM_Diff",
            "BIM_IfcExplorer",
            "Arch_IfcSpreadsheet",
            "BIM_ImagePlane",
            "BIM_Unclone",
            "BIM_Rewire",
            "BIM_Glue",
            "BIM_Reextrude",
            "Arch_PanelTools",
            "Arch_StructureTools",
        ]

        nudge = [
            "BIM_Nudge_Switch",
            "BIM_Nudge_Up",
            "BIM_Nudge_Down",
            "BIM_Nudge_Left",
            "BIM_Nudge_Right",
            "BIM_Nudge_RotateLeft",
            "BIM_Nudge_RotateRight",
            "BIM_Nudge_Extend",
            "BIM_Nudge_Shrink",
        ]

        # append BIM snaps

        from draftutils import init_tools
        self.snapbar = init_tools.get_draft_snap_commands()
        self.snapmenu = self.snapbar + [
            "BIM_SetWPTop",
            "BIM_SetWPFront",
            "BIM_SetWPSide",
        ]

        # create generic tools command

        class BIM_GenericTools:
            def __init__(self, tools):
                self.tools = tools
            def GetCommands(self):
                return self.tools
            def GetResources(self):
                t = QT_TRANSLATE_NOOP("BIM_GenericTools", "Generic 3D tools")
                return { "MenuText": t, "ToolTip": t, "Icon": "BIM_Box"}
            def IsActive(self):
                v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
                return v
        FreeCADGui.addCommand("BIM_GenericTools", BIM_GenericTools(self.generictools))
        self.bimtools.append("BIM_GenericTools")

        # load rebar tools (Reinforcement addon)

        try:
            import RebarTools
        except ImportError:
            pass
        else:
            # create popup group for Rebar tools
            class RebarGroupCommand:
                def GetCommands(self):
                    return tuple(["Arch_Rebar"] + RebarTools.RebarCommands)

                def GetResources(self):
                    return {
                        "MenuText": QT_TRANSLATE_NOOP(
                            "Arch_RebarTools", "Reinforcement tools"
                        ),
                        "ToolTip": QT_TRANSLATE_NOOP(
                            "Arch_RebarTools", "Reinforcement tools"
                        ),
                        "Icon": "Arch_Rebar",
                    }

                def IsActive(self):
                    v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
                    return v

            FreeCADGui.addCommand("Arch_RebarTools", RebarGroupCommand())
            self.bimtools[self.bimtools.index("Arch_Rebar")] = "Arch_RebarTools"
            RebarTools.load_translations()
            Log("Load Reinforcement Module...done\n")
            if hasattr(RebarTools, "updateLocale"):
                RebarTools.updateLocale()
            #self.rebar = RebarTools.RebarCommands + ["Arch_Rebar"]

        # load Reporting

        try:
            import report
        except ImportError:
            pass
        else:
            if "Report_Create" in Gui.listCommands():
                self.manage[self.manage.index("Arch_Schedule")] = "Report_Create"

        # load webtools

        try:
            import BIMServer, Git, Sketchfab
        except ImportError:
            pass
        else:
            self.utils.extend(
                [
                    "WebTools_Git",
                    "WebTools_BimServer",
                    "WebTools_Sketchfab",
                ]
            )

        # load flamingo

        try:
            import CommandsPolar, CommandsFrame, CommandsPipe
        except ImportError:
            flamingo = None
        else:
            flamingo = [
                "frameIt",
                "fillFrame",
                "insertPath",
                "insertSection",
                "FrameLineManager",
                "spinSect",
                "reverseBeam",
                "shiftBeam",
                "pivotBeam",
                "levelBeam",
                "alignEdge",
                "rotJoin",
                "alignFlange",
                "stretchBeam",
                "extend",
                "adjustFrameAngle",
                "insertPipe",
                "insertElbow",
                "insertReduct",
                "insertCap",
                "insertFlange",
                "insertUbolt",
                "insertPypeLine",
                "breakPipe",
                "mateEdges",
                "extend2intersection",
                "extend1intersection",
                "laydown",
                "raiseup",
            ]

        # load fasteners

        try:
            import FastenerBase, FastenersCmd
        except ImportError:
            fasteners = None
        else:
            fasteners = [
                c
                for c in FastenerBase.FSGetCommands("screws")
                if not isinstance(c, tuple)
            ]

        # load nativeifc tools

        ifctools = ifc_commands.get_commands()

        # create toolbars

        t1 = QT_TRANSLATE_NOOP("Workbench", "Drafting tools")
        t2 = QT_TRANSLATE_NOOP("Workbench", "Draft snap")
        t3 = QT_TRANSLATE_NOOP("Workbench", "3D/BIM tools")
        t4 = QT_TRANSLATE_NOOP("Workbench", "Annotation tools")
        t5 = QT_TRANSLATE_NOOP("Workbench", "2D modification tools")
        t6 = QT_TRANSLATE_NOOP("Workbench", "Manage tools")
        t7 = QT_TRANSLATE_NOOP("Workbench", "General modification tools")
        t8 = QT_TRANSLATE_NOOP("Workbench", "Object modification tools")
        t9 = QT_TRANSLATE_NOOP("Workbench", "3D modification tools")
        self.appendToolbar(t1, self.draftingtools)
        self.appendToolbar(t2, self.snapbar)
        self.appendToolbar(t3, self.bimtools)
        self.appendToolbar(t4, self.annotationtools)
        self.appendToolbar(t7, self.modify_gen)
        self.appendToolbar(t5, self.modify_2d)
        self.appendToolbar(t8, self.modify_obj)
        self.appendToolbar(t9, self.modify_3d)
        self.appendToolbar(t6, self.manage)

        # create menus

        t1 =  QT_TRANSLATE_NOOP("Workbench", "&2D Drafting")
        t2 =  QT_TRANSLATE_NOOP("Workbench", "&3D/BIM")
        t3 =  QT_TRANSLATE_NOOP("Workbench", "Reinforcement tools")
        t4 =  QT_TRANSLATE_NOOP("Workbench", "&Annotation")
        t5 =  QT_TRANSLATE_NOOP("Workbench", "&Snapping")
        t6 =  QT_TRANSLATE_NOOP("Workbench", "&Modify")
        t7 =  QT_TRANSLATE_NOOP("Workbench", "&Manage")
        #t8 =  QT_TRANSLATE_NOOP("Workbench", "&IFC")
        t9 =  QT_TRANSLATE_NOOP("Workbench", "&Flamingo")
        t10 = QT_TRANSLATE_NOOP("Workbench", "&Fasteners")
        t11 = QT_TRANSLATE_NOOP("Workbench", "&Utils")
        t12 = QT_TRANSLATE_NOOP("Workbench", "Nudge")

        #self.bimtools_menu = list(self.bimtools)
        #if "Arch_RebarTools" in self.bimtools_menu:
        #    self.bimtools_menu.remove("Arch_RebarTools")
        self.appendMenu(t1, self.draftingtools)
        self.appendMenu(t2, self.bimtools)
        #if self.rebar:
        #    self.appendMenu([t2, t3], self.rebar)
        self.appendMenu(t4, self.annotationtools)
        self.appendMenu(t5, self.snapmenu)
        self.appendMenu(t6, self.modify)
        self.appendMenu(t7, self.manage)
        #if ifctools:
        #    self.appendMenu(t8, ifctools)
        if flamingo:
            self.appendMenu(t9, flamingo)
        if fasteners:
            self.appendMenu(t10, fasteners)
        self.appendMenu(t11, self.utils + ifctools)
        self.appendMenu([t11, t12], nudge)

    def loadPreferences(self):

        """Set up preferences pages"""

        def QT_TRANSLATE_NOOP(context, text):
            return text

        t1 = QT_TRANSLATE_NOOP("QObject", "BIM")
        t2 = QT_TRANSLATE_NOOP("QObject", "Draft")
        FreeCADGui.addPreferencePage(":/ui/preferences-arch.ui", t1)
        FreeCADGui.addPreferencePage(":/ui/preferences-archdefaults.ui", t1)
        FreeCADGui.addPreferencePage(":/ui/preferencesNativeIFC.ui", t1)
        if hasattr(FreeCADGui, "draftToolBar"):
            if hasattr(FreeCADGui.draftToolBar, "loadedPreferences"):
                return
        from draftutils import params
        params._param_observer_start()
        FreeCADGui.addPreferencePage(":/ui/preferences-draft.ui", t2)
        FreeCADGui.addPreferencePage(":/ui/preferences-draftinterface.ui", t2)
        FreeCADGui.addPreferencePage(":/ui/preferences-draftsnap.ui", t2)
        FreeCADGui.addPreferencePage(":/ui/preferences-draftvisual.ui", t2)
        FreeCADGui.addPreferencePage(":/ui/preferences-drafttexts.ui", t2)
        FreeCADGui.draftToolBar.loadedPreferences = True

    def setupMultipleObjectSelection(self):

        import BimSelect

        if hasattr(FreeCADGui, "addDocumentObserver") and not hasattr(
            self, "BimSelectObserver"
        ):
            self.BimSelectObserver = BimSelect.Setup()
            FreeCADGui.addDocumentObserver(self.BimSelectObserver)

    def Activated(self):

        import WorkingPlane
        from draftutils import todo
        import BimStatus
        from nativeifc import ifc_observer
        from draftutils import grid_observer

        PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")

        if hasattr(FreeCADGui, "draftToolBar"):
            FreeCADGui.draftToolBar.Activated()
        if hasattr(FreeCADGui, "Snapper"):
            FreeCADGui.Snapper.show()
        WorkingPlane._view_observer_start()
        grid_observer._view_observer_setup()

        if PARAMS.GetBool("FirstTime", True) and (not hasattr(FreeCAD, "TestEnvironment")):
            todo.ToDo.delay(FreeCADGui.runCommand, "BIM_Welcome")
        todo.ToDo.delay(BimStatus.setStatusIcons, True)
        FreeCADGui.Control.clearTaskWatcher()

        class BimWatcher:
            def __init__(self, cmds, name, invert=False):
                self.commands = cmds
                self.title = name
                self.invert = invert

            def shouldShow(self):
                if self.invert:
                    return (FreeCAD.ActiveDocument != None) and (
                        FreeCADGui.Selection.getSelection() != []
                    )
                else:
                    return (FreeCAD.ActiveDocument != None) and (
                        not FreeCADGui.Selection.getSelection()
                    )

        FreeCADGui.Control.addTaskWatcher(
            [
                BimWatcher(self.draftingtools + self.annotationtools, "2D geometry"),
                BimWatcher(self.bimtools, "3D/BIM geometry"),
                BimWatcher(self.modify, "Modify", invert=True),
            ]
        )

        # restore views widget if needed
        if PARAMS.GetBool("RestoreBimViews", True):
            from bimcommands import BimViews

            w = BimViews.findWidget()
            if not w:
                FreeCADGui.runCommand("BIM_Views")
            else:
                w.show()
                w.toggleViewAction().setVisible(True)

        self.setupMultipleObjectSelection()

        # add NativeIFC document observer
        ifc_observer.add_observer()

        # adding a Help menu manipulator
        # https://github.com/FreeCAD/FreeCAD/pull/10933
        class BIM_WBManipulator:
            def modifyMenuBar(self):
                return [
                    {"insert": "BIM_Examples", "menuItem": "Std_ReportBug", "after": ""},
                    {"insert": "BIM_Tutorial", "menuItem": "Std_ReportBug", "after": ""},
                    {"insert": "BIM_Help", "menuItem": "Std_ReportBug", "after": ""},
                    {"insert": "BIM_Welcome", "menuItem": "Std_ReportBug", "after": ""},
                ]
        if not hasattr(Gui, "BIM_WBManipulator"):
            Gui.BIM_WBManipulator = BIM_WBManipulator()
        Gui.addWorkbenchManipulator(Gui.BIM_WBManipulator)

        Log("BIM workbench activated\n")


    def Deactivated(self):

        from draftutils import todo
        import BimStatus
        from bimcommands import BimViews
        import WorkingPlane
        from nativeifc import ifc_observer
        from draftutils import grid_observer

        PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")

        if hasattr(self, "BimSelectObserver"):
            FreeCADGui.removeDocumentObserver(self.BimSelectObserver)
            del self.BimSelectObserver

        if hasattr(FreeCADGui, "draftToolBar"):
            FreeCADGui.draftToolBar.Deactivated()
        if hasattr(FreeCADGui, "Snapper"):
            FreeCADGui.Snapper.hide()
        WorkingPlane._view_observer_stop()
        grid_observer._view_observer_setup()

        # print("Deactivating status icon")
        todo.ToDo.delay(BimStatus.setStatusIcons, False)
        FreeCADGui.Control.clearTaskWatcher()

        # store views widget state and vertical size
        w = BimViews.findWidget()
        if w:
            PARAMS.SetBool("RestoreBimViews", w.isVisible())
            PARAMS.SetInt("BimViewsSize", w.height())
            w.hide()
            w.toggleViewAction().setVisible(False)

        # add NativeIFC document observer
        ifc_observer.remove_observer()

        # Ifc stuff
        try:
            from nativeifc import ifc_status
            ifc_status.toggle_lock(False)
        except:
            pass

        # remove manipulator
        if hasattr(Gui, "BIM_WBManipulator"):
            Gui.removeWorkbenchManipulator(Gui.BIM_WBManipulator)
            del Gui.BIM_WBManipulator
            Gui.activeWorkbench().reloadActive()

        Log("BIM workbench deactivated\n")


    def ContextMenu(self, recipient):

        import DraftTools
        translate = FreeCAD.Qt.translate

        if recipient == "Tree":
            groups = False
            ungroupable = False
            for o in FreeCADGui.Selection.getSelection():
                if o.isDerivedFrom("App::DocumentObjectGroup") or o.hasExtension(
                    "App::GroupExtension"
                ):
                    groups = True
                else:
                    groups = False
                    break
            for o in FreeCADGui.Selection.getSelection():
                for parent in o.InList:
                    if parent.isDerivedFrom(
                        "App::DocumentObjectGroup"
                    ) or parent.hasExtension("App::GroupExtension"):
                        if o in parent.Group:
                            ungroupable = True
                        else:
                            ungroupable = False
                            break
            if groups:
                self.appendContextMenu("", ["Draft_SelectGroup"])
            if ungroupable:
                self.appendContextMenu("", ["BIM_Ungroup"])
            if (len(FreeCADGui.Selection.getSelection()) == 1) and (
                FreeCADGui.Selection.getSelection()[0].Name == "Trash"
            ):
                self.appendContextMenu("", ["BIM_EmptyTrash"])
        elif recipient == "View":
            self.appendContextMenu(translate("BIM", "Snapping"), self.snapmenu)
        if FreeCADGui.Selection.getSelection():
            if FreeCADGui.Selection.getSelection()[0].Name != "Trash":
                self.appendContextMenu("", ["BIM_Trash"])
            self.appendContextMenu("", ["Draft_AddConstruction", "Draft_AddToGroup"])
            allclones = False
            for obj in FreeCADGui.Selection.getSelection():
                if hasattr(obj, "CloneOf") and obj.CloneOf:
                    allclones = True
                else:
                    allclones = False
                    break
            if allclones:
                self.appendContextMenu("", ["BIM_ResetCloneColors"])
            if len(FreeCADGui.Selection.getSelection()) == 1:
                obj = FreeCADGui.Selection.getSelection()[0]
                if hasattr(obj, "Group"):
                    if obj.getTypeIdOfProperty("Group") == "App::PropertyLinkList":
                        self.appendContextMenu("", ["BIM_Reorder"])
                if obj.isDerivedFrom("TechDraw::DrawView"):
                    self.appendContextMenu("", ["BIM_MoveView"])

    def GetClassName(self):
        return "Gui::PythonWorkbench"


FreeCADGui.addWorkbench(BIMWorkbench)

# Preference pages for importing and exporting various file formats
# are independent of the loading of the workbench and can be loaded at startup

def QT_TRANSLATE_NOOP(context, text):
    return text
t = QT_TRANSLATE_NOOP("QObject", "Import-Export")
FreeCADGui.addPreferencePage(":/ui/preferences-ifc.ui", t)
FreeCADGui.addPreferencePage(":/ui/preferences-ifc-export.ui", t)
FreeCADGui.addPreferencePage(":/ui/preferences-dae.ui", t)

# Add unit tests
FreeCAD.__unit_test__ += ["TestArch"]
# The NativeIFC tests require internet connection and file download
# FreeCAD.__unit_test__ += ["nativeifc.ifc_selftest"]
