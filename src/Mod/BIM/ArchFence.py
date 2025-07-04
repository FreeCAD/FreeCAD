# SPDX-License-Identifier: LGPL-2.1-or-later

# *****************************************************************************
# *                                                                           *
# *   Copyright (c) 2019 furti <daniel.furtlehner@gmx.net>                    *
# *                                                                           *
# *   This file is part of FreeCAD.                                           *
# *                                                                           *
# *   FreeCAD is free software: you can redistribute it and/or modify it      *
# *   under the terms of the GNU Lesser General Public License as             *
# *   published by the Free Software Foundation, either version 2.1 of the    *
# *   License, or (at your option) any later version.                         *
# *                                                                           *
# *   FreeCAD is distributed in the hope that it will be useful, but          *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of              *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        *
# *   Lesser General Public License for more details.                         *
# *                                                                           *
# *   You should have received a copy of the GNU Lesser General Public        *
# *   License along with FreeCAD. If not, see                                 *
# *   <https://www.gnu.org/licenses/>.                                        *
# *                                                                           *
# *****************************************************************************

# Fence functionality for the Arch Workbench

import math

import FreeCAD
import ArchComponent
import draftobjects.patharray as patharray

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import PySide.QtGui as QtGui
    import FreeCADGui
else:
    # \cond
    def translate(ctxt, txt):
        return txt

    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt
    # \endcond

EAST = FreeCAD.Vector(1, 0, 0)


