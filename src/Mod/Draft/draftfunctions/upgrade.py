# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""This module provides the code for Draft upgrade function.
"""
## @package upgrade
# \ingroup DRAFT
# \brief This module provides the code for upgrade offset function.

import FreeCAD as App

import draftutils.gui_utils as gui_utils
import draftutils.utils as utils

from draftutils.translate import _tr

from draftmake.make_copy import make_copy

from draftmake.make_line import makeLine
from draftmake.make_wire import makeWire
from draftmake.make_block import makeBlock

from draftfunctions.draftify import draftify
from draftfunctions.fuse import fuse


def upgrade(objects, delete=False, force=None):
    """upgrade(objects,delete=False,force=None)
    
    Upgrade the given object(s).
    
    Parameters
    ----------
    objects :

    delete : bool
        If delete is True, old objects are deleted.

    force : string
        The force attribute can be used to force a certain way of upgrading.
        Accepted values: makeCompound, closeGroupWires, makeSolid, closeWire,
        turnToParts, makeFusion, makeShell, makeFaces, draftify, joinFaces,
        makeSketchFace, makeWires.
    
    Return
    ----------
    Returns a dictionary containing two lists, a list of new objects and a list
    of objects to be deleted
    """

    import Part
    import DraftGeomUtils

    if not isinstance(objects,list):
        objects = [objects]

    global deleteList, newList
    deleteList = []
    addList = []

    # definitions of actions to perform

    def turnToLine(obj):
        """turns an edge into a Draft line"""
        p1 = obj.Shape.Vertexes[0].Point
        p2 = obj.Shape.Vertexes[-1].Point
        newobj = makeLine(p1,p2)
        addList.append(newobj)
        deleteList.append(obj)
        return newobj

    def makeCompound(objectslist):
        """returns a compound object made from the given objects"""
        newobj = makeBlock(objectslist)
        addList.append(newobj)
        return newobj

    def closeGroupWires(groupslist):
        """closes every open wire in the given groups"""
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
        """turns an object into a solid, if possible"""
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
                    newobj = App.ActiveDocument.addObject("Part::Feature","Solid")
                    newobj.Shape = sol
                    addList.append(newobj)
                    deleteList.append(obj)
            return newobj

    def closeWire(obj):
        """closes a wire object, if possible"""
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
                    # sometimes an open wire can have its start and end points identical (OCC bug)
                    # in that case, although it is not closed, face works...
                    f = Part.Face(w)
                    newobj = App.ActiveDocument.addObject("Part::Feature","Face")
                    newobj.Shape = f
                else:
                    edges.append(Part.LineSegment(p1,p0).toShape())
                    w = Part.Wire(Part.__sortEdges__(edges))
                    newobj = App.ActiveDocument.addObject("Part::Feature","Wire")
                    newobj.Shape = w
                addList.append(newobj)
                deleteList.append(obj)
                return newobj
            else:
                return None

    def turnToParts(meshes):
        """turn given meshes to parts"""
        result = False
        import Arch
        for mesh in meshes:
            sh = Arch.getShapeFromMesh(mesh.Mesh)
            if sh:
                newobj = App.ActiveDocument.addObject("Part::Feature","Shell")
                newobj.Shape = sh
                addList.append(newobj)
                deleteList.append(mesh)
                result = True
        return result

    def makeFusion(obj1,obj2):
        """makes a Draft or Part fusion between 2 given objects"""
        newobj = fuse(obj1,obj2)
        if newobj:
            addList.append(newobj)
            return newobj
        return None

    def makeShell(objectslist):
        """makes a shell with the given objects"""
        params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        preserveFaceColor = params.GetBool("preserveFaceColor") # True
        preserveFaceNames = params.GetBool("preserveFaceNames") # True
        faces = []
        facecolors = [[], []] if (preserveFaceColor) else None
        for obj in objectslist:
            faces.extend(obj.Shape.Faces)
            if (preserveFaceColor):
                """ at this point, obj.Shape.Faces are not in same order as the
                original faces we might have gotten as a result of downgrade, nor do they
                have the same hashCode(); but they still keep reference to their original
                colors - capture that in facecolors.
                Also, cannot w/ .ShapeColor here, need a whole array matching the colors
                of the array of faces per object, only DiffuseColor has that """
                facecolors[0].extend(obj.ViewObject.DiffuseColor)
                facecolors[1] = faces
        sh = Part.makeShell(faces)
        if sh:
            if sh.Faces:
                newobj = App.ActiveDocument.addObject("Part::Feature","Shell")
                newobj.Shape = sh
                if (preserveFaceNames):
                    import re
                    firstName = objectslist[0].Label
                    nameNoTrailNumbers = re.sub("\d+$", "", firstName)
                    newobj.Label = "{} {}".format(newobj.Label, nameNoTrailNumbers)
                if (preserveFaceColor):
                    """ At this point, sh.Faces are completely new, with different hashCodes
                    and different ordering from obj.Shape.Faces; since we cannot compare
                    via hashCode(), we have to iterate and use a different criteria to find
                    the original matching color """
                    colarray = []
                    for ind, face in enumerate(newobj.Shape.Faces):
                        for fcind, fcface in enumerate(facecolors[1]):
                            if ((face.Area == fcface.Area) and (face.CenterOfMass == fcface.CenterOfMass)):
                                colarray.append(facecolors[0][fcind])
                                break
                    newobj.ViewObject.DiffuseColor = colarray;
                addList.append(newobj)
                deleteList.extend(objectslist)
                return newobj
        return None

    def joinFaces(objectslist):
        """makes one big face from selected objects, if possible"""
        faces = []
        for obj in objectslist:
            faces.extend(obj.Shape.Faces)
        u = faces.pop(0)
        for f in faces:
            u = u.fuse(f)
        if DraftGeomUtils.isCoplanar(faces):
            u = DraftGeomUtils.concatenate(u)
            if not DraftGeomUtils.hasCurves(u):
                # several coplanar and non-curved faces: they can become a Draft wire
                newobj = makeWire(u.Wires[0],closed=True,face=True)
            else:
                # if not possible, we do a non-parametric union
                newobj = App.ActiveDocument.addObject("Part::Feature","Union")
                newobj.Shape = u
            addList.append(newobj)
            deleteList.extend(objectslist)
            return newobj
        return None

    def makeSketchFace(obj):
        """Makes a Draft face out of a sketch"""
        newobj = makeWire(obj.Shape,closed=True)
        if newobj:
            newobj.Base = obj
            obj.ViewObject.Visibility = False
            addList.append(newobj)
            return newobj
        return None

    def makeFaces(objectslist):
        """make a face from every closed wire in the list"""
        result = False
        for o in objectslist:
            for w in o.Shape.Wires:
                try:
                    f = Part.Face(w)
                except Part.OCCError:
                    pass
                else:
                    newobj = App.ActiveDocument.addObject("Part::Feature","Face")
                    newobj.Shape = f
                    addList.append(newobj)
                    result = True
                    if not o in deleteList:
                        deleteList.append(o)
        return result

    def makeWires(objectslist):
        """joins edges in the given objects list into wires"""
        edges = []
        for o in objectslist:
            for e in o.Shape.Edges:
                edges.append(e)
        try:
            nedges = Part.__sortEdges__(edges[:])
            # for e in nedges: print("debug: ",e.Curve,e.Vertexes[0].Point,e.Vertexes[-1].Point)
            w = Part.Wire(nedges)
        except Part.OCCError:
            return None
        else:
            if len(w.Edges) == len(edges):
                newobj = App.ActiveDocument.addObject("Part::Feature","Wire")
                newobj.Shape = w
                addList.append(newobj)
                deleteList.extend(objectslist)
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
        elif hasattr(ob,'Shape'):
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

    #print("objects:",objects," edges:",edges," wires:",wires," openwires:",openwires," faces:",faces)
    #print("groups:",groups," curves:",curves," facewires:",facewires, "loneedges:", loneedges)

    if force:
        if force in ["makeCompound","closeGroupWires","makeSolid","closeWire","turnToParts","makeFusion",
                     "makeShell","makeFaces","draftify","joinFaces","makeSketchFace","makeWires","turnToLine"]:
            result = eval(force)(objects)
        else:
            App.Console.PrintMessage(_tr("Upgrade: Unknown force method:")+" "+force)
            result = None

    else:

        # applying transformations automatically

        result = None

        # if we have a group: turn each closed wire inside into a face
        if groups:
            result = closeGroupWires(groups)
            if result:
                App.Console.PrintMessage(_tr("Found groups: closing each open object inside")+"\n")

        # if we have meshes, we try to turn them into shapes
        elif meshes:
            result = turnToParts(meshes)
            if result:
                App.Console.PrintMessage(_tr("Found mesh(es): turning into Part shapes")+"\n")

        # we have only faces here, no lone edges
        elif faces and (len(wires) + len(openwires) == len(facewires)):

            # we have one shell: we try to make a solid
            if (len(objects) == 1) and (len(faces) > 3):
                result = makeSolid(objects[0])
                if result:
                    App.Console.PrintMessage(_tr("Found 1 solidifiable object: solidifying it")+"\n")

            # we have exactly 2 objects: we fuse them
            elif (len(objects) == 2) and (not curves):
                result = makeFusion(objects[0],objects[1])
                if result:
                    App.Console.PrintMessage(_tr("Found 2 objects: fusing them")+"\n")

            # we have many separate faces: we try to make a shell
            elif (len(objects) > 2) and (len(faces) > 1) and (not loneedges):
                result = makeShell(objects)
                if result:
                    App.Console.PrintMessage(_tr("Found several objects: creating a shell")+"\n")

            # we have faces: we try to join them if they are coplanar
            elif len(faces) > 1:
                result = joinFaces(objects)
                if result:
                    App.Console.PrintMessage(_tr("Found several coplanar objects or faces: creating one face")+"\n")

            # only one object: if not parametric, we "draftify" it
            elif len(objects) == 1 and (not objects[0].isDerivedFrom("Part::Part2DObjectPython")):
                result = draftify(objects[0])
                if result:
                    App.Console.PrintMessage(_tr("Found 1 non-parametric objects: draftifying it")+"\n")

        # we have only one object that contains one edge
        elif (not faces) and (len(objects) == 1) and (len(edges) == 1):
            # we have a closed sketch: Extract a face
            if objects[0].isDerivedFrom("Sketcher::SketchObject") and (len(edges[0].Vertexes) == 1):
                result = makeSketchFace(objects[0])
                if result:
                    App.Console.PrintMessage(_tr("Found 1 closed sketch object: creating a face from it")+"\n")
            else:
                # turn to Draft line
                e = objects[0].Shape.Edges[0]
                if isinstance(e.Curve,(Part.LineSegment,Part.Line)):
                    result = turnToLine(objects[0])
                    if result:
                        App.Console.PrintMessage(_tr("Found 1 linear object: converting to line")+"\n")

        # we have only closed wires, no faces
        elif wires and (not faces) and (not openwires):

            # we have a sketch: Extract a face
            if (len(objects) == 1) and objects[0].isDerivedFrom("Sketcher::SketchObject"):
                result = makeSketchFace(objects[0])
                if result:
                    App.Console.PrintMessage(_tr("Found 1 closed sketch object: creating a face from it")+"\n")

            # only closed wires
            else:
                result = makeFaces(objects)
                if result:
                    App.Console.PrintMessage(_tr("Found closed wires: creating faces")+"\n")

        # special case, we have only one open wire. We close it, unless it has only 1 edge!"
        elif (len(openwires) == 1) and (not faces) and (not loneedges):
            result = closeWire(objects[0])
            if result:
                App.Console.PrintMessage(_tr("Found 1 open wire: closing it")+"\n")

        # only open wires and edges: we try to join their edges
        elif openwires and (not wires) and (not faces):
            result = makeWires(objects)
            if result:
                App.Console.PrintMessage(_tr("Found several open wires: joining them")+"\n")

        # only loneedges: we try to join them
        elif loneedges and (not facewires):
            result = makeWires(objects)
            if result:
                App.Console.PrintMessage(_tr("Found several edges: wiring them")+"\n")

        # all other cases, if more than 1 object, make a compound
        elif (len(objects) > 1):
            result = makeCompound(objects)
            if result:
                App.Console.PrintMessage(_tr("Found several non-treatable objects: creating compound")+"\n")

        # no result has been obtained
        if not result:
            App.Console.PrintMessage(_tr("Unable to upgrade these objects.")+"\n")

    if delete:
        names = []
        for o in deleteList:
            names.append(o.Name)
        deleteList = []
        for n in names:
            App.ActiveDocument.removeObject(n)
    gui_utils.select(addList)
    return [addList,deleteList]
