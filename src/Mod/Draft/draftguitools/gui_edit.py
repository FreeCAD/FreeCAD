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
__url__ = "https://www.freecadweb.org"

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
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers
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
    TODO: since we introduced context menu for interacting
          with editTrackers, point 2 should become obsolete,
          because not consistent with multi-object editing.

    Draft_Edit uses taskpanel in 3 ways:

    1 - when waiting for user to select an object
        calling self.ui.selectUi()

    2 - when Trackers are displayed and user must click one, a
        custom task panel is displayed depending on edited
        object:
        self.ui.editUi()            -> the default one
        self.ui.editUi("Wire")      -> line and wire editing
        self.ui.editUi("BezCurve")  -> BezCurve editing
        self.ui.editUi("Circle")    -> circle editing
        self.ui.editUi("Arc")       -> arc editing
        When Draft_Edit evaluate mouse click, depending if some
        ui button have been pressed (.isChecked()), decide if
        the action is a startEditing or AddPoint or DelPoint or
        change BezCurve Continuity, ecc.

    3 - when in editing, lineUi support clicking destination point
        by self.startEditing
        self.ui.lineUi()
        self.ui.isRelative.show()

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

        # this are used to edit structure objects, it's a bit buggy i think
        self.objs_formats = {}

        # settings
        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        self.maxObjects = param.GetInt("DraftEditMaxObjects", 5)
        self.pick_radius = param.GetInt("DraftEditPickRadius", 20)
        
        self.alt_edit_mode = 0 # alternative edit mode for objects

        # preview
        self.ghost = None

        #list of supported objects
        self.supportedObjs = edit_draft.get_supported_draft_objects() + \
                             edit_arch.get_supported_arch_objects()
        self.supportedCppObjs = edit_part.get_supported_part_objects() + \
                                 edit_sketcher.get_supported_sketcher_objects()


    def GetResources(self):
        tooltip = ("Edits the active object.\n"
                   "Press E or ALT+LeftClick to display context menu\n"
                   "on supported nodes and on supported objects.")
        return {'Pixmap': 'Draft_Edit',
                'Accel': "D, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edit"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", tooltip)
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
            self.ui.selectUi()
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


    def numericInput(self, v, numy=None, numz=None):
        """Execute callback by the toolbar to activate the update function.

        This function gets called by the toolbar
        or by the mouse click and activate the update function.
        """
        if numy:
            v = App.Vector(v, numy, numz)
        self.endEditing(self.obj, self.editing, v)
        App.ActiveDocument.recompute()


    def finish(self, closed=False):
        """Terminate Edit Tool."""
        self.unregister_selection_callback()
        self.unregister_editing_callbacks()
        self.editing = None
        self.finalizeGhost()
        Gui.Snapper.setSelectMode(False)
        if self.obj and closed:
            if "Closed" in self.obj.PropertiesList:
                if not self.obj.Closed:
                    self.obj.Closed = True
        if self.ui:
            self.removeTrackers()

        if self.edited_objects:
            self.deformat_objects_after_editing(self.edited_objects)
        
        super(Edit, self).finish()
        App.DraftWorkingPlane.restore()
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
            if key == 97:  # "a"
                self.finish()
            if key == 111:  # "o"
                self.finish(closed=True)
            if key == 101:  # "e"
                self.display_tracker_menu(event)
            if key == 105:  # "i"
                if utils.get_type(self.obj) == "Circle":
                    edit_draft.arcInvert(self.obj)
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
                    self.startEditing(event)
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

    def startEditing(self, event):
        """Start editing selected EditNode."""
        pos = event.getPosition()
        node = self.getEditNode(pos)
        ep = self.getEditNodeIndex(node)
        if ep is None:
            return

        doc = App.getDocument(str(node.documentName.getValue()))
        self.obj = doc.getObject(str(node.objectName.getValue()))
        if self.obj is None:
            return

        App.Console.PrintMessage(self.obj.Name
                                 + ": editing node number "
                                 + str(ep) + "\n")

        self.ui.lineUi()
        self.ui.isRelative.show()
        self.editing = ep
        self.trackers[self.obj.Name][self.editing].off()
        self.finalizeGhost()
        self.ghost = self.initGhost(self.obj)
        self.node.append(self.trackers[self.obj.Name][self.editing].get())
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
        self.ui.displayPoint(snappedPos, self.node[-1])
        if self.ghost:
            self.updateGhost(obj=self.obj, idx=self.editing, pt=snappedPos)

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
        self.ui.editUi(self.ui.lastMode)
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
        """Initialize preview ghost."""
        if utils.get_type(obj) == "Wire":
            return trackers.wireTracker(obj.Shape)
        elif utils.get_type(obj) == "BSpline":
            return trackers.bsplineTracker()
        elif utils.get_type(obj) == "BezCurve":
            return trackers.bezcurveTracker()
        elif utils.get_type(obj) == "Circle":
            return trackers.arcTracker()

    def updateGhost(self, obj, idx, pt):
        if utils.get_type(obj) in ["Wire"]:
            self.ghost.on()
            pointList = self.globalize_vectors(obj, obj.Points)
            pointList[idx] = pt
            if obj.Closed:
                pointList.append(pointList[0])
            self.ghost.updateFromPointlist(pointList)
        elif utils.get_type(obj) == "BSpline":
            self.ghost.on()
            pointList = self.globalize_vectors(obj, obj.Points)
            pointList[idx] = pt
            if obj.Closed:
                pointList.append(pointList[0])
            self.ghost.update(pointList)
        elif utils.get_type(obj) == "BezCurve":
            self.ghost.on()
            plist = self.globalize_vectors(obj, obj.Points)
            pointList = edit_draft.recomputePointsBezier(obj,plist,idx,pt,obj.Degree,moveTrackers=False)
            self.ghost.update(pointList,obj.Degree)
        elif utils.get_type(obj) == "Circle":
            self.ghost.on()
            self.ghost.setCenter(obj.getGlobalPlacement().Base)
            self.ghost.setRadius(obj.Radius)
            if obj.FirstAngle == obj.LastAngle:
                # obj is a circle
                self.ghost.circle = True
                if self.editing == 0:
                    self.ghost.setCenter(pt)
                elif self.editing == 1:
                    radius = pt.sub(obj.getGlobalPlacement().Base).Length
                    self.ghost.setRadius(radius)
            else:
                if self.alt_edit_mode == 0:
                    # edit by 3 points
                    if self.editing == 0:
                        # center point
                        p1 = self.relativize_vector(obj, obj.Shape.Vertexes[0].Point)
                        p2 = self.relativize_vector(obj, obj.Shape.Vertexes[1].Point)
                        p0 = DraftVecUtils.project(self.relativize_vector(obj, pt),
                                                   self.relativize_vector(obj, (edit_draft.getArcMid(obj, global_placement=True))))
                        self.ghost.autoinvert=False
                        self.ghost.setRadius(p1.sub(p0).Length)
                        self.ghost.setStartPoint(obj.Shape.Vertexes[1].Point)
                        self.ghost.setEndPoint(obj.Shape.Vertexes[0].Point)
                        self.ghost.setCenter(self.globalize_vector(obj, p0))
                        return
                    else:
                        p1 = edit_draft.getArcStart(obj, global_placement=True)
                        p2 = edit_draft.getArcMid(obj, global_placement=True)
                        p3 = edit_draft.getArcEnd(obj, global_placement=True)
                        if self.editing == 1:
                            p1=pt
                        elif self.editing == 3:
                            p2=pt
                        elif self.editing == 2:
                            p3=pt
                        self.ghost.setBy3Points(p1,p2,p3)
                elif self.alt_edit_mode == 1:
                    # edit by center radius angles
                    self.ghost.setStartAngle(math.radians(obj.FirstAngle))
                    self.ghost.setEndAngle(math.radians(obj.LastAngle))
                    if self.editing == 0:
                        self.ghost.setCenter(pt)
                    elif self.editing == 1:
                        self.ghost.setStartPoint(pt)
                    elif self.editing == 2:
                        self.ghost.setEndPoint(pt)
                    elif self.editing == 3:
                        self.ghost.setRadius(self.relativize_vector(obj, pt).Length)
        gui_tool_utils.redraw_3d_view()

    def finalizeGhost(self):
        try:
            self.ghost.finalize()
            self.ghost = None
        except:
            return

    # -------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Add/Delete Vertexes
    # -------------------------------------------------------------------------

    def addPoint(self, event):
        """Add point to obj and reset trackers.
        """
        pos = event.getPosition()
        # self.setSelectState(obj, True)
        selobjs = Gui.ActiveDocument.ActiveView.getObjectsInfo((pos[0],pos[1]))
        if not selobjs:
            return
        for info in selobjs:
            if not info:
                return
            for o in self.edited_objects:
                if o.Name != info["Object"]:
                    continue
                obj = o
                break
            if utils.get_type(obj) == "Wire" and 'Edge' in info["Component"]:
                pt = App.Vector(info["x"], info["y"], info["z"])
                self.addPointToWire(obj, pt, int(info["Component"][4:]))
            elif utils.get_type(obj) in ["BSpline", "BezCurve"]: #to fix double vertex created
                # pt = self.point
                if "x" in info:# prefer "real" 3D location over working-plane-driven one if possible
                    pt = App.Vector(info["x"], info["y"], info["z"])
                else:
                    continue
                self.addPointToCurve(pt, obj, info)
        obj.recompute()
        self.resetTrackers(obj)
        return

    def addPointToWire(self, obj, newPoint, edgeIndex):
        newPoints = []
        if hasattr(obj, "ChamferSize") and hasattr(obj, "FilletRadius"):
            if obj.ChamferSize > 0 and obj.FilletRadius > 0:
                edgeIndex = (edgeIndex + 3) / 4
            elif obj.ChamferSize > 0 or obj.FilletRadius > 0:
                edgeIndex = (edgeIndex + 1) / 2

        for index, point in enumerate(obj.Points):
            if index == edgeIndex:
                newPoints.append(self.relativize_vector(obj, newPoint))
            newPoints.append(point)
        if obj.Closed and edgeIndex == len(obj.Points):
            # last segment when object is closed
            newPoints.append(self.relativize_vector(obj, newPoint))
        obj.Points = newPoints

    def addPointToCurve(self, point, obj, info=None):
        import Part
        if  utils.get_type(obj) not in ["BSpline", "BezCurve"]:
            return
        pts = obj.Points
        if utils.get_type(obj) == "BezCurve":
            if not info['Component'].startswith('Edge'):
                return  # clicked control point
            edgeindex = int(info['Component'].lstrip('Edge')) - 1
            wire = obj.Shape.Wires[0]
            bz = wire.Edges[edgeindex].Curve
            param = bz.parameter(point)
            seg1 = wire.Edges[edgeindex].copy().Curve
            seg2 = wire.Edges[edgeindex].copy().Curve
            seg1.segment(seg1.FirstParameter, param)
            seg2.segment(param, seg2.LastParameter)
            if edgeindex == len(wire.Edges):
                # we hit the last segment, we need to fix the degree
                degree=wire.Edges[0].Curve.Degree
                seg1.increase(degree)
                seg2.increase(degree)
            edges = wire.Edges[0:edgeindex] + [Part.Edge(seg1),Part.Edge(seg2)] \
                + wire.Edges[edgeindex + 1:]
            pts = edges[0].Curve.getPoles()[0:1]
            for edge in edges:
                pts.extend(edge.Curve.getPoles()[1:])
            if obj.Closed:
                pts.pop()
            c = obj.Continuity
            # assume we have a tangent continuity for an arbitrarily split
            # segment, unless it's linear
            cont = 1 if (obj.Degree >= 2) else 0
            obj.Continuity = c[0:edgeindex] + [cont] + c[edgeindex:]
        else:
            if (utils.get_type(obj) in ["BSpline"]):
                if (obj.Closed == True):
                    curve = obj.Shape.Edges[0].Curve
                else:
                    curve = obj.Shape.Curve
            uNewPoint = curve.parameter(point)
            uPoints = []
            for p in obj.Points:
                uPoints.append(curve.parameter(p))
            for i in range(len(uPoints) - 1):
                if ( uNewPoint > uPoints[i] ) and ( uNewPoint < uPoints[i+1] ):
                    pts.insert(i + 1, self.relativize_vector(obj, point))
                    break
            # DNC: fix: add points to last segment if curve is closed
            if obj.Closed and (uNewPoint > uPoints[-1]):
                pts.append(self.relativize_vector(obj, point))
        obj.Points = pts

    def delPoint(self, event):
        pos = event.getPosition()
        node = self.getEditNode(pos)
        ep = self.getEditNodeIndex(node)

        if ep is None:
            _msg = translate("draft", "Node not found")
            App.Console.PrintWarning(_msg + "\n")
            return 

        doc = App.getDocument(str(node.documentName.getValue()))
        obj = doc.getObject(str(node.objectName.getValue()))
        if obj is None:
            return
        if utils.get_type(obj) not in ["Wire", "BSpline", "BezCurve"]:
            return
        if len(obj.Points) <= 2:
            _msg = translate("draft", "Active object must have more than two points/nodes") 
            App.Console.PrintWarning(_msg + "\n")
            return

        pts = obj.Points
        pts.pop(ep)
        obj.Points = pts
        if utils.get_type(obj) == "BezCurve":
            obj.Proxy.resetcontinuity(obj)
        obj.recompute()

        # don't do tan/sym on DWire/BSpline!
        self.resetTrackers(obj)


    # ------------------------------------------------------------------------
    # DRAFT EDIT Context menu
    # ------------------------------------------------------------------------

    def display_tracker_menu(self, event):
        self.tracker_menu = QtGui.QMenu()
        self.event = event
        actions = None
        if self.overNode:
            # if user is over a node
            doc = self.overNode.get_doc_name()
            obj = App.getDocument(doc).getObject(self.overNode.get_obj_name())
            ep = self.overNode.get_subelement_index()
            if utils.get_type(obj) in ["Line", "Wire", "BSpline"]:
                actions = ["delete point"]
            elif utils.get_type(obj) in ["BezCurve"]:
                actions = ["make sharp", "make tangent",
                           "make symmetric", "delete point"]
            elif utils.get_type(obj) in ["Circle"]:
                if obj.FirstAngle != obj.LastAngle:
                    if ep == 0:  # user is over arc start point
                        actions = ["move arc"]
                    elif ep == 1:  # user is over arc start point
                        actions = ["set first angle"]
                    elif ep == 2:  # user is over arc end point
                        actions = ["set last angle"]
                    elif ep == 3:  # user is over arc mid point
                        actions = ["set radius"]
            else:
                return
        else:
            # if user is over an edited object
            pos = self.event.getPosition()
            obj = self.get_selected_obj_at_position(pos)
            if utils.get_type(obj) in ["Line", "Wire", "BSpline", "BezCurve"]:
                actions = ["add point"]
            elif utils.get_type(obj) in ["Circle"] and obj.FirstAngle != obj.LastAngle:
                actions = ["invert arc"]
        if actions is None:
            return
        for a in actions:
            self.tracker_menu.addAction(a)
        self.tracker_menu.popup(Gui.getMainWindow().cursor().pos())
        QtCore.QObject.connect(self.tracker_menu,
                               QtCore.SIGNAL("triggered(QAction *)"),
                               self.evaluate_menu_action)


    def evaluate_menu_action(self, labelname):
        action_label = str(labelname.text())
        # addPoint and deletePoint menu
        if action_label == "delete point":
            self.delPoint(self.event)
        elif action_label == "add point":
            self.addPoint(self.event)
        # Bezier curve menu
        elif action_label in ["make sharp", "make tangent", "make symmetric"]:
            doc = self.overNode.get_doc_name()
            obj = App.getDocument(doc).getObject(self.overNode.get_obj_name())
            idx = self.overNode.get_subelement_index()
            if action_label == "make sharp":
                edit_draft.smoothBezPoint(obj, idx, 'Sharp')
            elif action_label == "make tangent":
                edit_draft.smoothBezPoint(obj, idx, 'Tangent')
            elif action_label == "make symmetric":
                edit_draft.smoothBezPoint(obj, idx, 'Symmetric')
            self.resetTrackers(obj)
        # arc tools
        elif action_label in ("move arc", "set radius",
                              "set first angle", "set last angle"):
            self.alt_edit_mode = 1
            self.startEditing(self.event)
        elif action_label == "invert arc":
            pos = self.event.getPosition()
            obj = self.get_selected_obj_at_position(pos)
            edit_draft.arcInvert(obj)
        del self.event


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
        objectType = utils.get_type(obj)

        if objectType in ["Wire", "BSpline"]:
            eps = edit_draft.getWirePts(obj)

        elif objectType == "BezCurve":
            return

        elif objectType == "Circle":
            eps = edit_draft.getCirclePts(obj)

        elif objectType == "Rectangle":
            eps = edit_draft.getRectanglePts(obj)

        elif objectType == "Polygon":
            eps = edit_draft.getPolygonPts(obj)

        elif objectType == "Ellipse":
            eps = edit_draft.getEllipsePts(obj)

        elif objectType in ("Dimension","LinearDimension"):
            eps = edit_draft.getDimensionPts(obj)

        elif objectType == "Wall":
            eps = self.globalize_vectors(obj, edit_arch.getWallPts(obj))
            if obj.Base and utils.get_type(obj.Base) in ["Wire","Circle",
                "Rectangle", "Polygon", "Sketch"]:
                basepoints = self.getEditPoints(obj.Base)
                for point in basepoints:
                    eps.append(obj.Placement.multVec(point)) #works ok except if App::Part is rotated... why?
            return eps
        
        elif objectType == "Window":
            eps = edit_arch.getWindowPts(obj)

        elif objectType == "Space":
            eps = edit_arch.getSpacePts(obj)

        elif objectType == "Structure":
            eps = edit_arch.getStructurePts(obj)

        elif objectType == "PanelCut":
            eps = edit_arch.getPanelCutPts(obj)

        elif objectType == "PanelSheet":
            eps = edit_arch.getPanelSheetPts(obj)

        elif objectType == "Part::Line" and obj.TypeId == "Part::Line":
            eps = edit_part.getPartLinePts(obj)

        elif objectType == "Part" and obj.TypeId == "Part::Box":
            eps = edit_part.getPartBoxPts(obj)

        elif objectType == "Part" and obj.TypeId == "Part::Cylinder":
            eps = edit_part.getPartCylinderPts(obj)

        elif objectType == "Part" and obj.TypeId == "Part::Cone":
            eps = edit_part.getPartConePts(obj)

        elif objectType == "Part" and obj.TypeId == "Part::Sphere":
            eps = edit_part.getPartSpherePts(obj)

        elif objectType == "Sketch":
            eps = edit_sketcher.getSketchPts(obj)
        
        if eps:
            return self.globalize_vectors(obj, eps)
        else:
            return None


    def update(self, obj, nodeIndex, v):
        """Apply the App.Vector to the modified point and update obj."""
        v = self.relativize_vector(obj, v)
        App.ActiveDocument.openTransaction("Edit")
        self.update_object(obj, nodeIndex, v)
        App.ActiveDocument.commitTransaction()
        self.resetTrackers(obj)
        try:
            gui_tool_utils.redraw_3d_view()
        except AttributeError as err:
            pass


    def update_object(self, obj, nodeIndex, v):
        objectType = utils.get_type(obj)
        if objectType in ["Wire", "BSpline"]:
            edit_draft.updateWire(obj, nodeIndex, v)

        elif objectType == "BezCurve":
            edit_draft.updateBezCurve(obj, nodeIndex, v)

        elif objectType == "Circle":
            edit_draft.updateCircle(obj, nodeIndex, v, self.alt_edit_mode)

        elif objectType == "Rectangle":
            edit_draft.updateRectangle(obj, nodeIndex, v)

        elif objectType == "Polygon":
            edit_draft.updatePolygon(obj, nodeIndex, v)

        elif objectType == "Ellipse":
            edit_draft.updateEllipse(obj, nodeIndex, v)

        elif objectType in ("Dimension","LinearDimension"):
            edit_draft.updateDimension(obj, nodeIndex, v)

        elif objectType == "Sketch":
            edit_sketcher.updateSketch(obj, nodeIndex, v)

        elif objectType == "Wall":
            if nodeIndex == 0:
                edit_arch.updateWall(obj, nodeIndex, v)
            elif nodeIndex > 0:
                if obj.Base:
                    if utils.get_type(obj.Base) in ["Wire", "Circle", "Rectangle",
                                                    "Polygon", "Sketch"]:
                        self.update(obj.Base, nodeIndex - 1, v)

        elif objectType == "Window":
            edit_arch.updateWindow(obj, nodeIndex, v)

        elif objectType == "Space":
            edit_arch.updateSpace(obj, nodeIndex, v)

        elif objectType == "Structure":
            edit_arch.updateStructure(obj, nodeIndex, v)

        elif objectType == "PanelCut":
            edit_arch.updatePanelCut(obj, nodeIndex, v)

        elif objectType == "PanelSheet":
            edit_arch.updatePanelSheet(obj, nodeIndex, v)

        elif objectType == "Part::Line" and obj.TypeId == "Part::Line":
            edit_part.updatePartLine(obj, nodeIndex, v)

        elif objectType == "Part" and obj.TypeId == "Part::Box":
            edit_part.updatePartBox(obj, nodeIndex, v)

        elif objectType == "Part" and obj.TypeId == "Part::Cylinder":
            edit_part.updatePartCylinder(obj, nodeIndex, v)

        elif objectType == "Part" and obj.TypeId == "Part::Cone":
            edit_part.updatePartCone(obj, nodeIndex, v)

        elif objectType == "Part" and obj.TypeId == "Part::Sphere":
            edit_part.updatePartSphere(obj, nodeIndex, v)

        obj.recompute()


    # -------------------------------------------------------------------------
    # UTILS
    # -------------------------------------------------------------------------

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
            _err = translate("draft", "Too many objects selected, max number set to: ")
            App.Console.PrintMessage(_err + str(self.maxObjects) + "\n")
            return None
        for obj in selection:
            if utils.get_type(obj) in self.supportedObjs:
                self.edited_objects.append(obj)
                continue
            elif utils.get_type(obj) in self.supportedCppObjs:
                if obj.TypeId in self.supportedCppObjs:
                    self.edited_objects.append(obj)
                    continue
            _wrn = translate("draft", ": this object is not editable")
            App.Console.PrintWarning(obj.Name + _wrn + "\n")
        return self.edited_objects


    def format_objects_for_editing(self, objs):
        """Change objects style during editing mode.
        """
        for obj in objs:
            # TODO: Placeholder for changing the Selectable property of obj ViewProvide
            if utils.get_type(obj) == "Structure":
                self.objs_formats[obj.Name] = edit_arch.get_structure_format(obj)
                edit_arch.set_structure_editing_format(obj)
                

    def deformat_objects_after_editing(self, objs):
        """Restore objects style during editing mode.
        """
        for obj in objs:
            # TODO: Placeholder for changing the Selectable property of obj ViewProvide
            if utils.get_type(obj) == "Structure":
                edit_arch.restore_structure_format(obj, self.objs_formats[obj.Name])


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

    def relativize_vectors(self, obj, pointList):
        """Return the given point list in the given object coordinate system."""
        plist = []
        for p in pointList:
            point = self.relativize_vector(obj, p)
            plist.append(point)
        return plist

    def relativize_vector(self, obj, point):
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


Gui.addCommand('Draft_Edit', Edit())

## @}
