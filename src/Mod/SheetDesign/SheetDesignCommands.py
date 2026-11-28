# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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

"""Commands of the SheetDesign module.

The commands only make document objects and set their links. All geometry is in the C++ module.
"""

import FreeCAD
import FreeCADGui

import SheetDesignViewProviders

translate = FreeCAD.Qt.translate



def _selection():
    return FreeCADGui.Selection.getSelectionEx()


def _is_feature(obj):
    return obj is not None and obj.isDerivedFrom("SheetDesign::Feature")


RULE_PROPERTIES = ("Thickness", "BendRadius", "KFactor")


def make_rule(doc):
    """Add a variable set with the values that apply to a full sheet part."""
    rule = doc.addObject("App::VarSet", "SheetRule")
    rule.addProperty(
        "App::PropertyLength", "Thickness", "Sheet", translate("SheetDesign", "Thickness of the sheet")
    )
    rule.addProperty(
        "App::PropertyLength",
        "BendRadius",
        "Sheet",
        translate("SheetDesign", "Inner radius of the bends"),
    )
    rule.addProperty(
        "App::PropertyFloat",
        "KFactor",
        "Sheet",
        translate(
            "SheetDesign",
            "Position of the neutral axis as a fraction of the thickness, from the inner surface",
        ),
    )
    rule.addProperty(
        "App::PropertyEnumeration",
        "ReliefType",
        "Sheet",
        translate("SheetDesign", "Relief for a flange that is narrower than its edge"),
    )
    rule.addProperty(
        "App::PropertyLength", "ReliefWidth", "Sheet", translate("SheetDesign", "Width of the relief")
    )
    rule.addProperty(
        "App::PropertyLength",
        "ReliefDepth",
        "Sheet",
        translate("SheetDesign", "Depth of the relief, from the bend line into the panel"),
    )
    rule.addProperty(
        "App::PropertyEnumeration",
        "KFactorStandard",
        "Sheet",
        translate("SheetDesign", "Convention of the K-factor. A DIN value is two times the ANSI value"),
    )
    rule.addProperty(
        "App::PropertyFloatList",
        "KFactorRatios",
        "Sheet",
        translate(
            "SheetDesign",
            "Optional K-factor table: the ratios of bend radius to thickness. A bend uses the "
            "first ratio that is not smaller than its own ratio",
        ),
    )
    rule.addProperty(
        "App::PropertyFloatList",
        "KFactorValues",
        "Sheet",
        translate("SheetDesign", "Optional K-factor table: the K-factor for each ratio"),
    )
    rule.Thickness = 1.0
    rule.BendRadius = 1.0
    rule.KFactor = 0.5
    rule.KFactorStandard = ["ANSI", "DIN"]
    rule.KFactorStandard = "ANSI"
    rule.ReliefType = ["None", "Rectangle", "Round"]
    rule.ReliefType = "None"
    rule.ReliefWidth = 1.0
    rule.ReliefDepth = 1.0
    return rule


def _is_rule(obj):
    return all(hasattr(obj, name) for name in RULE_PROPERTIES)


def _body_of(obj):
    """Return the PartDesign Body that holds the object, or None."""
    parent = obj.getParentGeoFeatureGroup() if hasattr(obj, "getParentGeoFeatureGroup") else None
    if parent is not None and parent.isDerivedFrom("PartDesign::Body"):
        return parent
    return None


def _body_for(doc, *objects):
    """Return the Body for a new feature: that of an input, the active Body, or a new Body."""
    for obj in objects:
        body = _body_of(obj)
        if body is not None:
            return body
    view = FreeCADGui.ActiveDocument.ActiveView if FreeCADGui.ActiveDocument else None
    body = view.getActiveObject("pdbody") if hasattr(view, "getActiveObject") else None
    if body is not None and body.Document == doc:
        return body
    return doc.addObject("PartDesign::Body", "Body")


def _adopt(body, obj):
    """Put a sketch that has no Body into the Body, thus the part is in one container."""
    if _body_of(obj) is None and obj.isDerivedFrom("Part::Part2DObject"):
        if obj.getParentGeoFeatureGroup() is None:
            body.addObject(obj)


def _new_after(source, type_name, name):
    """Add a feature to the Body of its source. A source with no Body gives a plain object."""
    body = _body_of(source)
    if body is None:
        return source.Document.addObject(type_name, name)
    return body.newObject(type_name, name)


