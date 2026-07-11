########################################################################
#
#  ExtrudedCutout.py
#
#  Copyright 2024 by @sheetmetalman
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

import os  # Make sure 'os' is imported for the icon path.

import FreeCAD
import Part

import SheetMetalTools

SMException = SheetMetalTools.SMException

# List of properties to be saved as defaults.
smExtrudedCutoutDefaultVars = []


class ExtrudedCutout:
    def __init__(self, obj, sketch, selected_face):
        """Initialize the parametric Sheet Metal Cut object and add
        properties.
        """
        obj.addProperty("App::PropertyLink", "Sketch", "ExtrudedCutout",
                FreeCAD.Qt.translate("SheetMetal", "The sketch for the cut"),
        ).Sketch = sketch

        # Add other properties (is necessary this way to not cause
        # errors on old files).
        self.addVerifyProperties(obj, selected_face)

        obj.setEditorMode("ImproveLevel", 2)  # Hide by default
        obj.addProperty("App::PropertyLength", "ExtrusionLength1", "ExtrudedCutout",
                FreeCAD.Qt.translate("SheetMetal", "Length of the extrusion direction 1"),
        ).ExtrusionLength1 = 500.0
        obj.setEditorMode("ExtrusionLength1", 2)  # Hide by default
        obj.addProperty("App::PropertyLength", "ExtrusionLength2", "ExtrudedCutout",
                FreeCAD.Qt.translate("SheetMetal", "Length of the extrusion direction 2"),
        ).ExtrusionLength2 = 500.0
        obj.setEditorMode("ExtrusionLength2", 2)  # Hide by default

        # CutType property
        obj.addProperty("App::PropertyEnumeration", "CutType", "ExtrudedCutout",
                        FreeCAD.Qt.translate("SheetMetal", "Cut type")).CutType = [
                "Two dimensions",
                "Symmetric",
                "Through everything both sides",
                "Through everything side 1",
                "Through everything side 2",
        ]
        obj.CutType = "Through everything both sides"  # Default value.

        # CutSide property.
        obj.addProperty("App::PropertyEnumeration", "CutSide", "ExtrudedCutout",
                        FreeCAD.Qt.translate("SheetMetal", "Side of the cut")).CutSide = [
                "Inside",
                "Outside",
        ]
        obj.CutSide = "Inside"  # Default value.
        SheetMetalTools.taskRestoreDefaults(obj, smExtrudedCutoutDefaultVars)

        obj.Proxy = self

    def addVerifyProperties(self, obj, selected_face=None):
        SheetMetalTools.smAddProperty(obj, "App::PropertyLinkSub", "baseObject",
                                      FreeCAD.Qt.translate("SheetMetal", "Selected face"),
                                      selected_face,
                                      "ExtrudedCutout",
                                      # Rename SelectedFace to baseObject
                                      # for compatibility with GUI system.
                                      "SelectedFace")
        SheetMetalTools.smAddBoolProperty(obj, "Refine",
                FreeCAD.Qt.translate("SheetMetal", "Refine the geometry"),
                False,
                "ExtrudedCutoutImprovements")

        SheetMetalTools.smAddProperty(obj, "App::PropertyIntegerConstraint", "ImproveLevel",
                FreeCAD.Qt.translate("SheetMetal",
                    "Level of cut improvement quality. More than 10 can take a very long time"),
                (4, 2, 20, 1),
                "ExtrudedCutoutImprovements")

        SheetMetalTools.smAddBoolProperty(obj, "ImproveCut",
                FreeCAD.Qt.translate("SheetMetal",
                    "Improve cut geometry if it enters the cutting zone. Only select true if the "
                    "cut needs fix, 'cause it can be slow"),
                False,
                "ExtrudedCutoutImprovements")

    def onChanged(self, fp, prop):
        """Respond to property changes."""
        # Show or hide improvement of the cut.
        if prop == "ImproveCut":
            if fp.ImproveCut == True:
                fp.setEditorMode("ImproveLevel", 0)  # Show
            if fp.ImproveCut == False:
                fp.setEditorMode("ImproveLevel", 2)  # Hide

        # Show or hide length properties based in the CutType property.
        if prop == "CutType":
            if fp.CutType == "Two dimensions":
                fp.setEditorMode("ExtrusionLength1", 0)  # Show
                fp.setEditorMode("ExtrusionLength2", 0)  # Show
            elif fp.CutType == "Symmetric":
                fp.setEditorMode("ExtrusionLength1", 0)  # Show
                fp.setEditorMode("ExtrusionLength2", 2)  # Hide
            else:
                fp.setEditorMode("ExtrusionLength1", 2)  # Hide
                fp.setEditorMode("ExtrusionLength2", 2)  # Hide

    def execute(self, fp):
        """Perform the cut when the object is recomputed."""

        self.addVerifyProperties(fp)

        try:
            # Ensure the Sketch and baseObject properties are valid.
            if fp.Sketch is None or fp.baseObject is None:
                raise SMException("Both the Sketch and baseObject properties must be set.")

            # Get the sketch from the properties.
            cutSketch = fp.Sketch

            # Get selected object and selected face from the properties.
            selected_object, face_name = fp.baseObject
            face_name = face_name[0]
            selected_face = selected_object.Shape.getElement(face_name)
            normal_vector = selected_face.normalAt(0, 0)

            # Lengths.
            if fp.CutType == "Two dimensions":
                ExtLength1 = fp.ExtrusionLength1.Value
                ExtLength2 = fp.ExtrusionLength2.Value
            elif fp.CutType == "Symmetric":
                ExtLength1 = fp.ExtrusionLength1.Value / 2
                ExtLength2 = fp.ExtrusionLength1.Value / 2
            else:
                skCenter = cutSketch.Shape.BoundBox.Center
                objCenter = selected_object.Shape.BoundBox.Center
                distance = skCenter - objCenter
                TotalLength = selected_object.Shape.BoundBox.DiagonalLength + distance.Length

                if fp.CutType == "Through everything both sides":
                    ExtLength1 = TotalLength
                    ExtLength2 = TotalLength
                elif fp.CutType == "Through everything side 1":
                    ExtLength1 = TotalLength
                    ExtLength2 = -TotalLength
                else:
                    # "Through everything side 2".
                    ExtLength1 = -TotalLength
                    ExtLength2 = TotalLength

            # Step 1: Determine the sheet metal thickness.
            min_distance = float("inf")

            faces = selected_object.Shape.Faces
            for face in faces:
                if face is not selected_face:
                    # Test to find a face with opposite normal.
                    if normal_vector.isEqual(face.normalAt(0, 0).multiply(-1), 1e-6):
                        distance_info = selected_face.distToShape(face)
                        distance = distance_info[0]
                        # Test to find the closest opposite face.
                        if distance < min_distance:
                            try:
                                checkFace = face.makeOffsetShape(-distance, 0)
                                checkCut = selected_face.cut(checkFace)

                                # Test to ensure the opposite face is,
                                # in fact, the other side of the sheet
                                # metal part.
                                if checkCut.Area < 1e-6:
                                    min_distance = distance

                            # Is necessary 'try' and 'except' because
                            # rounded surfaces offset can lead to errors
                            # if offset is bigger than its radius.
                            except:
                                FreeCAD.Console.PrintLog("Skip wrong offset")
                                continue

            if min_distance == float("inf"):
                raise SMException("No opposite face found to calculate thickness.")

            # Appear that rounding can help on speed performance of the
            # rest of the code.
            thickness = round(min_distance, 4)

            # Step 2: Find pairs of parallel faces.
            parallel_faces = []
            for i, face1 in enumerate(faces):
                for j, face2 in enumerate(faces):
                    if i >= j:
                        continue
                    if face1.normalAt(0, 0).isEqual(face2.normalAt(0, 0).multiply(-1), 1e-6):
                        distance_info = face1.distToShape(face2)
                        distance = distance_info[0]

                        # In the past, this tolerance was `1e-6`, it's
                        # leads to errors.
                        if abs(distance - thickness) < 1e-5:
                            parallel_faces.extend([face1, face2])

            if parallel_faces:
                shell = Part.Shell(parallel_faces)
            else:
                raise SMException(
                        "No pairs of parallel faces with the specified"
                        " thickness distance were found.")

            # Surfaces to improve the cut geometry.
            if fp.ImproveCut:
                smSide1 = self.find_connected_faces(shell)
                smSide1 = Part.Shell(smSide1[0])

                tknOffStep = thickness / fp.ImproveLevel

                improvSurfaces = []
                tknOff = tknOffStep
                while abs(thickness - tknOff) > 1e-6:
                    sideOff = smSide1.makeOffsetShape(-tknOff, 0)
                    improvSurfaces.append(sideOff)
                    tknOff = tknOff + tknOffStep
                improvShell = improvSurfaces

            # Step 3: Extrude the cut sketch.
            #
            # Get all faces in sketch.
            skWiresList = cutSketch.Shape.Wires
            myFacesList = []
            for wire in skWiresList:
                myFace = Part.Face(wire)
                myFacesList.append(myFace)

            compFaces = Part.Compound(myFacesList)

            if ExtLength1 == 0 and ExtLength2 == 0:
                raise SMException("Cut length cannot be zero for both sides.")
            else:
                if ExtLength1 == 0:
                    ExtLength1 = -ExtLength2
                if ExtLength2 == 0:
                    ExtLength2 = -ExtLength1

                ExtLength1 = compFaces.Faces[0].normalAt(0, 0) * (-ExtLength1)
                ExtLength2 = compFaces.Faces[0].normalAt(0, 0) * ExtLength2
                myExtrusion1 = compFaces.extrude(ExtLength1)
                myExtrusion2 = compFaces.extrude(ExtLength2)

                if fp.Refine:
                    myUnion = Part.Solid.fuse(myExtrusion1, myExtrusion2).removeSplitter()
                else:
                    myUnion = Part.Solid.fuse(myExtrusion1, myExtrusion2)

                myCommon = myUnion.common(shell)
                # Intersection with the improvement surfaces.
                if fp.ImproveCut:
                    myCommImprov = myUnion.common(improvShell)

            # Step 4: Find connected components and offset shapes.
            connected_components = self.find_connected_faces(myCommon)
            offset_shapes = []
            for component in connected_components:
                component_shell = Part.Shell(component)
                if component_shell.isValid():
                    offset_shape = component_shell.makeOffsetShape(-thickness, 0, fill=True)
                    if offset_shape.isValid():
                        offset_shapes.append(offset_shape)

            if fp.ImproveCut:
                connected_improv = self.find_connected_faces(myCommImprov)
                offset_improv = []
                for improv in connected_improv:  # Offset to one side.
                    improv_shell = Part.Shell(improv)
                    offset_value = improv_shell.distToShape(smSide1)[0]
                    off_impr = improv_shell.makeOffsetShape(offset_value, 0, fill=True)
                    offset_improv.append(off_impr)
                for improv in connected_improv:  # Offset to other side.
                    improv_shell = Part.Shell(improv)
                    offset_value = thickness - improv_shell.distToShape(smSide1)[0]
                    off_impr = improv_shell.makeOffsetShape(-offset_value, 0, fill=True)
                    offset_improv.append(off_impr)

            # Step 5: Combine the offsets.
            if offset_shapes:
                combined_offset = Part.Solid(offset_shapes[0])
                for shape in offset_shapes[1:]:
                    combined_offset = combined_offset.fuse(shape)

                if fp.ImproveCut:
                    comb_impr_off = Part.Solid(offset_improv[0])
                    for impr_shape in offset_improv[1:]:
                        comb_impr_off = comb_impr_off.fuse(impr_shape)
                    combined_offset = combined_offset.fuse(comb_impr_off)

                    # Intersection with sheet metal faces.
                    cutOffsets = combined_offset.common(shell)
                    conn_offsetFaces = self.find_connected_faces(cutOffsets)
                    shapeCutOffsets = []
                    for offset in conn_offsetFaces:
                        offsetFace = Part.Shell(offset)
                        offsetSolid = offsetFace.makeOffsetShape(-thickness, 0, fill=True)
                        shapeCutOffsets.append(offsetSolid)

                    combined_offset = Part.Solid(shapeCutOffsets[0])
                    for shape in shapeCutOffsets[1:]:
                        combined_offset = combined_offset.fuse(shape)

                # Step 6: Cut.
                #
                # Check the "CutSide" property to decide how to perform
                # the cut.
                if fp.CutSide == "Inside":
                    if fp.Refine:
                        cut_result = selected_object.Shape.cut(combined_offset).removeSplitter()
                    else:
                        cut_result = selected_object.Shape.cut(combined_offset)
                elif fp.CutSide == "Outside":
                    if fp.Refine:
                        cut_result = selected_object.Shape.common(combined_offset).removeSplitter()
                    else:
                        cut_result = selected_object.Shape.common(combined_offset)
                else:
                    raise SMException("Invalid CutSide value.")

                fp.Shape = cut_result
            else:
                raise SMException("No valid offset shapes were created.")
        except SMException as e:
            FreeCAD.Console.PrintError(f"Error: {e}\n")

    def find_connected_faces(self, shape):
        """Find connected faces in a shape."""
        faces = shape.Faces
        visited = set()
        components = []

        def is_connected(face1, face2):
            for edge1 in face1.Edges:
                for edge2 in face2.Edges:
                    if edge1.isSame(edge2):
                        return True
            return False

        def dfs(face, component):
            visited.add(face)
            component.append(face)
            for next_face in faces:
                if next_face not in visited and is_connected(face, next_face):
                    dfs(next_face, component)

        for face in faces:
            if face not in visited:
                component = []
                dfs(face, component)
                components.append(component)
        return components


