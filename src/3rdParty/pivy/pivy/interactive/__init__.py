"""Interactive 3D graphics objects and scene management.

This module provides classes for creating interactive 3D graphics objects
that can be selected, dragged, highlighted, and manipulated in a Coin3D scene.
It includes basic shapes (points, lines, polygons, markers, arrows) and
an interaction system for handling mouse and keyboard events.
"""

from pivy import coin
from pivy.utils import get_point_on_screen
from .colors import COLORS
from .plot import plot
from .mesh import simple_quad_mesh, simple_poly_mesh


class Object3D(coin.SoSeparator):
    """Base class for interactive 3D graphics objects.

    Provides common functionality for 3D objects including color management,
    selection highlighting, mouse-over effects, and drag-and-drop interaction.
    Objects can be enabled/disabled and maintain their own state for interaction.

    Attributes:
        std_col: Standard color name (default: "black").
        ovr_col: Mouse-over color name (default: "red").
        sel_col: Selection color name (default: "yellow").
        non_col: Disabled color name (default: "grey").
        data: SoCoordinate3 node storing the object's 3D points.
        color: SoMaterial node controlling the object's appearance.
        dynamic: Boolean flag indicating if the object can be dynamically modified.
        enabled: Boolean flag indicating if the object is currently enabled.
        on_drag: List of callback functions called during drag operations.
        on_drag_release: List of callback functions called when drag ends.
        on_drag_start: List of callback functions called when drag begins.

    Example:
        >>> obj = Object3D([(0, 0, 0), (1, 1, 1)], dynamic=True)
        >>> obj.set_color("blue")
        >>> obj.select()
    """
    std_col = "black"
    ovr_col = "red"
    sel_col = "yellow"
    non_col = "grey"

    def __init__(self, points, dynamic=False):
        super(Object3D, self).__init__()
        self.data = coin.SoCoordinate3()
        self.color = coin.SoMaterial()
        self.set_color()
        self += [self.color, self.data]
        self.start_pos = None
        self.dynamic = dynamic

        # callback function lists
        self.on_drag = []
        self.on_drag_release = []
        self.on_drag_start = []

        self._delete = False
        self._tmp_points = None
        self.enabled = True
        self.points = points

    def set_disabled(self):
        """Disable the object and change its color to the disabled color."""
        self.color.diffuseColor = COLORS[self.non_col]
        self.enabled = False

    def set_enabled(self):
        """Enable the object and restore its standard color."""
        self.color.diffuseColor = COLORS[self.std_col]
        self.enabled = True

    def set_color(self, col=None):
        """Set the object's standard color.

        Args:
            col: Optional color name string. If None, uses the current std_col.
        """
        self.std_col = col or self.std_col
        self.color.diffuseColor = COLORS[self.std_col]

    @property
    def points(self):
        """Get the object's 3D points.

        Returns:
            List of 3D point tuples (x, y, z).
        """
        return self.data.point.getValues()

    @points.setter
    def points(self, points):
        """Set the object's 3D points.

        Args:
            points: List of 3D points (x, y, z tuples). All points must be 3D.

        Raises:
            AssertionError: If points are not 3D tuples.
        """
        # check if we got a list of 3D points
        assert(len(points[0]) == len(points[-1]) == 3)
        self.data.point.setValue(0, 0, 0)
        self.data.point.setValues(0, len(points), points)

    def set_mouse_over(self):
        """Set mouse-over highlighting (changes color to ovr_col)."""
        if self.enabled:
            self.color.diffuseColor = COLORS[self.ovr_col]

    def unset_mouse_over(self):
        """Remove mouse-over highlighting (restores standard color)."""
        if self.enabled:
            self.color.diffuseColor = COLORS[self.std_col]

    def select(self):
        """Select the object (changes color to sel_col)."""
        if self.enabled:
            self.color.diffuseColor = COLORS[self.sel_col]

    def unselect(self):
        """Deselect the object (restores standard color)."""
        if self.enabled:
            self.color.diffuseColor = COLORS[self.std_col]

    def drag(self, mouse_coords, fact=1.):
        """Drag the object by updating its points based on mouse movement.

        Args:
            mouse_coords: 3D coordinate tuple (x, y, z) representing mouse position.
            fact: Scaling factor for the drag movement (default: 1.0).
        """
        if self.enabled:
            pts = self.points
            for i, pt in enumerate(pts):
                pt[0] = mouse_coords[0] * fact + self._tmp_points[i][0]
                pt[1] = mouse_coords[1] * fact + self._tmp_points[i][1]
                pt[2] = mouse_coords[2] * fact + self._tmp_points[i][2]
            self.points = pts
            for foo in self.on_drag:
                foo()

    def drag_release(self):
        """Handle the end of a drag operation.

        Calls all registered on_drag_release callbacks.
        """
        if self.enabled:
            for foo in self.on_drag_release:
                foo()

    def drag_start(self):
        """Handle the start of a drag operation.

        Saves the current points and calls all registered on_drag_start callbacks.
        """
        self._tmp_points = self.points
        if self.enabled:
            for foo in self.on_drag_start:
                foo()

    @property
    def drag_objects(self):
        """Get list of objects that should be dragged when this object is dragged.

        Returns:
            List containing self if enabled, empty list otherwise.
        """
        if self.enabled:
            return [self]
        return []

    def delete(self):
        """Mark the object for deletion.

        Sets the internal _delete flag. Actual removal is handled by
        the InteractionSeparator's remove_selected method.
        """
        if self.enabled and not self._delete:
            self._delete = True

    def check_dependency(self):
        """Check object dependencies before deletion.

        Override this method in subclasses to implement dependency checking.
        By default, does nothing.
        """
        pass


