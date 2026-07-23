# SPDX-License-Identifier: LGPL-2.1-or-later

"""Interactive positioning tools for the Stair Designer task panel."""

import FreeCAD
import FreeCADGui


translate = FreeCAD.Qt.translate


def _belongs_to_stair(obj, stair):
    """Return whether an object is the stair or one of its generated children."""

    if obj is stair:
        return True
    if getattr(obj, "GeneratedBy", "") == stair.Name:
        return True
    if getattr(obj, "StairName", "") == stair.Name:
        return True
    return stair in getattr(obj, "InListRecursive", ())


def _selection_path(obj, subname):
    """Return the root object and full path for a selected subobject."""

    parents = getattr(obj, "Parents", ())
    if len(parents) == 1 and len(parents[0]) == 2:
        return parents[0][0], parents[0][1] + subname
    return obj, subname


def _global_placement(obj):
    """Return an object's placement in its unique selection context."""

    root, subname = _selection_path(obj, "")
    if root is not obj:
        try:
            return obj.getGlobalPlacementOf(obj, root, subname)
        except (AttributeError, RuntimeError):
            return root.getPlacementOf(subname, obj)
    return obj.getGlobalPlacement()


def _selected_vertices(selection, stair):
    """Return selected global vertex points classified by stair ownership."""

    vertices = []
    for selection_object in selection:
        names = tuple(selection_object.SubElementNames)
        subobjects = tuple(selection_object.SubObjects)
        for index, name in enumerate(names):
            if not name.rsplit(".", 1)[-1].startswith("Vertex"):
                continue
            if index >= len(subobjects):
                continue
            vertex = subobjects[index]
            if not hasattr(vertex, "Point"):
                continue
            try:
                root, subname = _selection_path(
                    selection_object.Object,
                    name,
                )
                owner = root.getSubObject(subname, retType=1)
            except (AttributeError, RuntimeError):
                owner = selection_object.Object
            vertices.append(
                (
                    _belongs_to_stair(owner, stair),
                    vertex.Point,
                )
            )
    return vertices


def _translated_stair_placement(stair, target, source=None):
    """Return a local placement translating a global source onto a target."""

    global_placement = _global_placement(stair)
    source_point = source if source is not None else global_placement.Base
    desired_global = FreeCAD.Placement(global_placement)
    desired_global.Base = desired_global.Base.add(target.sub(source_point))
    parent_placement = global_placement.multiply(stair.Placement.inverse())
    return parent_placement.inverse().multiply(desired_global)


