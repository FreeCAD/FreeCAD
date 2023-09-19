# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2019, 2020 Carlo Pavan <carlopav@gmail.com>             *
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
"""Provides GUI tools to start the edit mode of different objects."""
## @package gui_edit
# \ingroup draftguitools
# \brief Provides GUI tools to start the edit mode of different objects.

__title__ = "FreeCAD Draft Edit Tool"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin, Carlo Pavan")
__url__ = "https://www.freecad.org"

## \addtogroup draftguitools
# @{
import math
import pivy.coin as coin
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui

import FreeCAD as App
import FreeCADGui as Gui
import DraftVecUtils
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
import draftguitools.gui_trackers as trackers
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_edit_draft_objects as edit_draft
import draftguitools.gui_edit_arch_objects as edit_arch
import draftguitools.gui_edit_part_objects as edit_part
import draftguitools.gui_edit_sketcher_objects as edit_sketcher

from draftutils.translate import translate

COLORS = {
    "default": Gui.draftToolBar.getDefaultColor("snap"),
    "black":  (0., 0., 0.),
    "white":  (1., 1., 1.),
    "grey":   (.5, .5, .5),
    "red":    (1., 0., 0.),
    "green":  (0., 1., 0.),
    "blue":   (0., 0., 1.),
    "yellow": (1., 1., 0.),
    "cyan":   (0., 1., 1.),
    "magenta": (1., 0., 1.)
}