class Marker(Object3D):
    """A marker object displaying filled circle markers at specified points.

    Inherits from Object3D and adds SoMarkerSet rendering with filled circle markers.
    Useful for marking specific locations in a 3D scene.

    Example:
        >>> marker = Marker([(0, 0, 0), (1, 1, 1)])
    """
    def __init__(self, points, dynamic=False):
        """Initialize a marker object.

        Args:
            points: List of 3D points where markers should be displayed.
            dynamic: If True, markers can be modified during interaction.
        """
        super(Marker, self).__init__(points, dynamic)
        self.marker = coin.SoMarkerSet()
        self.marker.markerIndex = coin.SoMarkerSet.CIRCLE_FILLED_9_9
        self.addChild(self.marker)


class Line(Object3D):
    """A line object connecting points in 3D space.

    Renders a polyline connecting the provided points using SoLineSet.

    Example:
        >>> line = Line([(0, 0, 0), (1, 1, 1), (2, 0, 1)])
    """
    def __init__(self, points, dynamic=False):
        """Initialize a line object.

        Args:
            points: List of 3D points defining the line vertices.
            dynamic: If True, line can be modified during interaction.
        """
        super(Line, self).__init__(points, dynamic)
        self.drawstyle = coin.SoDrawStyle()
        self.line = coin.SoLineSet()
        self.addChild(self.drawstyle)
        self.addChild(self.line)

class Point(Object3D):
    """A point set object displaying individual points in 3D space.

    Renders points using SoPointSet. Each point in the input list is displayed
    as a single point in the scene.

    Example:
        >>> points = Point([(0, 0, 0), (1, 1, 1), (2, 2, 2)])
    """
    def __init__(self, points, dynamic=False):
        """Initialize a point set object.

        Args:
            points: List of 3D points to display.
            dynamic: If True, points can be modified during interaction.
        """
        super(Point, self).__init__(points, dynamic)
        self.drawstyle = coin.SoDrawStyle()
        self.point = coin.SoPointSet()
        self.addChild(self.drawstyle)
        self.addChild(self.point)

class Polygon(Object3D):
    """A polygon object displaying a filled face in 3D space.

    Renders a polygon face using SoFaceSet. The points define the vertices
    of the polygon in order.

    Example:
        >>> poly = Polygon([(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)])
    """
    def __init__(self, points, dynamic=False):
        """Initialize a polygon object.

        Args:
            points: List of 3D points defining the polygon vertices.
            dynamic: If True, polygon can be modified during interaction.
        """
        super(Polygon, self).__init__(points, dynamic)
        self.polygon = coin.SoFaceSet()
        self.addChild(self.polygon)

