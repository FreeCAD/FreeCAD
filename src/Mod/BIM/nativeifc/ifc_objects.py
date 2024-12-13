# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License (GPL)            *
# *   as published by the Free Software Foundation; either version 3 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""This module contains IFC object definitions"""

import FreeCAD
translate = FreeCAD.Qt.translate

# the property groups below should not be treated as psets
NON_PSETS = ["Base", "IFC", "", "Geometry", "Dimension", "Linear/radial dimension",
             "SectionPlane", "Axis", "PhysicalProperties"]

class ifc_object:
    """Base class for all IFC-based objects"""

    def __init__(self, otype=None):
        self.cached = True  # this marks that the object is freshly created and its shape should be taken from cache
        self.virgin_placement = True  # this allows to set the initial placement without triggering any placement change
        if otype:
            self.Type = (
                otype[0].upper() + otype[1:]
            )  # capitalize to match Draft standard
        else:
            self.Type = "IfcObject"

    def onBeforeChange(self, obj, prop):
        if prop == "Schema":
            self.old_schema = obj.Schema
        elif prop == "Placement":
            self.old_placement = obj.Placement

    def onChanged(self, obj, prop):
        # link class property to its hidder IfcClass counterpart
        if prop == "IfcClass" and hasattr(obj, "Class") and obj.Class != obj.IfcClass:
            obj.Class = obj.IfcClass
            self.rebuild_classlist(obj, setprops=True)
        elif prop == "Class" and hasattr(obj, "IfcClass") and obj.Class != obj.IfcClass:
            obj.IfcClass = obj.Class
            self.rebuild_classlist(obj, setprops=True)
        elif prop == "Schema":
            self.edit_schema(obj, obj.Schema)
        elif prop == "Type":
            self.edit_type(obj)
        elif prop == "Group":
            self.edit_group(obj)
        elif hasattr(obj, prop) and obj.getGroupOfProperty(prop) == "IFC":
            if prop not in ["StepId"]:
                self.edit_attribute(obj, prop)
        elif prop == "Label":
            self.edit_attribute(obj, "Name", obj.Label)
        elif prop == "Text":
            self.edit_annotation(obj, "Text", "\n".join(obj.Text))
        elif prop in ["Start", "End"]:
            self.edit_annotation(obj, prop)
        elif prop in ["DisplayLength","DisplayHeight","Depth"]:
            self.edit_annotation(obj, prop)
        elif prop == "Placement":
            if getattr(self, "virgin_placement", False):
                self.virgin_placement = False
            elif obj.Placement != getattr(self, "old_placement", None):
                # print("placement changed for",obj.Label,"to",obj.Placement)
                self.edit_placement(obj)
        elif prop == "Modified":
            if obj.ViewObject:
                obj.ViewObject.signalChangeIcon()
        elif hasattr(obj, prop) and obj.getGroupOfProperty(prop) == "Geometry":
            self.edit_geometry(obj, prop)
        elif hasattr(obj, prop) and obj.getGroupOfProperty(prop) not in NON_PSETS:
            # Treat all property groups outside the default ones as Psets
            # print("DEBUG: editinog pset prop",prop)
            self.edit_pset(obj, prop)

    def onDocumentRestored(self, obj):
        self.rebuild_classlist(obj)
        if hasattr(obj, "IfcFilePath"):
            # once we have loaded the project, recalculate child coin nodes
            from PySide import QtCore  # lazy loading

            if obj.OutListRecursive:
                for child in obj.OutListRecursive:
                    if getattr(child, "ShapeMode", None) == "Coin":
                        child.Proxy.cached = True
                        child.touch()
            else:
                obj.Proxy.cached = True
                QtCore.QTimer.singleShot(100, obj.touch)
            QtCore.QTimer.singleShot(100, obj.Document.recompute)
            QtCore.QTimer.singleShot(100, self.fit_all)

    def fit_all(self):
        """Fits the view"""

        import FreeCAD

        if FreeCAD.GuiUp:
            import FreeCADGui

            FreeCADGui.SendMsgToActiveView("ViewFit")

    def rebuild_classlist(self, obj, setprops=False):
        """rebuilds the list of Class enum property according to current class"""

        from nativeifc import ifc_tools  # lazy import

        obj.Class = [obj.IfcClass]
        obj.Class = ifc_tools.get_ifc_classes(obj, obj.IfcClass)
        obj.Class = obj.IfcClass
        if setprops:
            ifc_tools.remove_unused_properties(obj)
            ifc_tools.add_properties(obj)

    def __getstate__(self):
        return getattr(self, "Type", None)

    def __setstate__(self, state):
        self.loads(state)

    def dumps(self):
        return getattr(self, "Type", None)

    def loads(self, state):
        if state and hasattr(state, "Type"):
            self.Type = state
        return None

    def execute(self, obj):
        from nativeifc import ifc_generator  # lazy import

        if obj.isDerivedFrom("Part::Feature"):
            cached = getattr(self, "cached", False)
            ifc_generator.generate_geometry(obj, cached=cached)
            self.cached = False
            self.rebuild_classlist(obj)

    def addObject(self, obj, child):
        if child not in obj.Group:
            g = obj.Group
            g.append(child)
            obj.Group = g

    def removeObject(self, obj, child):
        if child in obj.Group:
            g = obj.Group
            g.remove(child)
            obj.Group = g

    def edit_attribute(self, obj, attribute, value=None):
        """Edits an attribute of an underlying IFC object"""

        from nativeifc import ifc_tools  # lazy import

        if not value:
            value = obj.getPropertyByName(attribute)
        ifcfile = ifc_tools.get_ifcfile(obj)
        elt = ifc_tools.get_ifc_element(obj, ifcfile)
        if elt:
            result = ifc_tools.set_attribute(ifcfile, elt, attribute, value)
            if result:
                if hasattr(result, "id") and (result.id() != obj.StepId):
                    obj.StepId = result.id()

    def edit_annotation(self, obj, attribute, value=None):
        """Edits an attribute of an underlying IFC annotation"""

        from nativeifc import ifc_tools  # lazy import
        from nativeifc import ifc_export

        if not value:
            if hasattr(obj, attribute):
                value = obj.getPropertyByName(attribute)
        ifcfile = ifc_tools.get_ifcfile(obj)
        elt = ifc_tools.get_ifc_element(obj, ifcfile)
        if elt:
            if attribute == "Text":
                text = ifc_export.get_text(elt)
                if text:
                    ifc_tools.set_attribute(ifcfile, text, "Literal", value)
            elif attribute in ["Start", "End"]:
                dim = ifc_export.get_dimension(elt)
                if dim:
                    rep = dim[0]
                    for curve in rep.Items:
                        if not hasattr(curve, "Elements"):
                            # this is a TextLiteral for the dimension text - skip it
                            continue
                        for sub in curve.Elements:
                            if sub.is_a("IfcIndexedPolyCurve"):
                                points = sub.Points
                                value = list(points.CoordList)
                                is2d = "2D" in points.is_a()
                                if attribute == "Start":
                                    value[0] = ifc_export.get_scaled_point(obj.Start, ifcfile, is2d)
                                else:
                                    value[-1] = ifc_export.get_scaled_point(obj.End, ifcfile, is2d)
                                ifc_tools.set_attribute(ifcfile, points, "CoordList", value)
                            else:
                                print("DEBUG: unknown dimension curve type:",sub)
            elif attribute in ["DisplayLength","DisplayHeight","Depth"]:
                l = w = h = 1000.0
                if obj.ViewObject:
                    if obj.ViewObject.DisplayLength.Value:
                        l = ifc_export.get_scaled_value(obj.ViewObject.DisplayLength.Value, ifcfile)
                    if obj.ViewObject.DisplayHeight.Value:
                        w = ifc_export.get_scaled_value(obj.ViewObject.DisplayHeight.Value, ifcfile)
                if obj.Depth.Value:
                    h = ifc_export.get_scaled_value(obj.Depth.Value, ifcfile)
                if elt.Representation.Representations:
                    for rep in elt.Representation.Representations:
                        for item in rep.Items:
                            if item.is_a("IfcCsgSolid"):
                                if item.TreeRootExpression.is_a("IfcBlock"):
                                    block = item.TreeRootExpression
                                    loc = block.Position.Location
                                    ifc_tools.set_attribute(ifcfile, block, "XLength", l)
                                    ifc_tools.set_attribute(ifcfile, block, "YLength", w)
                                    ifc_tools.set_attribute(ifcfile, block, "ZLength", h)
                                    ifc_tools.set_attribute(ifcfile, loc, "Coordinates", (-l/2, -h/2, -h))

    def edit_geometry(self, obj, prop):
        """Edits a geometry property of an object"""

        from nativeifc import ifc_geometry  # lazy loading
        from nativeifc import ifc_tools  # lazy import

        result = ifc_geometry.set_geom_property(obj, prop)
        if result:
            obj.touch()

    def edit_schema(self, obj, schema):
        """Changes the schema of an IFC document"""

        from nativeifc import ifc_tools  # lazy import

        ifcfile = ifc_tools.get_ifcfile(obj)
        if not ifcfile:
            return
        if not getattr(self, "old_schema", None):
            return
        if schema != ifcfile.wrapped_data.schema_name():
            # set obj.Proxy.silent = True to disable the schema change warning
            if obj.ViewObject and not getattr(self, "silent", False):
                if not obj.ViewObject.Proxy.schema_warning():
                    return
            ifcfile, migration_table = ifc_tools.migrate_schema(ifcfile, schema)
            self.ifcfile = ifcfile
            for old_id, new_id in migration_table.items():
                child = [
                    o
                    for o in obj.OutListRecursive
                    if getattr(o, "StepId", None) == old_id
                ]
                if len(child) == 1:
                    child[0].StepId = new_id

    def edit_placement(self, obj):
        """Syncs the internal IFC placement"""

        from nativeifc import ifc_tools  # lazy import

        ifc_tools.set_placement(obj)

    def edit_pset(self, obj, prop):
        """Edits a Pset value"""

        from nativeifc import ifc_psets  # lazy import

        ifc_psets.edit_pset(obj, prop)

    def edit_group(self, obj):
        """Edits the children list"""

        from nativeifc import ifc_tools  # lazy import
        from nativeifc import ifc_layers

        if obj.Class in [
            "IfcPresentationLayerAssignment",
            "IfcPresentationLayerWithStyle",
        ]:
            ifcfile = ifc_tools.get_ifcfile(obj)
            if not ifcfile:
                return
            newlist = []
            for child in obj.Group:
                if (
                    not getattr(child, "StepId", None)
                    or ifc_tools.get_ifcfile(child) != ifcfile
                ):
                    print(
                        "DEBUG: Not an IFC object. Removing",
                        child.Label,
                        "from layer",
                        obj.Label,
                    )
                else:
                    # print("DEBUG: adding", child.Label, "to layer", obj.Label)
                    newlist.append(child)
                    ifc_layers.add_to_layer(child, obj)
            if newlist != obj.Group:
                obj.Group = newlist

    def edit_type(self, obj):
        """Edits the type of this object"""

        from nativeifc import ifc_tools  # lazy import
        from nativeifc import ifc_types

        element = ifc_tools.get_ifc_element(obj)
        ifcfile = ifc_tools.get_ifcfile(obj)
        if not element or not ifcfile:
            return
        typerel = getattr(element, "IsTypedBy", None)
        if obj.Type:
            # verify the type is compatible -ex IFcWall in IfcWallType
            if obj.Type.Class != element.is_a() + "Type":
                t = translate("BIM","Error: Incompatible type")
                FreeCAD.Console.PrintError(obj.Label+": "+t+": "+obj.Type.Class+"\n")
                obj.Type = None
                return
            # change type
            new_type = ifc_tools.get_ifc_element(obj.Type)
            if not new_type:
                return
            for rel in typerel:
                if rel.RelatingType == new_type:
                    return
            # assign the new type
            ifc_tools.api_run("type.assign_type",
                              ifcfile,
                              related_objects=[element],
                              relating_type=new_type
            )
        elif typerel:
            # TODO remove type?
            # Not doing anything right now because an unset Type property could screw the ifc file
            pass

    def get_section_data(self, obj):
        """Returns two things: a list of objects and a cut plane"""

        from nativeifc import ifc_tools  # lazy import
        import Part

        if not obj.IfcClass == "IfcAnnotation":
            return [], None
        if obj.ObjectType != "DRAWING":
            return [], None
        objs = getattr(obj, "Objects", [])
        if not objs:
            # no object defined, we automatically use the project
            objs = []
            proj = ifc_tools.get_project(obj)
            if isinstance(proj, FreeCAD.DocumentObject):
                objs.append(proj)
            objs.extend(ifc_tools.get_freecad_children(proj))
        if objs:
            s = []
            for o in objs:
                # TODO print a better message
                if o.ShapeMode != "Shape":
                    s.append(o)
            if s:
                FreeCAD.Console.PrintLog("DEBUG: Generating shapes. This might take some time...\n")
                for o in s:
                        o.ShapeMode = "Shape"
                        o.recompute()
            l = 1
            h = 1
            if obj.ViewObject:
                if hasattr(obj.ViewObject,"DisplayLength"):
                    l = obj.ViewObject.DisplayLength.Value
                    h = obj.ViewObject.DisplayHeight.Value
            plane = Part.makePlane(l,h,FreeCAD.Vector(l/2,-h/2,0),FreeCAD.Vector(0,0,1))
            plane.Placement = obj.Placement
            return objs, plane
        else:
            print("DEBUG: Section plane returned no objects")
            return [], None


class document_object:
    """Holder for the document's IFC objects"""

    def __init__(self):
        pass
