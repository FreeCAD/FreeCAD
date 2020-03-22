# -*- coding: utf8 -*-
#***************************************************************************
#*   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
#*   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
#*   Copyright (c) 2019, 2020 Carlo Pavan <carlopav@gmail.com>             *
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

__title__= "FreeCAD Draft Edit Tool"
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, \
                Dmitry Chigrin, Carlo Pavan"
__url__ = "https://www.freecadweb.org"

import FreeCAD as App
import math
import Draft

if App.GuiUp:
    # Do not import GUI-related modules if GUI is not there
    import FreeCADGui as Gui
    import DraftTools
    from DraftTrackers import editTracker, wireTracker, arcTracker, bsplineTracker, bezcurveTracker
    from pivy import coin
    from PySide import QtCore, QtGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    from DraftTools import translate

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
        "magenta":(1., 0., 1.)
    }


class Edit():
    """
    The Draft_Edit FreeCAD command definition.
    A tool to graphically edit FreeCAD objects.
    Current implementation use many parts of pivy graphics code by user "looo".
    The tool collect editpoints from objects and display Trackers on them to allow
    editing their Shape and their parameters.


    Callbacks
    ----------
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
    ----------
    If the tool recognize mouse click as an attempt to startEditing,
    using soRayPickAction, it identifies the selected editTracker and
    start editing it. Here is where "looo" code was very useful.


    Editing preview
    ----------
    When object editing begins, self.ghost is initiated with the 
    corresponding DraftTracker of the object type. The object Tracker
    is deleted when user clicks again and endEditing.


    Context Menu
    ----------
    Activated with Alt+LeftClick or pressing key "e"
    It's a custom context menu, that depends on clicked tracker
    or on clicked object.

    display_tracker_menu
        populates the menu with custom actions
    
    evaluate_menu_action
        evaluate user chosen action and launch corresponding
        function.


    Preferences
    ----------
    maxObjects : Int
        set by "DraftEditMaxObjects" in user preferences
        The max number of FreeCAD objects the tool is
        allowed to edit at the same time.

    pick_radius : Int
        set by "DraftEditPickRadius" in user preferences
        The pick radius during editing operation.
        Increase if you experience problems in clicking
        on a editTracker because of screen resolution.


    Attributes
    ----------
    obj : Edited object
        I'm planning to discard this attribute.
        In old implementation every function was supposed to 
        act on self.obj, self.editpoints, self.trackers,
        self.pl, self.invpl.
        Due to multiple object editing, i'm planning to keep
        just self.trackers. Any other object will be identified
        and processed starting from editTracker information.
    
    editing : Int
        Index of the editTracker that has been clicked by the 
        user. Tracker selection mechanism is based on it.
        if self.editing == None :
            the user didn't click any node, and next click will
            be processed as an attempt to start editing operation
        if self.editing == o or 1 or 2 or 3 etc :
            the user is editing corresponding node, so next click
            will be processed as an attempt to end editing operation

    editpoints : List [FreeCAD::App.Vector]
        List of editpoints collected from the edited object, 
        on whick editTrackers will be placed.

    trackers : Dictionary {object.Name : [editTrackers]}
        It records the list of DraftTrackers.editTracker.
        {object.Name as String : [editTrackers for the object]}
        Each tracker is created with (position,obj.Name,idx), 
        so it's possible to recall it 
        self.trackers[str(node.objectName.getValue())][ep]

    overNode : DraftTrackers.editTracker
        It represent the editTracker under the cursor position.
        It is used to preview the tracker selection action.

    ghost : DraftTrackers.*
        Handles the tracker to preview editing operations.
        it is initialized when user clicks on a editTracker
        by self.startEditing() function.

    alt_edit_mode : Int
        Allows alternative editing modes for objects.
        ATM supported for:
        - arcs: if 0 edit by 3 points, if 1 edit by center, 
                radius, angles

    supportedObjs : List
        List of supported Draft Objects.
        The tool use Draft.getType(obj) to compare object type
        to the list.

    supportedPartObjs : List
        List of supported Part Objects.
        The tool use Draft.getType(obj) and obj.TypeId to compare 
        object type to the list.
        
    """
    
    def __init__(self):
        """
        Initialize Draft_Edit Command.
        """
        self.running = False
        self.trackers = {'object':[]}
        self.overNode = None # preselected node with mouseover
        self.obj = None
        self.editing = None

        # event callbacks
        self.selection_callback = None
        self._keyPressedCB = None
        self._mouseMovedCB = None
        self._mousePressedCB = None

        # this are used to edit structure objects, it's a bit buggy i think
        self.selectstate = None
        self.originalDisplayMode = None
        self.originalPoints = None
        self.originalNodes = None

        # settings
        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        self.maxObjects = param.GetInt("DraftEditMaxObjects", 5)
        self.pick_radius = param.GetInt("DraftEditPickRadius", 20)
        
        self.alt_edit_mode = 0 # alternative edit mode for objects

        # preview
        self.ghost = None

        #list of supported Draft and Arch objects
        self.supportedObjs = ["BezCurve","Wire","BSpline","Circle","Rectangle",
                            "Polygon","Dimension","Space","Structure","PanelCut",
                            "PanelSheet","Wall", "Window"]

        #list of supported Part objects (they don't have a proxy)
        #TODO: Add support for "Part::Circle" "Part::RegularPolygon" "Part::Plane" "Part::Ellipse" "Part::Vertex" "Part::Spiral"
        self.supportedPartObjs = ["Sketch", "Sketcher::SketchObject",
                                "Part", "Part::Line", "Part::Box"]

    def GetResources(self):
        
        tooltip = ("Edits the active object.\n"
                   "Press E or ALT+LeftClick to display context menu\n"
                   "on supported nodes and on supported objects.")
                  
        return {'Pixmap': 'Draft_Edit',
                'Accel': "D, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edit"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", tooltip)
                }


    #---------------------------------------------------------------------------
    # MAIN FUNCTIONS
    #---------------------------------------------------------------------------

    def Activated(self):
        """
        Activated is run when user launch Edit command.
        If something is selected -> call self.proceed()
        If nothing is selected -> self.register_selection_callback()
        """
        if self.running:
            self.finish()
        DraftTools.Modifier.Activated(self,"Edit")
        if not App.ActiveDocument:
            self.finish()

        self.ui = Gui.draftToolBar
        self.view = Draft.get3DView()

        if Gui.Selection.getSelection():
            self.proceed()
        else:
            self.ui.selectUi()
            App.Console.PrintMessage(translate("draft", 
                                                   "Select a Draft object to edit")
                                                   + "\n")
            self.register_selection_callback()

    def proceed(self):
        "this method defines editpoints and set the editTrackers"
        self.unregister_selection_callback()
        self.edited_objects = self.getObjsFromSelection()
        if not self.edited_objects:
            return self.finish() 

        # Save selectstate and turn selectable false.
        # Object can remain selectable commenting following lines:
        # self.saveSelectState(self.obj)        
        # self.setSelectState(self.obj, False)

        # start object editing
        Gui.Selection.clearSelection()
        Gui.Snapper.setSelectMode(True)
        
        self.ui.editUi()

        for obj in self.edited_objects:
            self.setEditPoints(obj)

        self.register_editing_callbacks()

        # TODO: align working plane when editing starts
        # App.DraftWorkingPlane.save()
        # self.alignWorkingPlane()


    def finish(self,closed=False):
        """
        terminates Edit Tool
        """
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
        self.restoreSelectState(self.obj)
        if Draft.getType(self.obj) == "Structure":
            if self.originalDisplayMode is not None:
                self.obj.ViewObject.DisplayMode = self.originalDisplayMode
            if self.originalPoints is not None:
                self.obj.ViewObject.NodeSize = self.originalPoints
            if self.originalNodes is not None:
                self.obj.ViewObject.ShowNodes = self.originalNodes
        self.selectstate = None
        self.originalDisplayMode = None
        self.originalPoints = None
        self.originalNodes = None
        DraftTools.Modifier.finish(self)
        App.DraftWorkingPlane.restore()
        if Gui.Snapper.grid:
            Gui.Snapper.grid.set()
        self.running = False
        # delay resetting edit mode otherwise it doesn't happen
        from PySide import QtCore
        QtCore.QTimer.singleShot(0,Gui.ActiveDocument.resetEdit)

    #---------------------------------------------------------------------------
    # SCENE EVENTS CALLBACKS
    #---------------------------------------------------------------------------

    def register_selection_callback(self):
        """
        register callback for selection when command is launched
        """
        self.unregister_selection_callback()
        self.selection_callback = self.view.addEventCallback("SoEvent",DraftTools.selectObject)

    def unregister_selection_callback(self):
        """
        remove selection callback if it exists
        """
        if self.selection_callback:
            self.view.removeEventCallback("SoEvent",self.selection_callback)
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

    #---------------------------------------------------------------------------
    # SCENE EVENT HANDLERS
    #---------------------------------------------------------------------------

    def keyPressed(self, event_callback):
        """
        keyboard event handler
        """
        #TODO: Get the keys from preferences
        event = event_callback.getEvent()
        if event.getState() == coin.SoKeyboardEvent.DOWN:
            key = event.getKey()
            #App.Console.PrintMessage("pressed key : "+str(key)+"\n")
            if key == 65307: # ESC
                self.finish()
            if key == 97: # "a"
                self.finish()
            if key == 111: # "o"
                self.finish(closed=True)
            if key == 101: # "e"
                self.display_tracker_menu(event)
            if key == 105: # "i"
                if Draft.getType(self.obj) == "Circle":
                    self.arcInvert(self.obj)

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
                    self.endEditing(self.obj,self.editing)
            elif event.wasAltDown(): #left click with ctrl down
                self.display_tracker_menu(event)
    
    def mouseMoved(self, event_callback):
        "mouse moved event handler, update tracker position and update preview ghost"
        event = event_callback.getEvent()
        if self.editing != None:
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
        "start editing selected EditNode"
        pos = event.getPosition()
        node = self.getEditNode(pos)
        ep = self.getEditNodeIndex(node)
        if ep is None:
            return

        doc = App.getDocument(str(node.documentName.getValue()))
        self.obj = doc.getObject(str(node.objectName.getValue()))
        if self.obj is None:
            return
        self.setPlacement(self.obj)

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
        "updates tracker position when editing and update ghost"
        pos = event.getPosition().getValue()
        orthoConstrain = False
        if event.wasShiftDown() == 1: orthoConstrain = True
        snappedPos = Gui.Snapper.snap((pos[0],pos[1]),self.node[-1], constrain=orthoConstrain)
        self.trackers[self.obj.Name][self.editing].set(snappedPos)
        self.ui.displayPoint(snappedPos,self.node[-1])
        if self.ghost:
            self.updateGhost(obj=self.obj,idx=self.editing,pt=snappedPos)

    def endEditing(self, obj, nodeIndex, v = None):
        "terminate editing and start object updating process"
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
        DraftTools.redraw3DView()

    #---------------------------------------------------------------------------
    # UTILS
    #---------------------------------------------------------------------------

    def getObjsFromSelection(self):
        "evaluate selection and returns a valid object to edit"
        selection = Gui.Selection.getSelection()
        self.edited_objects = []
        if len(selection) > self.maxObjects:
            App.Console.PrintMessage(translate("draft", 
                                               "Too many objects selected, max number set to: ")
                                     + str(self.maxObjects) + "\n")
            return None
        for obj in selection:
            if Draft.getType(obj) in self.supportedObjs:
                self.edited_objects.append(obj)
                continue
            elif Draft.getType(obj) in self.supportedPartObjs:
                if obj.TypeId in self.supportedPartObjs:
                    self.edited_objects.append(obj)
                    continue
            App.Console.PrintWarning(obj.Name 
                                     + translate("draft",
                                                 ": this object is not editable")
                                     + "\n")
        return self.edited_objects

    def get_selected_obj_at_position(self, pos):
        """return object at given position
        if object is one of the edited objects (self.edited_objects)
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

    def numericInput(self, v, numy=None, numz=None):
        """this function gets called by the toolbar
        or by the mouse click and activate the update function"""
        if (numy is not None):
            v = App.Vector(v,numy,numz)
        self.endEditing(self.obj, self.editing, v)
        App.ActiveDocument.recompute()

    def setSelectState(self, obj, selState = False):
        if hasattr(obj.ViewObject,"Selectable"):
            obj.ViewObject.Selectable = selState

    def saveSelectState(self, obj):
        if hasattr(obj.ViewObject,"Selectable"):
            self.selectstate = obj.ViewObject.Selectable

    def restoreSelectState(self,obj):
        if obj:
            if hasattr(obj.ViewObject,"Selectable") and (self.selectstate is not None):
                obj.ViewObject.Selectable = self.selectstate

    def setPlacement(self,obj):
        "set self.pl and self.invpl to self.obj placement and inverse placement"
        if not obj:
            return
        if "Placement" in obj.PropertiesList:
            self.pl = obj.getGlobalPlacement()
            self.invpl = self.pl.inverse()

    def alignWorkingPlane(self):
        "align working plane to self.obj"
        if "Shape" in self.obj.PropertiesList:
            if DraftTools.plane.weak:
                DraftTools.plane.alignToFace(self.obj.Shape)
        if self.planetrack:
            self.planetrack.set(self.editpoints[0])

    def getEditNode(self, pos):
        "get edit node from given screen position"
        node = self.sendRay(pos)
        return node
    
    def sendRay(self, mouse_pos):
        "sends a ray through the scene and return the nearest entity"
        ray_pick = coin.SoRayPickAction(self.render_manager.getViewportRegion())
        ray_pick.setPoint(coin.SbVec2s(*mouse_pos))
        ray_pick.setRadius(self.pick_radius)
        ray_pick.setPickAll(True)
        ray_pick.apply(self.render_manager.getSceneGraph())
        picked_point = ray_pick.getPickedPointList()
        return self.searchEditNode(picked_point)

    def searchEditNode(self, picked_point):
        "search edit node inside picked point list and retrurn node number"
        for point in picked_point:
            path = point.getPath()
            length = path.getLength()
            point = path.getNode(length - 2)
            #import DraftTrackers
            if hasattr(point,"subElementName") and 'EditNode' in str(point.subElementName.getValue()):
                return point
        return None

    def getEditNodeIndex(self, point):
        "get edit node index from given screen position"
        if point:
            subElement = str(point.subElementName.getValue())
            ep = int(subElement[8:])
            return ep
        else:
            return None


    #---------------------------------------------------------------------------
    # EDIT TRACKERS functions
    #---------------------------------------------------------------------------

    def setTrackers(self, obj, points=None):
        "set Edit Trackers for editpoints collected from self.obj"
        if points is None or len(points) == 0:
            App.Console.PrintWarning(translate("draft", 
                                                   "No edit point found for selected object")
                                                   + "\n")
            # do not finish if some trackers are still present
            if self.trackers == {'object':[]}:
                self.finish()
            return
        self.trackers[obj.Name] = []
        if Draft.getType(obj) == "BezCurve":
            self.resetTrackersBezier(obj)
        else:
            if obj.Name in self.trackers:
                self.removeTrackers(obj)
            for ep in range(len(points)):
                self.trackers[obj.Name].append(editTracker(pos=points[ep],name=obj.Name,idx=ep))

    def resetTrackers(self, obj):
        "reset Edit Trackers and set them again"
        self.removeTrackers(obj)
        self.setTrackers(obj, self.getEditPoints(obj))

    def removeTrackers(self, obj = None):
        "reset Edit Trackers and set them again"
        if obj:
            if obj.Name in self.trackers:
                for t in self.trackers[obj.Name]:
                    t.finalize()
            self.trackers[obj.Name] = []
        else:
            for key in self.trackers.keys():
                for t in self.trackers[key]:
                    t.finalize()
            self.trackers = {'object':[]}


    def hideTrackers(self, obj=None):
        """hide Edit Trackers

        Attributes
        ----------
        obj : FreeCAD object
            hides trackers only for given object, 
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
        """show Edit Trackers

        Attributes
        ----------
        obj : FreeCAD object
            shows trackers only for given object, 
            if obj is None, shows all trackers
        """
        if obj is None:
            for key in self.trackers:
                for t in self.trackers[key]:
                    t.on()
        else:
            for t in self.trackers[obj.Name]:
                t.on()

    #---------------------------------------------------------------------------
    # PREVIEW
    #---------------------------------------------------------------------------

    def initGhost(self,obj):
        "initialize preview ghost"
        if Draft.getType(obj) == "Wire":
            return wireTracker(obj.Shape)
        elif Draft.getType(obj) == "BSpline":
            return bsplineTracker()
        elif Draft.getType(obj) == "BezCurve":
            return bezcurveTracker()
        elif Draft.getType(obj) == "Circle":
            return arcTracker()

    def updateGhost(self,obj,idx,pt):
        if Draft.getType(obj) in ["Wire"]:
            self.ghost.on()
            pointList = self.applyPlacement(obj.Points)
            pointList[idx] = pt
            if obj.Closed:
                pointList.append(pointList[0])
            self.ghost.updateFromPointlist(pointList)
        elif Draft.getType(obj) == "BSpline":
            self.ghost.on()
            pointList = self.applyPlacement(obj.Points)
            pointList[idx] = pt
            if obj.Closed:
                pointList.append(pointList[0])
            self.ghost.update(pointList)
        elif Draft.getType(obj) == "BezCurve":
            self.ghost.on()
            plist = self.applyPlacement(obj.Points)
            pointList = self.recomputePointsBezier(obj,plist,idx,pt,obj.Degree,moveTrackers=True)
            self.ghost.update(pointList,obj.Degree)
        elif Draft.getType(obj) == "Circle":
            self.ghost.on()
            self.ghost.setCenter(obj.getGlobalPlacement().Base)
            self.ghost.setRadius(obj.Radius)
            if self.obj.FirstAngle == self.obj.LastAngle:
                # self.obj is a circle
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
                        import DraftVecUtils
                        p1 = self.invpl.multVec(self.obj.Shape.Vertexes[0].Point)
                        p2 = self.invpl.multVec(self.obj.Shape.Vertexes[1].Point)
                        p0 = DraftVecUtils.project(self.invpl.multVec(pt),self.invpl.multVec(self.getArcMid(obj, global_placement=True)))
                        self.ghost.autoinvert=False
                        self.ghost.setRadius(p1.sub(p0).Length)
                        self.ghost.setStartPoint(self.obj.Shape.Vertexes[1].Point)
                        self.ghost.setEndPoint(self.obj.Shape.Vertexes[0].Point)
                        self.ghost.setCenter(self.pl.multVec(p0))
                        return
                    else:
                        p1=self.getArcStart(obj,global_placement=True)
                        p2=self.getArcMid(obj,global_placement=True)
                        p3=self.getArcEnd(obj,global_placement=True)
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
                        self.ghost.setRadius(self.invpl.multVec(pt).Length)
        DraftTools.redraw3DView()

    def applyPlacement(self,pointList):
        if self.pl:
            plist = []
            for p in pointList:
                point = self.pl.multVec(p)
                plist.append(point)
            return plist
        else:
            return pointList

    def finalizeGhost(self):
        try:
            self.ghost.finalize()
            self.ghost = None
        except:
            return

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Add/Delete Vertexes
    #---------------------------------------------------------------------------

    def addPoint(self,event):
        "called by action, add point to obj and reset trackers"
        pos = event.getPosition()
        #self.setSelectState(self.obj, True)
        selobjs = Gui.ActiveDocument.ActiveView.getObjectsInfo((pos[0],pos[1]))
        if not selobjs:
            return
        for info in selobjs:
            if not info:
                return
            for o in self.edited_objects:
                if o.Name != info["Object"]:
                    continue
                self.obj = o
                break
            self.setPlacement(self.obj)
            if Draft.getType(self.obj) == "Wire" and 'Edge' in info["Component"]:
                pt = App.Vector(info["x"], info["y"], info["z"])
                self.addPointToWire(self.obj, pt, int(info["Component"][4:]))
            elif Draft.getType(self.obj) in ["BSpline", "BezCurve"]: #to fix double vertex created
                #pt = self.point
                if "x" in info:# prefer "real" 3D location over working-plane-driven one if possible
                    pt = App.Vector(info["x"], info["y"], info["z"])
                else:
                    continue
                self.addPointToCurve(pt,info)
        self.obj.recompute()
        self.removeTrackers(self.obj)
        self.setEditPoints(self.obj)
        #self.setSelectState(self.obj, False)
        return


    def addPointToWire(self, obj, newPoint, edgeIndex):
        newPoints = []
        hasAddedPoint = False
        if hasattr(obj, "ChamferSize") and hasattr(obj, "FilletRadius"):
            if obj.ChamferSize > 0 and obj.FilletRadius > 0:
                edgeIndex = (edgeIndex +3) / 4
            elif obj.ChamferSize > 0 or obj.FilletRadius > 0:
                edgeIndex = (edgeIndex +1) / 2

        for index, point in enumerate(self.obj.Points):
            if index == edgeIndex:
                hasAddedPoint = True
                newPoints.append(self.invpl.multVec(newPoint))
            newPoints.append(point)
        if obj.Closed and edgeIndex == len(obj.Points):
            # last segment when object is closed
            newPoints.append(self.invpl.multVec(newPoint))
        obj.Points = newPoints

    def addPointToCurve(self,point,info=None):
        import Part
        if not (Draft.getType(self.obj) in ["BSpline","BezCurve"]):
            return
        pts = self.obj.Points
        if Draft.getType(self.obj) == "BezCurve":
            if not info['Component'].startswith('Edge'):
                return # clicked control point
            edgeindex = int(info['Component'].lstrip('Edge')) -1
            wire = self.obj.Shape.Wires[0]
            bz = wire.Edges[edgeindex].Curve
            param = bz.parameter(point)
            seg1 = wire.Edges[edgeindex].copy().Curve
            seg2 = wire.Edges[edgeindex].copy().Curve
            seg1.segment(seg1.FirstParameter,param)
            seg2.segment(param,seg2.LastParameter)
            if edgeindex == len(wire.Edges):
                #we hit the last segment, we need to fix the degree
                degree=wire.Edges[0].Curve.Degree
                seg1.increase(degree)
                seg2.increase(degree)
            edges = wire.Edges[0:edgeindex] + [Part.Edge(seg1),Part.Edge(seg2)] \
                + wire.Edges[edgeindex + 1:]
            pts = edges[0].Curve.getPoles()[0:1]
            for edge in edges:
                pts.extend(edge.Curve.getPoles()[1:])
            if self.obj.Closed:
                pts.pop()
            c = self.obj.Continuity
            # assume we have a tangent continuity for an arbitrarily split
            # segment, unless it's linear
            cont = 1 if (self.obj.Degree >= 2) else 0
            self.obj.Continuity = c[0:edgeindex] + [cont] + c[edgeindex:]
        else:
            if (Draft.getType(self.obj) in ["BSpline"]):
                if (self.obj.Closed == True):
                    curve = self.obj.Shape.Edges[0].Curve
                else:
                    curve = self.obj.Shape.Curve
            uNewPoint = curve.parameter(point)
            uPoints = []
            for p in self.obj.Points:
                uPoints.append(curve.parameter(p))
            for i in range(len(uPoints) -1):
                if ( uNewPoint > uPoints[i] ) and ( uNewPoint < uPoints[i+1] ):
                    pts.insert(i + 1, self.invpl.multVec(point))
                    break
            # DNC: fix: add points to last segment if curve is closed
            if ( self.obj.Closed ) and ( uNewPoint > uPoints[-1] ) :
                pts.append(self.invpl.multVec(point))
        self.obj.Points = pts

    def delPoint(self,event):
        pos = event.getPosition()
        node = self.getEditNode(pos)
        ep = self.getEditNodeIndex(node)

        if ep is None:
            return App.Console.PrintWarning(translate("draft",
                                                          "Node not found")
                                                          + "\n")

        doc = App.getDocument(str(node.documentName.getValue()))
        self.obj = doc.getObject(str(node.objectName.getValue()))
        if self.obj is None:
            return
        if not (Draft.getType(self.obj) in ["Wire","BSpline","BezCurve"]):
            return
        if len(self.obj.Points) <= 2:
            App.Console.PrintWarning(translate("draft", 
                                                   "Active object must have more than two points/nodes") 
                                                   + "\n")
            return

        pts = self.obj.Points
        pts.pop(ep)
        self.obj.Points = pts
        if Draft.getType(self.obj) =="BezCurve":
            self.obj.Proxy.resetcontinuity(self.obj)
        self.obj.recompute()
        
        # don't do tan/sym on DWire/BSpline!
        self.removeTrackers(self.obj)
        self.setEditPoints(self.obj)

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : GENERAL
    #---------------------------------------------------------------------------

    def setEditPoints(self,obj):
        "append given object's editpoints to self.edipoints and set EditTrackers"
        
        self.setPlacement(obj)
        self.editpoints = self.getEditPoints(obj)
        if self.editpoints: # set trackers and align plane
            self.setTrackers(obj, self.editpoints)
            self.editpoints = []

    def getEditPoints(self, obj):
        """
        (object) return a list of App.Vectors relative to object edit nodes
        """
        objectType = Draft.getType(obj)

        if objectType in ["Wire","BSpline"]:
            self.ui.editUi("Wire")
            return self.getWirePts(obj)
        elif objectType == "BezCurve":
            self.ui.editUi("BezCurve")
            self.resetTrackersBezier(obj)
            self.editpoints = []
            return
        elif objectType == "Circle":
            return self.getCirclePts(obj)
        elif objectType == "Rectangle":
            return self.getRectanglePts(obj)
        elif objectType == "Polygon":
            return self.getPolygonPts(obj)
        elif objectType == "Dimension":
            return self.getDimensionPts(obj)
        elif objectType == "Wall":
            return self.getWallPts(obj)            
        elif objectType == "Window":
            return self.getWindowPts(obj)
        elif objectType == "Space":
            return self.getSpacePts(obj)
        elif objectType == "Structure":
            return self.getStructurePts(obj)
        elif objectType == "PanelCut":
            return self.getPanelCutPts(obj)
        elif objectType == "PanelSheet":
            return self.getPanelSheetPts(obj)
        elif objectType == "Part" and obj.TypeId == "Part::Box":
            return self.getPartBoxPts(obj)
        elif objectType == "Part::Line" and obj.TypeId == "Part::Line":
            return self.getPartLinePts(obj)
        elif objectType == "Sketch":
            return self.getSketchPts(obj)
        else:
            return None

    def update(self,obj, nodeIndex, v):
        "apply the App.Vector to the modified point and update self.obj"

        objectType = Draft.getType(obj)
        App.ActiveDocument.openTransaction("Edit")

        if objectType in ["Wire","BSpline"]:
            self.updateWire(obj, nodeIndex, v)
        elif objectType == "BezCurve":
            self.updateWire(obj, nodeIndex, v)
        elif objectType == "Circle":
            self.updateCircle(obj, nodeIndex, v)
        elif objectType == "Rectangle":
            self.updateRectangle(obj, nodeIndex, v)
        elif objectType == "Polygon":
            self.updatePolygon(obj, nodeIndex, v)
        elif objectType == "Dimension":
            self.updateDimension(obj, nodeIndex, v)
        elif objectType == "Sketch":
            self.updateSketch(obj, nodeIndex, v)
        elif objectType == "Wall":
            self.updateWall(obj, nodeIndex, v)
        elif objectType == "Window":
            self.updateWindow(obj, nodeIndex, v)
        elif objectType == "Space":
            self.updateSpace(obj, nodeIndex, v)
        elif objectType == "Structure":
            self.updateStructure(obj, nodeIndex, v)
        elif objectType == "PanelCut":
            self.updatePanelCut(obj, nodeIndex, v)
        elif objectType == "PanelSheet":
            self.updatePanelSheet(obj, nodeIndex, v)
        elif objectType == "Part::Line" and self.obj.TypeId == "Part::Line":
            self.updatePartLine(obj, nodeIndex, v)
        elif objectType == "Part" and self.obj.TypeId == "Part::Box":
            self.updatePartBox(obj, nodeIndex, v)
        
        obj.recompute()

        App.ActiveDocument.commitTransaction()

        try:
            Gui.ActiveDocument.ActiveView.redraw()
        except AttributeError as err:
            pass

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Line/Wire/Bspline/Bezcurve
    #---------------------------------------------------------------------------

    def getWirePts(self,obj):
        editpoints = []
        for p in obj.Points:
            p = obj.getGlobalPlacement().multVec(p)
            editpoints.append(p)
        return editpoints

    def updateWire(self, obj, nodeIndex, v):
        pts = obj.Points
        editPnt = obj.getGlobalPlacement().inverse().multVec(v)
        # DNC: allows to close the curve by placing ends close to each other
        tol = 0.001
        if ( ( nodeIndex == 0 ) and ( (editPnt - pts[-1]).Length < tol) ) or ( 
                nodeIndex == len(pts) - 1 ) and ( (editPnt - pts[0]).Length < tol):
            obj.Closed = True
        # DNC: fix error message if edited point coincides with one of the existing points
        if ( editPnt in pts ) == True: # checks if point enter is equal to other, this could cause a OCC problem
            App.Console.PrintMessage(translate("draft", 
                                               "This object does not support possible "
                                               "coincident points, please try again.")
                                     + "\n")
            if Draft.getType(obj) in ["BezCurve"]:
                self.resetTrackers(obj)
            else:
                self.trackers[obj.Name][nodeIndex].set(obj.getGlobalPlacement().
                    multVec(obj.Points[nodeIndex]))
            return
        if Draft.getType(obj) in ["BezCurve"]:
            pts = self.recomputePointsBezier(obj,pts,nodeIndex,v,obj.Degree,moveTrackers=False)
            
        if obj.Closed:
            # check that the new point lies on the plane of the wire
            if hasattr(obj.Shape,"normalAt"):
                normal = obj.Shape.normalAt(0,0)
                point_on_plane = obj.Shape.Vertexes[0].Point
                print(v)
                v.projectToPlane(point_on_plane, normal)
                print(v)
                editPnt = obj.getGlobalPlacement().inverse().multVec(v)
        pts[nodeIndex] = editPnt
        obj.Points = pts
        self.trackers[obj.Name][nodeIndex].set(v)


    def recomputePointsBezier(self,obj,pts,idx,v,degree,moveTrackers=True):
        """
        (object, Points as list, nodeIndex as Int, App.Vector of new point, moveTrackers as Bool)
        return the new point list, applying the App.Vector to the given index point
        """
        editPnt = v
        # DNC: allows to close the curve by placing ends close to each other
        tol = 0.001
        if ( ( idx == 0 ) and ( (editPnt - pts[-1]).Length < tol) ) or (
                idx == len(pts) - 1 ) and ( (editPnt - pts[0]).Length < tol):
            obj.Closed = True
        # DNC: fix error message if edited point coincides with one of the existing points
        #if ( editPnt in pts ) == False:
        knot = None
        ispole = idx % degree

        if ispole == 0: #knot
            if degree >= 3:
                if idx >= 1: #move left pole
                    knotidx = idx if idx < len(pts) else 0
                    pts[idx-1] = pts[idx-1] + editPnt - pts[knotidx]
                    if moveTrackers:
                        self.trackers[obj.Name][idx-1].set(pts[idx-1])
                if idx < len(pts)-1: #move right pole
                    pts[idx+1] = pts[idx+1] + editPnt - pts[idx]
                    if moveTrackers:
                        self.trackers[obj.Name][idx+1].set(pts[idx+1])
                if idx == 0 and obj.Closed: # move last pole
                    pts[-1] = pts [-1] + editPnt -pts[idx]
                    if moveTrackers:
                        self.trackers[obj.Name][-1].set(pts[-1])

        elif ispole == 1 and (idx >=2 or obj.Closed): #right pole
            knot = idx -1
            changep = idx -2 # -1 in case of closed curve

        elif ispole == degree-1 and idx <= len(pts)-3: #left pole
            knot = idx +1
            changep = idx +2

        elif ispole == degree-1 and obj.Closed and idx == len(pts)-1: #last pole
            knot = 0
            changep = 1

        if knot is not None: # we need to modify the opposite pole
            segment = int(knot / degree) -1
            cont = obj.Continuity[segment] if len(obj.Continuity) > segment else 0
            if cont == 1: #tangent
                pts[changep] = obj.Proxy.modifytangentpole(pts[knot],
                    editPnt,pts[changep])
                if moveTrackers:
                    self.trackers[obj.Name][changep].set(pts[changep])
            elif cont == 2: #symmetric
                pts[changep] = obj.Proxy.modifysymmetricpole(pts[knot],editPnt)
                if moveTrackers:
                    self.trackers[obj.Name][changep].set(pts[changep])
        pts[idx]=v

        return pts #returns the list of new points, taking into account knot continuity

    def resetTrackersBezier(self, obj):
        #in future move tracker definition to DraftTrackers
        from pivy import coin
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
            p,marker = pwm
            #if self.pl: p = self.pl.multVec(p)
            self.trackers[obj.Name].append(editTracker(p,obj.Name,
                index,obj.ViewObject.LineColor,marker=marker))

    def smoothBezPoint(self, obj, point, style='Symmetric'):
        "called when changing the continuity of a knot"
        style2cont = {'Sharp':0,'Tangent':1,'Symmetric':2}
        if point is None:
            return
        if not (Draft.getType(obj) == "BezCurve"):
            return
        pts = obj.Points
        deg = obj.Degree
        if deg < 2:
            return
        if point % deg != 0: #point is a pole
            if deg >=3: #allow to select poles
                if (point % deg == 1) and (point > 2 or obj.Closed): #right pole
                    knot = point -1
                    keepp = point
                    changep = point -2
                elif point < len(pts) -3 and point % deg == deg -1: #left pole
                    knot = point +1
                    keepp = point
                    changep = point +2
                elif point == len(pts)-1 and obj.Closed: #last pole
                    # if the curve is closed the last pole has the last
                    # index in the points lists
                    knot = 0
                    keepp = point
                    changep = 1
                else:
                    App.Console.PrintWarning(translate("draft", 
                                                           "Can't change Knot belonging to pole %d"%point)
                                                           + "\n")
                    return
                if knot:
                    if style == 'Tangent':
                        pts[changep] = obj.Proxy.modifytangentpole(pts[knot],
                            pts[keepp],pts[changep])
                    elif style == 'Symmetric':
                        pts[changep] = obj.Proxy.modifysymmetricpole(pts[knot],
                            pts[keepp])
                    else: #sharp
                        pass #
            else:
                App.Console.PrintWarning(translate("draft", 
                                                       "Selection is not a Knot")
                                                       + "\n")
                return
        else: #point is a knot
            if style == 'Sharp':
                if obj.Closed and point == len(pts)-1:
                    knot = 0
                else:
                    knot = point
            elif style == 'Tangent' and point > 0 and point < len(pts)-1:
                prev, next = obj.Proxy.tangentpoles(pts[point], pts[point-1], pts[point+1])
                pts[point-1] = prev
                pts[point+1] = next
                knot = point #index for continuity
            elif style == 'Symmetric' and point > 0 and point < len(pts)-1:
                prev, next = obj.Proxy.symmetricpoles(pts[point], pts[point-1], pts[point+1])
                pts[point-1] = prev
                pts[point+1] = next
                knot = point #index for continuity
            elif obj.Closed and (style == 'Symmetric' or style == 'Tangent'):
                if style == 'Tangent':
                    pts[1], pts[-1] = obj.Proxy.tangentpoles(pts[0], pts[1], pts[-1])
                elif style == 'Symmetric':
                    pts[1], pts[-1] = obj.Proxy.symmetricpoles(pts[0], pts[1], pts[-1])
                knot = 0
            else:
                App.Console.PrintWarning(translate("draft",
                                                       "Endpoint of BezCurve can't be smoothed")
                                                       + "\n")
                return
        segment = knot // deg #segment index
        newcont = obj.Continuity[:] #don't edit a property inplace !!!
        if not obj.Closed and (len(obj.Continuity) == segment -1 or
            segment == 0) :
            pass # open curve
        elif (len(obj.Continuity) >= segment or obj.Closed and segment == 0 and
                len(obj.Continuity) >1):
            newcont[segment-1] = style2cont.get(style)
        else: #should not happen
            App.Console.PrintWarning('Continuity indexing error:'
                                         + 'point:%d deg:%d len(cont):%d' % (knot,deg,
                                         len(obj.Continuity)))
        obj.Points = pts
        obj.Continuity = newcont
        self.resetTrackers(obj)

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Rectangle
    #---------------------------------------------------------------------------

    def getRectanglePts(self, obj):
        """
        returns the list of edipoints for the given Draft Rectangle
        0 : Placement.Base
        1 : Length
        2 : Height
        """
        editpoints = []
        editpoints.append(obj.getGlobalPlacement().Base)
        editpoints.append(obj.getGlobalPlacement().multVec(App.Vector(obj.Length,0,0)))
        editpoints.append(obj.getGlobalPlacement().multVec(App.Vector(0,obj.Height,0)))
        return editpoints

    def updateRectangleTrackers(self, obj):
        self.trackers[obj.Name][0].set(obj.getGlobalPlacement().Base)
        self.trackers[obj.Name][1].set(obj.getGlobalPlacement().multVec(App.Vector(obj.Length,0,0)))
        self.trackers[obj.Name][2].set(obj.getGlobalPlacement().multVec(App.Vector(0,obj.Height,0)))

    def updateRectangle(self, obj, nodeIndex, v):
        import DraftVecUtils
        delta = obj.getGlobalPlacement().inverse().multVec(v)
        if nodeIndex == 0:
            #p = obj.getGlobalPlacement()
            #p.move(delta)
            obj.Placement.move(delta)
        elif self.editing == 1:
            obj.Length = DraftVecUtils.project(delta,App.Vector(1,0,0)).Length
        elif self.editing == 2:
            obj.Height = DraftVecUtils.project(delta,App.Vector(0,1,0)).Length
        self.updateRectangleTrackers(obj)

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Ellipse (# TODO: yet to be implemented)
    #---------------------------------------------------------------------------

    def setEllipsePts(self):
        return

    def updateEllipse(self,v):
        return

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Circle/Arc
    #---------------------------------------------------------------------------

    def getCirclePts(self, obj):
        """
        returns the list of edipoints for the given Draft Arc or Circle
        circle:
        0 : Placement.Base or center
        1 : radius
        arc:
        0 : Placement.Base or center
        1 : first endpoint
        2 : second endpoint
        3 : midpoint
        """        
        editpoints = []
        editpoints.append(obj.getGlobalPlacement().Base)
        if obj.FirstAngle == obj.LastAngle:
            # obj is a circle
            self.ui.editUi("Circle")
            editpoints.append(obj.getGlobalPlacement().multVec(App.Vector(obj.Radius,0,0)))
        else:
            # obj is an arc
            self.ui.editUi("Arc")
            editpoints.append(self.getArcStart(obj, global_placement=True))#First endpoint
            editpoints.append(self.getArcEnd(obj, global_placement=True))#Second endpoint
            editpoints.append(self.getArcMid(obj, global_placement=True))#Midpoint
        return editpoints

    def updateCircleTrackers(self, obj):
        self.trackers[obj.Name][0].set(obj.getGlobalPlacement().Base)
        self.trackers[obj.Name][1].set(self.getArcStart(obj, global_placement=True))
        if len(self.trackers[obj.Name]) > 2: 
            # object is an arc
            self.trackers[obj.Name][2].set(self.getArcEnd(obj, global_placement=True))
            self.trackers[obj.Name][3].set(self.getArcMid(obj, global_placement=True))

    def updateCircle(self, obj, nodeIndex, v):
        delta = obj.getGlobalPlacement().inverse().multVec(v)
        local_v = obj.Placement.multVec(delta)

        if obj.FirstAngle == obj.LastAngle:
            # object is a circle
            if nodeIndex == 0:
                obj.Placement.Base = local_v
            elif nodeIndex == 1:
                obj.Radius = delta.Length

        else:
            # obj is an arc
            if self.alt_edit_mode == 0:
                # edit arc by 3 points
                import Part
                if nodeIndex == 0:
                    #center point
                    import DraftVecUtils
                    p1 = self.getArcStart(obj)
                    p2 = self.getArcEnd(obj)
                    p0 = DraftVecUtils.project(delta,self.getArcMid(obj))
                    obj.Radius = p1.sub(p0).Length
                    obj.FirstAngle = -math.degrees(DraftVecUtils.angle(p1.sub(p0)))
                    obj.LastAngle = -math.degrees(DraftVecUtils.angle(p2.sub(p0)))
                    obj.Placement.Base = obj.Placement.multVec(p0)
                    self.setPlacement(obj)

                else:
                    if nodeIndex == 1:#first point
                        p1=v
                        p2=self.getArcMid(obj,global_placement=True)
                        p3=self.getArcEnd(obj,global_placement=True)
                    elif nodeIndex == 3:#midpoint
                        p1=self.getArcStart(obj,global_placement=True)
                        p2=v
                        p3=self.getArcEnd(obj,global_placement=True)
                    elif nodeIndex == 2:#second point
                        p1=self.getArcStart(obj,global_placement=True)
                        p2=self.getArcMid(obj,global_placement=True)
                        p3=v
                    arc=Part.ArcOfCircle(p1,p2,p3)
                    obj.Placement.Base = obj.Placement.multVec(obj.getGlobalPlacement().inverse().multVec(arc.Location))
                    self.setPlacement(obj)
                    obj.Radius = arc.Radius
                    delta = self.invpl.multVec(p1)
                    obj.FirstAngle = math.degrees(math.atan2(delta[1],delta[0]))
                    delta = self.invpl.multVec(p3)
                    obj.LastAngle = math.degrees(math.atan2(delta[1],delta[0]))

            elif self.alt_edit_mode == 1:
                # edit arc by center radius FirstAngle LastAngle
                if nodeIndex == 0:
                    obj.Placement.Base = local_v
                    self.setPlacement(obj)
                else:
                    dangle = math.degrees(math.atan2(delta[1],delta[0]))
                    if nodeIndex == 1:
                        obj.FirstAngle = dangle
                    elif nodeIndex == 2:
                        obj.LastAngle = dangle
                    elif nodeIndex == 3:
                        obj.Radius = delta.Length

        obj.recompute()
        self.updateCircleTrackers(obj)


    def getArcStart(self, obj, global_placement=False):#Returns object midpoint
        if Draft.getType(obj) == "Circle":
            return self.pointOnCircle(obj, obj.FirstAngle, global_placement)
    
    def getArcEnd(self, obj, global_placement=False):#Returns object midpoint
        if Draft.getType(obj) == "Circle":
            return self.pointOnCircle(obj, obj.LastAngle, global_placement)

    def getArcMid(self, obj, global_placement=False):#Returns object midpoint
        if Draft.getType(obj) == "Circle":
            if obj.LastAngle > obj.FirstAngle:
                midAngle = obj.FirstAngle + (obj.LastAngle - obj.FirstAngle) / 2.0
            else:
                midAngle = obj.FirstAngle + (obj.LastAngle - obj.FirstAngle) / 2.0
                midAngle += App.Units.Quantity(180,App.Units.Angle)
            return self.pointOnCircle(obj, midAngle, global_placement)

    def pointOnCircle(self, obj, angle, global_placement=False):
        if Draft.getType(obj) == "Circle":
            px = obj.Radius * math.cos(math.radians(angle))
            py = obj.Radius * math.sin(math.radians(angle))
            p = App.Vector(px, py, 0.0)
            if global_placement == True:
                p = obj.getGlobalPlacement().multVec(p)
            return p
        return None

    def arcInvert(self, obj):
        obj.FirstAngle, obj.LastAngle = obj.LastAngle, obj.FirstAngle
        obj.recompute()
        self.updateCircleTrackers(obj)

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Polygon (maybe could also rotate the polygon)
    #---------------------------------------------------------------------------

    def getPolygonPts(self, obj):
        editpoints = []
        editpoints.append(obj.Placement.Base)
        editpoints.append(obj.Shape.Vertexes[0].Point)
        return editpoints

    def updatePolygon(self, obj, nodeIndex, v):
        delta = v.sub(self.obj.Placement.Base)
        if self.editing == 0:
            p = self.obj.Placement
            p.move(delta)
            self.obj.Placement = p
            self.trackers[self.obj.Name][0].set(self.obj.Placement.Base)
        elif self.editing == 1:
            if self.obj.DrawMode == 'inscribed':
                self.obj.Radius = delta.Length
            else:
                halfangle = ((math.pi*2)/self.obj.FacesNumber)/2
                rad = math.cos(halfangle)*delta.Length
                self.obj.Radius = rad
            self.obj.recompute()
        self.trackers[self.obj.Name][1].set(self.obj.Shape.Vertexes[0].Point)

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Dimension (point on dimension line is not clickable)
    #---------------------------------------------------------------------------

    def getDimensionPts(self, obj):
        editpoints = []
        p = obj.ViewObject.Proxy.textpos.translation.getValue()
        editpoints.append(obj.Start)
        editpoints.append(obj.End)
        editpoints.append(obj.Dimline)
        editpoints.append(App.Vector(p[0],p[1],p[2]))
        return editpoints

    def updateDimension(self, obj, nodeIndex, v):
        if self.editing == 0:
            self.obj.Start = v
        elif self.editing == 1:
            self.obj.End = v
        elif self.editing == 2:
            self.obj.Dimline = v
        elif self.editing == 3:
            self.obj.ViewObject.TextPosition = v

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : ARCH Wall, Windows, Structure, Panel, etc.
    #---------------------------------------------------------------------------

    # SKETCH: just if it's composed by a single segment-------------------------

    def getSketchPts(self, obj):
        """
        returns the list of edipoints for the given single line sketch (WallTrace)
        0 : startpoint
        1 : endpoint
        """
        editpoints = []
        if obj.GeometryCount == 1:
            editpoints.append(obj.getGlobalPlacement().multVec(obj.getPoint(0,1)))
            editpoints.append(obj.getGlobalPlacement().multVec(obj.getPoint(0,2)))
            return editpoints
        else:
            App.Console.PrintWarning(translate("draft",
                                                   "Sketch is too complex to edit: "
                                                   "it is suggested to use sketcher default editor")
                                     + "\n")
            return None

    def updateSketch(self, obj, nodeIndex, v):
        """
        (single segment sketch object, node index as Int, App.Vector)
        move a single line sketch (WallTrace) vertex according to a given App.Vector
        0 : startpoint
        1 : endpoint
        """
        if nodeIndex == 0:
            obj.movePoint(0,1,obj.getGlobalPlacement().inverse().multVec(v))
        elif nodeIndex == 1:
            obj.movePoint(0,2,obj.getGlobalPlacement().inverse().multVec(v))
        obj.recompute()


    #WALL-----------------------------------------------------------------------

    def getWallPts(self, obj):
        """
        returns the list of edipoints for the given Arch Wall object
        0 : height of the wall
        1-to end : base object editpoints, in place with the wall
        """
        editpoints = []
        #height of the wall
        editpoints.append(obj.getGlobalPlacement().multVec(App.Vector(0,0,obj.Height)))
        # try to add here an editpoint based on wall height (maybe should be good to associate it with a circular tracker)
        if obj.Base:
            # base points are added to self.trackers under wall-name key
            basepoints = []
            if Draft.getType(obj.Base) in ["Wire","Circle","Rectangle",
                                            "Polygon", "Sketch"]:
                basepoints = self.getEditPoints(obj.Base)
                for point in basepoints:
                    editpoints.append(obj.Placement.multVec(point)) #works ok except if App::Part is rotated... why?
        return editpoints


    def updateWallTrackers(self, obj):
        """
        update self.trackers[obj.Name][0] to match with given object
        """
        pass

    def updateWall(self, obj, nodeIndex, v):
        import DraftVecUtils
        if nodeIndex == 0:
            delta= obj.getGlobalPlacement().inverse().multVec(v)
            vz=DraftVecUtils.project(delta,App.Vector(0,0,1))
            if vz.Length > 0:
                obj.Height = vz.Length
        elif nodeIndex > 0:
            if obj.Base:
                if Draft.getType(obj.Base) in ["Wire","Circle","Rectangle",
                                                "Polygon", "Sketch"]:
                    self.update(obj.Base, nodeIndex - 1, 
                        obj.Placement.inverse().multVec(v))
        obj.recompute()


    #WINDOW---------------------------------------------------------------------

    def getWindowPts(self, obj):
        import DraftGeomUtils
        editpoints = []
        pos = obj.Base.Placement.Base
        h = float(obj.Height) + pos.z
        normal = obj.Normal
        angle = normal.getAngle(App.Vector(1, 0, 0))
        editpoints.append(pos)
        editpoints.append(App.Vector(pos.x + float(obj.Width) * math.cos(angle-math.pi / 2.0),
                                                pos.y + float(obj.Width) * math.sin(angle-math.pi / 2.0),
                                                pos.z))
        editpoints.append(App.Vector(pos.x, pos.y, h))
        return editpoints

    def updateWindow(self, obj, nodeIndex, v):
        pos=self.obj.Base.Placement.Base
        if self.editing == 0:
            self.obj.Base.Placement.Base=v
            self.obj.Base.recompute()
        if self.editing == 1:
            self.obj.Width = pos.sub(v).Length
            self.obj.Base.recompute()
        if self.editing == 2:
            self.obj.Height = pos.sub(v).Length
            self.obj.Base.recompute()
        for obj in self.obj.Hosts:
            obj.recompute()
        self.obj.recompute()

    #STRUCTURE-------------------------------------------------------------------

    def getStructurePts(self, obj):
        if obj.Nodes:
            editpoints = []
            self.originalDisplayMode = obj.ViewObject.DisplayMode
            self.originalPoints = obj.ViewObject.NodeSize
            self.originalNodes = obj.ViewObject.ShowNodes
            self.obj.ViewObject.DisplayMode = "Wireframe"
            self.obj.ViewObject.NodeSize = 1
            #   self.obj.ViewObject.ShowNodes = True
            for p in obj.Nodes:
                if self.pl:
                    p = self.pl.multVec(p)
                editpoints.append(p)
            return editpoints
        else:
            return None

    def updateStructure(self, obj, nodeIndex, v):
        nodes = self.obj.Nodes
        nodes[self.editing] = self.invpl.multVec(v)
        self.obj.Nodes = nodes

    #SPACE----------------------------------------------------------------------

    def getSpacePts(self, obj):
        try:
            editpoints = []
            self.editpoints.append(obj.ViewObject.Proxy.getTextPosition(obj.ViewObject))
            return editpoints
        except:
            pass

    def updateSpace(self, obj, nodeIndex, v):
        if self.editing == 0:
            self.obj.ViewObject.TextPosition = v

    #PANELS---------------------------------------------------------------------

    def getPanelCutPts(self, obj):
        editpoints = []
        if self.obj.TagPosition.Length == 0:
            pos = obj.Shape.BoundBox.Center
        else:
            pos = self.pl.multVec(obj.TagPosition)
        editpoints.append(pos)
        return editpoints

    def updatePanelCut(self, obj, nodeIndex, v):
        if self.editing == 0:
            self.obj.TagPosition = self.invpl.multVec(v)

    def getPanelSheetPts(self, obj):
        editpoints = []
        editpoints.append(self.pl.multVec(obj.TagPosition))
        for o in obj.Group:
            editpoints.append(self.pl.multVec(o.Placement.Base))
        return editpoints

    def updatePanelSheet(self, obj, nodeIndex, v):
        if self.editing == 0:
            self.obj.TagPosition = self.invpl.multVec(v)
        else:
            self.obj.Group[self.editing-1].Placement.Base = self.invpl.multVec(v)
    
    # PART::LINE----------------------------------------------------------------

    def getPartLinePts(self, obj):
        editpoints = []
        editpoints.append(self.pl.multVec(App.Vector(obj.X1,obj.Y1,obj.Z1)))
        editpoints.append(self.pl.multVec(App.Vector(obj.X2,obj.Y2,obj.Z2)))
        return editpoints
    
    def updatePartLine(self, obj, nodeIndex, v):
        pt=self.invpl.multVec(v)
        if self.editing == 0:
            self.obj.X1 = pt.x
            self.obj.Y1 = pt.y
            self.obj.Z1 = pt.z
        elif self.editing == 1:
            self.obj.X2 = pt.x
            self.obj.Y2 = pt.y
            self.obj.Z2 = pt.z

    # PART::BOX-----------------------------------------------------------------

    def getPartBoxPts(self, obj):
        editpoints = []
        editpoints.append(obj.Placement.Base)
        editpoints.append(self.pl.multVec(App.Vector(obj.Length,0,0)))
        editpoints.append(self.pl.multVec(App.Vector(0,obj.Width,0)))
        editpoints.append(self.pl.multVec(App.Vector(0,0,obj.Height)))
        return editpoints

    def updatePartBox(self, obj, nodeIndex, v):
        import DraftVecUtils
        delta = self.invpl.multVec(v)
        if self.editing == 0:
            self.obj.Placement.Base = v
            self.setPlacement(self.obj)
        elif self.editing == 1:
            xApp.Vector = DraftVecUtils.project(delta,App.Vector(1,0,0))
            self.obj.Length = xApp.Vector.Length            
        elif self.editing == 2:
            xApp.Vector = DraftVecUtils.project(delta,App.Vector(0,1,0))
            self.obj.Width = xApp.Vector.Length            
        elif self.editing == 3:
            xApp.Vector = DraftVecUtils.project(delta,App.Vector(0,0,1))
            self.obj.Height = xApp.Vector.Length            
        self.trackers[self.obj.Name][0].set(self.obj.Placement.Base)
        self.trackers[self.obj.Name][1].set(self.pl.multVec(App.Vector(self.obj.Length,0,0)))
        self.trackers[self.obj.Name][2].set(self.pl.multVec(App.Vector(0,self.obj.Width,0)))
        self.trackers[self.obj.Name][3].set(self.pl.multVec(App.Vector(0,0,self.obj.Height)))

    #---------------------------------------------------------------------------
    # Context menu
    #---------------------------------------------------------------------------

    def display_tracker_menu(self, event):
        self.tracker_menu = QtGui.QMenu()
        self.event = event
        actions = None
        if self.overNode:
            # if user is over a node
            doc = self.overNode.get_doc_name()
            obj = App.getDocument(doc).getObject(self.overNode.get_obj_name())
            ep = self.overNode.get_subelement_index()
            if Draft.getType(obj) in ["Line", "Wire"]:
                actions = ["delete point"]
            elif Draft.getType(obj) in ["Circle"]:
                if obj.FirstAngle != obj.LastAngle:
                    if ep == 0: # user is over arc start point
                        actions = ["move arc"]
                    elif ep == 1: # user is over arc start point
                        actions = ["set first angle"]
                    elif ep == 2: # user is over arc end point
                        actions = ["set last angle"]
                    elif ep == 3: # user is over arc mid point
                        actions = ["set radius"]
            elif Draft.getType(obj) in ["BezCurve"]:
                actions = ["make sharp", "make tangent", "make symmetric", "delete point"]
            else:
                return
        else:
            # if user is over an edited object
            pos = self.event.getPosition()
            obj = self.get_selected_obj_at_position(pos)
            if Draft.getType(obj) in ["Line", "Wire","BSpline", "BezCurve"]:
                actions = ["add point"]
            elif Draft.getType(obj) in ["Circle"] and obj.FirstAngle != obj.LastAngle:
                actions = ["invert arc"]
        if actions is None:
            return
        for a in actions:
            self.tracker_menu.addAction(a)
        self.tracker_menu.popup(Gui.getMainWindow().cursor().pos())
        QtCore.QObject.connect(self.tracker_menu,QtCore.SIGNAL("triggered(QAction *)"),self.evaluate_menu_action)

    def evaluate_menu_action(self,labelname):
        action_label = str(labelname.text())
        # Bezier curve menu
        if action_label in ["make sharp", "make tangent", "make symmetric"]:
            doc = self.overNode.get_doc_name()
            obj = App.getDocument(doc).getObject(self.overNode.get_obj_name())
            idx = self.overNode.get_subelement_index()
            if action_label == "make sharp":
                self.smoothBezPoint(obj, idx, 'Sharp')
            elif action_label == "make tangent":
                self.smoothBezPoint(obj, idx, 'Tangent')
            elif action_label == "make symmetric":
                self.smoothBezPoint(obj, idx, 'Symmetric')
        # addPoint and deletePoint menu
        elif action_label == "delete point":
            self.delPoint(self.event)
        elif action_label == "add point":
            self.addPoint(self.event)
        # arc tools
        elif action_label in ["move arc","set radius", "set first angle", "set last angle"]:
            self.alt_edit_mode = 1
            self.startEditing(self.event)
        elif action_label == "invert arc":
            pos = self.event.getPosition()
            obj = self.get_selected_obj_at_position(pos)
            self.arcInvert(obj)
        del self.event



if App.GuiUp:
    # setup command
    Gui.addCommand('Draft_Edit', Edit())