class Arrow(Line):
    """An arrow object with a cone-shaped arrowhead.

    Extends Line to add a 3D arrowhead (cone) at the end of the line.
    The arrow direction is automatically calculated from the last two points.

    Attributes:
        arrow_size: Scale factor for the arrowhead size (default: 0.04).
        length: Length scaling factor for the arrowhead cone (default: 2).

    Example:
        >>> arrow = Arrow([(0, 0, 0), (1, 1, 1)], arrow_size=0.05)
    """
    def __init__(self, points, dynamic=False, arrow_size=0.04, length=2):
        """Initialize an arrow object.

        Args:
            points: List of 3D points. The arrowhead points from the second-to-last
                to the last point.
            dynamic: If True, arrow can be modified during interaction.
            arrow_size: Scale factor for the arrowhead size (default: 0.04).
            length: Length scaling factor for the arrowhead cone (default: 2).
        """
        super(Arrow, self).__init__(points, dynamic)
        self.arrow_sep = coin.SoSeparator()
        self.arrow_rot = coin.SoRotation()
        self.arrow_scale = coin.SoScale()
        self.arrow_translate = coin.SoTranslation()
        self.arrow_scale.scaleFactor.setValue(arrow_size, arrow_size, arrow_size)
        self.cone = coin.SoCone()
        arrow_length = coin.SoScale()
        arrow_length.scaleFactor = (1, length, 1)
        arrow_origin = coin.SoTranslation()
        arrow_origin.translation = (0, -1, 0)
        self.arrow_sep += [self.arrow_translate, self.arrow_rot, self.arrow_scale]
        self.arrow_sep += [arrow_length, arrow_origin, self.cone]
        self += [self.arrow_sep]
        self.set_arrow_direction()

    def set_arrow_direction(self):
        """Update the arrowhead direction based on the current points.

        Calculates the direction vector from the second-to-last to the last point
        and rotates the arrowhead cone to align with this direction.
        """
        pts = self.points
        self.arrow_translate.translation = tuple(pts[-1])
        direction = pts[-1] - pts[-2]
        direction.normalize()
        _rot = coin.SbRotation()
        _rot.setValue(coin.SbVec3f(0, 1, 0), coin.SbVec3f(*direction))
        self.arrow_rot.rotation.setValue(_rot)