class PositionPanelMixin:
    """Show a transform dragger and provide vertex-based stair snapping."""

    def _initialize_position_tools(self):
        self._position_dragger = None
        self._position_dragger_view = None
        self._position_dragger_root = None
        self._position_dragger_callbacks = []
        self._position_dragger_camera_sensor = None
        self._position_dragger_camera = None
        self._position_dragger_scale_node = None
        self._position_dragger_syncing = False

    def _setup_position_dragger(self):
        if not FreeCAD.GuiUp or self._position_dragger is not None:
            return
        try:
            from pivy import coin

            node_type = coin.SoType.fromName("SoTransformDragger")
            if node_type.isBad():
                return
            dragger = node_type.createInstance()
            if dragger is None:
                return
            dragger.draggerSize.setValue(0.03)

            view = FreeCADGui.activeDocument().activeView()
            root = view.getSceneGraph()
            root.addChild(dragger)
            self._position_dragger = dragger
            self._position_dragger_view = view
            self._position_dragger_root = root
            self._setup_position_dragger_autoscale(coin, view)
            self._sync_position_dragger()
            self._position_dragger_callbacks = [
                (
                    "addMotionCallback",
                    view.addDraggerCallback(
                        dragger,
                        "addMotionCallback",
                        self._position_dragger_moved,
                    ),
                ),
                (
                    "addFinishCallback",
                    view.addDraggerCallback(
                        dragger,
                        "addFinishCallback",
                        self._position_dragger_finished,
                    ),
                ),
            ]
        except (AttributeError, RuntimeError):
            self._remove_position_dragger()

    def _setup_position_dragger_autoscale(self, coin, view):
        """Keep the dragger at FreeCAD's standard fixed on-screen size."""

        camera = view.getCameraNode()
        scale_node = self._position_dragger.getPart("scaleNode", True)
        scale_node.scaleFactor.disconnect()
        self._position_dragger.autoScaleResult.disconnect()
        sensor = coin.SoFieldSensor(
            self._position_dragger_camera_changed,
            None,
        )
        if camera.getTypeId().isDerivedFrom(
            coin.SoOrthographicCamera.getClassTypeId()
        ):
            sensor.attach(camera.height)
        else:
            sensor.attach(camera.position)
        self._position_dragger_camera = camera
        self._position_dragger_scale_node = scale_node
        self._position_dragger_camera_sensor = sensor
        self._update_position_dragger_scale()

    def _position_dragger_camera_changed(self, _data, _sensor):
        self._update_position_dragger_scale()

    def _update_position_dragger_scale(self):
        dragger = self._position_dragger
        camera = self._position_dragger_camera
        scale_node = self._position_dragger_scale_node
        if dragger is None or camera is None or scale_node is None:
            return
        origin = dragger.translation.getValue()
        radius = dragger.draggerSize.getValue() / 2.0
        scale = camera.getViewVolume().getWorldToScreenScale(origin, radius)
        scale_node.scaleFactor.setValue(scale, scale, scale)
        dragger.autoScaleResult.setValue(scale)

    def _sync_position_dragger(self):
        if self._position_dragger is None:
            return
        try:
            from pivy import coin

            placement = _global_placement(self.stair)
            self._position_dragger_syncing = True
            self._position_dragger.translation.setValue(
                placement.Base.x,
                placement.Base.y,
                placement.Base.z,
            )
            self._position_dragger.rotation.setValue(
                coin.SbRotation(placement.Rotation.Q)
            )
        finally:
            self._position_dragger_syncing = False
        self._update_position_dragger_scale()

    def _position_dragger_moved(self, dragger):
        if self._position_dragger_syncing:
            return
        translation = dragger.translation.getValue()
        quaternion = dragger.rotation.getValue().getValue()
        global_placement = FreeCAD.Placement(
            FreeCAD.Vector(*translation.getValue()),
            FreeCAD.Rotation(*quaternion),
        )
        current_global = _global_placement(self.stair)
        parent_placement = current_global.multiply(
            self.stair.Placement.inverse()
        )
        self.stair.Placement = parent_placement.inverse().multiply(
            global_placement
        )
        self._update_position_dragger_scale()

    def _position_dragger_finished(self, dragger):
        self._position_dragger_moved(dragger)
        self.stair.Document.recompute()

    def _remove_position_dragger(self):
        sensor = self._position_dragger_camera_sensor
        if sensor is not None:
            try:
                sensor.detach()
            except (AttributeError, RuntimeError):
                pass
        self._position_dragger_camera_sensor = None
        self._position_dragger_camera = None
        self._position_dragger_scale_node = None
        view = self._position_dragger_view
        dragger = self._position_dragger
        if view is not None and dragger is not None:
            for callback_type, callback in self._position_dragger_callbacks:
                try:
                    view.removeDraggerCallback(
                        dragger,
                        callback_type,
                        callback,
                    )
                except (AttributeError, RuntimeError):
                    pass
        if self._position_dragger_root is not None and dragger is not None:
            try:
                self._position_dragger_root.removeChild(dragger)
            except (AttributeError, RuntimeError):
                pass
        self._position_dragger_callbacks = []
        self._position_dragger = None
        self._position_dragger_view = None
        self._position_dragger_root = None

    def _snap_position(self):
        vertices = _selected_vertices(
            FreeCADGui.Selection.getSelectionEx("", 0),
            self.stair,
        )
        external = [point for owned, point in vertices if not owned]
        stair_vertices = [point for owned, point in vertices if owned]
        if len(external) != 1 or len(stair_vertices) > 1:
            FreeCAD.Console.PrintWarning(
                translate(
                    "BIM",
                    "Select one external vertex, optionally with one stair "
                    "vertex, then click Snap position.\n",
                )
            )
            return
        self.stair.Placement = _translated_stair_placement(
            self.stair,
            external[0],
            stair_vertices[0] if stair_vertices else None,
        )
        self.stair.Document.recompute()
        self._sync_position_dragger()
        FreeCADGui.activeDocument().activeView().redraw()