class _Fence(ArchComponent.Component):
    def __init__(self, obj):

        ArchComponent.Component.__init__(self, obj)
        self.Type = "Fence"
        self.setProperties(obj)
        # Does a IfcType exist?
        # obj.IfcType = "Fence"
        obj.MoveWithHost = False

    def setProperties(self, obj):
        ArchComponent.Component.setProperties(self, obj)

        pl = obj.PropertiesList

        if not "Section" in pl:
            obj.addProperty("App::PropertyLink", "Section", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "A single section of the fence"), locked=True)

        if not "Post" in pl:
            obj.addProperty("App::PropertyLink", "Post", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "A single fence post"), locked=True)

        if not "Path" in pl:
            obj.addProperty("App::PropertyLink", "Path", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "The Path the fence should follow"), locked=True)

        if not "NumberOfSections" in pl:
            obj.addProperty("App::PropertyInteger", "NumberOfSections", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "The number of sections the fence is built of"), locked=True)
            obj.setEditorMode("NumberOfSections", 1)

        if not "NumberOfPosts" in pl:
            obj.addProperty("App::PropertyInteger", "NumberOfPosts", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "The number of posts used to build the fence"), locked=True)
            obj.setEditorMode("NumberOfPosts", 1)

    def dumps(self):
        if hasattr(self, 'sectionFaceNumbers'):
            return self.sectionFaceNumbers
        return None

    def loads(self, state):
        if state is not None and isinstance(state, tuple):
            self.sectionFaceNumbers = state[0]
        self.Type = "Fence"

    def execute(self, obj):
        import Part

        pathwire = self.calculatePathWire(obj)

        if not pathwire:
            FreeCAD.Console.PrintLog(
                "ArchFence.execute: path " + obj.Path.Name + " has no edges\n")

            return

        if not obj.Section:
            FreeCAD.Console.PrintLog(
                "ArchFence.execute: Section not set\n")

            return

        if not obj.Post:
            FreeCAD.Console.PrintLog(
                "ArchFence.execute: Post not set\n")

            return

        pathLength = pathwire.Length
        sectionLength = obj.Section.Shape.BoundBox.XMax
        postLength = obj.Post.Shape.BoundBox.XMax

        obj.NumberOfSections = self.calculateNumberOfSections(
            pathLength, sectionLength, postLength)
        obj.NumberOfPosts = obj.NumberOfSections + 1

        # We assume that the section was drawn in front view.
        # We have to rotate the shape down so that it is aligned
        # correctly by the algorithm later on
        downRotation = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), -90)

        postPlacements = self.calculatePostPlacements(
            obj, pathwire, downRotation)

        postShapes = self.calculatePosts(obj, postPlacements)
        sectionShapes, sectionFaceNumbers = self.calculateSections(
            obj, postPlacements, postLength, sectionLength)

        allShapes = []
        allShapes.extend(postShapes)
        allShapes.extend(sectionShapes)

        compound = Part.makeCompound(allShapes)

        self.sectionFaceNumbers = sectionFaceNumbers

        obj.Shape = compound

    def calculateNumberOfSections(self, pathLength, sectionLength, postLength):
        realSectionLength = sectionLength + postLength

        return math.ceil(pathLength / realSectionLength)

    def calculatePostPlacements(self, obj, pathwire, rotation):
        postWidth = obj.Post.Shape.BoundBox.YMax

        # We want to center the posts on the path. So move them the half width in
        transformationVector = FreeCAD.Vector(0, - postWidth / 2, 0)

        return patharray.placements_on_path(rotation, pathwire,
                                            obj.NumberOfPosts,
                                            transformationVector, True)

    def calculatePosts(self, obj, postPlacements):
        posts = []

        for placement in postPlacements:
            postCopy = obj.Post.Shape.copy()
            postCopy.Placement = placement

            posts.append(postCopy)

        return posts

    def calculateSections(self, obj, postPlacements, postLength, sectionLength):
        import Part

        shapes = []

        # For the colorization algorithm we have to store the number of faces for each section
        # It is possible that a section is clipped. Then the number of faces is not equal to the
        # number of faces in the original section
        faceNumbers = []

        for i in range(obj.NumberOfSections):
            startPlacement = postPlacements[i]
            endPlacement = postPlacements[i + 1]

            sectionLine = Part.LineSegment(
                startPlacement.Base, endPlacement.Base)
            sectionBase = sectionLine.value(postLength)

            if startPlacement.Rotation.isSame(endPlacement.Rotation):
                sectionRotation = endPlacement.Rotation
            else:
                direction = endPlacement.Base.sub(startPlacement.Base)

                sectionRotation = FreeCAD.Rotation(EAST, direction)

            placement = FreeCAD.Placement()
            placement.Base = sectionBase
            placement.Rotation = sectionRotation

            sectionCopy = obj.Section.Shape.copy()

            if sectionLength > sectionLine.length() - postLength:
                # Part.show(Part.Shape([sectionLine]), 'line')
                sectionCopy = self.clipSection(
                    sectionCopy, sectionLength, sectionLine.length() - postLength)

            sectionCopy = Part.Compound([sectionCopy])  # nest in compound to ensure correct Placement
            sectionCopy.Placement = placement

            shapes.append(sectionCopy)
            faceNumbers.append(len(sectionCopy.Faces))

        return (shapes, faceNumbers)

    def clipSection(self, shape, length, clipLength):
        import Part

        boundBox = shape.BoundBox
        lengthToCut = length - clipLength
        halfLengthToCut = lengthToCut / 2

        leftBox = Part.makeBox(halfLengthToCut, boundBox.YMax + 1, boundBox.ZMax + 1,
                               FreeCAD.Vector(boundBox.XMin, boundBox.YMin, boundBox.ZMin))
        rightBox = Part.makeBox(halfLengthToCut, boundBox.YMax + 1, boundBox.ZMax + 1,
                                FreeCAD.Vector(boundBox.XMin + halfLengthToCut + clipLength, boundBox.YMin, boundBox.ZMin))

        newShape = shape.cut([leftBox, rightBox])
        newBoundBox = newShape.BoundBox

        newShape.translate(FreeCAD.Vector(-newBoundBox.XMin, 0, 0))

        return newShape.removeSplitter()

    def calculatePathWire(self, obj):
        if (hasattr(obj.Path.Shape, 'Wires') and obj.Path.Shape.Wires):
            return obj.Path.Shape.Wires[0]
        elif obj.Path.Shape.Edges:
            return Part.Wire(obj.Path.Shape.Edges)

        return None


