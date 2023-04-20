# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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

import re
import lazy_loader.lazy_loader as lz

import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
import draftfunctions.draftify as ext_draftify
import draftfunctions.fuse as fuse
import draftmake.make_line as make_line
import draftmake.make_wire as make_wire
import draftmake.make_block as make_block

from draftutils.messages import _msg, _err
from draftutils.translate import translate
from draftgeoutils.geometry import is_straight_line

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

    None
        If there is a problem it will return `None`.

    See Also
    --------
    downgrade
    """
    _name = "upgrade"
    utils.print_header(_name, "Upgrade objects")

    if not isinstance(objects, list):
        objects = [objects]

    delete_list = []
    add_list = []
    doc = App.ActiveDocument

    # definitions of actions to perform
    def turnToLine(obj):
        """Turn an edge into a Draft Line."""
        p1 = obj.Shape.Vertexes[0].Point
        p2 = obj.Shape.Vertexes[-1].Point
        newobj = make_line.make_line(p1, p2)
        add_list.append(newobj)
        delete_list.append(obj)
        return newobj

    def makeCompound(objectslist):
        """Return a compound object made from the given objects."""
        newobj = make_block.make_block(objectslist)
        add_list.append(newobj)
        return newobj

    def closeGroupWires(groupslist):
        """Close every open wire in the given groups."""
        result = False
        for grp in groupslist:
            for obj in grp.Group:
                newobj = closeWire(obj)
                # add new objects to their respective groups
                if newobj:
                    result = True
                    grp.addObject(newobj)
        return result

    def makeSolid(obj):
        """Turn an object into a solid, if possible."""
        if obj.Shape.Solids:
            return None
        sol = None
        try:
            sol = Part.makeSolid(obj.Shape)
        except Part.OCCError:
            return None
        else:
            if sol:
                if sol.isClosed():
                    newobj = doc.addObject("Part::Feature", "Solid")
                    newobj.Shape = sol
                    add_list.append(newobj)
                    delete_list.append(obj)
                    return newobj
                else:
                    _err(translate("draft","Object must be a closed shape"))
            else:
                _err(translate("draft","No solid object created"))
        return None

    def closeWire(obj):
        """Close a wire object, if possible."""
        if obj.Shape.Faces:
            return None
        if len(obj.Shape.Wires) != 1:
            return None
        if len(obj.Shape.Edges) == 1:
            return None
        if is_straight_line(obj.Shape):
            return None
        if utils.get_type(obj) == "Wire":
            obj.Closed = True
            return True
        else:
            w = obj.Shape.Wires[0]
            if not w.isClosed():
                edges = w.Edges
                p0 = w.Vertexes[0].Point
                p1 = w.Vertexes[-1].Point
                if p0 == p1:
                    # sometimes an open wire can have the same start
                    # and end points (OCCT bug); in this case,
                    # although it is not closed, the face works.
                    f = Part.Face(w)
                    newobj = doc.addObject("Part::Feature", "Face")
                    newobj.Shape = f
                else:
                    edges.append(Part.LineSegment(p1, p0).toShape())
                    w = Part.Wire(Part.__sortEdges__(edges))
                    newobj = doc.addObject("Part::Feature", "Wire")
                    newobj.Shape = w
                add_list.append(newobj)
                delete_list.append(obj)
                return newobj
            else:
                return None

    def turnToParts(meshes):
        """Turn given meshes to parts."""
        result = False
        for mesh in meshes:
            sh = Arch.getShapeFromMesh(mesh.Mesh)
            if sh:
                newobj = doc.addObject("Part::Feature", "Shell")
                newobj.Shape = sh
                add_list.append(newobj)
                delete_list.append(mesh)
                result = True
        return result

    def makeFusion(obj1, obj2=None):
        """Make a Draft or Part fusion between 2 given objects."""
        if not obj2 and isinstance(obj1, (list, tuple)):
            obj1, obj2 = obj1[0], obj1[1]

        newobj = fuse.fuse(obj1, obj2)
        if newobj:
            add_list.append(newobj)
            return newobj
        return None

    def makeShell(objectslist):
        """Make a shell or compound with the given objects."""
        params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        preserveFaceColor = params.GetBool("preserveFaceColor")  # True
        preserveFaceNames = params.GetBool("preserveFaceNames")  # True
        faces = []
        facecolors = [[], []] if preserveFaceColor else None
        for obj in objectslist:
            faces.extend(obj.Shape.Faces)
            if App.GuiUp and preserveFaceColor:
                # at this point, obj.Shape.Faces are not in same order as the
                # original faces we might have gotten as a result
                # of downgrade, nor do they have the same hashCode().
                # Nevertheless, they still keep reference to their original
                # colors, capture that in facecolors.
                # Also, cannot use ShapeColor here, we need a whole array
                # matching the colors of the array of faces per object,
                # only DiffuseColor has that
                facecolors[0].extend(obj.ViewObject.DiffuseColor)
                facecolors[1] = faces
        sh = Part.makeShell(faces)
        if sh:
            if sh.Faces:
                newobj = doc.addObject("Part::Feature", str(sh.ShapeType))
                newobj.Shape = sh
                if preserveFaceNames:
                    firstName = objectslist[0].Label
                    nameNoTrailNumbers = re.sub(r"\d+$", "", firstName)
                    newobj.Label = "{} {}".format(newobj.Label,
                                                  nameNoTrailNumbers)
                if App.GuiUp and preserveFaceColor:
                    # At this point, sh.Faces are completely new,
                    # with different hashCodes and different ordering
                    # from obj.Shape.Faces. Since we cannot compare
                    # via hashCode(), we have to iterate and use a different
                    # criteria to find the original matching color
                    colarray = []
                    for ind, face in enumerate(newobj.Shape.Faces):
                        for fcind, fcface in enumerate(facecolors[1]):
                            if (face.Area == fcface.Area
                                    and face.CenterOfMass == fcface.CenterOfMass):
                                colarray.append(facecolors[0][fcind])
                                break
                    newobj.ViewObject.DiffuseColor = colarray
                add_list.append(newobj)
                delete_list.extend(objectslist)
                return newobj
        return None

    def joinFaces(objectslist, coplanarity=False, checked=False):
        """Make one big face from selected objects, if possible."""
        faces = []
        for obj in objectslist:
            faces.extend(obj.Shape.Faces)

        # check coplanarity if needed
        if not checked:
            coplanarity = DraftGeomUtils.is_coplanar(faces, 1e-3)
        if not coplanarity:
            _err(translate("draft","Faces must be coplanar to be refined"))
            return None

        # fuse faces
        fuse_face = faces.pop(0)
        for face in faces:
            fuse_face = fuse_face.fuse(face)

        face = DraftGeomUtils.concatenate(fuse_face)
        # to prevent create new object if concatenate fails
        if face.isEqual(fuse_face):
            face = None

        if face:
            # several coplanar and non-curved faces,
            # they can become a Draft Wire
            if (not DraftGeomUtils.hasCurves(face)
                and len(face.Wires) == 1):
                newobj = make_wire.make_wire(face.Wires[0],
                                             closed=True, face=True)
            # if not possible, we do a non-parametric union
            else:
                newobj = doc.addObject("Part::Feature", "Union")
                newobj.Shape = face
            add_list.append(newobj)
            delete_list.extend(objectslist)
            return newobj
        return None

    def makeSketchFace(obj):
        """Make a face from a sketch."""
        face = Part.makeFace(obj.Shape.Wires, "Part::FaceMakerBullseye")
        if face:
            newobj = doc.addObject("Part::Feature", "Face")
            newobj.Shape = face

            add_list.append(newobj)
            if App.GuiUp:
                obj.ViewObject.Visibility = False
            return newobj
        return None

    def makeFaces(objectslist):
        """Make a face from every closed wire in the list."""
        result = False
        for o in objectslist:
            for w in o.Shape.Wires:
                try:
                    f = Part.Face(w)
                except Part.OCCError:
                    pass
                else:
                    newobj = doc.addObject("Part::Feature", "Face")
                    newobj.Shape = f
                    add_list.append(newobj)
                    result = True
                    if o not in delete_list:
                        delete_list.append(o)
        return result

    def makeWires(objectslist):
        """Join edges in the given objects list into wires."""
        edges = []
        for object in objectslist:
            for edge in object.Shape.Edges:
                edges.append(edge)

        try:
            sorted_edges = Part.sortEdges(edges)
            if _DEBUG:
                for item_sorted_edges in sorted_edges:
                    for e in item_sorted_edges:
                        print("Curve: {}".format(e.Curve))
                        print("first: {}, last: {}".format(e.Vertexes[0].Point,
                                                       e.Vertexes[-1].Point))
            wires = [Part.Wire(e) for e in sorted_edges]
        except Part.OCCError:
            return None
        else:
            if (len(objectslist) > 1) and (len(wires) == len(objectslist)):
                # we still have the same number of objects, we actually didn't join anything!
                return makeCompound(objectslist)
            for wire in wires:
                newobj = doc.addObject("Part::Feature", "Wire")
                newobj.Shape = wire
                add_list.append(newobj)
            # delete object only if there are no links to it
            # TODO: A more refined criteria to delete object
            for object in objectslist:
                if object.InList:
                    if App.GuiUp:
                        object.ViewObject.Visibility = False
                else:
                    delete_list.append(object)
            return True
        return None

    # analyzing what we have in our selection
    edges = []
    wires = []
    openwires = []
    faces = []
    groups = []
    parts = []
    curves = []
    facewires = []
    loneedges = []
    meshes = []

    for ob in objects:
        if ob.TypeId == "App::DocumentObjectGroup":
            groups.append(ob)
        elif hasattr(ob, 'Shape'):
            parts.append(ob)
            faces.extend(ob.Shape.Faces)
            wires.extend(ob.Shape.Wires)
            edges.extend(ob.Shape.Edges)
            for f in ob.Shape.Faces:
                facewires.extend(f.Wires)
            wirededges = []
            for w in ob.Shape.Wires:
                if len(w.Edges) > 1:
                    for e in w.Edges:
                        wirededges.append(e.hashCode())
                if not w.isClosed():
                    openwires.append(w)
            for e in ob.Shape.Edges:
                if DraftGeomUtils.geomType(e) != "Line":
                    curves.append(e)
                if not e.hashCode() in wirededges and not e.isClosed():
                    loneedges.append(e)
        elif ob.isDerivedFrom("Mesh::Feature"):
            meshes.append(ob)
    objects = parts

    if _DEBUG:
        print("objects: {}, edges: {}".format(objects, edges))
        print("wires: {}, openwires: {}".format(wires, openwires))
        print("faces: {}".format(faces))
        print("groups: {}, curves: {}".format(groups, curves))
        print("facewires: {}, loneedges: {}".format(facewires, loneedges))

    if force:
        all_func = {"makeCompound" : makeCompound,
                    "closeGroupWires" : closeGroupWires,
                    "makeSolid" : makeSolid,
                    "closeWire" : closeWire,
                    "turnToParts" : turnToParts,
                    "makeFusion" : makeFusion,
                    "makeShell" : makeShell,
                    "makeFaces" : makeFaces,
                    "draftify" : ext_draftify.draftify,
                    "joinFaces" : joinFaces,
                    "makeSketchFace" : makeSketchFace,
                    "makeWires" : makeWires,
                    "turnToLine" : turnToLine}
        if force in all_func:
            result = all_func[force](objects)
        else:
            _msg(translate("draft","Upgrade: Unknown force method:") + " " + force)
            result = None

    else:
        # checking faces coplanarity
        # The precision needed in Part.makeFace is 1e-7. Here we use a
        # higher value to let that function throw the exception when
        # joinFaces is called if the precision is insufficient
        if faces:
            faces_coplanarity = DraftGeomUtils.is_coplanar(faces, 1e-3)

        # applying transformations automatically
        result = None

        # if we have a group: turn each closed wire inside into a face
        if groups:
            result = closeGroupWires(groups)
            if result:
                _msg(translate("draft","Found groups: closing each open object inside"))

        # if we have meshes, we try to turn them into shapes
        elif meshes:
            result = turnToParts(meshes)
            if result:
                _msg(translate("draft","Found meshes: turning into Part shapes"))

        # we have only faces here, no lone edges
        elif faces and (len(wires) + len(openwires) == len(facewires)):
            # we have one shell: we try to make a solid
            if len(objects) == 1 and len(faces) > 3 and not faces_coplanarity:
                result = makeSolid(objects[0])
                if result:
                    _msg(translate("draft","Found 1 solidifiable object: solidifying it"))
            # we have exactly 2 objects: we fuse them
            elif len(objects) == 2 and not curves and not faces_coplanarity:
                result = makeFusion(objects[0], objects[1])
                if result:
                    _msg(translate("draft","Found 2 objects: fusing them"))
            # we have many separate faces: we try to make a shell or compound
            elif len(objects) >= 2 and len(faces) > 1 and not loneedges:
                result = makeShell(objects)
                if result:
                    _msg(translate("draft","Found several objects: creating a "
                             + str(result.Shape.ShapeType)))
            # we have faces: we try to join them if they are coplanar
            elif len(objects) == 1 and len(faces) > 1:
                result = joinFaces(objects, faces_coplanarity, True)
                if result:
                    _msg(translate("draft","Found object with several coplanar faces: refine them"))
            # only one object: if not parametric, we "draftify" it
            elif (len(objects) == 1
                  and not objects[0].isDerivedFrom("Part::Part2DObjectPython")):
                result = ext_draftify.draftify(objects[0])
                if result:
                    add_list.append(result)
                    _msg(translate("draft","Found 1 non-parametric objects: draftifying it"))

        # in the following cases there are no faces
        elif not faces:
            # we have only closed wires
            if wires and not openwires and not loneedges:
                # we have a sketch: extract a face
                if (len(objects) == 1
                    and objects[0].isDerivedFrom("Sketcher::SketchObject")):
                    result = makeSketchFace(objects[0])
                    if result:
                        _msg(translate("draft","Found 1 closed sketch object: creating a face from it"))
                # only closed wires
                else:
                    result = makeFaces(objects)
                    if result:
                        _msg(translate("draft","Found closed wires: creating faces"))
            # wires or edges: we try to join them
            elif len(objects) > 1 and len(edges) > 1:
                result = makeWires(objects)
                if result:
                    _msg(translate("draft","Found several wires or edges: wiring them"))
                else:
                    _msg(translate("draft","Found several non-treatable objects: creating compound"))
            # special case, we have only one open wire. We close it,
            # unless it has only 1 edge!
            elif len(objects) == 1 and len(openwires) == 1:
                result = closeWire(objects[0])
                _msg(translate("draft","trying: closing it"))
                if result:
                    _msg(translate("draft","Found 1 open wire: closing it"))
            # we have only one object that contains one edge
            # TODO: improve draftify function
            # only one object: if not parametric, we "draftify" it
            # elif (len(objects) == 1
            #       and not objects[0].isDerivedFrom("Part::Part2DObjectPython")):
            #     result = ext_draftify.draftify(objects[0])
            #     if result:
            #         _msg(translate("draft","Found 1 non-parametric objects: draftifying it"))
            elif (len(objects) == 1 and len(edges) == 1
                  and not objects[0].isDerivedFrom("Part::Part2DObjectPython")):
                e = objects[0].Shape.Edges[0]
                edge_type = DraftGeomUtils.geomType(e)
                # currently only support Line and Circle
                if edge_type in ("Line", "Circle"):
                    result = ext_draftify.draftify(objects[0])
                    if result:
                        add_list.append(result)
                        _msg(translate("draft","Found 1 object: draftifying it"))
            # only points, no edges
            elif not edges and len(objects) > 1:
                result = makeCompound(objects)
                if result:
                    _msg(translate("draft","Found points: creating compound"))
        # all other cases, if more than 1 object, make a compound
        elif len(objects) > 1:
            result = makeCompound(objects)
            if result:
                _msg(translate("draft","Found several non-treatable objects: creating compound"))
        # no result has been obtained
        if not result:
            _msg(translate("draft","Unable to upgrade these objects."))

    if delete:
        names = []
        for o in delete_list:
            names.append(o.Name)
        delete_list = []
        for n in names:
            doc.removeObject(n)

    gui_utils.select(add_list)
    return add_list, delete_list

## @}
