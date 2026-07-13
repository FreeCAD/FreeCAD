# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileNotice: Part of the FreeCAD project.
################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""GUI commands for BIM wall relations.

These commands create, reuse, edit, and remove wall relation objects. The
relation object owns the join settings and solved trim planes; walls apply the
derived trims during recompute.
"""

import Arch
import ArchWallRelation
import ArchWallJunctionSolver
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


def _has_active_3d_view():
    active_window = FreeCADGui.getMainWindow().getActiveWindow()
    return callable(getattr(active_window, "getSceneGraph", None))


class BIM_Join:
    """
    Base class for the different BIM Join commands. It contains the common
    logic to select objects, find their baselines, and determine the
    intersection point.
    """

    Supported = translate("BIM", "Supported objects: Walls")
    SupportedBaselines = translate(
        "BIM", "The Join command only supports walls with a single straight baseline"
    )
    JointType = "Miter"

    def IsActive(self):
        return _has_active_3d_view()

    def Activated(self):
        """Executes the command's main logic."""
        sel = FreeCADGui.Selection.getSelection()

        if len(sel) != 2:
            FreeCAD.Console.PrintError(
                translate("BIM", "The BIM Join command needs exactly 2 objects selected.") + "\n"
            )
            return

        if sel[0].Document is None or sel[0].Document != sel[1].Document:
            FreeCAD.Console.PrintError(
                translate("BIM", "The selected walls must belong to the same document.") + "\n"
            )
            return

        join_data = self._get_join_data(sel[0], sel[1])
        if not join_data:
            return

        doc = sel[0].Document
        doc.openTransaction(translate("BIM", "Join objects"))
        try:
            joint = ArchWallRelation.find_existing_joint(doc, sel[0], sel[1])
            if not joint:
                joint = Arch.makeWallJoint(sel[0], sel[1], self.JointType)
            if not joint or not self._configure_joint(joint, sel[0], sel[1], join_data):
                doc.abortTransaction()
                return
        except Exception:
            doc.abortTransaction()
            raise
        doc.commitTransaction()
        doc.recompute()
        self._report_joint_status(joint)

    def _report_unsupported_baseline(self, obj):
        FreeCAD.Console.PrintError(self.SupportedBaselines + f": {obj.Label}\n")

    def _get_join_path(self, obj):
        """Return the resolved path for a supported wall join."""
        if not (getattr(obj, "Document", None) and obj.Proxy):
            FreeCAD.Console.PrintError(
                translate("BIM", "This object is not supported by the Join command")
                + f": {obj.Label}\n"
            )
            return None

        path = ArchWallRelation.get_join_path(obj)
        if not path:
            self._report_unsupported_baseline(obj)
            return None

        return path

    def find_best_intersection(self, path1, path2):
        return ArchWallRelation.find_best_intersection(path1, path2)

    def _get_join_data(self, wall1, wall2):
        """Validate a proposed join without mutating the document."""
        path1 = self._get_join_path(wall1)
        path2 = self._get_join_path(wall2)
        if not path1 or not path2:
            return None

        intersection, _end_name1, _end_name2 = self.find_best_intersection(path1, path2)
        if not intersection:
            FreeCAD.Console.PrintError(
                translate("BIM", "The baselines of the selected walls do not intersect.") + "\n"
            )
            return None
        if not path1.contains_point(intersection) or not path2.contains_point(intersection):
            FreeCAD.Console.PrintError(
                translate(
                    "BIM",
                    "The selected wall baselines intersect only outside their finite segments; extending walls is not supported.",
                )
                + "\n"
            )
            return None
        return path1, path2, intersection

    def _configure_joint(self, joint, wall1, wall2, join_data=None):
        if join_data is None:
            join_data = self._get_join_data(wall1, wall2)
        if not join_data:
            return False
        path1, path2, intersection = join_data

        joint.Enabled = True
        joint.JointType = self.JointType
        joint.EndA = "Auto"
        joint.EndB = "Auto"
        joint.ButtTrimmed = "Auto"
        joint.TeeStem = "Auto"
        return self.configure_joint(joint, wall1, wall2, path1, path2, intersection)

    def configure_joint(self, _joint, _wall1, _wall2, _path1, _path2, _intersection):
        return True

    @staticmethod
    def _report_joint_status(joint):
        if not joint or joint.Status == "OK":
            return
        message = joint.StatusMessage or translate("BIM", "The wall joint could not be solved.")
        FreeCAD.Console.PrintError(message + "\n")
        if joint.Status == "Conflict":
            FreeCAD.Console.PrintMessage(
                translate(
                    "BIM",
                    "Use Unjoin on the blocking relation, or edit the new joint's end or role properties.",
                )
                + "\n"
            )


class BIM_Join_Miter(BIM_Join):
    """The BIM_Join_Miter command creates a miter joint between two objects."""

    JointType = "Miter"

    def GetResources(self):
        return {
            "Pixmap": "BIM_Join_Miter",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Join_Miter", "Miter joint"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Join_Miter", "Creates a miter joint between two supported objects."
            ),
        }


class BIM_Join_Tee(BIM_Join):
    """The BIM_Join_Tee command creates a tee joint between two objects."""

    JointType = "Tee"

    def GetResources(self):
        return {
            "Pixmap": "BIM_Join_Tee",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Join_Tee", "Tee joint"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Join_Tee", "Creates a tee joint between two supported objects."
            ),
        }

    def configure_joint(self, joint, wall1, wall2, path1, path2, intersection):
        stem_role = ArchWallRelation.get_auto_tee_stem_role(path1, path2, intersection)
        stem_wall = wall1 if stem_role == "WallA" else wall2
        joint.TeeStem = "WallA" if joint.WallA == stem_wall else "WallB"
        return True


