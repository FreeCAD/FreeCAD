# SPDX-License-Identifier: LGPL-2.1-or-later

"""BIM support for Draft Trimex."""

import FreeCAD as App
import FreeCADGui as Gui
from draftguitools.gui_trimex import Trimex
from draftutils import utils


class BimTrimex(Trimex):
    """Trim or extend supported BIM objects."""

    def _setupObjectTrimex(self, sel):
        if len(sel.SubObjects) != 1 or sel.SubObjects[0].ShapeType != "Face":
            return False

        face = sel.SubObjects[0]
        proxy = getattr(self.obj, "Proxy", None)
        obj_type = getattr(proxy, "Type", None)
        if obj_type == "Wall":
            if self.obj.Base:
                return self._setupBaseTrimex(face)
            return self._setupWallTrimex(face)
        if obj_type == "Pipe":
            return self._setupPipeTrimex(face)
        if obj_type == "Structure":
            return self._setupStructureTrimex(face)
        if obj_type in ("Frame", "Truss") and self.obj.Base:
            return self._setupBaseTrimex(face)
        if obj_type == "Panel" and not self.obj.Base:
            return self._setupPanelTrimex(face)
        return False

    def _setupBaseTrimex(self, face):
        """Edit the end of a Wire or Part::Line used as a BIM object's Base."""
        base = self.obj.Base
        data = self._getBaseData(base)
        if data is None:
            return False

        points, axes, edge_count = data
        end = self._getEndFace(face, points, axes)
        if end is None:
            return False

        self.trimexHost = self.obj
        self.obj = base
        self.lockedActivePoint = edge_count if end else 0
        self._startWireTrimex()
        return True

    def _setupWallTrimex(self, face):
        """Edit the length of a wall without a Base."""
        obj = self.obj
        placement = obj.Placement
        axis = placement.Rotation.multVec(App.Vector(1, 0, 0))
        points = [
            placement.multVec(App.Vector(-obj.Length.Value / 2, 0, 0)),
            placement.multVec(App.Vector(obj.Length.Value / 2, 0, 0)),
        ]

        def set_wall(new_points):
            direction = new_points[1].sub(new_points[0])
            if direction.Length:
                obj.Length = direction.Length
                obj.Placement = App.Placement(
                    (new_points[0] + new_points[1]) * 0.5,
                    App.Rotation(App.Vector(1, 0, 0), direction),
                )

        return self._setupAxisTrimex(face, points, [axis.negative(), axis], set_wall)

    def _setupPipeTrimex(self, face):
        """Edit a pipe's visible endpoints, preserving its offsets."""
        obj = self.obj
        if obj.Base:
            if not obj.OffsetStart.Value and not obj.OffsetEnd.Value:
                return self._setupBaseTrimex(face)
            data = self._getBaseData(obj.Base)
            if data is None:
                return False
            points, axes, _ = data
            offsets = (obj.OffsetStart.Value, obj.OffsetEnd.Value)
            points = [
                points[0] - axes[0] * offsets[0],
                points[1] - axes[1] * offsets[1],
            ]

            def set_pipe(new_points):
                end = self.lockedActivePoint
                self._setBaseEndpoint(
                    obj.Base,
                    points[end] + axes[end] * offsets[end],
                    new_points[end] + axes[end] * offsets[end],
                )

            return self._setupAxisTrimex(face, points, axes, set_pipe)

        placement = obj.Placement
        axis = placement.Rotation.multVec(App.Vector(0, 0, 1))
        points = [
            placement.multVec(App.Vector(0, 0, obj.OffsetStart.Value)),
            placement.multVec(App.Vector(0, 0, obj.Length.Value - obj.OffsetEnd.Value)),
        ]

        def set_pipe(new_points):
            obj.Length = (
                new_points[0].distanceToPoint(new_points[1])
                + obj.OffsetStart.Value
                + obj.OffsetEnd.Value
            )
            obj.Placement = App.Placement(
                new_points[0] - axis * obj.OffsetStart.Value,
                placement.Rotation,
            )

        return self._setupAxisTrimex(face, points, [axis.negative(), axis], set_pipe)

    def _setupStructureTrimex(self, face):
        """Edit the extrusion dimension of a Structure without a path Tool."""
        obj = self.obj
        if obj.Tool:
            return False
        data = obj.Proxy.getExtrusionData(obj)
        if not data:
            return False
        _, extrusion, base_placement = data
        if isinstance(extrusion, list) or isinstance(base_placement, list):
            return False

        placement = obj.Placement
        axis = placement.Rotation.multVec(base_placement.Rotation.multVec(extrusion))
        length = axis.Length
        if not length:
            return False
        axis.normalize()
        start = placement.multVec(base_placement.Base)
        points = [start, start + axis * length]
        if obj.IfcType in ("Beam", "Column") and obj.Length.Value > obj.Height.Value:
            prop = "Length"
        else:
            prop = "Height"

        def set_structure(new_points):
            length = new_points[1].sub(new_points[0]).dot(axis)
            if length > 0:
                obj.Placement.Base = obj.Placement.Base + axis * new_points[0].sub(points[0]).dot(
                    axis
                )
                setattr(obj, prop, length)

        return self._setupAxisTrimex(face, points, [axis.negative(), axis], set_structure)

    def _setupPanelTrimex(self, face):
        """Edit the thickness of a panel without a Base."""
        obj = self.obj
        placement = obj.Placement
        normal = App.Vector(obj.Normal) if obj.Normal.Length else App.Vector(0, 0, 1)
        axis = placement.Rotation.multVec(normal)
        axis.normalize()
        if not obj.Thickness.Value:
            return False
        points = [placement.Base, placement.Base + axis * obj.Thickness.Value]

        def set_panel(new_points):
            thickness = new_points[1].sub(new_points[0]).dot(axis)
            if thickness > 0:
                obj.Placement.Base = obj.Placement.Base + new_points[0].sub(points[0])
                obj.Thickness = thickness

        return self._setupAxisTrimex(face, points, [axis.negative(), axis], set_panel)

    def _getBaseData(self, base):
        """Return the world-space ends and tangents of an editable base."""
        import Part

        base_type = utils.getType(base)
        if base_type not in ("Wire", "Part::Line"):
            return None
        if utils.get_trimex_unsupported_reason(base):
            return None

        if base_type == "Wire":
            wire = base.Shape.Wires[0]
            if wire.isClosed():
                return None
            edges = Part.__sortEdges__(wire.Edges)
        else:
            edges = base.Shape.Edges

        placement = self.obj.Placement
        start = edges[0]
        end = edges[-1]
        start_axis = placement.Rotation.multVec(start.tangentAt(start.FirstParameter)).negative()
        end_axis = placement.Rotation.multVec(end.tangentAt(end.LastParameter))
        start_axis.normalize()
        end_axis.normalize()
        return (
            [
                placement.multVec(start.Vertexes[0].Point),
                placement.multVec(end.Vertexes[-1].Point),
            ],
            [start_axis, end_axis],
            len(edges),
        )

    def _setBaseEndpoint(self, base, old, new):
        old = base.Placement.inverse().multVec(self.obj.Placement.inverse().multVec(old))
        new = base.Placement.inverse().multVec(self.obj.Placement.inverse().multVec(new))
        if utils.getType(base) == "Part::Line":
            start = App.Vector(base.X1, base.Y1, base.Z1)
            end = App.Vector(base.X2, base.Y2, base.Z2)
            if start.distanceToPoint(old) < end.distanceToPoint(old):
                base.X1, base.Y1, base.Z1 = new.x, new.y, new.z
            else:
                base.X2, base.Y2, base.Z2 = new.x, new.y, new.z
        else:
            points = list(base.Points)
            if points[0].distanceToPoint(old) < points[-1].distanceToPoint(old):
                points[0] = new
            else:
                points[-1] = new
            base.Points = points


Gui.addCommand("Draft_Trimex", BimTrimex())