###################################################################################################
# Gui code
###################################################################################################

if SheetMetalTools.isGuiLoaded():
    Gui = FreeCAD.Gui
    icons_path = SheetMetalTools.icons_path


    class SMExtrudedCutoutVP(SheetMetalTools.SMViewProvider):
        """Part WB style ViewProvider."""

        def getIcon(self):
            return os.path.join(icons_path, "SheetMetal_AddCutout.svg")

        def getTaskPanel(self, obj):
            return SMExtrudedCutoutTaskPanel(obj)


    class SMExtrudedCutoutPDVP(SMExtrudedCutoutVP):
        """Part Design WB style ViewProvider.

        Note:
            Backward compatibility only.

        """


    class SMExtrudedCutoutTaskPanel:
        """A TaskPanel for the SheetMetal Extruded Cutout."""

        def __init__(self, obj):
            self.obj = obj
            self.form = SheetMetalTools.taskLoadUI("ExtrudedCutoutPanel.ui")
            self.LengthAText = FreeCAD.Qt.translate("SheetMetal", "Side A Length")
            self.LengthText = FreeCAD.Qt.translate("SheetMetal", "Length")

            # Make sure all properties are added.
            obj.Proxy.addVerifyProperties(obj)

            self.updateDisplay()

            self.faceSelParams = SheetMetalTools.taskConnectSelectionSingle(self.form.pushFace,
                    self.form.txtFace, obj, "baseObject", ["Face"])
            self.sketchSelParams = SheetMetalTools.taskConnectSelectionSingle(self.form.pushSketch,
                    self.form.txtSketch, obj, "Sketch", ("Sketcher::SketchObject", []))
            self.form.groupCutSide.buttonToggled.connect(self.cutSideChanged)
            SheetMetalTools.taskConnectEnum(obj, self.form.comboCutoutType, "CutType",
                                            self.cutTypeChanged)
            SheetMetalTools.taskConnectSpin(obj, self.form.unitLengthA, "ExtrusionLength1")
            SheetMetalTools.taskConnectSpin(obj, self.form.unitLengthB, "ExtrusionLength2")
            SheetMetalTools.taskConnectSpin(obj, self.form.intImproveLevel, "ImproveLevel")
            SheetMetalTools.taskConnectCheck(obj, self.form.checkImprove, "ImproveCut",
                                             self.improveChanged)
            SheetMetalTools.taskConnectCheck(obj, self.form.checkRefine, "Refine")

        def updateDisplay(self):
            if self.obj.CutSide == "Inside":
                self.form.radioInside.setChecked(True)
            else:
                self.form.radioOutside.setChecked(True)
            self.updateWidgetsVisibility()

        def cutSideChanged(self, button, checked):
            if not checked:
                return
            self.obj.CutSide = "Inside" if button == self.form.radioInside else "Outside"
            self.obj.Document.recompute()

        def improveChanged(self, isImprove):
            self.form.frameImproveLevel.setVisible(isImprove)

        def updateWidgetsVisibility(self):
            self.form.frameSideA.setVisible(self.obj.CutType in ["Two dimensions", "Symmetric"])
            self.form.frameSideB.setVisible(self.obj.CutType == "Two dimensions")
            self.form.labelSideA.setText(
                self.LengthText if self.obj.CutType == "Symmetric" else self.LengthAText)

        def cutTypeChanged(self, value):
            self.updateWidgetsVisibility()

        def isAllowedAlterSelection(self):
            return True

        def isAllowedAlterView(self):
            return True

        def accept(self):
            SheetMetalTools.taskAccept(self)
            SheetMetalTools.taskSaveDefaults(self.obj, smExtrudedCutoutDefaultVars)
            self.obj.Sketch.ViewObject.hide()  # Hide sketch after click OK button
            return True

        def reject(self):
            SheetMetalTools.taskReject(self)


    class AddExtrudedCutoutCommandClass:
        """Add Extruded Cutout command."""

        def GetResources(self):
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(icons_path, "SheetMetal_AddCutout.svg"),
                    "MenuText": FreeCAD.Qt.translate("SheetMetal", "Extruded Cutout"),
                    "Accel": "E, C",
                    "ToolTip": FreeCAD.Qt.translate(
                        "SheetMetal",
                        "Extruded cutout from sketch extrusion\n"
                        "1. Select a face of the sheet metal part (must not be the thickness face) and\n"
                        "2. Select a sketch for the extruded cut (the sketch must be closed).\n"
                        "3. Use Property editor to modify other parameters",
                        ),
                    }

        def Activated(self):
            """Create an Extruded Cutout object from user selections."""
            # Get the selected object and face.
            selection = Gui.Selection.getSelectionEx()[0]
            # When user select first the sketch
            if selection.Object.isDerivedFrom("Sketcher::SketchObject"):
                # Get selected sketch
                cutSketch = selection.Object
                # Check if we have any sub-objects (faces) selected.
                selection = Gui.Selection.getSelectionEx()[1]
                if len(selection.SubObjects) == 0:
                    raise SMException("No face selected. Please select a face.")
                # Get selected object.
                selected_object = selection.Object
                # Get the selected face.
                selected_face = [selected_object, selection.SubElementNames[0]]
            # When user select first the object face.
            else:
                # Check if we have any sub-objects (faces) selected.
                if len(selection.SubObjects) == 0:
                    raise SMException("No face selected. Please select a face.")
                # Get selected object.
                selected_object = selection.Object
                # Get the selected face.
                selected_face = [selected_object, selection.SubElementNames[0]]
                # Get selected sketch.
                selection = Gui.Selection.getSelectionEx()[1]
                cutSketch = selection.Object

            if cutSketch is None or not selected_object.Shape:
                raise SMException(
                    "Both a valid sketch and an object with a shape must be selected.")

            # Create and assign the ExtrudedCutout object.
            newObj, activeBody = SheetMetalTools.smCreateNewObject(selected_object,
                                                                   "ExtrudedCutout")
            if newObj is None:
                return
            ExtrudedCutout(newObj, cutSketch, selected_face)
            SMExtrudedCutoutVP(newObj.ViewObject)
            SheetMetalTools.smAddNewObject(selected_object, newObj, activeBody,
                                           SMExtrudedCutoutTaskPanel)

        def IsActive(self):
            return len(Gui.Selection.getSelection()) == 2


    Gui.addCommand("SheetMetal_AddCutout", AddExtrudedCutoutCommandClass())
