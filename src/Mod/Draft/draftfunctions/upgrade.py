# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2025 The FreeCAD Project Association                    *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides functions to upgrade objects by different methods.

See also the `downgrade` function.
"""
## @package downgrade
# \ingroup draftfunctions
# \brief Provides functions to upgrade objects by different methods.

import math
import re
import lazy_loader.lazy_loader as lz

import FreeCAD as App
from draftfunctions import draftify
from draftgeoutils.geometry import is_straight_line
from draftmake import make_block
from draftmake import make_wire
from draftutils import gui_utils
from draftutils import params
from draftutils import utils
from draftutils.groups import is_group
from draftutils.messages import _msg
from draftutils.translate import translate

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
Arch = lz.LazyLoader("Arch", globals(), "Arch")

_DEBUG = False

## \addtogroup draftfunctions
# @{


def upgrade(objects, delete=False, force=None):
    """Upgrade the given objects.

    This is a counterpart to `downgrade`.

    Parameters
    ----------
    objects: Part::Feature or list
        A single object to upgrade or a list
        containing various such objects.

    delete: bool, optional
        It defaults to `False`.
        If it is `True`, the old objects are deleted, and only the resulting
        object is kept.

    force: str, optional
        It defaults to `None`.
        Its value can be used to force a certain method of upgrading.
        It can be any of: `'makeCompound'`, `'closeGroupWires'`,
        `'makeSolid'`, `'closeWire'`, `'turnToParts'`, `'makeFusion'`,
        `'makeShell'`, `'makeFaces'`, `'draftify'`, `'joinFaces'`,
        `'makeSketchFace'`, `'makeWires'`.

    Returns
    -------
    tuple
        A tuple containing two lists, a list of new objects
        and a list of objects to be deleted.

    See Also
    --------
    downgrade
    """

    # definitions of actions to perform

    def makeCompound(objects):
        """Return a compound object made from the given objects."""
        newobj = make_block.make_block(objects)
        format(objects[0], [newobj])
        add_to_parent(objects[0], [newobj])
        add_list.append(newobj)
        return True

    def closeGroupWires(groups):
        """Close every open wire in the given groups."""
        result = False
        for grp in groups:
            if any([closeWire(obj) for obj in grp.Group]):
                result = True
        return result

    def makeSolid(obj):
        """Turn an object into a solid, if possible."""
        if obj.Shape.Solids:
            return False
        try:
            solid = Part.makeSolid(obj.Shape)
        except Part.OCCError:
            return False
        if not solid.isClosed():
            return False
        newobj = doc.addObject("Part::Feature", "Solid")
        newobj.Shape = solid
        format(obj, [newobj])
        add_to_parent(obj, [newobj])
        add_list.append(newobj)
        delete_list.append(obj)
        return True

    def closeWire(obj):
        """Close a wire object, if possible."""
        if obj.Shape.Faces:
            return False
        if len(obj.Shape.Wires) != 1:
            return False
        if len(obj.Shape.Edges) == 1:
            return False
        if is_straight_line(obj.Shape):
            return False
        if utils.get_type(obj) == "Wire":
            obj.Closed = True
            return True
        wire = obj.Shape.Wires[0]
        if wire.isClosed():
            return False
        verts = wire.OrderedVertexes
        p0 = verts[0].Point
        p1 = verts[-1].Point
        edges = wire.Edges
        edges.append(Part.LineSegment(p1, p0).toShape())
        wire = Part.Wire(Part.__sortEdges__(edges))
        newobj = doc.addObject("Part::Feature", "Wire")
        newobj.Shape = wire
        format(obj, [newobj])
        add_to_parent(obj, [newobj])
        add_list.append(newobj)
        delete_list.append(obj)
        return True

    def turnToParts(meshes):
        """Turn given meshes to parts."""
        result = False
        for mesh in meshes:
            shp = Arch.getShapeFromMesh(mesh.Mesh)
            if shp:
                newobj = doc.addObject("Part::Feature", shp.ShapeType)
                newobj.Shape = shp
                format(mesh, [newobj])
                add_to_parent(mesh, [newobj])
                add_list.append(newobj)
                delete_list.append(mesh)
                result = True
        return result

    def makeFusion(objects):
        """Make a Draft or Part fusion between 2 given objects."""
        newobj = doc.addObject("Part::Fuse", "Fusion")
        newobj.Base = objects[0]
        newobj.Tool = objects[1]
        format(objects[0], [newobj])
        add_to_parent(objects[0], [newobj])
        add_list.append(newobj)
        return True

    def makeShell(objects):
        """Make a shell or compound with the given objects."""
        faces = []
        done_list = []
        for obj in objects:
            if obj.Shape.Faces:
                faces.extend(obj.Shape.Faces)
                done_list.append(obj)
        if not faces:
            return None
        shp = Part.makeShell(faces)
        if shp.isNull():
            return None
        newobj = doc.addObject("Part::Feature", shp.ShapeType)
        newobj.Shape = shp
        # Format before applying diffuse color:
        format(done_list[0], [newobj])
        add_to_parent(done_list[0], [newobj])
        add_list.append(newobj)
        delete_list.extend(done_list)

        if App.GuiUp and params.get_param("preserveFaceColor"):
            # Must happen after add_to_parent for correct CenterOfMass.
            colors = gui_utils.get_diffuse_color(done_list)
            if len(faces) != len(colors):
                newobj.ViewObject.DiffuseColor = [colors[0]]
            else:
                # The ordering of shp.Faces may be different. Since we cannot
                # compare via hashCode(), we have to iterate and use different
                # criteria to find the correct color.
                old_data = []
                for face, color in zip(faces, colors):
                    old_data.append([face.Area, face.CenterOfMass, color])
                new_colors = []
                for new_face in shp.Faces:
                    new_area = new_face.Area
                    new_cen = new_face.CenterOfMass
                    for old_area, old_cen, old_color in old_data:
                        if math.isclose(new_area, old_area, abs_tol=1e-7) \
                                and new_cen.isEqual(old_cen, 1e-7):
                            new_colors.append(old_color)
                            break
                newobj.ViewObject.DiffuseColor = new_colors

        if params.get_param("preserveFaceNames"):
            firstName = done_list[0].Label
            nameNoTrailNumbers = re.sub(r"\d+$", "", firstName)
            newobj.Label = "{} {}".format(newobj.Label, nameNoTrailNumbers)

        return newobj

    def joinFaces(objects):
        """Make one big face from the given objects, if possible."""
        faces = []
        done_list = []
        for obj in objects:
            if obj.Shape.Faces:
                faces.extend(obj.Shape.Faces)
                done_list.append(obj)
        if not faces:
            return False
        if not DraftGeomUtils.is_coplanar(faces, 1e-3):
            return False
        fuse_face = faces.pop(0)
        for face in faces:
            fuse_face = fuse_face.fuse(face)
        face = DraftGeomUtils.concatenate(fuse_face)
        # check if concatenate failed
        if face.isEqual(fuse_face):
            return False
        # several coplanar and non-curved faces, they can become a Draft Wire
        if len(face.Wires) == 1 and not DraftGeomUtils.hasCurves(face):
            newobj = make_wire.make_wire(face.Wires[0], closed=True, face=True)
        # if not possible, we do a non-parametric union
        else:
            newobj = doc.addObject("Part::Feature", "Union")
            newobj.Shape = face
        format(done_list[0], [newobj])
        add_to_parent(done_list[0], [newobj])
        add_list.append(newobj)
        delete_list.extend(done_list)
        return True

    def makeSketchFace(obj):
        """Make a face from a sketch."""
        face = Part.makeFace(obj.Shape.Wires, "Part::FaceMakerBullseye")
        if not face:
            return False
        newobj = doc.addObject("Part::Feature", "Face")
        newobj.Shape = face
        format(obj, [newobj])
        add_to_parent(obj, [newobj])
        add_list.append(newobj)
        delete_list.append(obj)
        return True

    def makeFaces(objects):
        """Make a face from every closed wire in the given objects."""
        result = False
        for obj in objects:
            new_list = []
            for wire in obj.Shape.Wires:
                try:
                    face = Part.Face(wire)
                except Part.OCCError:
                    continue
                newobj = doc.addObject("Part::Feature", "Face")
                newobj.Shape = face
                new_list.append(newobj)
            if not new_list:
                continue
            format(obj, new_list)
            add_to_parent(obj, new_list)
            add_list.extend(new_list)
            delete_list.append(obj)
            result = True
        return result

    def makeWires(objects):
        """Join edges in the given objects into wires."""
        edges = []
        done_list = []
        for obj in objects:
            if obj.Shape.Edges:
                edges.extend(obj.Shape.Edges)
                done_list.append(obj)
        if not edges:
            return False
        try:
            sorted_edges = Part.sortEdges(edges)
            if _DEBUG:
                for cluster in sorted_edges:
                    for edge in cluster:
                        print("Curve: {}".format(edge.Curve))
                        print("first: {}, last: {}".format(edge.Vertexes[0].Point,
                                                           edge.Vertexes[-1].Point))
            wires = [Part.Wire(cluster) for cluster in sorted_edges]
        except Part.OCCError:
            return False
        if len(objects) > 1 and len(wires) == len(objects):
            # we still have the same number of objects, we actually didn't join anything!
            return False
        new_list = []
        for wire in wires:
            newobj = doc.addObject("Part::Feature", "Wire")
            newobj.Shape = wire
            new_list.append(newobj)
        # We don't know which wire came from which obj, we format them the same:
        format(done_list[0], new_list)
        add_to_parent(done_list[0], new_list)
        add_list.extend(new_list)
        delete_list.extend(done_list)
        return True

    def _draftify(obj):
        """Wrapper for draftify."""
        new_list = draftify.draftify(obj, delete=False)
        if not new_list:
            return False
        if not isinstance(new_list, list):
            new_list = [new_list]
        format(obj, new_list)
        add_to_parent(obj, new_list)
        add_list.extend(new_list)
        delete_list.append(obj)
        return True


    # helper functions (same as in downgrade.py)

    def get_parent(obj):
        # Problem with obj.getParent():
        # https://github.com/FreeCAD/FreeCAD/issues/19600
        parent = obj.getParentGroup()
        if parent is not None:
            return parent
        return obj.getParentGeoFeatureGroup()

    def can_be_deleted(obj):
        if not obj.InList:
            return True
        for other in obj.InList:
            if is_group(other):
                continue
            if other.TypeId == "App::Part":
                continue
            return False
        return True

    def delete_object(obj):
        if utils.is_deleted(obj):
            return
        parent = get_parent(obj)
        if parent is not None and parent.TypeId == "PartDesign::Body":
            obj = parent
        if not can_be_deleted(obj):
            # Make obj invisible instead:
            obj.Visibility = False
            return
        if obj.TypeId == "PartDesign::Body":
            obj.removeObjectsFromDocument()
        doc.removeObject(obj.Name)

    def add_to_parent(obj, new_list):
        parent = get_parent(obj)
        if parent is None:
            if doc.getObject("Draft_Construction"):
                # This cludge is required because the make_* commands may
                # put new objects in the construction group.
                constr_group = doc.getObject("Draft_Construction")
                for newobj in new_list:
                    constr_group.removeObject(newobj)
            return
        if parent.TypeId == "PartDesign::Body":
            # We don't add to a PD Body. We process its placement and
            # add to its parent instead.
            for newobj in new_list:
                newobj.Placement = parent.Placement.multiply(newobj.Placement)
            add_to_parent(parent, new_list)
            return
        for newobj in new_list:
            # Using addObject is different from just changing the Group property.
            # With addObject the object will be added to the parent group, but if
            # that is a normal group, also to that group's parent GeoFeatureGroup,
            # if available.
            parent.addObject(newobj)

    def format(obj, new_list):
        for newobj in new_list:
            gui_utils.format_object(newobj, obj, ignore_construction=True)


    doc = App.ActiveDocument
    add_list = []
    delete_list = []
    result = False

    if not isinstance(objects, list):
        objects = [objects]
    if not objects:
        return add_list, delete_list

    # analyzing objects
    faces = []
    wires = []
    openwires = []
    facewires = []
    edges = []
    loneedges = []
    groups = []
    meshes = []
    parts = []

    for obj in objects:
        if obj.TypeId == "App::DocumentObjectGroup":
            groups.append(obj)
        elif hasattr(obj, "Shape"):
            parts.append(obj)
            faces.extend(obj.Shape.Faces)
            wires.extend(obj.Shape.Wires)
            edges.extend(obj.Shape.Edges)
            for face in obj.Shape.Faces:
                facewires.extend(face.Wires)
            wirededges = []
            for wire in obj.Shape.Wires:
                if len(wire.Edges) > 1:
                    for edge in wire.Edges:
                        wirededges.append(edge.hashCode())
                if not wire.isClosed():
                    openwires.append(wire)
            for edge in obj.Shape.Edges:
                if not edge.hashCode() in wirededges and not edge.isClosed():
                    loneedges.append(edge)
        elif obj.isDerivedFrom("Mesh::Feature"):
            meshes.append(obj)
    objects = parts

    if _DEBUG:
        print("objects: {}, edges: {}".format(objects, edges))
        print("wires: {}, openwires: {}".format(wires, openwires))
        print("faces: {}".format(faces))
        print("groups: {}".format(groups))
        print("facewires: {}, loneedges: {}".format(facewires, loneedges))

    if not (groups or objects or meshes):
        result = False

    elif force:
        if force == "closeGroupWires":
            result = closeGroupWires(groups)
        elif force == "turnToParts":
            result = turnToParts(meshes)
        else:
            # functions that work on a single object:
            single_funcs = {"closeWire": closeWire,
                            "draftify": _draftify,
                            "makeSketchFace": makeSketchFace,
                            "makeSolid": makeSolid}
            # functions that work on multiple objects:
            multi_funcs = {"joinFaces": joinFaces,
                           "makeCompound": makeCompound,
                           "makeFaces": makeFaces,
                           "makeFusion": makeFusion,
                           "makeShell": makeShell,
                           "makeWires": makeWires}
            if force in single_funcs:
                result = any([single_funcs[force](obj) for obj in objects])
            elif force in multi_funcs:
                result = multi_funcs[force](objects)
            else:
                _msg(translate("draft", "Upgrade: Unknown force method:") + " " + force)
                result = False

    # if we have a group: close each wire inside
    elif groups:
        result = closeGroupWires(groups)
        if result:
            _msg(translate("draft", "Found groups: closing open wires inside"))

    # if we have meshes, we try to turn them into shapes
    elif meshes:
        result = turnToParts(meshes)
        if result:
            _msg(translate("draft", "Found meshes: turning them into Part shapes"))

    else:
        # checking faces coplanarity
        # The precision needed in Part.makeFace is 1e-7. Here we use a
        # higher value to let that function throw the exception when
        # joinFaces is called if the precision is insufficient.
        if faces:
            faces_coplanarity = DraftGeomUtils.is_coplanar(faces, 1e-3)

        parent = get_parent(objects[0])
        same_parent = True
        same_parent_type = getattr(parent, "TypeId", "")  # "" for global space.
        if len(objects) > 1:
            for obj in objects[1:]:
                if get_parent(obj) != parent:
                    same_parent = False
                    same_parent_type = None
                    break

        # we have only faces
        if faces and len(facewires) == len(wires) and not openwires and not loneedges:

            # we have one shell: we try to make a solid
            # this also handles PD Bodies and PD features with solids (result will be False)
            if len(objects) == 1 and len(faces) > 3 and not faces_coplanarity:
                result = makeSolid(objects[0])
                if result:
                    _msg(translate("draft", "Found 1 solidifiable object: solidifying it"))

            # we have exactly 2 objects: we fuse them
            elif len(objects) == 2 \
                    and not faces_coplanarity \
                    and same_parent \
                    and same_parent_type != "PartDesign::Body":
                result = makeFusion(objects)
                if result:
                    _msg(translate("draft", "Found 2 objects: fusing them"))

            # we have many separate faces: we try to make a shell or compound
            elif len(objects) > 1 \
                    and len(faces) > 1 \
                    and same_parent \
                    and same_parent_type != "PartDesign::Body":
                result = makeShell(objects)
                if result:
                    _msg(translate(
                        "draft",
                        "Found several objects: creating a " + result.Shape.ShapeType
                    ))

            # we have faces: we try to join them if they are coplanar
            elif len(objects) == 1 and len(faces) > 1 and faces_coplanarity:
                result = joinFaces(objects)
                if result:
                    _msg(translate("draft", "Found object with several coplanar faces: refining them"))

            # only one object: if not parametric, we "draftify" it
            elif len(objects) == 1 \
                    and not objects[0].isDerivedFrom("Part::Part2DObjectPython") \
                    and not utils.get_type(objects[0]) in ["BezCurve", "BSpline", "Wire"]:
                result = _draftify(objects[0])
                if result:
                    _msg(translate("draft", "Found 1 non-parametric object: draftifying it"))

        # in the following cases there are no faces
        elif not faces:

            # we have only closed wires
            if wires and not openwires and not loneedges:
                # we have a sketch: extract a face
                if len(objects) == 1 and objects[0].isDerivedFrom("Sketcher::SketchObject"):
                    result = makeSketchFace(objects[0])
                    if result:
                        _msg(translate("draft", "Found 1 closed sketch object: creating a face from it"))
                # only closed wires
                else:
                    result = makeFaces(objects)
                    if result:
                        _msg(translate("draft", "Found closed wires: creating faces"))

            # wires or edges: we try to join them
            elif len(objects) > 1 and len(edges) > 1 and same_parent:
                result = makeWires(objects)
                if result:
                    _msg(translate("draft", "Found several wires or edges: wiring them"))
                else:
                    result = makeCompound(objects)
                    if result:
                        _msg(translate("draft", "Found several non-treatable objects: creating compound"))

            # special case, we have only one open wire. We close it, unless it has only 1 edge!
            elif len(objects) == 1 and len(openwires) == 1:
                result = closeWire(objects[0])
                if result:
                    _msg(translate("draft", "Found 1 open wire: closing it"))

            # only one object: if not parametric, we "draftify" it
            elif len(objects) == 1 \
                    and len(edges) == 1 \
                    and not objects[0].isDerivedFrom("Part::Part2DObjectPython") \
                    and not utils.get_type(objects[0]) in ["BezCurve", "BSpline", "Wire"]:
                edge_type = DraftGeomUtils.geomType(objects[0].Shape.Edges[0])
                # currently only support Line and Circle
                if edge_type in ("Line", "Circle"):
                    result = _draftify(objects[0])
                    if result:
                        _msg(translate("draft", "Found 1 non-parametric object: draftifying it"))

            # only points, no edges
            elif not edges and len(objects) > 1:
                result = makeCompound(objects)
                if result:
                    _msg(translate("draft", "Found points: creating compound"))

        # all other cases, if more than 1 object, make a compound
        elif len(objects) > 1:
            result = makeCompound(objects)
            if result:
                _msg(translate("draft", "Found several non-treatable objects: creating compound"))

    # no result has been obtained
    if not result:
        _msg(translate("draft", "Unable to upgrade these objects"))

    if delete:
        for obj in delete_list:
            delete_object(obj)

    gui_utils.select(add_list)
    return add_list, delete_list

## @}