class BIM_Join_Butt(BIM_Join):
    """The BIM_Join_Butt command creates a butt joint between two objects."""

    JointType = "Butt"

    def GetResources(self):
        return {
            "Pixmap": "BIM_Join_Butt",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Join_Butt", "Butt joint"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Join_Butt", "Creates a butt joint between two supported objects."
            ),
        }

    def configure_joint(self, joint, _wall1, wall2, _path1, _path2, _intersection):
        joint.ButtTrimmed = "WallA" if joint.WallA == wall2 else "WallB"
        return True


class BIM_Join_Junction:
    """The BIM_Join_Junction command creates a junction between three or more walls."""

    def IsActive(self):
        return _has_active_3d_view()

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()

        if len(sel) < 3:
            FreeCAD.Console.PrintError(
                translate(
                    "BIM",
                    "The BIM Wall Junction command needs at least 3 objects selected.",
                )
                + "\n"
            )
            return

        doc = sel[0].Document
        if doc is None or any(obj.Document != doc for obj in sel):
            FreeCAD.Console.PrintError(
                translate("BIM", "The selected walls must belong to the same document.") + "\n"
            )
            return

        solution = ArchWallJunctionSolver.solve_wall_junction_inputs(sel)
        if not solution.is_ok():
            FreeCAD.Console.PrintError(solution.status_message + "\n")
            return

        doc.openTransaction(translate("BIM", "Create wall junction"))
        try:
            junction = Arch.makeWallJunction(sel)
            if not junction:
                doc.abortTransaction()
                return
            doc.commitTransaction()
        except Exception:
            doc.abortTransaction()
            raise

        doc.recompute()
        self._report_junction_status(junction)

    @staticmethod
    def _report_junction_status(junction):
        if not junction or junction.Status == "OK":
            return
        message = junction.StatusMessage or translate(
            "BIM", "The wall junction could not be solved."
        )
        FreeCAD.Console.PrintError(message + "\n")
        if junction.Status == "Conflict":
            FreeCAD.Console.PrintMessage(
                translate(
                    "BIM",
                    "Use Unjoin on the blocking relation, or edit the new junction's properties.",
                )
                + "\n"
            )

    def GetResources(self):
        return {
            "Pixmap": "BIM_Join_Junction",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Join_Junction", "Wall junction"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Join_Junction",
                "Creates a wall junction between three or more supported walls.",
            ),
        }


class BIM_Unjoin:
    """The BIM_Unjoin command removes wall relation objects."""

    def IsActive(self):
        return _has_active_3d_view()

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        relations = self._get_selected_relations(sel)
        if not relations:
            return

        doc = relations[0].Document
        if any(relation.Document != doc for relation in relations):
            FreeCAD.Console.PrintError(
                translate("BIM", "Selected wall relations must belong to the same document.") + "\n"
            )
            return
        doc.openTransaction(translate("BIM", "Unjoin objects"))
        try:
            for relation in relations:
                doc.removeObject(relation.Name)
            doc.commitTransaction()
        except Exception:
            doc.abortTransaction()
            raise
        doc.recompute()

    @staticmethod
    def _get_selected_relations(sel):
        if sel and all(ArchWallRelation.is_wall_relation(obj) for obj in sel):
            return list(sel)

        if len(sel) == 2:
            joint = ArchWallRelation.find_existing_joint(sel[0].Document, sel[0], sel[1])
            if joint:
                return [joint]
            FreeCAD.Console.PrintError(
                translate("BIM", "The selected objects are not joined by a wall joint.") + "\n"
            )
            return []

        FreeCAD.Console.PrintError(
            translate(
                "BIM",
                "The BIM Unjoin command needs selected wall relation objects or 2 joined walls.",
            )
            + "\n"
        )
        return []

    def GetResources(self):
        return {
            "Pixmap": "BIM_Unjoin",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Unjoin", "Unjoin"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Unjoin",
                "Removes selected wall relations, or the wall joint between two selected walls.",
            ),
        }


class BIM_EditWallJoint:
    """The BIM_EditWallJoint command opens a task panel to edit a selected joint."""

    def IsActive(self):
        sel = FreeCADGui.Selection.getSelection()
        return _has_active_3d_view() and len(sel) == 1 and ArchWallRelation.is_wall_joint(sel[0])

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) != 1 or not ArchWallRelation.is_wall_joint(sel[0]):
            FreeCAD.Console.PrintError(
                translate(
                    "BIM",
                    "The BIM Edit Wall Joint command needs 1 wall joint selected.",
                )
                + "\n"
            )
            return
        FreeCADGui.ActiveDocument.setEdit(sel[0].Name, 0)

    def GetResources(self):
        return {
            "Pixmap": "BIM_EditWallJoint",
            "MenuText": QT_TRANSLATE_NOOP("BIM_EditWallJoint", "Edit wall joint"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_EditWallJoint",
                "Opens a task panel to edit the selected wall joint relation.",
            ),
        }


# Register the commands with FreeCAD's GUI
FreeCADGui.addCommand("BIM_Join_Miter", BIM_Join_Miter())
FreeCADGui.addCommand("BIM_Join_Tee", BIM_Join_Tee())
FreeCADGui.addCommand("BIM_Join_Butt", BIM_Join_Butt())
FreeCADGui.addCommand("BIM_Join_Junction", BIM_Join_Junction())
FreeCADGui.addCommand("BIM_Unjoin", BIM_Unjoin())
FreeCADGui.addCommand("BIM_EditWallJoint", BIM_EditWallJoint())