class Edit(gui_base_original.Modifier):
    """The Draft_Edit FreeCAD command definition.

    A tool to graphically edit FreeCAD objects.
    Current implementation use many parts of pivy graphics code by user "looo".
    The tool collect editpoints from objects and display Trackers on them
    to allow editing their Shape and their parameters.

    Callbacks
    ---------
    selection_callback
        registered when tool is launched, identify
        selected objects.

    editing_callbacks
        self._keyPressedCB      -> self.keyPressed
        self._mouseMovedCB      -> self._mouseMovedCB
        if self._mousePressedCB -> self.mousePressed
        when trackers are displayed for selected objects,
        these callbacks capture user events and forward
        them to related functions

    Task panel (Draft Toolbar)
    ----------
    self.ui = Gui.draftToolBar

    Draft_Edit uses the taskpanel in 2 ways:

    1 - the user can select select an object and close the operation
        self.ui.editUi()

    2 - when editing, lineUi support clicking destination point
        by self.startEditing
        self.ui.lineUi()

    Tracker selection
    -----------------
    If the tool recognize mouse click as an attempt to startEditing,
    using soRayPickAction, it identifies the selected editTracker and
    start editing it. Here is where "looo" code was very useful.

    Editing preview
    ---------------
    When object editing begins, self.ghost is initiated with the
    corresponding DraftTracker of the object type. The object Tracker
    is deleted when user clicks again and endEditing.

    Context Menu
    ------------
    Activated with Alt+LeftClick or pressing key "e"
    It's a custom context menu, that depends on clicked tracker
    or on clicked object.

    display_tracker_menu
        populates the menu with custom actions

    evaluate_menu_action
        evaluate user chosen action and launch corresponding
        function.

    Preferences
    -----------
    maxObjects: Int
        set by "DraftEditMaxObjects" in user preferences
        The max number of FreeCAD objects the tool is
        allowed to edit at the same time.

    pick_radius: Int
        set by "DraftEditPickRadius" in user preferences
        The pick radius during editing operation.
        Increase if you experience problems in clicking
        on a editTracker because of screen resolution.

    Attributes
    ----------
    obj: Edited object
        I'm planning to discard this attribute.
        In old implementation every function was supposed to
        act on self.obj, self.editpoints, self.trackers,
        self.pl, self.invpl.
        Due to multiple object editing, i'm planning to keep
        just self.trackers. Any other object will be identified
        and processed starting from editTracker information.

    editing: Int
        Index of the editTracker that has been clicked by the
        user. Tracker selection mechanism is based on it.
        if self.editing is None :
            the user didn't click any node, and next click will
            be processed as an attempt to start editing operation
        if self.editing == o or 1 or 2 or 3 etc :
            the user is editing corresponding node, so next click
            will be processed as an attempt to end editing operation

    trackers: Dictionary {object.Name : [editTrackers]}
        It records the list of DraftTrackers.editTracker.
        {object.Name as String : [editTrackers for the object]}
        Each tracker is created with (position,obj.Name,idx),
        so it's possible to recall it
        self.trackers[str(node.objectName.getValue())][ep]

    overNode: DraftTrackers.editTracker
        It represent the editTracker under the cursor position.
        It is used to preview the tracker selection action.

    ghost: DraftTrackers.*
        Handles the tracker to preview editing operations.
        it is initialized when user clicks on a editTracker
        by self.startEditing() function.

    alt_edit_mode: Int
        Allows alternative editing modes for objects.
        ATM supported for:
        - arcs: if 0 edit by 3 points, if 1 edit by center,
                radius, angles

    supportedObjs: List
        List of supported Draft Objects.
        The tool use utils.get_type(obj) to compare object type
        to the list.

    supportedCppObjs: List
        List of supported Part Objects.
        The tool use utils.get_type(obj) and obj.TypeId to compare
        object type to the list.
    """

    def __init__(self):
        super().__init__()
        """Initialize Draft_Edit Command."""
        self.running = False
        self.trackers = {'object': []}
        self.overNode = None  # preselected node with mouseover
        self.edited_objects = []
        self.obj = None
        self.editing = None

        # event callbacks
        self.selection_callback = None
        self._keyPressedCB = None
        self._mouseMovedCB = None
        self._mousePressedCB = None

        # this stores the DisplayMode of the object to restore it after editing
        # only used by Arch Structure
        self.objs_formats = {}

        # settings
        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        self.maxObjects = param.GetInt("DraftEditMaxObjects", 5)
        self.pick_radius = param.GetInt("DraftEditPickRadius", 20)

        self.alt_edit_mode = 0 # default edit mode for objects

        # preview
        self.ghost = None

        # setup gui_tools for every supported object
        self.gui_tools_repository = GuiToolsRepository()

        self.gui_tools_repository.add('Wire', edit_draft.DraftWireGuiTools())
        self.gui_tools_repository.add('BSpline', edit_draft.DraftBSplineGuiTools())
        self.gui_tools_repository.add('BezCurve', edit_draft.DraftBezCurveGuiTools())
        self.gui_tools_repository.add('Circle', edit_draft.DraftCircleGuiTools())
        self.gui_tools_repository.add('Rectangle', edit_draft.DraftRectangleGuiTools())
        self.gui_tools_repository.add('Polygon', edit_draft.DraftPolygonGuiTools())
        self.gui_tools_repository.add('Ellipse', edit_draft.DraftEllipseGuiTools())
        self.gui_tools_repository.add('Dimension', edit_draft.DraftDimensionGuiTools()) # Backward compatibility
        self.gui_tools_repository.add('LinearDimension', edit_draft.DraftDimensionGuiTools())

        self.gui_tools_repository.add('Wall', edit_arch.ArchWallGuiTools())
        self.gui_tools_repository.add('Window', edit_arch.ArchWindowGuiTools())
        self.gui_tools_repository.add('Structure', edit_arch.ArchStructureGuiTools())
        self.gui_tools_repository.add('Space', edit_arch.ArchSpaceGuiTools())
        self.gui_tools_repository.add('PanelCut', edit_arch.ArchPanelCutGuiTools())
        self.gui_tools_repository.add('PanelSheet', edit_arch.ArchPanelSheetGuiTools())

        self.gui_tools_repository.add('Part::Line', edit_part.PartLineGuiTools())
        self.gui_tools_repository.add('Part::Box', edit_part.PartBoxGuiTools())
        self.gui_tools_repository.add('Part::Cylinder', edit_part.PartCylinderGuiTools())
        self.gui_tools_repository.add('Part::Cone', edit_part.PartConeGuiTools())
        self.gui_tools_repository.add('Part::Sphere', edit_part.PartSphereGuiTools())

        self.gui_tools_repository.add('Sketcher::SketchObject', edit_sketcher.SketcherSketchObjectGuiTools())


    def GetResources(self):
        return {'Pixmap': 'Draft_Edit',
                'Accel': "D, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edit"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edits the active object.\nPress E or ALT+LeftClick to display context menu\non supported nodes and on supported objects.")
                }


    # -------------------------------------------------------------------------
    # MAIN FUNCTIONS
    # -------------------------------------------------------------------------

    def Activated(self):
        """
        Activated is run when user launch Edit command.
        If something is selected -> call self.proceed()
        If nothing is selected -> self.register_selection_callback()
        """
        if self.running:
            self.finish()
        super(Edit, self).Activated("Edit")
        if not App.ActiveDocument:
            self.finish()

        self.ui = Gui.draftToolBar
        self.view = gui_utils.get_3d_view()

        if Gui.Selection.getSelection():
            self.proceed()
        else:
            self.ui.selectUi(on_close_call=self.finish)
            App.Console.PrintMessage(translate("draft",
                                               "Select a Draft object to edit")
                                               + "\n")
            self.register_selection_callback()


    def proceed(self):
        """this method set the editTrackers"""
        self.unregister_selection_callback()
        self.edited_objects = self.getObjsFromSelection()
        if not self.edited_objects:
            return self.finish()

        self.format_objects_for_editing(self.edited_objects)

        # start object editing
        Gui.Selection.clearSelection()
        Gui.Snapper.setSelectMode(True)

        self.ui.editUi()

        for obj in self.edited_objects:
            self.setTrackers(obj, self.getEditPoints(obj))

        self.register_editing_callbacks()

        # TODO: align working plane when editing starts
        # App.DraftWorkingPlane.save()
        # self.alignWorkingPlane()


    def numericInput(self, numx, numy, numz):
        """Execute callback by the toolbar to activate the update function.

        This function gets called by the toolbar
        or by the mouse click and activate the update function.
        """
        self.endEditing(self.obj, self.editing, App.Vector(numx, numy, numz))
        App.ActiveDocument.recompute()


    def finish(self, cont=False):
        """Terminate Edit Tool."""
        self.unregister_selection_callback()
        self.unregister_editing_callbacks()
        self.editing = None
        self.finalizeGhost()
        Gui.Snapper.setSelectMode(False)

        if self.ui:
            self.removeTrackers()

        if self.edited_objects:
            self.deformat_objects_after_editing(self.edited_objects)

        super(Edit, self).finish()
        if Gui.Snapper.grid:
            Gui.Snapper.grid.set()
        self.running = False
        # delay resetting edit mode otherwise it doesn't happen
        from PySide import QtCore
        QtCore.QTimer.singleShot(0, Gui.ActiveDocument.resetEdit)


    # -------------------------------------------------------------------------
    # SCENE EVENTS CALLBACKS
    # -------------------------------------------------------------------------

    def register_selection_callback(self):
        """Register callback for selection when command is launched."""
        self.unregister_selection_callback()
        self.selection_callback = self.view.addEventCallback("SoEvent", gui_tool_utils.selectObject)

    def unregister_selection_callback(self):
        """
        remove selection callback if it exists
        """
        if self.selection_callback:
            self.view.removeEventCallback("SoEvent", self.selection_callback)
        self.selection_callback = None

    def register_editing_callbacks(self):
        """
        register editing callbacks (former action function)
        """
        viewer = Gui.ActiveDocument.ActiveView.getViewer()
        self.render_manager = viewer.getSoRenderManager()
        view = Gui.ActiveDocument.ActiveView
        if self._keyPressedCB is None:
            self._keyPressedCB = view.addEventCallbackPivy(
            coin.SoKeyboardEvent.getClassTypeId(), self.keyPressed)
        if self._mouseMovedCB is None:
            self._mouseMovedCB = view.addEventCallbackPivy(
            coin.SoLocation2Event.getClassTypeId(), self.mouseMoved)
        if self._mousePressedCB is None:
            self._mousePressedCB = view.addEventCallbackPivy(
            coin.SoMouseButtonEvent.getClassTypeId(), self.mousePressed)
        #App.Console.PrintMessage("Draft edit callbacks registered \n")

    def unregister_editing_callbacks(self):
        """
        remove callbacks used during editing if they exist
        """
        view = Gui.ActiveDocument.ActiveView
        if self._keyPressedCB:
            view.removeEventCallbackSWIG(coin.SoKeyboardEvent.getClassTypeId(), self._keyPressedCB)
            self._keyPressedCB = None
            #App.Console.PrintMessage("Draft edit keyboard callback unregistered \n")
        if self._mouseMovedCB:
            view.removeEventCallbackSWIG(coin.SoLocation2Event.getClassTypeId(), self._mouseMovedCB)
            self._mouseMovedCB = None
            #App.Console.PrintMessage("Draft edit location callback unregistered \n")
        if self._mousePressedCB:
            view.removeEventCallbackSWIG(coin.SoMouseButtonEvent.getClassTypeId(), self._mousePressedCB)
            self._mousePressedCB = None
            #App.Console.PrintMessage("Draft edit mouse button callback unregistered \n")

    # -------------------------------------------------------------------------
    # SCENE EVENT HANDLERS
    # -------------------------------------------------------------------------

    def keyPressed(self, event_callback):
        """Execute as callback for keyboard event."""
        # TODO: Get the keys from preferences
        event = event_callback.getEvent()
        if event.getState() == coin.SoKeyboardEvent.DOWN:
            key = event.getKey()
            # App.Console.PrintMessage("pressed key : "+str(key)+"\n")
            if key == 65307:  # ESC
                self.finish()
            if key == 101:  # "e"
                self.display_tracker_menu(event)
            if key == 65535 and Gui.Selection.GetSelection() is None: # BUG: delete key activate Std::Delete command at the same time!
                print("DELETE PRESSED\n")
                self.delPoint(event)

    def mousePressed(self, event_callback):
        """
        mouse button event handler, calls: startEditing, endEditing, addPoint, delPoint
        """
        event = event_callback.getEvent()
        if (event.getState() == coin.SoMouseButtonEvent.DOWN and
            event.getButton() == event.BUTTON1
            ):#left click
            if not event.wasAltDown():
                if self.editing is None:

                    pos = event.getPosition()
                    node = self.getEditNode(pos)
                    node_idx = self.getEditNodeIndex(node)
                    if node_idx is None:
                        return
                    doc = App.getDocument(str(node.documentName.getValue()))
                    obj = doc.getObject(str(node.objectName.getValue()))

                    self.startEditing(obj, node_idx)
                else:
                    self.endEditing(self.obj, self.editing)
            elif event.wasAltDown():  # left click with ctrl down
                self.display_tracker_menu(event)

    def mouseMoved(self, event_callback):
        """Execute as callback for mouse movement.

        Update tracker position and update preview ghost.
        """
        event = event_callback.getEvent()
        if self.editing is not None:
            self.updateTrackerAndGhost(event)
        else:
            # look for a node in mouse position and highlight it
            pos = event.getPosition()
            node = self.getEditNode(pos)
            ep = self.getEditNodeIndex(node)
            if ep is not None:
                if self.overNode is not None:
                    self.overNode.setColor(COLORS["default"])
                self.trackers[str(node.objectName.getValue())][ep].setColor(COLORS["red"])
                self.overNode = self.trackers[str(node.objectName.getValue())][ep]
            else:
                if self.overNode is not None:
                    self.overNode.setColor(COLORS["default"])
                    self.overNode = None

    def startEditing(self, obj, node_idx):
        """Start editing selected EditNode."""
        self.obj = obj # this is still needed to handle preview
        if obj is None:
            return

        App.Console.PrintMessage(obj.Name
                                 + ": editing node number "
                                 + str(node_idx) + "\n")

        self.ui.lineUi(title=translate("draft", "Edit node"), icon="Draft_Edit")
        self.ui.isRelative.hide()
        self.ui.continueCmd.hide()
        self.editing = node_idx
        self.trackers[obj.Name][node_idx].off()

        self.finalizeGhost()
        self.initGhost(obj)

        self.node.append(self.trackers[obj.Name][node_idx].get())
        Gui.Snapper.setSelectMode(False)
        self.hideTrackers()

    def updateTrackerAndGhost(self, event):
        """Update tracker position when editing and update ghost."""
        pos = event.getPosition().getValue()
        orthoConstrain = False
        if event.wasShiftDown() == 1:
            orthoConstrain = True
        snappedPos = Gui.Snapper.snap((pos[0],pos[1]),self.node[-1], constrain=orthoConstrain)
        self.trackers[self.obj.Name][self.editing].set(snappedPos)
        self.ui.displayPoint(snappedPos, self.node[-1], mask=Gui.Snapper.affinity)
        if self.ghost:
            self.updateGhost(obj=self.obj, node_idx=self.editing, v=snappedPos)

    def endEditing(self, obj, nodeIndex, v=None):
        """Terminate editing and start object updating process."""
        self.finalizeGhost()
        self.trackers[obj.Name][nodeIndex].on()
        Gui.Snapper.setSelectMode(True)
        if v is None:
            # endEditing is called by mousePressed
            v = self.trackers[obj.Name][nodeIndex].get()
        else:
            # endEditing is called by numericInput, so tracker
            # position should be updated manually
            self.trackers[obj.Name][nodeIndex].set(v)
        self.update(obj, nodeIndex, v)
        self.alt_edit_mode = 0
        self.ui.editUi()
        self.node = []
        self.editing = None
        self.showTrackers()
        gui_tool_utils.redraw_3d_view()


    # -------------------------------------------------------------------------
    # EDIT TRACKERS functions
    # -------------------------------------------------------------------------

    def setTrackers(self, obj, points=None):
        """Set Edit Trackers for editpoints collected from self.obj."""
        if utils.get_type(obj) == "BezCurve":
            return self.resetTrackersBezier(obj)
        if points is None or len(points) == 0:
            _wrn = translate("draft", "No edit point found for selected object")
            App.Console.PrintWarning(_wrn + "\n")
            # do not finish if some trackers are still present
            if self.trackers == {'object': []}:
                self.finish()
            return
        self.trackers[obj.Name] = []
        if obj.Name in self.trackers:
            self.removeTrackers(obj)
        for ep in range(len(points)):
            self.trackers[obj.Name].append(trackers.editTracker(pos=points[ep], name=obj.Name, idx=ep))

    def resetTrackers(self, obj):
        """Reset Edit Trackers and set them again."""
        self.removeTrackers(obj)
        self.setTrackers(obj, self.getEditPoints(obj))

    def resetTrackersBezier(self, obj):
        # in future move tracker definition to DraftTrackers
        knotmarkers = (coin.SoMarkerSet.DIAMOND_FILLED_9_9,#sharp
                coin.SoMarkerSet.SQUARE_FILLED_9_9,        #tangent
                coin.SoMarkerSet.HOURGLASS_FILLED_9_9)     #symmetric
        polemarker = coin.SoMarkerSet.CIRCLE_FILLED_9_9    #pole
        self.trackers[obj.Name] = []
        cont = obj.Continuity
        firstknotcont = cont[-1] if (obj.Closed and cont) else 0
        pointswithmarkers = [(obj.Shape.Edges[0].Curve.
                getPole(1),knotmarkers[firstknotcont])]
        for edgeindex, edge in enumerate(obj.Shape.Edges):
            poles = edge.Curve.getPoles()
            pointswithmarkers.extend([(point,polemarker) for point in poles[1:-1]])
            if not obj.Closed or len(obj.Shape.Edges) > edgeindex +1:
                knotmarkeri = cont[edgeindex] if len(cont) > edgeindex else 0
                pointswithmarkers.append((poles[-1],knotmarkers[knotmarkeri]))
        for index, pwm in enumerate(pointswithmarkers):
            p, marker = pwm
            p = obj.Placement.inverse().multVec(p)
            p = obj.getGlobalPlacement().multVec(p)
            self.trackers[obj.Name].append(trackers.editTracker(p, obj.Name,
                index, obj.ViewObject.LineColor, marker=marker))

    def removeTrackers(self, obj=None):
        """Remove Edit Trackers."""
        if obj:
            if obj.Name in self.trackers:
                for t in self.trackers[obj.Name]:
                    t.finalize()
            self.trackers[obj.Name] = []
        else:
            for key in self.trackers.keys():
                for t in self.trackers[key]:
                    t.finalize()
            self.trackers = {'object': []}


    def hideTrackers(self, obj=None):
        """Hide Edit Trackers.

        Attributes
        ----------
        obj: FreeCAD object
            Hides trackers only for given object,
            if obj is None, hides all trackers
        """
        if obj is None:
            for key in self.trackers:
                for t in self.trackers[key]:
                    t.off()
        else:
            for t in self.trackers[obj.Name]:
                t.off()

    def showTrackers(self, obj=None):
        """Show Edit Trackers.

        Attributes
        ----------
        obj: FreeCAD object
            Shows trackers only for given object,
            if obj is None, shows all trackers
        """
        if obj is None:
            for key in self.trackers:
                for t in self.trackers[key]:
                    t.on()
        else:
            for t in self.trackers[obj.Name]:
                t.on()

    # -------------------------------------------------------------------------
    # PREVIEW
    # -------------------------------------------------------------------------

    def initGhost(self, obj):
        self.current_editing_object_gui_tools = self.get_obj_gui_tools(obj)
        if self.current_editing_object_gui_tools:
            self.ghost = self.current_editing_object_gui_tools.init_preview_object(obj)

    def updateGhost(self, obj, node_idx, v):
        self.ghost.on()

        if self.current_editing_object_gui_tools:
            self.current_editing_object_gui_tools.update_preview_object(self, obj, node_idx, v)

        gui_tool_utils.redraw_3d_view()

    def finalizeGhost(self):
        try:
            self.current_editing_object_gui_tools = None
            self.ghost.finalize()
            self.ghost = None
        except Exception:
            return

    # ------------------------------------------------------------------------
    # DRAFT EDIT Context menu
    # ------------------------------------------------------------------------

    def display_tracker_menu(self, event):
        self.tracker_menu = QtGui.QMenu()
        actions = None

        if self.overNode:
            # if user is over a node
            doc = self.overNode.get_doc_name()
            obj = App.getDocument(doc).getObject(self.overNode.get_obj_name())
            ep = self.overNode.get_subelement_index()

            obj_gui_tools = self.get_obj_gui_tools(obj)
            if obj_gui_tools:
                actions = obj_gui_tools.get_edit_point_context_menu(self, obj, ep)

        else:
            # try if user is over an edited object
            pos = event.getPosition()
            obj = self.get_selected_obj_at_position(pos)

            obj_gui_tools = self.get_obj_gui_tools(obj)
            if obj_gui_tools:
                actions = obj_gui_tools.get_edit_obj_context_menu(self, obj, pos)

        if actions is None:
            return

        for (label, callback) in actions:
            def wrapper(callback=callback):
                callback()
                self.resetTrackers(obj)

            action = self.tracker_menu.addAction(label)
            action.setData(wrapper)

        self.tracker_menu.popup(Gui.getMainWindow().cursor().pos())

        QtCore.QObject.connect(self.tracker_menu,
                               QtCore.SIGNAL("triggered(QAction *)"),
                               self.evaluate_menu_action)


    def evaluate_menu_action(self, action):
        callback = action.data()
        callback()


    # -------------------------------------------------------------------------
    # EDIT OBJECT TOOLS
    #
    # This section contains the code to retrieve the object points and update them
    #
    # -------------------------------------------------------------------------

    def getEditPoints(self, obj):
        """Return a list of App.Vectors according to the given object edit nodes.
        """
        eps = None

        obj_gui_tools = self.get_obj_gui_tools(obj)
        if obj_gui_tools:
            eps = obj_gui_tools.get_edit_points(obj)

        if eps:
            return self.globalize_vectors(obj, eps)
        else:
            return None


    def update(self, obj, nodeIndex, v):
        """Apply the App.Vector to the modified point and update obj."""
        v = self.localize_vector(obj, v)
        App.ActiveDocument.openTransaction("Edit")
        self.update_object(obj, nodeIndex, v)
        App.ActiveDocument.commitTransaction()
        self.resetTrackers(obj)
        try:
            gui_tool_utils.redraw_3d_view()
        except AttributeError as err:
            pass


    def update_object(self, obj, nodeIndex, v):
        """Update the object according to the given modified editpoint.
        """
        obj_gui_tools = self.get_obj_gui_tools(obj)
        if obj_gui_tools:
            eps = obj_gui_tools.update_object_from_edit_points(obj, nodeIndex, v, self.alt_edit_mode)

        obj.recompute()


    # -------------------------------------------------------------------------
    # UTILS
    # -------------------------------------------------------------------------

    def has_obj_gui_tools(self, obj):
        """ Check if the object has the GuiTools to provide information to edit it.
        """
        if (hasattr(obj, 'obj_gui_tools') or
            (hasattr(obj, 'Proxy') and hasattr(obj.Proxy, 'obj_gui_tools')) or
            (utils.get_type(obj) in self.gui_tools_repository.keys()) ):
            return True
        else:
            return False


    def get_obj_gui_tools(self, obj):
        """ Retrieve the obj_gui_tools to support Draft Edit.
        """
        try:
            obj_gui_tools = obj.obj_gui_tools
        except AttributeError:
            try:
                obj_gui_tools = obj.Proxy.obj_gui_tools
            except AttributeError:
                try:
                    obj_gui_tools = self.gui_tools_repository.get(utils.get_type(obj))
                except Exception:
                    obj_gui_tools = None
        return obj_gui_tools


    def getObjsFromSelection(self):
        """Evaluate selection and return a valid object to edit.

        #to be used for app link support

        for selobj in Gui.Selection.getSelectionEx('', 0):
    	    for sub in selobj.SubElementNames:
                obj = selobj.Object
                obj_matrix = selobj.Object.getSubObject(sub, retType=4)
        """
        selection = Gui.Selection.getSelection()
        self.edited_objects = []
        if len(selection) > self.maxObjects:
            _err = translate("draft", "Too many objects selected, max number set to:")
            App.Console.PrintMessage(_err + " " + str(self.maxObjects) + "\n")
            return None

        for obj in selection:
            if self.has_obj_gui_tools(obj):
                self.edited_objects.append(obj)
            else:
                _wrn = translate("draft", ": this object is not editable")
                App.Console.PrintWarning(obj.Name + _wrn + "\n")
        return self.edited_objects


    def format_objects_for_editing(self, objs):
        """Change objects style during editing mode.
        """
        for obj in objs:
            obj_gui_tools = self.get_obj_gui_tools(obj)
            if obj_gui_tools:
                self.objs_formats[obj.Name] = obj_gui_tools.get_object_style(obj)
                obj_gui_tools.set_object_editing_style(obj)


    def deformat_objects_after_editing(self, objs):
        """Restore objects style during editing mode.
        """
        for obj in objs:
            obj_gui_tools = self.get_obj_gui_tools(obj)
            if obj_gui_tools:
                obj_gui_tools.restore_object_style(obj, self.objs_formats[obj.Name])


    def get_specific_object_info(self, obj, pos):
        """Return info of a specific object at a given position.
        """
        selobjs = Gui.ActiveDocument.ActiveView.getObjectsInfo((pos[0],pos[1]))
        if not selobjs:
            return
        for info in selobjs:
            if not info:
                continue
            if obj.Name == info["Object"] and "x" in info:
                # prefer "real" 3D location over working-plane-driven one if possible
                pt = App.Vector(info["x"], info["y"], info["z"])
                return info, pt

    def get_selected_obj_at_position(self, pos):
        """Return object at given position.

        If object is one of the edited objects (self.edited_objects).
        """
        selobjs = Gui.ActiveDocument.ActiveView.getObjectsInfo((pos[0],pos[1]))
        if not selobjs:
            return
        for info in selobjs:
            if not info:
                return
            for obj in self.edited_objects:
                if obj.Name == info["Object"]:
                    return obj

    def globalize_vectors(self, obj, pointList):
        """Return the given point list in the global coordinate system."""
        plist = []
        for p in pointList:
            point = self.globalize_vector(obj, p)
            plist.append(point)
        return plist

    def globalize_vector(self, obj, point):
        """Return the given point in the global coordinate system."""
        if hasattr(obj, "getGlobalPlacement"):
            return obj.getGlobalPlacement().multVec(point)
        else:
            return point

    def localize_vectors(self, obj, pointList):
        """Return the given point list in the given object coordinate system."""
        plist = []
        for p in pointList:
            point = self.localize_vector(obj, p)
            plist.append(point)
        return plist

    def localize_vector(self, obj, point):
        """Return the given point in the given object coordinate system."""
        if hasattr(obj, "getGlobalPlacement"):
            return obj.getGlobalPlacement().inverse().multVec(point)
        else:
            return point

    def getEditNode(self, pos):
        """Get edit node from given screen position."""
        node = self.sendRay(pos)
        return node

    def sendRay(self, mouse_pos):
        """Send a ray through the scene and return the nearest entity."""
        ray_pick = coin.SoRayPickAction(self.render_manager.getViewportRegion())
        ray_pick.setPoint(coin.SbVec2s(*mouse_pos))
        ray_pick.setRadius(self.pick_radius)
        ray_pick.setPickAll(True)
        ray_pick.apply(self.render_manager.getSceneGraph())
        picked_point = ray_pick.getPickedPointList()
        return self.searchEditNode(picked_point)

    def searchEditNode(self, picked_point):
        """Search edit node inside picked point list and return node number."""
        for point in picked_point:
            path = point.getPath()
            length = path.getLength()
            point = path.getNode(length - 2)
            #import DraftTrackers
            if hasattr(point,"subElementName") and 'EditNode' in str(point.subElementName.getValue()):
                return point
        return None

    def getEditNodeIndex(self, point):
        """Get edit node index from given screen position."""
        if point:
            subElement = str(point.subElementName.getValue())
            ep = int(subElement[8:])
            return ep
        else:
            return None



class GuiToolsRepository():
    """ This object provide a repository to collect all the specific objects
    editing tools.
    """
    def __init__(self):
         self.obj_gui_tools = {}

    def get(self, obj_type):
         return self.obj_gui_tools[obj_type]

    def add(self, type, gui_tools):
        self.obj_gui_tools[type] = gui_tools

    def keys(self):
        return self.obj_gui_tools.keys()



Gui.addCommand('Draft_Edit', Edit())

## @}