class _ViewProviderFence(ArchComponent.ViewProviderComponent):

    "A View Provider for the Fence object"

    def __init__(self, vobj):
        ArchComponent.ViewProviderComponent.__init__(self, vobj)
        # setProperties of ArchComponent will be overwritten
        # thus setProperties from ArchComponent will be explicit called to get the properties
        ArchComponent.ViewProviderComponent.setProperties(self, vobj)
        self.setProperties(vobj)

    def setProperties(self, vobj):
        pl = vobj.PropertiesList

        if not "UseOriginalColors" in pl:
            vobj.addProperty("App::PropertyBool", "UseOriginalColors", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "When true, the fence will be colored like the original post and section."), locked=True)

    def attach(self, vobj):
        self.setProperties(vobj)

        return super().attach(vobj)

    def getIcon(self):
        import Arch_rc

        return ":/icons/Arch_Fence_Tree.svg"

    def claimChildren(self):
        children = []

        if self.Object.Section:
            children.append(self.Object.Section)

        if self.Object.Post:
            children.append(self.Object.Post)

        if self.Object.Path:
            children.append(self.Object.Path)

        return children

    def updateData(self, obj, prop):
        colorProps = ["Shape", "Section", "Post", "Path"]

        if prop in colorProps:
            self.applyColors(obj)
        else:
            super().updateData(obj, prop)

    def onChanged(self, vobj, prop):
        if prop == "UseOriginalColors":
            self.applyColors(vobj.Object)
        else:
            super().onChanged(vobj, prop)

    def applyColors(self, obj):
        # Note that the clipSection function changes the face numbering of the
        # fence section. This happens even if the total number of faces does not
        # change. If UseOriginalColors is True, the end result of this function
        # will only be correct if all faces of the section have the same color.

        vobj = obj.ViewObject
        if not vobj.UseOriginalColors:
            vobj.ShapeAppearance = [vobj.ShapeAppearance[0]]
        else:
            post = obj.Post
            section = obj.Section

            # If post and/or section are Std_Parts they may not have a Shape attr (yet):
            if not hasattr(post, "Shape"):
                return
            if not hasattr(section, "Shape"):
                return

            numberOfPostFaces = len(post.Shape.Faces)
            numberOfSectionFaces = len(section.Shape.Faces)

            if hasattr(obj.Proxy, 'sectionFaceNumbers'):
                sectionFaceNumbers = obj.Proxy.sectionFaceNumbers
            else:
                sectionFaceNumbers = [0]

            if numberOfPostFaces == 0 or sum(sectionFaceNumbers) == 0:
                return

            postColors = self.normalizeColors(post, numberOfPostFaces)
            defaultSectionColors = self.normalizeColors(
                section, numberOfSectionFaces)

            ownColors = []

            # At first all posts are added to the shape
            for i in range(obj.NumberOfPosts):
                ownColors.extend(postColors)

            # Next all sections are added
            for i in range(obj.NumberOfSections):
                actualSectionFaceCount = sectionFaceNumbers[i]

                if actualSectionFaceCount == numberOfSectionFaces:
                    ownColors.extend(defaultSectionColors)
                else:
                    ownColors.extend(self.normalizeColors(
                        section, actualSectionFaceCount))

            vobj.DiffuseColor = ownColors

    def normalizeColors(self, obj, numberOfFaces):
        if obj.TypeId == "PartDesign::Body":
            # When colorizing a PartDesign Body we have two options
            # 1. The whole body got a shape color, that means the tip has only a single diffuse color set
            #   so we use the shape color of the body
            # 2. "Set colors" was called on the tip and the individual faces where colorized.
            #   We use the diffuseColors of the tip in that case
            if len(obj.Tip.ViewObject.DiffuseColor) > 1:
                colors = obj.Tip.ViewObject.DiffuseColor
            else:
                colors = obj.ViewObject.DiffuseColor
        else:
            import Draft
            colors = Draft.get_diffuse_color(obj)  # To handle Std_Parts for example.

        numberOfColors = len(colors)

        if numberOfColors == 1:
            return colors * numberOfFaces

        if numberOfColors == numberOfFaces:
            return colors

        # It is possible, that we have fewer faces than colors when something
        # got clipped. Remove the unneeded colors at the beginning and end.

        # Even if clipSection did not change the face numbering this code would
        # not work properly.
        halfNumberOfFacesToRemove = (numberOfColors - numberOfFaces) / 2
        start = int(math.ceil(halfNumberOfFacesToRemove))
        end = start + numberOfFaces
        return colors[start:end]


def hide(obj):
    if hasattr(obj, 'ViewObject') and obj.ViewObject:
        obj.ViewObject.Visibility = False