class InteractionSeparator(coin.SoSeparator):
    """A scene graph separator that handles interactive object manipulation.

    Manages selection, highlighting, dragging, and deletion of Object3D instances
    in a 3D scene. Handles mouse and keyboard events to provide interactive
    manipulation capabilities.

    Attributes:
        pick_radius: Radius in pixels for picking objects (default: 10).
        render_manager: The render manager providing viewport information.
        objects: SoSeparator containing all graphics objects.
        dynamic_objects: List of objects marked as dynamic.
        static_objects: List of objects marked as static.
        over_object: Currently highlighted object (mouse-over).
        selected_objects: List of currently selected objects.
        drag_objects: Set of objects currently being dragged.
        on_drag: List of callback functions called during drag operations.
        on_drag_release: List of callback functions called when drag ends.
        on_drag_start: List of callback functions called when drag begins.

    Example:
        >>> sep = InteractionSeparator(render_manager)
        >>> sep.addChild(Line([(0, 0, 0), (1, 1, 1)], dynamic=True))
        >>> sep.register()
    """
    pick_radius = 10

    def __init__(self, render_manager):
        """Initialize the interaction separator.

        Args:
            render_manager: Render manager instance providing viewport region
                and scene graph access.
        """
        super(InteractionSeparator, self).__init__()
        self.render_manager = render_manager
        self.objects = coin.SoSeparator()
        self.dynamic_objects = []
        self.static_objects = []
        self.over_object = None
        self.selected_objects = []
        self.drag_objects = []

        self.on_drag = []
        self.on_drag_release = []
        self.on_drag_start = []

        self._direction = None

        self.events = coin.SoEventCallback()
        self += self.events, self.objects

    def register(self):
        """Register event callbacks for interaction.

        Sets up callbacks for mouse movement (highlighting), mouse clicks (selection),
        keyboard 'g' key (grab/drag), keyboard delete key, and keyboard 'a' key
        (select all).
        """
        self._highlight_cb = self.events.addEventCallback(
            coin.SoLocation2Event.getClassTypeId(), self.highlight_cb)
        self._select_cb = self.events.addEventCallback(
            coin.SoMouseButtonEvent.getClassTypeId(), self.select_cb)
        self._grab_cb = self.events.addEventCallback(
            coin.SoKeyboardEvent.getClassTypeId(), self.grab_cb)
        self._delete_cb = self.events.addEventCallback(
            coin.SoKeyboardEvent.getClassTypeId(), self.delete_cb)
        self._select_all_cb = self.events.addEventCallback(
            coin.SoKeyboardEvent.getClassTypeId(), self.select_all_cb)

    def unregister(self):
        """Unregister all event callbacks.

        Removes all previously registered event callbacks, typically called
        before starting a drag operation or when disabling interaction.
        """
        self.events.removeEventCallback(
            coin.SoLocation2Event.getClassTypeId(), self._highlight_cb)
        self.events.removeEventCallback(
            coin.SoMouseButtonEvent.getClassTypeId(), self._select_cb)
        self.events.removeEventCallback(
            coin.SoKeyboardEvent.getClassTypeId(), self._grab_cb)
        self.events.removeEventCallback(
            coin.SoKeyboardEvent.getClassTypeId(), self._delete_cb)
        self.events.removeEventCallback(
            coin.SoKeyboardEvent.getClassTypeId(), self._select_all_cb)
 

    #-----------------------HIGHLIGHTING-----------------------#
    # a SoLocation2Event calling a function which sends rays   #
    # into the scene. This will return the object the mouse is #
    # currently hoovering.                                     #

    def highlight_object(self, obj):
        """Highlight an object under the mouse cursor.

        Args:
            obj: Object3D instance to highlight, or None to clear highlighting.
        """
        if self.over_object:
            self.over_object.unset_mouse_over()
        self.over_object = obj
        if self.over_object:
            self.over_object.set_mouse_over()
        self.color_selected()

    def highlight_cb(self, attr, event_callback):
        """Callback for mouse movement events to update highlighting.

        Args:
            attr: Event callback attribute (unused).
            event_callback: Event callback instance containing the event.
        """
        event = event_callback.getEvent()
        pos = event.getPosition()
        obj = self.send_ray(pos)
        self.highlight_object(obj)

    def send_ray(self, mouse_pos):
        """Send a ray through the scene and return the nearest picked object.

        Performs ray picking from the mouse position into the scene graph
        and returns the first Object3D instance found.

        Args:
            mouse_pos: 2D screen coordinates tuple (x, y).

        Returns:
            Object3D instance or None if no object was picked.
        """
        ray_pick = coin.SoRayPickAction(self.render_manager.getViewportRegion())
        ray_pick.setPoint(coin.SbVec2s(*mouse_pos))
        ray_pick.setRadius(10)
        ray_pick.setPickAll(True)
        ray_pick.apply(self.render_manager.getSceneGraph())
        picked_point = ray_pick.getPickedPointList()
        return self.obj_by_id(picked_point)

    def obj_by_id(self, picked_point):
        """Find an Object3D instance from a picked point list.

        Searches through the picked point list to find a matching Object3D
        instance by comparing node IDs.

        Args:
            picked_point: List of SoPickedPoint instances from ray picking.

        Returns:
            Object3D instance or None if no match found.
        """
        for point in picked_point:
            path = point.getPath()
            length = path.getLength()
            point = path.getNode(length - 2)
            point = list(filter(
                lambda ctrl: ctrl.getNodeId() == point.getNodeId(),
                self.dynamic_objects))
            if point != []:
                return point[0]
        return None
        


