# SPDX-License-Identifier: LGPL-2.1-or-later

"""Trim the beginning and end of a single open profile toolpath."""

import FreeCAD
import math
import Path
import Path.Dressup.Utils as PathDressup
import PathScripts.PathUtils as PathUtils
from PySide.QtCore import QT_TRANSLATE_NOOP

translate = FreeCAD.Qt.translate


class DressupProfileTrim:
    def __init__(self, obj, base):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base toolpath to modify"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "StartOffset",
            "Profile Trim",
            QT_TRANSLATE_NOOP(
                "App::Property", "Distance to trim from the start of the open profile"
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "StopOffset",
            "Profile Trim",
            QT_TRANSLATE_NOOP("App::Property", "Distance to trim from the end of the open profile"),
        )
        obj.Base = base
        radius = self._toolRadius(base)
        obj.StartOffset = radius
        obj.StopOffset = radius
        base_op = PathDressup.baseOp(base)
        if hasattr(base_op, "OpToolDiameter"):
            diameter_expression = f"{base_op.Name}.OpToolDiameter / 2"
            obj.setExpression("StartOffset", diameter_expression)
            obj.setExpression("StopOffset", diameter_expression)
        obj.Proxy = self

    @staticmethod
    def _toolRadius(base):
        try:
            controller = PathDressup.baseOp(base).ToolController
            return controller.Tool.Diameter.Value / 2.0
        except (AttributeError, TypeError):
            return 0.0

    def execute(self, obj):
        if not obj.Base or not obj.Base.Path:
            return
        PathDressup.placeWithBase(obj)
        commands = list(obj.Base.Path.Commands)
        # The offsets are measured along cutting moves, not rapid/plunge moves.
        # Offsets are measured on XY profile motion. Vertical plunge/retracts
        # are preserved but do not consume either offset.
        start = FreeCAD.Vector(0, 0, 0)
        segments = []
        for index, command in enumerate(commands):
            params = command.Parameters
            if command.Name in ("G0", "G1", "G2", "G3"):
                end = FreeCAD.Vector(
                    params.get("X", start.x), params.get("Y", start.y), params.get("Z", start.z)
                )
                if command.Name in ("G1", "G2", "G3"):
                    segment = self._profileSegment(index, command, start, end)
                    if segment:
                        segments.append(segment)
                start = end

        length = sum(segment["length"] for segment in segments)
        trim_start = max(0.0, obj.StartOffset.Value)
        trim_end = max(0.0, obj.StopOffset.Value)
        if trim_start + trim_end >= length:
            Path.Log.warning(
                translate(
                    "CAM_DressupProfileTrim",
                    "The start and stop offsets consume the entire profile path.",
                )
            )
            obj.Path = Path.Path()
            return

        # Keep the interval [trim_start, length-trim_end], clipping the first
        # and last retained profile moves to their exact offset positions.
        low, high, distance = trim_start, length - trim_end, 0.0
        replacements = {}
        profile_indices = {segment["index"] for segment in segments}
        first_trim_point = None
        for segment in segments:
            index = segment["index"]
            segment_length = segment["length"]
            seg_low, seg_high = max(low, distance), min(high, distance + segment_length)
            if seg_high > seg_low and segment_length:
                a = (seg_low - distance) / segment_length
                b = (seg_high - distance) / segment_length
                p0 = self._pointAt(segment, a)
                p1 = self._pointAt(segment, b)
                original = commands[index]
                params = dict(original.Parameters)
                for axis, value in (("X", p1.x), ("Y", p1.y), ("Z", p1.z)):
                    if axis in params or abs(value) > 1e-12:
                        params[axis] = value
                if original.Name in ("G2", "G3"):
                    center = segment["center"]
                    params["I"], params["J"] = center.x - p0.x, center.y - p0.y
                replacements[index] = Path.Command(original.Name, params, original.Annotations)
                if trim_start > 0 and first_trim_point is None:
                    first_trim_point = (index, p0)
            distance += segment_length

        # Move the approach rapid to the new start point, so the plunge happens
        # at B rather than plunging at A and then cutting across A-to-B.
        if first_trim_point:
            first_index, point = first_trim_point
            approach_rapids = [
                i
                for i in range(first_index)
                if commands[i].Name == "G0"
                and ("X" in commands[i].Parameters or "Y" in commands[i].Parameters)
            ]
            if approach_rapids:
                # Reposition every explicit XY in the approach sequence. Some
                # operations repeat the original XY on a plunge or on multiple
                # rapids; changing only one can create a diagonal plunge or a
                # rapid back to the untrimmed start point.
                for i in range(approach_rapids[0], first_index):
                    command = commands[i]
                    if command.Name in ("G0", "G1") and (
                        "X" in command.Parameters or "Y" in command.Parameters
                    ):
                        params = dict(command.Parameters)
                        params["X"], params["Y"] = point.x, point.y
                        replacements[i] = Path.Command(command.Name, params, command.Annotations)
            else:
                # Some paths omit a positioning rapid. Keep a safe linear
                # positioning command before the first retained cut.
                replacements[first_index] = (
                    Path.Command("G1", {"X": point.x, "Y": point.y, "Z": point.z}),
                    replacements[first_index],
                )

        output = Path.Path()
        for index, command in enumerate(commands):
            if index in replacements:
                replacement = replacements[index]
                output.addCommands(
                    list(replacement) if isinstance(replacement, tuple) else [replacement]
                )
            elif index not in profile_indices:
                output.addCommands([command])
        obj.Path = output

    @staticmethod
    def _profileSegment(index, command, start, end):
        dx, dy = end.x - start.x, end.y - start.y
        if command.Name == "G1":
            distance = math.hypot(dx, dy)
            if distance < 1e-9:
                return None
            return {"index": index, "start": start, "end": end, "length": distance}

        params = command.Parameters
        center = FreeCAD.Vector(start.x + params.get("I", 0), start.y + params.get("J", 0), start.z)
        radius = math.hypot(start.x - center.x, start.y - center.y)
        if radius < 1e-9:
            return None
        angle0 = math.atan2(start.y - center.y, start.x - center.x)
        angle1 = math.atan2(end.y - center.y, end.x - center.x)
        if command.Name == "G2":
            sweep = -((angle0 - angle1) % (2 * math.pi))
        else:
            sweep = (angle1 - angle0) % (2 * math.pi)
        if abs(sweep) < 1e-12:
            sweep = -2 * math.pi if command.Name == "G2" else 2 * math.pi
        dz = end.z - start.z
        length = math.hypot(radius * abs(sweep), dz)
        return {
            "index": index,
            "start": start,
            "end": end,
            "length": length,
            "center": center,
            "radius": radius,
            "angle": angle0,
            "sweep": sweep,
        }

    @staticmethod
    def _pointAt(segment, fraction):
        start, end = segment["start"], segment["end"]
        if "center" not in segment:
            return start + (end - start) * fraction
        center = segment["center"]
        angle = segment["angle"] + segment["sweep"] * fraction
        return FreeCAD.Vector(
            center.x + segment["radius"] * math.cos(angle),
            center.y + segment["radius"] * math.sin(angle),
            start.z + (end.z - start.z) * fraction,
        )

    def onDelete(self, obj, args):
        if obj.Base:
            job = PathUtils.findParentJob(obj)
            if job:
                job.Proxy.addOperation(obj.Base, obj)
            obj.Base.ViewObject.Visibility = True
            obj.Base = None
        return True

    def onDocumentRestored(self, obj):
        pass


def Create(base, name="DressupProfileTrim"):
    if not base.isDerivedFrom("Path::Feature"):
        return None
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    job = PathUtils.findParentJob(base)
    obj.Proxy = DressupProfileTrim(obj, base)
    job.Proxy.addOperation(obj, base, True)
    return obj
