"""This module provides the object code for Draft Bezcurve.
"""
## @package bezcurve
# \ingroup DRAFT
# \brief This module provides the object code for Draft Bezcurve object.

# ***************************************************************************
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD as App
import Draft # used for _ViewProviderWire
from .baseobject import _DraftObject
from draftutils.utils import get_param, translate
from draftutils.gui_utils import format_object, select

if App.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
    gui = True
    #from DraftGui import translate
else:
    # def QT_TRANSLATE_NOOP(ctxt,txt):
    #     return txt
    #print("FreeCAD Gui not present. Draft module will have some features disabled.")
    gui = False


def make_bez_curve(pointslist,closed=False,placement=None,face=None,support=None,degree=None):
    """makeBezCurve(pointslist,[closed],[placement]): Creates a Bezier Curve object
    from the given list of vectors.   Instead of a pointslist, you can also pass a Part Wire."""
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    if not isinstance(pointslist,list):
        nlist = []
        for v in pointslist.Vertexes:
            nlist.append(v.Point)
        pointslist = nlist
    if placement: typecheck([(placement,App.Placement)], "makeBezCurve")
    if len(pointslist) == 2: fname = "Line"
    else: fname = "BezCurve"
    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython",fname)
    _BezCurve(obj)
    obj.Points = pointslist
    if degree:
        obj.Degree = degree
    else:
        import Part
        obj.Degree = min((len(pointslist)-(1 * (not closed))),
                         Part.BezierCurve().MaxDegree)
    obj.Closed = closed
    obj.Support = support
    if face != None:
        obj.MakeFace = face
    obj.Proxy.resetcontinuity(obj)
    if placement: obj.Placement = placement
    if gui:
        Draft._ViewProviderWire(obj.ViewObject)
#        if not face: obj.ViewObject.DisplayMode = "Wireframe"
#        obj.ViewObject.DisplayMode = "Wireframe"
        format_object(obj)
        select(obj)

    return obj