#------------------------SELECTION------------------------#
    def select_object(self, obj, multi=False):
        """Select or deselect an object.

        Args:
            obj: Object3D instance to select/deselect, or None to clear selection.
            multi: If True, allows multiple selection (toggle behavior).
                If False, clears previous selection first.
        """
        if not multi:
            for o in self.selected_objects:
                o.unselect()
            self.selected_objects = []
        if obj:
            if obj in self.selected_objects:
                self.selected_objects.remove(obj)
            else:
                self.selected_objects.append(obj)
        self.color_selected()
        self.selection_changed()

    def select_cb(self, attr, event_callback):
        """Callback for mouse button events to handle selection.

        Args:
            attr: Event callback attribute (unused).
            event_callback: Event callback instance containing the event.
        """
        event = event_callback.getEvent()
        if (event.getState() == coin.SoMouseButtonEvent.DOWN and
                event.getButton() == event.BUTTON1):
            pos = event.getPosition()
            obj = self.send_ray(pos)
            self.select_object(obj, event.wasCtrlDown())

    def deselect_all(self):
        """Deselect all currently selected objects."""
        if self.selected_objects:
            for o in self.selected_objects:
                o.unselect()
            self.selected_objects = []

    def color_selected(self):
        """Update colors of all selected objects."""
        for obj in self.selected_objects:
            obj.select()

    def selection_changed(self):
        """Callback method called when selection changes.

        Override this method in subclasses to respond to selection changes.
        """
        pass

    def select_all_cb(self, attr, event_callback):
        """Callback for 'a' key to toggle select all dynamic objects.

        Args:
            attr: Event callback attribute (unused).
            event_callback: Event callback instance containing the event.
        """
        event = event_callback.getEvent()
        if (event.getKey() == ord("a")):
            if event.getState() == event.DOWN:
                if self.selected_objects:
                    for o in self.selected_objects:
                        o.unselect()
                    self.selected_objects = []
                else:
                    for obj in self.dynamic_objects:
                        if obj.dynamic:
                            self.selected_objects.append(obj)
                self.color_selected()
                self.selection_changed()