def _find_rule(doc):
    """Return the first rule of the document. Make one if the document has none."""
    for obj in doc.Objects:
        if obj.isDerivedFrom("App::VarSet") and _is_rule(obj):
            return obj
    return make_rule(doc)


def _advance_flat_patterns(old, new):
    """Make the flat patterns of the old feature follow the new feature."""
    for obj in old.Document.Objects:
        if obj.isDerivedFrom("SheetDesign::FlatPattern") and obj.Source == old:
            obj.Source = new


def _attach(obj):
    """Give a new feature its view provider proxy, thus it has its icon and its task panel."""
    return SheetDesignViewProviders.attach(obj)


def _finish(doc, hide=(), edit=None):
    """End a command. With edit, open the task panel of the new feature."""
    for obj in hide:
        obj.ViewObject.hide()
    doc.recompute()
    if edit is None:
        doc.commitTransaction()
    else:
        # The task panel takes the open transaction: Cancel removes the new feature.
        SheetDesignViewProviders.edit(edit, created=True, hidden=hide)


class _Command:
    name = ""
    menu = ""
    tip = ""

    def GetResources(self):
        return {
            # The icon is in the compiled resource of the SheetDesignGui library.
            "Pixmap": self.name,
            "MenuText": translate("SheetDesign", self.menu),
            "ToolTip": translate("SheetDesign", self.tip),
        }

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None


class CommandRule(_Command):
    name = "SheetDesign_Rule"
    menu = "Sheet Rule"
    tip = "Adds a variable set with the thickness, the bend radius, and the K-factor"

    def Activated(self):
        doc = FreeCAD.ActiveDocument
        doc.openTransaction("Sheet Rule")
        make_rule(doc)
        _finish(doc)


class CommandBase(_Command):
    name = "SheetDesign_Base"
    menu = "Base Panel"
    tip = "Makes the first panel from the selected closed sketch, or a part from an open sketch of its section"

    def IsActive(self):
        sel = FreeCADGui.Selection.getSelection()
        return len(sel) == 1 and hasattr(sel[0], "Shape") and not _is_feature(sel[0])

    def Activated(self):
        profile = FreeCADGui.Selection.getSelection()[0]
        doc = profile.Document
        doc.openTransaction("Base Panel")
        body = _body_for(doc, profile)
        _adopt(body, profile)
        obj = _attach(body.newObject("SheetDesign::BasePanel", "BasePanel"))
        obj.Rule = _find_rule(doc)
        obj.Profile = profile
        is_open = not _has_closed_wire(profile)
        _finish(doc, hide=[profile], edit=obj if is_open else None)


class CommandRefold(_Command):
    name = "SheetDesign_Refold"
    menu = "Refold"
    tip = "Folds a selected flat outline along the lines of a selected sketch of bend lines"

    def _pick(self):
        sel = [obj for obj in FreeCADGui.Selection.getSelection() if hasattr(obj, "Shape")]
        if len(sel) != 2 or any(_is_feature(obj) for obj in sel):
            return None
        closed = [obj for obj in sel if _has_closed_wire(obj)]
        if len(closed) != 1:
            return None
        lines = sel[1] if sel[0] is closed[0] else sel[0]
        return closed[0], lines

    def IsActive(self):
        return self._pick() is not None

    def Activated(self):
        profile, lines = self._pick()
        doc = profile.Document
        doc.openTransaction("Refold")
        body = _body_for(doc, profile, lines)
        _adopt(body, profile)
        _adopt(body, lines)
        obj = _attach(body.newObject("SheetDesign::Refold", "Refold"))
        obj.Rule = _find_rule(doc)
        obj.Profile = profile
        obj.BendLines = lines
        _finish(doc, hide=[profile, lines], edit=obj)


def _has_closed_wire(obj):
    shape = obj.Shape
    return bool(shape.Faces) or any(wire.isClosed() for wire in shape.Wires)