class _BezCurve(_DraftObject):
    """The BezCurve object"""

    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"BezCurve")
        obj.addProperty("App::PropertyVectorList","Points","Draft",QT_TRANSLATE_NOOP("App::Property","The points of the Bezier curve"))
        obj.addProperty("App::PropertyInteger","Degree","Draft",QT_TRANSLATE_NOOP("App::Property","The degree of the Bezier function"))
        obj.addProperty("App::PropertyIntegerList","Continuity","Draft",QT_TRANSLATE_NOOP("App::Property","Continuity"))
        obj.addProperty("App::PropertyBool","Closed","Draft",QT_TRANSLATE_NOOP("App::Property","If the Bezier curve should be closed or not"))
        obj.addProperty("App::PropertyBool","MakeFace","Draft",QT_TRANSLATE_NOOP("App::Property","Create a face if this curve is closed"))
        obj.addProperty("App::PropertyLength","Length","Draft",QT_TRANSLATE_NOOP("App::Property","The length of this object"))
        obj.addProperty("App::PropertyArea","Area","Draft",QT_TRANSLATE_NOOP("App::Property","The area of this object"))
        obj.MakeFace = get_param("fillmode",True)
        obj.Closed = False
        obj.Degree = 3
        obj.Continuity = []
        #obj.setEditorMode("Degree",2)#hide
        obj.setEditorMode("Continuity",1)#ro

    def execute(self, fp):
        self.createGeometry(fp)
        fp.positionBySupport()

    def _segpoleslst(self,fp):
        """split the points into segments"""
        if not fp.Closed and len(fp.Points) >= 2: #allow lower degree segment
            poles=fp.Points[1:]
        elif fp.Closed and len(fp.Points) >= fp.Degree: #drawable
            #poles=fp.Points[1:(fp.Degree*(len(fp.Points)//fp.Degree))]+fp.Points[0:1]
            poles=fp.Points[1:]+fp.Points[0:1]
        else:
            poles=[]
        return [poles[x:x+fp.Degree] for x in \
            range(0, len(poles), (fp.Degree or 1))]

    def resetcontinuity(self,fp):
        fp.Continuity = [0]*(len(self._segpoleslst(fp))-1+1*fp.Closed)
        #nump= len(fp.Points)-1+fp.Closed*1
        #numsegments = (nump // fp.Degree) + 1 * (nump % fp.Degree > 0) -1
        #fp.Continuity = [0]*numsegments

    def onChanged(self, fp, prop):
        if prop == 'Closed': # if remove the last entry when curve gets opened
            oldlen = len(fp.Continuity)
            newlen = (len(self._segpoleslst(fp))-1+1*fp.Closed)
            if oldlen > newlen:
                fp.Continuity = fp.Continuity[:newlen]
            if oldlen < newlen:
                fp.Continuity = fp.Continuity + [0]*(newlen-oldlen)
        if hasattr(fp,'Closed') and fp.Closed and prop in  ['Points','Degree','Closed'] and\
                len(fp.Points) % fp.Degree: # the curve editing tools can't handle extra points
            fp.Points=fp.Points[:(fp.Degree*(len(fp.Points)//fp.Degree))] #for closed curves
        if prop in ["Degree"] and fp.Degree >= 1: #reset Continuity
            self.resetcontinuity(fp)
        if prop in ["Points","Degree","Continuity","Closed"]:
            self.createGeometry(fp)

    def createGeometry(self,fp):
        import Part
        plm = fp.Placement
        if fp.Points:
            startpoint=fp.Points[0]
            edges = []
            for segpoles in self._segpoleslst(fp):
#                if len(segpoles) == fp.Degree # would skip additional poles
                 c = Part.BezierCurve() #last segment may have lower degree
                 c.increase(len(segpoles))
                 c.setPoles([startpoint]+segpoles)
                 edges.append(Part.Edge(c))
                 startpoint = segpoles[-1]
            w = Part.Wire(edges)
            if fp.Closed and w.isClosed():
                try:
                    if hasattr(fp,"MakeFace"):
                        if fp.MakeFace:
                            w = Part.Face(w)
                    else:
                        w = Part.Face(w)
                except Part.OCCError:
                    pass
            fp.Shape = w
            if hasattr(fp,"Area") and hasattr(w,"Area"):
                fp.Area = w.Area
            if hasattr(fp,"Length") and hasattr(w,"Length"):
                fp.Length = w.Length            
        fp.Placement = plm

    @classmethod
    def symmetricpoles(cls,knot, p1, p2):
        """make two poles symmetric respective to the knot"""
        p1h=App.Vector(p1)
        p2h=App.Vector(p2)
        p1h.multiply(0.5)
        p2h.multiply(0.5)
        return ( knot+p1h-p2h , knot+p2h-p1h)

    @classmethod
    def tangentpoles(cls,knot, p1, p2,allowsameside=False):
        """make two poles have the same tangent at knot"""
        p12n=p2.sub(p1)
        p12n.normalize()
        p1k=knot-p1
        p2k=knot-p2
        p1k_= App.Vector(p12n)
        kon12=(p1k*p12n)
        if allowsameside or not (kon12 < 0 or p2k*p12n > 0):# instead of moving
            p1k_.multiply(kon12)
            pk_k=knot-p1-p1k_
            return (p1+pk_k,p2+pk_k)
        else:
            return cls.symmetricpoles(knot, p1, p2)

    @staticmethod
    def modifysymmetricpole(knot,p1):
        """calculate the coordinates of the opposite pole
        of a symmetric knot"""
        return knot+knot-p1

    @staticmethod
    def modifytangentpole(knot,p1,oldp2):
        """calculate the coordinates of the opposite pole
        of a tangent knot"""
        pn=knot-p1
        pn.normalize()
        pn.multiply((knot-oldp2).Length)
        return pn+knot

    @staticmethod
    def smoothBezPoint(obj, point, style='Symmetric'):
        "called when changing the continuity of a knot"
        style2cont = {'Sharp':0,'Tangent':1,'Symmetric':2}
        if point is None:
            return
        if not (Draft.getType(obj) == "BezCurve"):
            return
        pts = obj.Points
        deg = obj.Degree
        if deg < 2:
            return
        if point % deg != 0: #point is a pole
            if deg >=3: #allow to select poles
                if (point % deg == 1) and (point > 2 or obj.Closed): #right pole
                    knot = point -1
                    keepp = point
                    changep = point -2
                elif point < len(pts) -3 and point % deg == deg -1: #left pole
                    knot = point +1
                    keepp = point
                    changep = point +2
                elif point == len(pts)-1 and obj.Closed: #last pole
                    # if the curve is closed the last pole has the last
                    # index in the points lists
                    knot = 0
                    keepp = point
                    changep = 1
                else:
                    App.Console.PrintWarning(translate("draft", 
                                                        "Can't change Knot belonging to pole %d"
                                                        %point)
                                                        + "\n")
                    return
                if knot:
                    if style == 'Tangent':
                        pts[changep] = obj.Proxy.modifytangentpole(pts[knot],
                            pts[keepp],pts[changep])
                    elif style == 'Symmetric':
                        pts[changep] = obj.Proxy.modifysymmetricpole(pts[knot],
                            pts[keepp])
                    else: #sharp
                        pass #
            else:
                App.Console.PrintWarning(translate("draft", 
                                                       "Selection is not a Knot")
                                                       + "\n")
                return
        else: #point is a knot
            if style == 'Sharp':
                if obj.Closed and point == len(pts)-1:
                    knot = 0
                else:
                    knot = point
            elif style == 'Tangent' and point > 0 and point < len(pts)-1:
                prev, next = obj.Proxy.tangentpoles(pts[point], pts[point-1], pts[point+1])
                pts[point-1] = prev
                pts[point+1] = next
                knot = point #index for continuity
            elif style == 'Symmetric' and point > 0 and point < len(pts)-1:
                prev, next = obj.Proxy.symmetricpoles(pts[point], pts[point-1], pts[point+1])
                pts[point-1] = prev
                pts[point+1] = next
                knot = point #index for continuity
            elif obj.Closed and (style == 'Symmetric' or style == 'Tangent'):
                if style == 'Tangent':
                    pts[1], pts[-1] = obj.Proxy.tangentpoles(pts[0], pts[1], pts[-1])
                elif style == 'Symmetric':
                    pts[1], pts[-1] = obj.Proxy.symmetricpoles(pts[0], pts[1], pts[-1])
                knot = 0
            else:
                App.Console.PrintWarning(translate("draft",
                                                       "Endpoint of BezCurve can't be smoothed")
                                                       + "\n")
                return
        segment = knot // deg #segment index
        newcont = obj.Continuity[:] #don't edit a property inplace !!!
        if not obj.Closed and (len(obj.Continuity) == segment -1 or
            segment == 0) :
            pass # open curve
        elif (len(obj.Continuity) >= segment or obj.Closed and segment == 0 and
                len(obj.Continuity) >1):
            newcont[segment-1] = style2cont.get(style)
        else: #should not happen
            App.Console.PrintWarning('Continuity indexing error:'
                                         + 'point:%d deg:%d len(cont):%d' % (knot,deg,
                                         len(obj.Continuity)))
        obj.Points = pts
        obj.Continuity = newcont

    @staticmethod
    def recomputePointsBezier(obj,pts,idx,v,degree):# ,moveTrackers=True):
        """
        (object, Points as list, nodeIndex as Int, App.Vector of new point)
        return the new point list, applying the App.Vector to the given index point
        """
        editPnt = v
        # DNC: allows to close the curve by placing ends close to each other
        tol = 0.001
        if ( ( idx == 0 ) and ( (editPnt - pts[-1]).Length < tol) ) or (
                idx == len(pts) - 1 ) and ( (editPnt - pts[0]).Length < tol):
            obj.Closed = True
        # DNC: fix error message if edited point coincides with one of the existing points
        #if ( editPnt in pts ) == False:
        knot = None
        ispole = idx % degree

        if ispole == 0: #knot
            if degree >= 3:
                if idx >= 1: #move left pole
                    knotidx = idx if idx < len(pts) else 0
                    pts[idx-1] = pts[idx-1] + editPnt - pts[knotidx]
                    #if moveTrackers:
                    #    self.trackers[obj.Name][idx-1].set(pts[idx-1])
                if idx < len(pts)-1: #move right pole
                    pts[idx+1] = pts[idx+1] + editPnt - pts[idx]
                    #if moveTrackers:
                    #    self.trackers[obj.Name][idx+1].set(pts[idx+1])
                if idx == 0 and obj.Closed: # move last pole
                    pts[-1] = pts [-1] + editPnt -pts[idx]
                    #if moveTrackers:
                    #    self.trackers[obj.Name][-1].set(pts[-1])

        elif ispole == 1 and (idx >=2 or obj.Closed): #right pole
            knot = idx -1
            changep = idx -2 # -1 in case of closed curve

        elif ispole == degree-1 and idx <= len(pts)-3: #left pole
            knot = idx +1
            changep = idx +2

        elif ispole == degree-1 and obj.Closed and idx == len(pts)-1: #last pole
            knot = 0
            changep = 1

        if knot is not None: # we need to modify the opposite pole
            segment = int(knot / degree) -1
            cont = obj.Continuity[segment] if len(obj.Continuity) > segment else 0
            if cont == 1: #tangent
                pts[changep] = obj.Proxy.modifytangentpole(pts[knot],
                    editPnt,pts[changep])
                #if moveTrackers:
                #    self.trackers[obj.Name][changep].set(pts[changep])
            elif cont == 2: #symmetric
                pts[changep] = obj.Proxy.modifysymmetricpole(pts[knot],editPnt)
                #if moveTrackers:
                #    self.trackers[obj.Name][changep].set(pts[changep])
        pts[idx]=v

        return pts #returns the list of new points, taking into account knot continuity
