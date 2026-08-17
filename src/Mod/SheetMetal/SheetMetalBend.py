########################################################################
#
#  SheetMetalBend.py
#
#  Copyright 2015 Shai Seger <shaise at gmail dot com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

import os

import FreeCAD
import Part

import SheetMetalTools

smEpsilon = SheetMetalTools.smEpsilon

# IMPORTANT: please remember to change the element map version in case
# of any changes in modeling logic.
smElementMapVersion = "sm1."

# List of properties to be saved as defaults.
smAddBendDefaultVars = [("radius", "defaultRadius")]


def smGetClosestVert(vert, face):
    closestVert = None
    closestDist = 99999999
    for v in face.Vertexes:
        if vert.isSame(v):
            continue
        d = vert.distToShape(v)[0]
        if d < closestDist:
            closestDist = d
            closestVert = v
    return closestVert


# We look for a matching inner edge to the selected outer one.
# This function finds a single vertex of that edge.
def smFindMatchingVert(shape, edge, vertid):
    facelist = shape.ancestorsOfType(edge, Part.Face)
    edgeVerts = edge.Vertexes
    v = edgeVerts[vertid]
    vfacelist = shape.ancestorsOfType(v, Part.Face)
    # Find the face that is not in facelist.
    for vface in vfacelist:
        if not vface.isSame(facelist[0]) and not vface.isSame(facelist[1]):
            break
    return smGetClosestVert(v, vface)


def smFindEdgeByVerts(shape, vert1, vert2):
    for edge in shape.Edges:
        if vert1.isSame(edge.Vertexes[0]) and vert2.isSame(edge.Vertexes[1]):
            break
        if vert1.isSame(edge.Vertexes[1]) and vert2.isSame(edge.Vertexes[0]):
            break
    else:
        edge = None
    return edge


def smSolidBend(radius=1.0, selEdgeNames="", MainObject=None):
    InnerEdgesToBend = []
    OuterEdgesToBend = []
    for selEdgeName in selEdgeNames:
        edge = MainObject.getElement(SheetMetalTools.getElementFromTNP(selEdgeName))

        # Find matching inner edge to selected outer one.
        v1 = smFindMatchingVert(MainObject, edge, 0)
        v2 = smFindMatchingVert(MainObject, edge, 1)
        matchingEdge = smFindEdgeByVerts(MainObject, v1, v2)
        if matchingEdge is not None:
            InnerEdgesToBend.append(matchingEdge)
            OuterEdgesToBend.append(edge)

    if len(InnerEdgesToBend) > 0:
        # Find thickness of sheet by distance from v1 to one of the
        # edges coming out of edge[0].
        #
        # We assume all corners have same thickness.
        for dedge in MainObject.ancestorsOfType(edge.Vertexes[0], Part.Edge):
            if not dedge.isSame(edge):
                break
        thickness = v1.distToShape(dedge)[0]
        resultSolid = MainObject.makeFillet(radius, InnerEdgesToBend)
        resultSolid = resultSolid.makeFillet(radius + thickness, OuterEdgesToBend)
    return resultSolid


class SMSolidBend:
    def __init__(self, obj, selobj, sel_elements):
        """Add Bend to Solid."""
        self.addVerifyProperties(obj)
        _tip_ = FreeCAD.Qt.translate("App::Property", "Base object")
        obj.addProperty("App::PropertyLinkSub", "baseObject", "Parameters", _tip_
                        ).baseObject = (selobj, sel_elements)
        obj.Proxy = self
        SheetMetalTools.taskRestoreDefaults(obj, smAddBendDefaultVars)

    def addVerifyProperties(self, obj):
        SheetMetalTools.smAddLengthProperty(obj, "radius",
                                            FreeCAD.Qt.translate("App::Property", "Bend Radius"),
                                            1.0)
        # SheetMetalTools.smAddBoolProperty(obj, "Refine",
        #                                   FreeCAD.Qt.translate("App::Property", "Use Refine"),
        #                                   False)

    def getElementMapVersion(self, _fp, ver, _prop, restored):
        if not restored:
            return smElementMapVersion + ver
        return None

    def execute(self, fp):
        """Print a short message when doing a recomputation.

        Note:
            This method is mandatory.

        """
        self.addVerifyProperties(fp)
        Main_Object = fp.baseObject[0].Shape.copy()
        fp.Shape = smSolidBend(radius=fp.radius.Value,
                               selEdgeNames=fp.baseObject[1],
                               MainObject=Main_Object)


