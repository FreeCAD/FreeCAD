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
# \ingroup draftfuctions
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

from draftutils.messages import _msg
from draftutils.translate import _tr

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
Arch = lz.LazyLoader("Arch", globals(), "Arch")

_DEBUG = False

## \addtogroup draftfuctions
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

    def closeWire(obj):
        """Close a wire object, if possible."""
        if obj.Shape.Faces:
            return None
        if len(obj.Shape.Wires) != 1:
            return None
        if len(obj.Shape.Edges) == 1:
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
        """Make a shell with the given objects."""
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
                newobj = doc.addObject("Part::Feature", "Shell")
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

    def joinFaces(objectslist):
        """Make one big face from selected objects, if possible."""
        faces = []
        for obj in objectslist:
            faces.extend(obj.Shape.Faces)
        u = faces.pop(0)
        for f in faces:
            u = u.fuse(f)
        if DraftGeomUtils.isCoplanar(faces):
            u = DraftGeomUtils.concatenate(u)
            if not DraftGeomUtils.hasCurves(u):
                # several coplanar and non-curved faces,
                # they can become a Draft Wire
                newobj = make_wire.make_wire(u.Wires[0],
                                             closed=True, face=True)
            else:
                # if not possible, we do a non-parametric union
                newobj = doc.addObject("Part::Feature", "Union")
                newobj.Shape = u
            add_list.append(newobj)
            delete_list.extend(objectslist)
            return newobj
        return None

    def makeSketchFace(obj):
        """Make a Draft Wire closed and filled out of a sketch."""
        newobj = make_wire.make_wire(obj.Shape, closed=True)
        if newobj:
            newobj.Base = obj
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
        for o in objectslist:
            for e in o.Shape.Edges:
                edges.append(e)
        try:
            nedges = Part.__sortEdges__(edges[:])
            if _DEBUG:
                for e in nedges:
                    print("Curve: {}".format(e.Curve))
                    print("first: {}, last: {}".format(e.Vertexes[0].Point,
                                                       e.Vertexes[-1].Point))
            w = Part.Wire(nedges)
        except Part.OCCError:
            return None
        else:
            if len(w.Edges) == len(edges):
                newobj = doc.addObject("Part::Feature", "Wire")
                newobj.Shape = w
                add_list.append(newobj)
                delete_list.extend(objectslist)
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
                if not e.hashCode() in wirededges:
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
        if force in ("makeCompound", "closeGroupWires", "makeSolid",
                     "closeWire", "turnToParts", "makeFusion",
                     "makeShell", "makeFaces", "draftify",
                     "joinFaces", "makeSketchFace", "makeWires",
                     "turnToLine"):
            # TODO: Using eval to evaluate a string is not ideal
            # and potentially a security risk.
            # How do we execute the function without calling eval?
            # Best case, a series of if-then statements.
            draftify = ext_draftify.draftify
            result = eval(force)(objects)
        else:
            _msg(_tr("Upgrade: Unknown force method:") + " " + force)
            result = None
    else:
        # applying transformations automatically
        result = None

        # if we have a group: turn each closed wire inside into a face
        if groups:
            result = closeGroupWires(groups)
            if result:
                _msg(_tr("Found groups: closing each open object inside"))

        # if we have meshes, we try to turn them into shapes
        elif meshes:
            result = turnToParts(meshes)
            if result:
                _msg(_tr("Found meshes: turning into Part shapes"))

        # we have only faces here, no lone edges
        elif faces and (len(wires) + len(openwires) == len(facewires)):
            # we have one shell: we try to make a solid
            if len(objects) == 1 and len(faces) > 3:
                result = makeSolid(objects[0])
                if result:
                    _msg(_tr("Found 1 solidifiable object: solidifying it"))
            # we have exactly 2 objects: we fuse them
            elif len(objects) == 2 and not curves:
                result = makeFusion(objects[0], objects[1])
                if result:
                    _msg(_tr("Found 2 objects: fusing them"))
            # we have many separate faces: we try to make a shell
            elif len(objects) > 2 and len(faces) > 1 and not loneedges:
                result = makeShell(objects)
                if result:
                    _msg(_tr("Found several objects: creating a shell"))
            # we have faces: we try to join them if they are coplanar
            elif len(faces) > 1:
                result = joinFaces(objects)
                if result:
                    _msg(_tr("Found several coplanar objects or faces: "
                             "creating one face"))
            # only one object: if not parametric, we "draftify" it
            elif (len(objects) == 1
                  and not objects[0].isDerivedFrom("Part::Part2DObjectPython")):
                result = ext_draftify.draftify(objects[0])
                if result:
                    _msg(_tr("Found 1 non-parametric objects: "
                             "draftifying it"))

        # we have only one object that contains one edge
        elif not faces and len(objects) == 1 and len(edges) == 1:
            # we have a closed sketch: extract a face
            if (objects[0].isDerivedFrom("Sketcher::SketchObject")
                    and len(edges[0].Vertexes) == 1):
                result = makeSketchFace(objects[0])
                if result:
                    _msg(_tr("Found 1 closed sketch object: "
                             "creating a face from it"))
            else:
                # turn to Draft Line
                e = objects[0].Shape.Edges[0]
                if isinstance(e.Curve, (Part.LineSegment, Part.Line)):
                    result = turnToLine(objects[0])
                    if result:
                        _msg(_tr("Found 1 linear object: converting to line"))

        # we have only closed wires, no faces
        elif wires and not faces and not openwires:
            # we have a sketch: extract a face
            if (len(objects) == 1
                    and objects[0].isDerivedFrom("Sketcher::SketchObject")):
                result = makeSketchFace(objects[0])
                if result:
                    _msg(_tr("Found 1 closed sketch object: "
                             "creating a face from it"))
            # only closed wires
            else:
                result = makeFaces(objects)
                if result:
                    _msg(_tr("Found closed wires: creating faces"))

        # special case, we have only one open wire. We close it,
        # unless it has only 1 edge!
        elif len(openwires) == 1 and not faces and not loneedges:
            result = closeWire(objects[0])
            if result:
                _msg(_tr("Found 1 open wire: closing it"))
        # only open wires and edges: we try to join their edges
        elif openwires and not wires and not faces:
            result = makeWires(objects)
            if result:
                _msg(_tr("Found several open wires: joining them"))
        # only loneedges: we try to join them
        elif loneedges and not facewires:
            result = makeWires(objects)
            if result:
                _msg(_tr("Found several edges: wiring them"))
        # all other cases, if more than 1 object, make a compound
        elif len(objects) > 1:
            result = makeCompound(objects)
            if result:
                _msg(_tr("Found several non-treatable objects: "
                         "creating compound"))
        # no result has been obtained
        if not result:
            _msg(_tr("Unable to upgrade these objects."))

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
