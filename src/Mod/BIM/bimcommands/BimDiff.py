# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""The BIM Diff command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_Diff:
    def GetResources(self):
        return {
            "Pixmap": "BIM_Diff",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Diff", "IFC Diff"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Diff", "Shows the difference between two IFC-based documents"
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        # you need two documents open, containing BIM objects with same IDs
        # make the main doc the active one before running this script!

        # what will be compared: IDs, geometry, materials. Everything else is discarded.
        from PySide import QtGui
        import Draft
        import Part

        MOVE_TOLERANCE = 0.2  # the max allowed move in mm
        VOL_TOLERANCE = 250  # the max allowed volume diff in mm^3

        documents = FreeCAD.listDocuments()

        if len(documents) == 2:
            reply = QtGui.QMessageBox.question(
                None,
                "",
                translate(
                    "BIM",
                    "The current document must be the main one. The other contains newer objects to merge into it. Ensure that only the objects intended for comparison are visible in both documents. Proceed?",
                ),
                QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                QtGui.QMessageBox.No,
            )
            if reply == QtGui.QMessageBox.Yes:
                activedoc = FreeCAD.ActiveDocument
                if list(documents.keys())[0] == activedoc.Name:
                    otherdoc = list(documents.values())[1]
                else:
                    otherdoc = list(documents.values())[0]

                # build lists of BIM objects with IFC ID

                objswithoutid = []  # let's try to match these later on

                activedocids = {}  # main document, the original freecad one
                for obj in activedoc.Objects:
                    if hasattr(obj, "IfcData") and obj.ViewObject.Visibility:
                        if "IfcUID" in obj.IfcData:
                            activedocids[obj.IfcData["IfcUID"]] = obj
                        elif obj.isDerivedFrom(
                            "Part::Feature"
                        ):  # discard BuildingParts
                            objswithoutid.append(obj)

                otherdocids = {}  # other doc to be merged to the main one
                for obj in otherdoc.Objects:
                    if hasattr(obj, "IfcData") and obj.ViewObject.Visibility:
                        if "IfcUID" in obj.IfcData:
                            otherdocids[obj.IfcData["IfcUID"]] = obj

                toselect = []  # objects to select when finished
                additions = []  # objects added
                subtractions = []  # objects subtracted
                modified = []  # objects modified
                moved = []  # objects moved
                matchanged = []  # object is same, but material changed
                matchangedghost = (
                    []
                )  # store shapes of objects whose material has changed to print a blue ghost later on
                renamed = {}  # object label changes
                propertieschanged = {}  # objects whose IFC properties are different

                for id, obj in otherdocids.items():
                    if id in activedocids:
                        # this object already exists
                        mainobj = activedocids[id]
                        if obj.Label != mainobj.Label:
                            # object has a different name
                            renamed[mainobj.Name] = obj.Label
                        if obj.IfcProperties and (
                            obj.IfcProperties != mainobj.IfcProperties
                        ):
                            # properties have changed
                            propertieschanged[id] = obj.IfcProperties
                        if hasattr(obj, "Shape") and hasattr(mainobj, "Shape"):
                            v = abs(obj.Shape.Volume - mainobj.Shape.Volume)
                            if v < VOL_TOLERANCE:
                                # identical volume
                                l = (
                                    obj.Shape.BoundBox.Center.sub(
                                        mainobj.Shape.BoundBox.Center
                                    )
                                ).Length
                                if l < MOVE_TOLERANCE:
                                    # identical position
                                    if (
                                        abs(
                                            obj.Shape.BoundBox.XMin
                                            - mainobj.Shape.BoundBox.XMin
                                        )
                                        < MOVE_TOLERANCE
                                        and abs(
                                            obj.Shape.BoundBox.YMin
                                            - mainobj.Shape.BoundBox.YMin
                                        )
                                        < MOVE_TOLERANCE
                                        and abs(
                                            obj.Shape.BoundBox.YMin
                                            - mainobj.Shape.BoundBox.YMin
                                        )
                                        < MOVE_TOLERANCE
                                    ):
                                        # same boundbox
                                        if (
                                            hasattr(obj, "Material")
                                            and hasattr(mainobj, "Material")
                                            and (
                                                obj.Material
                                                and mainobj.Material
                                                and (
                                                    obj.Material.Label
                                                    == mainobj.Material.Label
                                                )
                                            )
                                            or (obj.Material == mainobj.Material)
                                        ):
                                            # same material names
                                            obj.ViewObject.hide()
                                        else:
                                            print(
                                                "Object",
                                                mainobj.Label,
                                                "material has changed",
                                            )
                                            obj.ViewObject.hide()  # we hide these objects since the shape hasn't changed but we keep their shapes
                                            matchangedghost.append(obj.Shape)
                                            matchanged.append(obj)
                                    else:
                                        print(
                                            "Object",
                                            mainobj.Label,
                                            "shape bound box has changed",
                                        )
                                        toselect.append(obj)
                                        modified.append(obj)
                                else:
                                    print(
                                        "Object",
                                        mainobj.Label,
                                        "position has moved by",
                                        l,
                                        "mm",
                                    )
                                    toselect.append(obj)
                                    moved.append(obj)
                            else:
                                print(
                                    "Object",
                                    mainobj.Label,
                                    "shape has changed by",
                                    v,
                                    "mm^3",
                                )
                                toselect.append(obj)
                                modified.append(obj)
                        else:
                            print(
                                "Object",
                                mainobj.Label,
                                "one of the objects has no shape",
                            )
                            toselect.append(obj)
                    else:
                        print("Object", obj.Label, "does not exist yet in main document")
                        toselect.append(obj)
                        additions.append(obj)

                for id, obj in activedocids.items():
                    if not id in otherdocids:
                        if obj.isDerivedFrom(
                            "Part::Feature"
                        ):  # don't count building parts
                            print(
                                "Object", obj.Label, "does not exist anymore in new document"
                            )
                            subtractions.append(obj)

                # try to find our objects without ID
                newids = {}
                for obj in objswithoutid:
                    for id, otherobj in otherdocids.items():
                        if not id in activedocids:
                            if (
                                abs(otherobj.Shape.Volume - obj.Shape.Volume)
                                < VOL_TOLERANCE
                            ):
                                if (
                                    otherobj.Shape.BoundBox.Center.sub(
                                        obj.Shape.BoundBox.Center
                                    )
                                ).Length < MOVE_TOLERANCE:
                                    if (
                                        abs(
                                            obj.Shape.BoundBox.XMin
                                            - otherobj.Shape.BoundBox.XMin
                                        )
                                        < MOVE_TOLERANCE
                                        and abs(
                                            obj.Shape.BoundBox.YMin
                                            - otherobj.Shape.BoundBox.YMin
                                        )
                                        < MOVE_TOLERANCE
                                        and abs(
                                            obj.Shape.BoundBox.YMin
                                            - otherobj.Shape.BoundBox.YMin
                                        )
                                        < MOVE_TOLERANCE
                                    ):
                                        # shapes are identical. It's the same object!
                                        newids[obj.Name] = id
                                        break
                    else:
                        print(
                            "Object",
                            obj.Label,
                            "has no ID and was not found in the new document",
                        )
                        subtractions.append(obj)

                matnames = {}  # existing materials
                for obj in activedoc.Objects:
                    if Draft.getType(obj) == "Material":
                        matnames[obj.Label] = obj

                newmats = {}  # new materials
                for obj in otherdoc.Objects:
                    if Draft.getType(obj) == "Material":
                        if not obj.Label in matnames:
                            print("Material", obj.Label, "does not exist in main document")
                            toselect.append(obj)
                            newmats[obj.Label] = obj

                if newmats:
                    group = otherdoc.addObject(
                        "App::DocumentObjectGroup", "New_materials"
                    )
                    for newmat in newmats.values():
                        group.addObject(newmat)

                if toselect:
                    FreeCAD.setActiveDocument(otherdoc.Name)
                    FreeCAD.ActiveDocument = FreeCADGui.getDocument(otherdoc.Name)
                    FreeCADGui.ActiveDocument = FreeCADGui.getDocument(otherdoc.Name)
                    FreeCADGui.Selection.clearSelection()
                    for obj in toselect:
                        FreeCADGui.Selection.addSelection(obj)

                if additions:
                    shape = Part.makeCompound([a.Shape for a in additions])
                    obj = activedoc.addObject("Part::Feature", "Additions")
                    obj.Shape = shape
                    obj.ViewObject.LineWidth = 5
                    obj.ViewObject.LineColor = (0.0, 1.0, 0.0)
                    obj.ViewObject.ShapeColor = (0.0, 1.0, 0.0)
                    obj.ViewObject.Transparency = 60

                if subtractions:
                    shape = Part.makeCompound([s.Shape for s in subtractions])
                    obj = activedoc.addObject("Part::Feature", "Subtractions")
                    obj.Shape = shape
                    obj.ViewObject.LineWidth = 5
                    obj.ViewObject.LineColor = (1.0, 0.0, 0.0)
                    obj.ViewObject.ShapeColor = (1.0, 0.0, 0.0)
                    obj.ViewObject.Transparency = 60

                if modified:
                    shape = Part.makeCompound([m.Shape for m in modified])
                    obj = activedoc.addObject("Part::Feature", "Modified")
                    obj.Shape = shape
                    obj.ViewObject.LineWidth = 5
                    obj.ViewObject.LineColor = (1.0, 0.5, 0.0)
                    obj.ViewObject.ShapeColor = (1.0, 0.5, 0.0)
                    obj.ViewObject.Transparency = 60

                if moved:
                    shape = Part.makeCompound([m.Shape for m in moved])
                    obj = activedoc.addObject("Part::Feature", "Moved")
                    obj.Shape = shape
                    obj.ViewObject.LineWidth = 5
                    obj.ViewObject.LineColor = (1.0, 1.0, 0.0)
                    obj.ViewObject.ShapeColor = (1.0, 1.0, 0.0)
                    obj.ViewObject.Transparency = 60

                if matchangedghost:
                    shape = Part.makeCompound(matchangedghost)
                    obj = otherdoc.addObject("Part::Feature", "Material_changed")
                    obj.Shape = shape
                    obj.ViewObject.LineWidth = 1
                    obj.ViewObject.LineColor = (0.0, 0.0, 1.0)
                    obj.ViewObject.ShapeColor = (0.0, 0.0, 1.0)
                    obj.ViewObject.Transparency = 90

                if matchanged:
                    reply = QtGui.QMessageBox.question(
                        None,
                        "",
                        str(len(matchanged))
                        + " "
                        + translate(
                            "BIM",
                            "objects still have the same shape but have a different material. Update them in the main document?",
                        ),
                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                        QtGui.QMessageBox.No,
                    )
                    if reply == QtGui.QMessageBox.Yes:
                        for obj in matchanged:
                            mat = obj.Material
                            if mat:
                                mainobj = activedocids[obj.IfcData["IfcUID"]]
                                if mainobj.Material:
                                    mainmatlabel = mainobj.Material.Label
                                else:
                                    mainmatlabel = "None"
                                if mat.Label in matnames:
                                    # the new material already exists, just change it
                                    print(
                                        "Changing material of",
                                        mainobj.Label,
                                        "from",
                                        mainmatlabel,
                                        "to",
                                        mat.Label,
                                    )
                                    mainobj.Material = matnames[mat.Label]
                                else:
                                    # copy the material over
                                    newmat = activedoc.addObject(
                                        "App::MaterialObjectPython", "Material"
                                    )
                                    newmat.Label = mat.Label
                                    import ArchMaterial

                                    ArchMaterial._ArchMaterial(newmat)
                                    ArchMaterial._ViewProviderArchMaterial(
                                        newmat.ViewObject
                                    )
                                    newmat.Material = mat.Material
                                    print(
                                        "Changing material of",
                                        mainobj.Label,
                                        "from",
                                        mainmatlabel,
                                        "to",
                                        newmat.Label,
                                    )
                                    mainobj.Material = newmat
                                    matnames[newmat.Label] = newmat

                if newids:
                    reply = QtGui.QMessageBox.question(
                        None,
                        "",
                        str(len(newids))
                        + " "
                        + translate(
                            "BIM",
                            "objects have no IFC ID in the main document, but an identical object with an ID exists in the new document. Transfer these IDs to the original objects?",
                        ),
                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                        QtGui.QMessageBox.No,
                    )
                    if reply == QtGui.QMessageBox.Yes:
                        for name, id in newids.items():
                            obj = activedoc.getObject(name)
                            if obj:
                                print("Transferring new ID to object", obj.Label)
                                a = obj.IfcData
                                a["IfcUID"] = id
                                obj.IfcData = a

                if renamed:
                    reply = QtGui.QMessageBox.question(
                        None,
                        "",
                        str(len(renamed))
                        + " "
                        + translate(
                            "BIM", "objects had their name changed. Rename them?"
                        ),
                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                        QtGui.QMessageBox.No,
                    )
                    if reply == QtGui.QMessageBox.Yes:
                        for name, label in renamed.items():
                            obj = activedoc.getObject(name)
                            if obj:
                                print("Renaming object", obj.Label, "to", label)
                                obj.Label = label

                if propertieschanged:
                    reply = QtGui.QMessageBox.question(
                        None,
                        "",
                        str(len(propertieschanged))
                        + " "
                        + translate(
                            "BIM", "objects had their properties changed. Update?"
                        ),
                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                        QtGui.QMessageBox.No,
                    )
                    if reply == QtGui.QMessageBox.Yes:
                        for id, prop in propertieschanged.items():
                            obj = activedocids[id]
                            print("Updating properties of ", obj.Label)
                            obj.IfcProperties = prop

                if moved:
                    reply = QtGui.QMessageBox.question(
                        None,
                        "",
                        str(len(moved))
                        + " "
                        + translate(
                            "BIM",
                            "objects have their location changed. Move them to their new position?",
                        ),
                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                        QtGui.QMessageBox.No,
                    )
                    if reply == QtGui.QMessageBox.Yes:
                        for obj in moved:
                            mainobj = activedocids[obj.IfcData["IfcUID"]]
                            otherobj = otherdocids[obj.IfcData["IfcUID"]]
                            delta = otherobj.Shape.BoundBox.Center.sub(
                                mainobj.Shape.BoundBox.Center
                            )
                            print("Moving object ", mainobj.Label)
                            Draft.move(mainobj, delta)
                    reply = QtGui.QMessageBox.question(
                        None,
                        "",
                        translate(
                            "BIM",
                            "Colorize the objects that have moved in yellow in the other file (to serve as a diff)?",
                        ),
                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                        QtGui.QMessageBox.No,
                    )
                    if reply == QtGui.QMessageBox.Yes:
                        for obj in moved:
                            otherobj = otherdocids[obj.IfcData["IfcUID"]]
                            try:
                                otherobj.ViewObject.LineColor = (1.0, 1.0, 0.0)
                                otherobj.ViewObject.ShapeColor = (1.0, 1.0, 0.0)
                                otherobj.ViewObject.Transparency = 60
                            except AttributeError:
                                print(otherobj.Label, "cannot be colorized")

                if modified:
                    reply = QtGui.QMessageBox.question(
                        None,
                        "",
                        translate(
                            "BIM",
                            "Colorize the objects that have been modified in orange in the other file (to serve as a diff)?",
                        ),
                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                        QtGui.QMessageBox.No,
                    )
                    if reply == QtGui.QMessageBox.Yes:
                        for obj in modified:
                            otherobj = otherdocids[obj.IfcData["IfcUID"]]
                            try:
                                otherobj.ViewObject.LineColor = (1.0, 0.5, 0.0)
                                otherobj.ViewObject.ShapeColor = (1.0, 0.5, 0.0)
                                otherobj.ViewObject.Transparency = 60
                            except AttributeError:
                                print(otherobj.Label, "cannot be colorized")

                if subtractions:
                    reply = QtGui.QMessageBox.question(
                        None,
                        "",
                        str(len(subtractions))
                        + " "
                        + translate(
                            "BIM",
                            "objects do not exist anymore in the new document. Move them to a 'To Delete' group?",
                        ),
                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                        QtGui.QMessageBox.No,
                    )
                    if reply == QtGui.QMessageBox.Yes:
                        group = activedoc.addObject(
                            "App::DocumentObjectGroup", "ToDelete"
                        )
                        group.Label = "To Delete"
                        for obj in subtractions:
                            group.addObject(obj)
                    reply = QtGui.QMessageBox.question(
                        None,
                        "",
                        translate(
                            "BIM",
                            "Colorize the objects that have been removed in red in the other file (to serve as a diff)?",
                        ),
                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                        QtGui.QMessageBox.No,
                    )
                    if reply == QtGui.QMessageBox.Yes:
                        for obj in subtractions:
                            otherobj = otherdoc.addObject("Part::Feature", "Deleted")
                            otherobj.Shape = obj.Shape
                            otherobj.ViewObject.LineColor = (1.0, 0.0, 0.0)
                            otherobj.ViewObject.ShapeColor = (1.0, 0.0, 0.0)
                            otherobj.ViewObject.Transparency = 60

                if additions:
                    reply = QtGui.QMessageBox.question(
                        None,
                        "",
                        translate(
                            "BIM",
                            "Colorize the objects that have been added in green in the other file (to serve as a diff)?",
                        ),
                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                        QtGui.QMessageBox.No,
                    )
                    if reply == QtGui.QMessageBox.Yes:
                        for obj in additions:
                            otherobj = otherdocids[obj.IfcData["IfcUID"]]
                            try:
                                otherobj.ViewObject.LineColor = (0.0, 1.0, 0.0)
                                otherobj.ViewObject.ShapeColor = (0.0, 1.0, 0.0)
                                otherobj.ViewObject.Transparency = 60
                            except AttributeError:
                                print(otherobj.Label, "cannot be colorized")

        else:
            QtGui.QMessageBox.information(
                None,
                "",
                translate(
                    "BIM",
                    "Two documents are required to be open to run this tool. One which is the main document, and one that contains new objects to compare against the existing one. Make sure only the objects to compare in both documents are visible.",
                ),
            )


FreeCADGui.addCommand("BIM_Diff", BIM_Diff())