#------------------------INTERACTION------------------------#

    def cursor_pos(self, event):
        """Convert mouse event position to 3D screen coordinates.

        Args:
            event: Mouse or keyboard event.

        Returns:
            3D coordinate tuple (x, y, z) in screen space.
        """
        pos = event.getPosition()
        return get_point_on_screen(self.render_manager, pos)
    

    def constrained_vector(self, vector):
        """Apply axis constraint to a movement vector.

        If a direction constraint is active (x, y, or z), returns a vector
        with only that component non-zero.

        Args:
            vector: 3D movement vector tuple (x, y, z).

        Returns:
            Constrained 3D vector tuple.
        """
        if self._direction is None:
            return vector
        if self._direction == "x":
            return [vector[0], 0, 0]
        elif self._direction == "y":
            return [0, vector[1], 0]
        elif self._direction == "z":
            return [0, 0, vector[2]]

    def grab_cb(self, attr, event_callback):
        """Callback for 'g' key to start dragging selected objects.

        Press 'g' to enter grab mode. Unregisters selection/highlight callbacks
        and sets up drag callbacks. Objects can be constrained to x/y/z axes
        by pressing those keys during dragging.

        Args:
            attr: Event callback attribute (unused).
            event_callback: Event callback instance containing the event.
        """
        # press g to move an entity
        event = event_callback.getEvent()
        # get all drag objects, every selected object can add some drag objects
        # but the eventhandler is not allowed to call the drag twice on an object
        if event.getKey() == ord("g"):
            self.drag_objects = set()
            for i in self.selected_objects:
                for j in i.drag_objects:
                    self.drag_objects.add(j)
            # check if something is selected
            if self.drag_objects:
                # first delete the selection_cb, and higlight_cb
                self.unregister()
                # now add a callback that calls the dragfunction of the selected entities
                self.start_pos = self.cursor_pos(event)
                self._dragCB = self.events.addEventCallback(
                    coin.SoEvent.getClassTypeId(), self.dragCB)
                for obj in self.drag_objects:
                    obj.drag_start()
                for foo in self.on_drag_start:
                    foo()


    def dragCB(self, attr, event_callback, force=False):
        """Callback for drag operations.

        Handles mouse movement, keyboard constraints (x/y/z), and mouse release
        to end dragging. Press ESC to cancel and reset positions.

        Args:
            attr: Event callback attribute (unused).
            event_callback: Event callback instance containing the event.
            force: If True, force end the drag operation (used for ESC key).
        """
        event = event_callback.getEvent()
        if ((isinstance(event, coin.SoMouseButtonEvent) and
                event.getState() == coin.SoMouseButtonEvent.DOWN
                and event.getButton() == coin.SoMouseButtonEvent.BUTTON1) or 
                force):
            self.register()
            if self._dragCB:
                self.events.removeEventCallback(
                    coin.SoEvent.getClassTypeId(), self._dragCB)
                self._direction = None
                self._dragCB = None
            self.start_pos = None
            for obj in self.drag_objects:
                obj.drag_release()
            for foo in self.on_drag_release:
                foo()
            self.drag_objects = []
        elif (isinstance(event, coin.SoKeyboardEvent) and
                event.getState() == coin.SoMouseButtonEvent.DOWN):
            if event.getKey() == 65307:     # esc
                for obj in self.drag_objects:
                    obj.drag([0, 0, 0], 1)  # set back to zero
                self.dragCB(attr, event_callback, force=True)
                return
            try:
                key = chr(event.getKey())
            except ValueError:
                # there is no character for this value
                key = "_"
            if key in "xyz" and key != self._direction:
                self._direction = key
            else:
                self._direction = None
            diff = self.cursor_pos(event) - self.start_pos
            diff = self.constrained_vector(diff)
            for obj in self.drag_objects:
                obj.drag(diff, 1)
            for foo in self.on_drag:
                foo()

        elif isinstance(event, coin.SoLocation2Event):
            fact = 0.1 if event.wasShiftDown() else 1.
            diff = self.cursor_pos(event) - self.start_pos
            diff = self.constrained_vector(diff)
            for obj in self.drag_objects:
                obj.drag(diff, fact)
            for foo in self.on_drag:
                foo()

    def delete_cb(self, attr, event_callback):
        """Callback for delete key to remove selected objects.

        Args:
            attr: Event callback attribute (unused).
            event_callback: Event callback instance containing the event.
        """
        event = event_callback.getEvent()
        # get all drag objects, every selected object can add some drag objects
        # but the eventhandler is not allowed to call the drag twice on an object
        if event.getKey() == ord(u"\uffff") and (event.getState() == 1):
            self.remove_selected()

    def remove_selected(self):
        """Remove all selected objects from the scene.

        Marks objects for deletion, checks dependencies, and removes them
        from both the scene graph and the internal object lists.
        """
        temp = []
        for i in self.selected_objects:
            i.delete()
        for i in self.dynamic_objects + self.static_objects:
            i.check_dependency()    #dependency length max = 1
        for i in self.dynamic_objects + self.static_objects:
            if i._delete:
                temp.append(i)
        self.selected_objects = []
        self.over_object = None
        self.selection_changed()
        for i in temp:
            if i in self.dynamic_objects:
                self.dynamic_objects.remove(i)
            else:
                self.static_objects.remove(i)
            import sys
            self.objects.removeChild(i)
            del(i)
        self.selection_changed()

    # needs upper case as this must overwrite the addChild from coin.SoSeparator
    def removeAllChildren(self, clear_all=False):
        """Remove all children from the separator.

        Args:
            clear_all: If True, removes all children including event callbacks.
                If False, only removes graphics objects, keeping event handling intact.
        """
        for i in self.dynamic_objects:
            i.delete()
        self.dynamic_objects = []
        self.static_objects = []
        self.selected_objects = []
        self.over_object = None
        if clear_all:
            super(InteractionSeparator, self).removeAllChildren()
        else:
            # only deletes graphics objects
            self.objects.removeAllChildren()

    # needs upper case as this must overwrite the addChild from coin.SoSeparator
    def addChild(self, child):
        """Add a child node to the separator.

        If the child is an Object3D instance, adds it to the objects separator
        and tracks it in dynamic_objects or static_objects based on its dynamic flag.
        Otherwise, adds it directly to this separator.

        Args:
            child: Child node to add. Can be an Object3D instance or any Coin3D node.
        """
        if hasattr(child, "dynamic"):
            self.objects.addChild(child)
            if child.dynamic:
                self.dynamic_objects.append(child)
            else:
                self.static_objects.append(child)
        else:
            super(InteractionSeparator, self).addChild(child) 