###################################################################################################
# Gui code
###################################################################################################

if SheetMetalTools.isGuiLoaded():
    Gui = FreeCAD.Gui
    icons_path = SheetMetalTools.icons_path


    class SMBendViewProviderTree(SheetMetalTools.SMViewProvider):
        """Part WB style ViewProvider."""

        def getIcon(self):
            return os.path.join(icons_path, "SheetMetal_AddBend.svg")

        def getTaskPanel(self, obj):
            return SMBendTaskPanel(obj)


    class SMBendViewProviderFlat(SMBendViewProviderTree):
        """Part Design WB style ViewProvider.

        Note:
            Backward compatibility only.

        """


    class SMBendTaskPanel:
        """A TaskPanel for the SheetMetal."""

        def __init__(self, obj):
            self.obj = obj
            self.form = SheetMetalTools.taskLoadUI("BendCornerPanel.ui")
            # Make sure all properties are added
            obj.Proxy.addVerifyProperties(obj)

            self.selParams = SheetMetalTools.taskConnectSelection(self.form.AddRemove,
                                                                  self.form.tree,
                                                                  self.obj,
                                                                  ["Edge"],
                                                                  self.form.pushClearSel)
            SheetMetalTools.taskConnectSpin(obj, self.form.Radius, "radius")
            # SheetMetalTools.taskConnectCheck(obj, self.form.RefineCheckbox, "Refine")

        def isAllowedAlterSelection(self):
            return True

        def isAllowedAlterView(self):
            return True

        def accept(self):
            SheetMetalTools.taskAccept(self)
            SheetMetalTools.taskSaveDefaults(self.obj, smAddBendDefaultVars)
            return True

        def reject(self):
            SheetMetalTools.taskReject(self)

        # def retranslateUi(self, SMBendTaskPanel):


    class AddBendCommandClass:
        """Add Solid Bend command."""

        def GetResources(self):
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(icons_path, "SheetMetal_AddBend.svg"),
                    "MenuText": FreeCAD.Qt.translate("SheetMetal", "Make Bend"),
                    "Accel": "S, B",
                    "ToolTip": FreeCAD.Qt.translate(
                        "SheetMetal",
                        "Create Bend where two walls come together on solids\n"
                        "1. Select edge(s) to create bend on corner edge(s).\n"
                        "2. Use Property editor to modify parameters"),
                    }

        def Activated(self):
            sel = Gui.Selection.getSelectionEx()[0]
            selobj = sel.Object
            newObj, activeBody = SheetMetalTools.smCreateNewObject(selobj, "SolidBend")
            if newObj is None:
                return
            SMSolidBend(newObj, selobj, sel.SubElementNames)
            SMBendViewProviderFlat(newObj.ViewObject)
            SheetMetalTools.smAddNewObject(selobj, newObj, activeBody, SMBendTaskPanel)
            return

        def IsActive(self):
            if (len(Gui.Selection.getSelection()) < 1
                    or len(Gui.Selection.getSelectionEx()[0].SubElementNames) < 1):
                return False
            for selFace in Gui.Selection.getSelectionEx()[0].SubObjects:
                if not isinstance(selFace, Part.Edge):
                    return False
            return True


    Gui.addCommand("SheetMetal_AddBend", AddBendCommandClass())