class CommandUnfold(_Command):
    name = "SheetDesign_Unfold"
    menu = "Unfold"
    tip = "Reads a folded solid and adds its flat pattern. Select one flat face of the solid"

    def _pick(self):
        sel = _selection()
        if len(sel) != 1 or _is_feature(sel[0].Object) or len(sel[0].SubElementNames) != 1:
            return None
        name = sel[0].SubElementNames[0]
        if not name.startswith("Face"):
            return None
        return sel[0].Object, name

    def IsActive(self):
        return self._pick() is not None

    def Activated(self):
        source, face = self._pick()
        doc = source.Document
        doc.openTransaction("Unfold")
        obj = _attach(_body_for(doc, source).newObject("SheetDesign::Unfold", "Unfold"))
        obj.Rule = _find_rule(doc)
        obj.Face = (source, [face])
        flat = _attach(doc.addObject("SheetDesign::FlatPattern", "FlatPattern"))
        flat.Source = obj
        _finish(doc, hide=[source])


class CommandFlange(_Command):
    name = "SheetDesign_Flange"
    menu = "Flange"
    tip = "Adds a bend and a panel on each selected straight edge. Adjacent edges can get a closed corner"

    def IsActive(self):
        sel = _selection()
        return len(sel) == 1 and _is_feature(sel[0].Object) and len(sel[0].SubElementNames) >= 1

    def Activated(self):
        sel = _selection()[0]
        source = sel.Object
        doc = source.Document
        doc.openTransaction("Flange")
        # One feature holds all selected edges.
        obj = _attach(_new_after(source, "SheetDesign::Flange", "Flange"))
        obj.Edge = (source, list(sel.SubElementNames))
        _advance_flat_patterns(source, obj)
        _finish(doc, hide=[source], edit=obj)


def _element_name(shape, element):
    """Find the name of the edge or face of the shape that has the same geometry."""
    if element.ShapeType == "Edge":
        candidates, prefix = shape.Edges, "Edge"
    else:
        candidates, prefix = shape.Faces, "Face"
    center = element.CenterOfMass
    for index, candidate in enumerate(candidates):
        if (
            candidate.CenterOfMass.distanceToPoint(center) < 1e-6
            and abs(candidate.Length - element.Length) < 1e-6
        ):
            return f"{prefix}{index + 1}"
    return None


class CommandCut(_Command):
    name = "SheetDesign_Cut"
    menu = "Cut"
    tip = "Cuts the selected sketch through the selected sheet design feature"
    space = "Folded"

    def _pick(self):
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) != 2:
            return None
        features = [obj for obj in sel if _is_feature(obj)]
        others = [obj for obj in sel if not _is_feature(obj) and hasattr(obj, "Shape")]
        if len(features) != 1 or len(others) != 1:
            return None
        return features[0], others[0]

    def IsActive(self):
        return self._pick() is not None

    def Activated(self):
        source, profile = self._pick()
        doc = source.Document
        doc.openTransaction(self.menu)
        obj = _attach(_new_after(source, "SheetDesign::Cut", "Cut"))
        obj.Source = source
        obj.Profile = profile
        obj.Space = self.space
        _advance_flat_patterns(source, obj)
        _finish(doc, hide=[source, profile], edit=obj)


class CommandCutFlat(CommandCut):
    name = "SheetDesign_CutFlat"
    menu = "Cut in Flat Pattern"
    tip = "Cuts the selected sketch through the flat pattern of the selected feature"
    space = "Flat"


class CommandBaseShape(_Command):
    name = "SheetDesign_BaseShape"
    menu = "Base Shape"
    tip = "Makes a plate, an angle, a channel, a tub, a hat, or a box from sizes"

    def Activated(self):
        doc = FreeCAD.ActiveDocument
        doc.openTransaction("Base Shape")
        body = _body_for(doc)
        obj = _attach(body.newObject("SheetDesign::BaseShape", "BaseShape"))
        obj.Rule = _find_rule(doc)
        obj.Type = "Channel"
        _finish(doc, edit=obj)


