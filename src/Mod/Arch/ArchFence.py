import math

import FreeCAD
import ArchComponent
import Draft

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import PySide.QtGui as QtGui
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
        self.setProperties(obj)
        # Does a IfcType exist?
        # obj.IfcType = "Fence"
        obj.MoveWithHost = False

    def setProperties(self, obj):
        ArchComponent.Component.setProperties(self, obj)

        pl = obj.PropertiesList

        if not "Section" in pl:
            obj.addProperty("App::PropertyLink", "Section", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "A single section of the fence"))

        if not "Post" in pl:
            obj.addProperty("App::PropertyLink", "Post", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "A single fence post"))

        if not "Path" in pl:
            obj.addProperty("App::PropertyLink", "Path", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "The Path the fence should follow"))

        if not "NumberOfSections" in pl:
            obj.addProperty("App::PropertyInteger", "NumberOfSections", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "The number of sections the fence is built of"))
            obj.setEditorMode("NumberOfSections", 1)

        if not "NumberOfPosts" in pl:
            obj.addProperty("App::PropertyInteger", "NumberOfPosts", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "The number of posts used to build the fence"))
            obj.setEditorMode("NumberOfPosts", 1)

        self.Type = "Fence"

    def __getstate__(self):
        return (self.sectionFaceNumbers)

    def __setstate__(self, state):
        if state is not None and isinstance(state, tuple):
            self.sectionFaceNumbers = state[0]

        return None

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

        # We assume that the section was drawn in front view
        # We have to rotate the shape down so that it is aligned correctly by the algorithm later on
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

        self.applyShape(obj, compound, obj.Placement,
                        allowinvalid=True, allownosolid=True)

    def calculateNumberOfSections(self, pathLength, sectionLength, postLength):
        withoutLastPost = pathLength - postLength
        realSectionLength = sectionLength + postLength

        return math.ceil(withoutLastPost / realSectionLength)

    def calculatePostPlacements(self, obj, pathwire, rotation):
        postWidth = obj.Post.Shape.BoundBox.YMax

        # We want to center the posts on the path. So move them the half width in
        transformationVector = FreeCAD.Vector(0, - postWidth / 2, 0)

        placements = Draft.calculatePlacementsOnPath(
            rotation, pathwire, obj.NumberOfSections + 1, transformationVector, True)

        # The placement of the last object is always the second entry in the list.
        # So we move it to the end
        placements.append(placements.pop(1))

        return placements

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
        # It is possible that a section is clipped. Then the number of faces is not equals to the
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

            if sectionLength > sectionLine.length():
                # Part.show(Part.Shape([sectionLine]), 'line')
                sectionCopy = self.clipSection(
                    sectionCopy, sectionLength, sectionLine.length() - postLength)

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
        self.setProperties(vobj)

    def setProperties(self, vobj):
        pl = vobj.PropertiesList

        if not "UseOriginalColors" in pl:
            vobj.addProperty("App::PropertyBool", "UseOriginalColors", "Fence", QT_TRANSLATE_NOOP(
                "App::Property", "When true, the fence will be colored like the original post and section."))

    def onDocumentRestored(self, vobj):
        self.setProperties(vobj)

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
        if not hasattr(obj.ViewObject, "UseOriginalColors") or not obj.ViewObject.UseOriginalColors:
            obj.ViewObject.DiffuseColor = [obj.ViewObject.ShapeColor]
        else:
            post = obj.Post
            section = obj.Section

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

            viewObject = obj.ViewObject
            viewObject.DiffuseColor = ownColors

    def normalizeColors(self, obj, numberOfFaces):
        colors = obj.ViewObject.DiffuseColor
        numberOfColors = len(colors)

        if numberOfColors == 1:
            return colors * numberOfFaces

        colorsToUse = colors.copy()

        if numberOfColors == numberOfFaces:
            return colorsToUse
        else:
            # It is possible, that we have less faces than colors when something got clipped.
            # Remove the unneeded colors at the beginning and end
            halfNumberOfFacesToRemove = (numberOfColors - numberOfFaces) / 2
            start = int(math.ceil(halfNumberOfFacesToRemove))
            end = start + numberOfFaces

            return colorsToUse[start:end]


class _CommandFence:
    "the Arch Fence command definition"

    def GetResources(self):
        return {'Pixmap': 'Arch_Fence',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Fence", "Fence"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Fence", "Creates a fence object from a selected section, post and path")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()

        if len(sel) != 3:
            QtGui.QMessageBox.information(QtGui.QApplication.activeWindow(
            ), 'Arch Fence selection', 'Select a section, post and path in exactly this order to build a fence.')

            return

        section = sel[0]
        post = sel[1]
        path = sel[2]

        makeFence(section, post, path)


def makeFence(section, post, path):
    obj = FreeCAD.ActiveDocument.addObject(
        'Part::FeaturePython', 'Fence')

    _Fence(obj)
    obj.Section = section
    obj.Post = post
    obj.Path = path

    if FreeCAD.GuiUp:
        _ViewProviderFence(obj.ViewObject)

        hide(section)
        hide(post)
        hide(path)

    FreeCAD.ActiveDocument.recompute()

    return obj


def hide(obj):
    if hasattr(obj, 'ViewObject') and obj.ViewObject:
        obj.ViewObject.Visibility = False


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Fence', _CommandFence())

if __name__ == '__main__':
    # For testing purposes. When someone runs the File as a macro a default fence will be generated
    import Part

    def buildSection():
        parts = []

        parts.append(Part.makeBox(
            2000, 50, 30, FreeCAD.Vector(0, 0, 1000 - 30)))
        parts.append(Part.makeBox(2000, 50, 30))
        parts.append(Part.makeBox(20, 20, 1000 -
                                  60, FreeCAD.Vector(0, 15, 30)))
        parts.append(Part.makeBox(20, 20, 1000 - 60,
                                  FreeCAD.Vector(1980, 15, 30)))

        for i in range(8):
            parts.append(Part.makeBox(20, 20, 1000 - 60,
                                      FreeCAD.Vector((2000 / 9 * (i + 1)) - 10, 15, 30)))

        Part.show(Part.makeCompound(parts), "Section")

        return FreeCAD.ActiveDocument.getObject('Section')

    def buildPath():
        sketch = FreeCAD.ActiveDocument.addObject(
            'Sketcher::SketchObject', 'Path')
        sketch.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Rotation(0, 0, 0, 1))

        sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(
            0, 0, 0), FreeCAD.Vector(20000, 0, 0)), False)
        sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(
            20000, 0, 0), FreeCAD.Vector(20000, 20000, 0)), False)

        return sketch

    def buildPost():
        post = Part.makeBox(100, 100, 1000, FreeCAD.Vector(0, 0, 0))

        Part.show(post, 'Post')

        return FreeCAD.ActiveDocument.getObject('Post')

    def colorizeFaces(o, color=(0.6, 0.0, 0.0, 0.0), faceIndizes=[2]):
        numberOfFaces = len(o.Shape.Faces)
        vo = o.ViewObject

        originalColors = vo.DiffuseColor

        if len(originalColors) == 1:
            newColors = originalColors * numberOfFaces
        else:
            newColors = originalColors.copy()

        for i in faceIndizes:
            newColors[i] = color

        vo.DiffuseColor = newColors

    section = buildSection()
    path = buildPath()
    post = buildPost()

    colorizeFaces(post)

    print(makeFence(section, post, path))
