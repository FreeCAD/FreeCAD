# --- src/Mod/BIM/bimcommands/BimJoin.py ---

# ***************************************************************************
# * (License Header as in other BIM files)                                  *
# ***************************************************************************

"""
BIM join command
This command joins different objects that can be joined, currently only Walls
"""

import math
import FreeCAD
import FreeCADGui
import Part
import Draft
import DraftGeomUtils

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_Join:
    """
    Base class for the different BIM Join commands. It contains the common
    logic to select objects, find their baselines, and determine the
    intersection point.
    """

    Supported = translate("BIM", "Supported objects: Walls")

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        """Executes the command's main logic."""
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) != 2:
            FreeCAD.Console.PrintError(
                translate("BIM", "The BIM Join command needs exactly 2 objects selected.") + "\n"
            )
            return

        baselines = []
        for obj in sel:
            if hasattr(obj, "Proxy") and hasattr(obj.Proxy, "get_baseline"):
                baseline = obj.Proxy.get_baseline(obj)
                if baseline:
                    baselines.append(baseline)
                else:
                    FreeCAD.Console.PrintError(
                        translate("BIM", "Could not determine a baseline for") + f": {obj.Label}\n"
                    )
                    return
            else:
                FreeCAD.Console.PrintError(
                    translate("BIM", "This object is not supported by the Join command") + f": {obj.Label}\n"
                )
                return

        # Find the best intersection point and which ends of the walls are involved.
        intersection, end_name1, end_name2 = self.find_best_intersection(baselines[0], baselines[1])
        if not intersection:
            FreeCAD.Console.PrintError(
                translate("BIM", "The baselines of the selected walls do not intersect.") + "\n"
            )
            return

        # Let the specific join command calculate the cutting planes.
        # This method is overridden by the subclasses.
        plane1_placement, plane2_placement = self.calculate_cutting_planes(
            baselines[0], baselines[1], intersection, sel[0].Width.Value, sel[1].Width.Value
        )

        # Apply the calculated placements to the wall objects.
        doc = sel[0].Document
        doc.openTransaction(translate("BIM", "Join objects"))

        if plane1_placement and end_name1:
            # The placement is calculated in global coordinates. We need to store it
            # relative to the wall's own placement.
            relative_placement1 = sel[0].Placement.inverse().multiply(plane1_placement)
            setattr(sel[0], "Ending" + end_name1, relative_placement1)

        if plane2_placement and end_name2:
            relative_placement2 = sel[1].Placement.inverse().multiply(plane2_placement)
            setattr(sel[1], "Ending" + end_name2, relative_placement2)

        doc.commitTransaction()
        doc.recompute()

    def find_best_intersection(self, line1, line2):
        """
        Finds the intersection point of two lines and determines which
        end of each line is closest to that intersection.
        """
        # A baseline can be a Wire or an Edge. Get the first edge for intersection.
        edge1 = line1.Edges[0] if hasattr(line1, "Edges") else line1
        edge2 = line2.Edges[0] if hasattr(line2, "Edges") else line2

        intersections = DraftGeomUtils.findIntersection(edge1, edge2, infinite1=True, infinite2=True)
        if not intersections:
            return None, None, None

        intersection_point = intersections[0]

        # Determine which end of line1 is closer to the intersection
        dist_start1 = intersection_point.distanceToPoint(line1.Vertexes[0].Point)
        dist_end1 = intersection_point.distanceToPoint(line1.Vertexes[-1].Point)
        end_name1 = "Start" if dist_start1 < dist_end1 else "End"

        # Determine which end of line2 is closer to the intersection
        dist_start2 = intersection_point.distanceToPoint(line2.Vertexes[0].Point)
        dist_end2 = intersection_point.distanceToPoint(line2.Vertexes[-1].Point)
        end_name2 = "Start" if dist_start2 < dist_end2 else "End"

        return intersection_point, end_name1, end_name2

    def calculate_cutting_planes(self, baseline1, baseline2, intersection, width1, width2):
        """
        Placeholder method to be overridden by subclasses (Miter, Tee, Butt).
        It must return two placements representing the cutting planes.
        """
        FreeCAD.Console.PrintWarning(
            translate("BIM", "This join type is not yet implemented.") + "\n"
        )
        return None, None


class BIM_Join_Miter(BIM_Join):
    """The BIM_Join_Miter command creates a miter joint between two objects."""

    def GetResources(self):
        return {
            "Pixmap": "BIM_Join_Miter",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Join_Miter", "Miter joint"),
            "ToolTip": QT_TRANSLATE_NOOP("BIM_Join_Miter", "Creates a miter joint between two supported objects."),
        }

    def calculate_cutting_planes(self, baseline1, baseline2, intersection, width1, width2):
        # Phase 3: Logic for Miter join will go here.
        return super().calculate_cutting_planes(baseline1, baseline2, intersection, width1, width2)


class BIM_Join_Tee(BIM_Join):
    """The BIM_Join_Tee command creates a tee joint between two objects."""

    def GetResources(self):
        return {
            "Pixmap": "BIM_Join_Tee",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Join_Tee", "Tee joint"),
            "ToolTip": QT_TRANSLATE_NOOP("BIM_Join_Tee", "Creates a tee joint between two supported objects."),
        }

    def calculate_cutting_planes(self, baseline1, baseline2, intersection, width1, width2):
        # Phase 3: Logic for Tee join will go here.
        return super().calculate_cutting_planes(baseline1, baseline2, intersection, width1, width2)


class BIM_Join_Butt(BIM_Join):
    """The BIM_Join_Butt command creates a butt joint between two objects."""

    def GetResources(self):
        return {
            "Pixmap": "BIM_Join_Butt",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Join_Butt", "Butt joint"),
            "ToolTip": QT_TRANSLATE_NOOP("BIM_Join_Butt", "Creates a butt joint between two supported objects."),
        }

    def calculate_cutting_planes(self, baseline1, baseline2, intersection, width1, width2):
        # Phase 3: Logic for Butt join will go here.
        return super().calculate_cutting_planes(baseline1, baseline2, intersection, width1, width2)


# Register the commands with FreeCAD's GUI
FreeCADGui.addCommand("BIM_Join_Miter", BIM_Join_Miter())
FreeCADGui.addCommand("BIM_Join_Tee", BIM_Join_Tee())
FreeCADGui.addCommand("BIM_Join_Butt", BIM_Join_Butt())