class _EdgeCommand(_Command):
    """A command that adds one feature on each selected edge of a sheet design feature."""

    type_name = ""

    def IsActive(self):
        sel = _selection()
        return len(sel) == 1 and _is_feature(sel[0].Object) and len(sel[0].SubElementNames) >= 1

    def Activated(self):
        sel = _selection()[0]
        source = sel.Object
        doc = source.Document
        doc.openTransaction(self.menu)
        edges = [source.Shape.getElement(name) for name in sel.SubElementNames]
        obj = None
        for edge in edges:
            name = _element_name(source.Shape, edge)
            if name is None:
                FreeCAD.Console.PrintWarning("Sheet design: an edge was not found after a feature\n")
                continue
            obj = _attach(_new_after(source, "SheetDesign::" + self.type_name, self.type_name))
            obj.Edge = (source, [name])
            _advance_flat_patterns(source, obj)
            source.ViewObject.hide()
            doc.recompute()
            source = obj
        if obj is not None:
            SheetDesignViewProviders.edit(obj, created=True, hidden=[sel.Object])
        else:
            doc.commitTransaction()


class CommandHem(_EdgeCommand):
    name = "SheetDesign_Hem"
    menu = "Hem"
    tip = "Folds the selected straight edge back on the sheet"
    type_name = "Hem"


class CommandExtend(_Command):
    name = "SheetDesign_Extend"
    menu = "Extend"
    tip = "Moves the selected straight edge out, or adds the selected closed sketch to the panel that it touches"

    def _pick(self):
        sel = _selection()
        features = [s for s in sel if _is_feature(s.Object)]
        others = [s.Object for s in sel if not _is_feature(s.Object) and hasattr(s.Object, "Shape")]
        if len(features) != 1 or len(others) > 1 or len(sel) != 1 + len(others):
            return None
        names = list(features[0].SubElementNames)
        if not others and len(names) != 1:
            return None
        return features[0].Object, names[:1], others[0] if others else None

    def IsActive(self):
        return self._pick() is not None

    def Activated(self):
        source, names, profile = self._pick()
        doc = source.Document
        doc.openTransaction("Extend")
        obj = _attach(_new_after(source, "SheetDesign::Extend", "Extend"))
        obj.Edge = (source, [] if profile is not None else names)
        if profile is not None:
            obj.Profile = profile
        _advance_flat_patterns(source, obj)
        _finish(doc, hide=[source] + ([profile] if profile is not None else []), edit=obj)


class CommandFold(CommandCut):
    name = "SheetDesign_Fold"
    menu = "Fold"
    tip = "Bends the selected sheet design feature on the lines of the selected sketch"

    def Activated(self):
        source, lines = self._pick()
        doc = source.Document
        doc.openTransaction("Fold")
        obj = _attach(_new_after(source, "SheetDesign::Fold", "Fold"))
        obj.Source = source
        obj.BendLines = lines
        _advance_flat_patterns(source, obj)
        _finish(doc, hide=[source, lines], edit=obj)


class CommandConvert(_Command):
    name = "SheetDesign_Convert"
    menu = "Convert"
    tip = "Makes a sheet part from the selected flat faces of a solid. The first face stays in its position"

    def _pick(self):
        sel = _selection()
        if len(sel) != 1 or _is_feature(sel[0].Object):
            return None
        names = [name for name in sel[0].SubElementNames if name.startswith("Face")]
        if not names or len(names) != len(sel[0].SubElementNames):
            return None
        return sel[0].Object, names

    def IsActive(self):
        return self._pick() is not None

    def Activated(self):
        source, faces = self._pick()
        doc = source.Document
        doc.openTransaction("Convert")
        obj = _attach(_body_for(doc, source).newObject("SheetDesign::Convert", "Convert"))
        obj.Rule = _find_rule(doc)
        obj.Faces = (source, faces)
        _finish(doc, hide=[source], edit=obj)


class CommandForm(_Command):
    name = "SheetDesign_Form"
    menu = "Form"
    tip = (
        "Presses a tool solid into the selected sheet design feature. Select the feature, then "
        "the base face of the tool. More faces of the tool give openings"
    )

    def _pick(self):
        sel = _selection()
        features = [s.Object for s in sel if _is_feature(s.Object)]
        tools = [s for s in sel if not _is_feature(s.Object) and s.SubElementNames]
        if len(sel) != 2 or len(features) != 1 or len(tools) != 1:
            return None
        names = list(tools[0].SubElementNames)
        if not all(name.startswith("Face") for name in names):
            return None
        return features[0], tools[0].Object, names

    def IsActive(self):
        return self._pick() is not None

    def Activated(self):
        source, tool, faces = self._pick()
        doc = source.Document
        doc.openTransaction("Form")
        obj = _attach(_new_after(source, "SheetDesign::Form", "Form"))
        obj.Source = source
        obj.Tool = (tool, faces)
        _advance_flat_patterns(source, obj)
        _finish(doc, hide=[source, tool], edit=obj)


class CommandPerforation(_Command):
    name = "SheetDesign_Perforation"
    menu = "Perforation"
    tip = "Cuts slots along the bends of the selected feature. Select edges on bends to cut only those bends"

    def IsActive(self):
        sel = _selection()
        return len(sel) == 1 and _is_feature(sel[0].Object)

    def Activated(self):
        sel = _selection()[0]
        source = sel.Object
        doc = source.Document
        doc.openTransaction("Perforation")
        obj = _attach(_new_after(source, "SheetDesign::Perforation", "Perforation"))
        obj.Edges = (source, list(sel.SubElementNames))
        _advance_flat_patterns(source, obj)
        _finish(doc, hide=[source], edit=obj)


class CommandFlatPattern(_Command):
    name = "SheetDesign_FlatPattern"
    menu = "Flat Pattern"
    tip = "Adds the flat pattern of the selected sheet design feature"

    def IsActive(self):
        sel = FreeCADGui.Selection.getSelection()
        return len(sel) == 1 and _is_feature(sel[0])

    def Activated(self):
        source = FreeCADGui.Selection.getSelection()[0]
        doc = source.Document
        doc.openTransaction("Flat Pattern")
        obj = _attach(doc.addObject("SheetDesign::FlatPattern", "FlatPattern"))
        obj.Source = source
        _finish(doc)


class CommandCornerRelief(CommandFlatPattern):
    name = "SheetDesign_CornerRelief"
    menu = "Corner Relief"
    tip = "Cuts an opening at each corner where two bends of one panel meet"

    def Activated(self):
        source = FreeCADGui.Selection.getSelection()[0]
        doc = source.Document
        doc.openTransaction("Corner Relief")
        obj = _attach(_new_after(source, "SheetDesign::CornerRelief", "CornerRelief"))
        obj.Source = source
        _advance_flat_patterns(source, obj)
        _finish(doc, hide=[source], edit=obj)


class CommandExport(_Command):
    name = "SheetDesign_ExportDxf"
    menu = "Export Flat Pattern…"
    tip = "Exports the selected flat pattern to a DXF or an SVG file with layers"

    def IsActive(self):
        sel = FreeCADGui.Selection.getSelection()
        return len(sel) == 1 and sel[0].isDerivedFrom("SheetDesign::FlatPattern")

    def Activated(self):
        from PySide import QtGui

        flat = FreeCADGui.Selection.getSelection()[0]
        path, _filter = QtGui.QFileDialog.getSaveFileName(
            FreeCADGui.getMainWindow(),
            translate("SheetDesign", "Export Flat Pattern"),
            flat.Label + ".dxf",
            "DXF (*.dxf);;SVG (*.svg)",
        )
        if path:
            import SheetDesignExport

            SheetDesignExport.export(flat, path)


class CommandDrawing(_Command):
    name = "SheetDesign_Drawing"
    menu = "Flat Pattern Drawing"
    tip = "Makes a drawing page with the selected flat pattern, its bend notes, and its bend table"

    def IsActive(self):
        sel = FreeCADGui.Selection.getSelection()
        return len(sel) == 1 and sel[0].isDerivedFrom("SheetDesign::FlatPattern")

    def Activated(self):
        import SheetDesignDrawing

        flat = FreeCADGui.Selection.getSelection()[0]
        doc = flat.Document
        doc.openTransaction("Flat Pattern Drawing")
        page = SheetDesignDrawing.make_drawing(flat)[0]
        doc.commitTransaction()
        if page.ViewObject is not None:
            page.ViewObject.show()


def register():
    commands = [
        CommandRule,
        CommandBase,
        CommandBaseShape,
        CommandRefold,
        CommandUnfold,
        CommandConvert,
        CommandFlange,
        CommandHem,
        CommandExtend,
        CommandFold,
        CommandCut,
        CommandCutFlat,
        CommandCornerRelief,
        CommandForm,
        CommandPerforation,
        CommandFlatPattern,
        CommandDrawing,
        CommandExport,
    ]
    for command in commands:
        FreeCADGui.addCommand(command.name, command())
    return [command.name for command in commands]
