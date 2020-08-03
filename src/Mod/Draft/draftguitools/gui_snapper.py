# ***************************************************************************
# *   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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
"""Provides the Snapper class to define the snapping tools and modes.

This module provides tools to handle point snapping and
everything that goes with it (toolbar buttons, cursor icons, etc.).
It also creates the Draft grid, which is actually a tracker
defined by `gui_trackers.gridTracker`.
"""
## @package gui_snapper
#  \ingroup draftguitools
#  \brief Provides the Snapper class to define the snapping tools and modes.
#
#  This module provides tools to handle point snapping and
#  everything that goes with it (toolbar buttons, cursor icons, etc.).

## \addtogroup draftguitools
# @{
import collections as coll
import inspect
import itertools
import math

import pivy.coin as coin
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui

import Draft
import DraftGeomUtils
from draftutils.init_tools import get_draft_snap_commands
from draftutils.messages import _msg, _wrn
import DraftVecUtils
import FreeCAD as App
import FreeCADGui as Gui
import Part
from . import gui_trackers as trackers

__title__ = "FreeCAD Draft Snap tools"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecadweb.org"


class Snapper:
    """Classes to manage snapping in Draft and Arch.

    The Snapper objects contains all the functionality used by draft
    and arch module to manage object snapping. It is responsible for
    finding snap points and displaying snap markers. Usually You
    only need to invoke it's snap() function, all the rest is taken
    care of.

    3 functions are useful for the scriptwriter: snap(), constrain()
    or getPoint() which is an all-in-one combo.

    The individual snapToXXX() functions return a snap definition in
    the form [real_point,marker_type,visual_point], and are not
    meant to be used directly, they are all called when necessary by
    the general snap() function.

    The Snapper lives inside FreeCADGui once the Draft module has been
    loaded.

    """

    def __init__(self):
        self.activeview = None
        self.lastObj = [None, None]
        self.maxEdges = 0
        self.radius = 0
        self.constraintAxis = None
        self.basepoint = None
        self.affinity = None
        self.mask = None
        self.cursorMode = None
        if Draft.get_param("maxSnap", 0):
            self.maxEdges = Draft.get_param("maxSnapEdges", 0)
        self.snapStyle = Draft.get_param("snapStyle", 0)

        # we still have no 3D view when the draft module initializes
        self.tracker = None
        self.extLine = None
        self.grid = None
        self.constrainLine = None
        self.trackLine = None
        self.extLine2 = None
        self.radiusTracker = None
        self.dim1 = None
        self.dim2 = None
        self.snapInfo = None
        self.lastSnappedObject = None
        self.active = True
        self.forceGridOff = False
        self.lastExtensions = []
        # the trackers are stored in lists because there can be several views,
        # each with its own set
        # view, grid, snap, extline, radius, dim1, dim2, trackLine,
        # extline2, crosstrackers
        self.trackers = [[], [], [], [], [], [], [], [], [], []]
        self.polarAngles = [90, 45]
        self.selectMode = False
        self.holdTracker = None
        self.holdPoints = []
        self.running = False
        self.callbackClick = None
        self.callbackMove = None
        self.snapObjectIndex = 0

        # snap keys, it's important that they are in this order for
        # saving in preferences and for properly restoring the toolbar
        self.snaps = ['Lock',           # 0 
                      'Near',           # 1 former "passive" snap
                      'Extension',      # 2
                      'Parallel',       # 3
                      'Grid',           # 4
                      "Endpoint",       # 5
                      'Midpoint',       # 6
                      'Perpendicular',  # 7
                      'Angle',          # 8
                      'Center',         # 9
                      'Ortho',          # 10
                      'Intersection',   # 11
                      'Special',        # 12
                      'Dimensions',     # 13
                      'WorkingPlane'    # 14
                     ]

        self.init_active_snaps()

        # the snapmarker has "dot","circle" and "square" available styles
        if self.snapStyle:
            self.mk = coll.OrderedDict([('passive',       'empty'),
                                        ('extension',     'empty'),
                                        ('parallel',      'empty'),
                                        ('grid',          'quad'),
                                        ('endpoint',      'quad'),
                                        ('midpoint',      'quad'),
                                        ('perpendicular', 'quad'),
                                        ('angle',         'quad'),
                                        ('center',        'quad'),
                                        ('ortho',         'quad'),
                                        ('intersection',  'quad'),
                                        ('special',       'quad')])
        else:
            self.mk = coll.OrderedDict([('passive',       'circle'),
                                        ('extension',     'circle'),
                                        ('parallel',      'circle'),
                                        ('grid',          'circle'),
                                        ('endpoint',      'dot'),
                                        ('midpoint',      'square'),
                                        ('perpendicular', 'dot'),
                                        ('angle',         'square'),
                                        ('center',        'dot'),
                                        ('ortho',         'dot'),
                                        ('intersection',  'dot'),
                                        ('special',       'dot')])

        self.cursors = \
            coll.OrderedDict([('passive',       ':/icons/Snap_Near.svg'),
                              ('extension',     ':/icons/Snap_Extension.svg'),
                              ('parallel',      ':/icons/Snap_Parallel.svg'),
                              ('grid',          ':/icons/Snap_Grid.svg'),
                              ('endpoint',      ':/icons/Snap_Endpoint.svg'),
                              ('midpoint',      ':/icons/Snap_Midpoint.svg'),
                              ('perpendicular', ':/icons/Snap_Perpendicular.svg'),
                              ('angle',         ':/icons/Snap_Angle.svg'),
                              ('center',        ':/icons/Snap_Center.svg'),
                              ('ortho',         ':/icons/Snap_Ortho.svg'),
                              ('intersection',  ':/icons/Snap_Intersection.svg'),
                              ('special',       ':/icons/Snap_Special.svg')])

    def init_active_snaps(self):
        """
        set self.active_snaps according to user prefs
        """
        self.active_snaps = []
        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        snap_modes = param.GetString("snapModes")
        i = 0
        for snap in snap_modes:
            if bool(int(snap)):
                self.active_snaps.append(self.snaps[i])
            i += 1

    def cstr(self, lastpoint, constrain, point):
        """Return constraints if needed."""
        if constrain or self.mask:
            fpt = self.constrain(point, lastpoint)
        else:
            self.unconstrain()
            fpt = point
        if self.radiusTracker:
            self.radiusTracker.update(fpt)
        return fpt

    def snap(self, screenpos,
             lastpoint=None, active=True,
             constrain=False, no_tracker=False):
        """Return a snapped point from the given (x, y) screen position.

        snap(screenpos,lastpoint=None,active=True,constrain=False,
        noTracker=False): returns a snapped point from the given
        (x,y) screenpos (the position of the mouse cursor), active is to
        activate active point snapping or not (passive),
        lastpoint is an optional other point used to draw an
        imaginary segment and get additional snap locations. Constrain can
        be True to constrain the point against the closest working plane axis.
        Screenpos can be a list, a tuple or a coin.SbVec2s object.
        If noTracker is True, the tracking line is not displayed.
        """
        if self.running:
            # do not allow concurrent runs
            return None

        self.running = True

        global Part, DraftGeomUtils
        import Part, DraftGeomUtils

        self.spoint = None

        if not hasattr(self, "toolbar"):
            self.make_snap_toolbar()
        mw = Gui.getMainWindow()
        bt = mw.findChild(QtGui.QToolBar,"Draft Snap")
        if not bt:
            mw.addToolBar(self.toolbar)
        else:
            if Draft.getParam("showSnapBar", True):
                bt.show()

        self.snapInfo = None

        # Type conversion if needed
        if isinstance(screenpos, list):
            screenpos = tuple(screenpos)
        elif isinstance(screenpos, coin.SbVec2s):
            screenpos = tuple(screenpos.getValue())
        elif not isinstance(screenpos, tuple):
            _wrn("Snap needs valid screen position (list, tuple or sbvec2s)")
            self.running = False
            return None

        # Setup trackers if needed
        self.set_trackers()

        # Show the grid if it's off (new view, for ex)
        if self.grid and Draft.getParam("grid", True):
            self.grid.on()

        # Get current snap radius
        self.radius = self.get_screen_distance(Draft.getParam("snapRange", 8),
                                         screenpos)
        if self.radiusTracker:
            self.radiusTracker.update(self.radius)
            self.radiusTracker.off()

        # Activate snap
        old_active = False
        if Draft.getParam("alwaysSnap", True):
            old_active = active
            active = True
        if not self.active:
            active = False

        self.set_cursor('passive')
        if self.tracker:
            self.tracker.off()
        if self.extLine2:
            self.extLine2.off()
        if self.extLine:
            self.extLine.off()
        if self.trackLine:
            self.trackLine.off()
        if self.dim1:
            self.dim1.off()
        if self.dim2:
            self.dim2.off()

        point = self.project_to_working_plane_apparent(screenpos[0], screenpos[1])

        # Set up a track line if we got a last point
        if lastpoint and self.trackLine:
            self.trackLine.p1(lastpoint)

        # Check if parallel to one of the edges of the last objects
        # or to a polar direction
        eline = None
        if active:
            point, eline = self.snap_to_polar(point, lastpoint)
            point, eline = self.snap_to_extensions(point, lastpoint,
                                                 constrain, eline)

        _view = Draft.get3DView()
        objects_under_cursor = _view.getObjectsInfo((screenpos[0], screenpos[1]))
        if objects_under_cursor:
            if self.snapObjectIndex >= len(objects_under_cursor):
                self.snapObjectIndex = 0
            self.snapInfo = objects_under_cursor[self.snapObjectIndex]

        if self.snapInfo and "Component" in self.snapInfo:
            return self.snap_to_object(lastpoint, active, constrain,
                                     eline, point, old_active)

        # Nothing has been snapped.
        # Check for grid snap and ext crossings
        if active:
            epoint = self.snap_to_cross_extensions(point)
            if epoint:
                point = epoint
            else:
                point = self.snap_to_grid(point)
        fp = self.cstr(lastpoint, constrain, point)
        if self.trackLine and lastpoint and (not no_tracker):
            self.trackLine.p2(fp)
            self.trackLine.on()
        # Set the arch point tracking
        if lastpoint:
            self.set_arc_dimensions(lastpoint, fp)

        self.spoint = fp
        self.running = False
        return fp

    def cycle_snap_object(self):
        """Increse the index of the snap object by one."""
        self.snapObjectIndex = self.snapObjectIndex + 1

    def snap_to_object(self, lastpoint, active, constrain,
                     eline, point, old_active):
        """Snap to an object."""
        parent = self.snapInfo.get('ParentObject', None)
        if parent:
            subname = self.snapInfo['SubName']
            obj = parent.getSubObject(subname, retType=1)
        else:
            obj = App.ActiveDocument.getObject(self.snapInfo['Object'])
            parent = obj
            subname = self.snapInfo['Component']
        if not obj:
            self.spoint = self.cstr(point)
            self.running = False
            return self.spoint

        snaps = []
        self.lastSnappedObject = obj

        if hasattr(obj.ViewObject, "Selectable"):
            if not obj.ViewObject.Selectable:
                self.spoint = self.cstr(lastpoint, constrain, point)
                self.running = False
                return self.spoint

        if not active:
            # Passive snapping
            snaps = [self.snap_to_vertex(self.snapInfo)]
        else:
            # First stick to the snapped object
            s = self.snap_to_vertex(self.snapInfo)
            if s:
                point = s[0]
                snaps = [s]

            # Active snapping
            comp = self.snapInfo['Component']

            shape = Part.getShape(parent, subname,
                                  needSubElement=True,
                                  noElementMap=True)

            if not shape.isNull():
                snaps.extend(self.snap_to_specials(obj, lastpoint, eline))

                if Draft.get_type(obj) == "Polygon":
                    # Special snapping for polygons: add the center
                    snaps.extend(self.snap_to_polygon(obj))

                if (not self.maxEdges) or (len(shape.Edges) <= self.maxEdges):
                    if "Edge" in comp:
                        # we are snapping to an edge
                        edge = None
                        if shape.ShapeType == "Edge":
                            edge = shape
                        else:
                            en = int(comp[4:])-1
                            if len(shape.Edges) > en:
                                edge = shape.Edges[en]
                        if edge:
                            snaps.extend(self.snap_to_endpoints(edge))
                            snaps.extend(self.snap_to_midpoint(edge))
                            snaps.extend(self.snap_to_perpendicular(edge, lastpoint))
                            snaps.extend(self.snap_to_intersection(edge))
                            snaps.extend(self.snap_to_edge_lines(edge, eline))

                            et = DraftGeomUtils.geomType(edge)
                            if et == "Circle":
                                # the edge is an arc, we have extra options
                                snaps.extend(self.snap_to_angles(edge))
                                snaps.extend(self.snap_to_center(edge))
                            elif et == "Ellipse":
                                # extra ellipse options
                                snaps.extend(self.snap_to_center(edge))
                    elif "Face" in comp:
                        en = int(comp[4:])-1
                        if len(shape.Faces) > en:
                            face = shape.Faces[en]
                            snaps.extend(self.snap_to_face(face))
                    elif "Vertex" in comp:
                        # directly snapped to a vertex
                        snaps.append(self.snap_to_vertex(self.snapInfo, active=True))
                    elif comp == '':
                        # workaround for the new view provider
                        snaps.append(self.snap_to_vertex(self.snapInfo, active=True))
                    else:
                        # all other cases (face, etc...) default to passive snap
                        snaps = [self.snap_to_vertex(self.snapInfo)]

            elif Draft.get_type(obj) == "Dimension":
                # for dimensions we snap to their 2 points:
                snaps.extend(self.snap_to_dimension(obj))

            elif Draft.get_type(obj) == "Axis":
                for edge in obj.Shape.Edges:
                    snaps.extend(self.snap_to_endpoints(edge))
                    snaps.extend(self.snap_to_intersection(edge))

            elif Draft.get_type(obj) == "Mesh":
                # for meshes we only snap to vertices
                snaps.extend(self.snap_to_endpoints(obj.Mesh))

            elif Draft.get_type(obj) == "Points":
                # for points we only snap to points
                snaps.extend(self.snap_to_endpoints(obj.Points))

            elif Draft.get_type(obj) in ("WorkingPlaneProxy", "BuildingPart"):
                # snap to the center of WPProxies and BuildingParts
                snaps.append([obj.Placement.Base, 'endpoint',
                              self.project_to_working_plane_normal(obj.Placement.Base)])

            elif Draft.get_type(obj) == "SectionPlane":
                # snap to corners of section planes
                snaps.extend(self.snap_to_endpoints(obj.Shape))

        # updating last objects list
        if not self.lastObj[1]:
            self.lastObj[1] = obj.Name
        elif self.lastObj[1] != obj.Name:
            self.lastObj[0] = self.lastObj[1]
            self.lastObj[1] = obj.Name

        if not snaps:
            self.spoint = self.cstr(lastpoint, constrain, point)
            self.running = False
            if self.trackLine and lastpoint:
                self.trackLine.p2(self.spoint)
                self.trackLine.on()
            return self.spoint

        # calculating the nearest snap point
        shortest = 1000000000000000000
        origin = App.Vector(self.snapInfo['x'],
                            self.snapInfo['y'],
                            self.snapInfo['z'])
        winner = None
        fp = point
        for snap in snaps:
            if (not snap) or (snap[0] is None):
                pass
                # print("debug: Snapper: invalid snap point: ",snaps)
            else:
                delta = snap[0].sub(origin)
                if delta.Length < shortest:
                    shortest = delta.Length
                    winner = snap

        if winner:
            # see if we are out of the max radius, if any
            if self.radius:
                dv = point.sub(winner[2])
                if (dv.Length > self.radius):
                    if (not old_active) and self.is_enabled("Near"):
                        winner = self.snap_to_vertex(self.snapInfo)

            # setting the cursors
            if self.tracker and not self.selectMode:
                self.tracker.setCoords(winner[2])
                self.tracker.setMarker(self.mk[winner[1]])
                self.tracker.on()
            # setting the trackline
            fp = self.cstr(lastpoint, constrain, winner[2])
            if self.trackLine and lastpoint:
                self.trackLine.p2(fp)
                self.trackLine.on()
            # set the cursor
            self.set_cursor(winner[1])

            # set the arch point tracking
            if lastpoint:
                self.set_arc_dimensions(lastpoint, fp)

        # return the final point
        self.spoint = fp
        self.running = False
        return self.spoint

    def project_to_working_plane_normal(self, point):
        """Project the given point on the working plane, if needed."""
        if self.is_enabled("WorkingPlane"):
            if hasattr(App, "DraftWorkingPlane"):
                return App.DraftWorkingPlane.projectPoint(point)
        return point

    def project_to_working_plane_apparent(self, x, y):
        """Return a 3D point, projected on the current working plane."""
        view = Draft.get3DView()
        pt = view.getPoint(x, y)
        if self.mask != "z":
            if hasattr(App,"DraftWorkingPlane"):
                if view.getCameraType() == "Perspective":
                    camera = view.getCameraNode()
                    p = camera.getField("position").getValue()
                    dv = pt.sub(App.Vector(p[0], p[1], p[2]))
                else:
                    dv = view.getViewDirection()
                return App.DraftWorkingPlane.projectPoint(pt, dv)
        return pt

    def snap_to_dimension(self, obj):
        snaps = []
        if obj.ViewObject:
            if hasattr(obj.ViewObject.Proxy, "p2") and hasattr(obj.ViewObject.Proxy, "p3"):
                snaps.append([obj.ViewObject.Proxy.p2, 'endpoint', self.project_to_working_plane_normal(obj.ViewObject.Proxy.p2)])
                snaps.append([obj.ViewObject.Proxy.p3, 'endpoint', self.project_to_working_plane_normal(obj.ViewObject.Proxy.p3)])
        return snaps

    def snap_to_extensions(self, point, last, constrain, eline):
        """Return a point snapped to extension or parallel line.

        The parallel line of the last object, if any.
        """
        tsnap = self.snap_to_hold(point)
        if tsnap:
            if self.tracker and not self.selectMode:
                self.tracker.setCoords(tsnap[2])
                self.tracker.setMarker(self.mk[tsnap[1]])
                self.tracker.on()
            if self.extLine:
                self.extLine.p1(tsnap[0])
                self.extLine.p2(tsnap[2])
                self.extLine.on()
            self.set_cursor(tsnap[1])
            return tsnap[2], eline
        if self.is_enabled("Extension"):
            tsnap = self.snap_to_extension_ortho(last, constrain, eline)
            if tsnap:
                if (tsnap[0].sub(point)).Length < self.radius:
                    if self.tracker and not self.selectMode:
                        self.tracker.setCoords(tsnap[2])
                        self.tracker.setMarker(self.mk[tsnap[1]])
                        self.tracker.on()
                    if self.extLine:
                        self.extLine.p2(tsnap[2])
                        self.extLine.on()
                    self.set_cursor(tsnap[1])
                    return tsnap[2], eline
            else:
                tsnap = self.snap_to_extension_perpendicular(last)
                if tsnap:
                    if (tsnap[0].sub(point)).Length < self.radius:
                        if self.tracker and not self.selectMode:
                            self.tracker.setCoords(tsnap[2])
                            self.tracker.setMarker(self.mk[tsnap[1]])
                            self.tracker.on()
                        if self.extLine:
                            self.extLine.p2(tsnap[2])
                            self.extLine.on()
                        self.set_cursor(tsnap[1])
                        return tsnap[2], eline

        for o in (self.lastObj[1], self.lastObj[0]): 
            if o and (self.is_enabled('Extension') 
                      or self.is_enabled('Parallel')):            
                ob = App.ActiveDocument.getObject(o)
                if not ob:
                    continue
                if not ob.isDerivedFrom("Part::Feature"):
                    continue
                edges = ob.Shape.Edges
                if Draft.get_type(ob) == "Wall":
                    for so in [ob]+ob.Additions:
                        if Draft.get_type(so) == "Wall":
                            if so.Base:
                                edges.extend(so.Base.Shape.Edges)
                                edges.reverse()
                if (not self.maxEdges) or (len(edges) <= self.maxEdges):
                    for e in edges:
                        if DraftGeomUtils.geomType(e) != "Line":
                            continue
                        np = self.get_perpendicular(e,point)
                        if DraftGeomUtils.isPtOnEdge(np,e):
                            continue
                        if (np.sub(point)).Length < self.radius:
                            if self.is_enabled('Extension'):
                                if np != e.Vertexes[0].Point:
                                    p0 = e.Vertexes[0].Point
                                    if self.tracker and not self.selectMode:
                                        self.tracker.setCoords(np)
                                        self.tracker.setMarker(self.mk['extension'])
                                        self.tracker.on()
                                    if self.extLine:
                                        if self.snapStyle:
                                            dv = np.sub(p0)
                                            self.extLine.p1(p0.add(dv.multiply(0.5)))
                                        else:
                                            self.extLine.p1(p0)
                                        self.extLine.p2(np)
                                        self.extLine.on()
                                    self.set_cursor('extension')
                                    ne = Part.LineSegment(p0,np).toShape()
                                    # storing extension line for intersection calculations later
                                    if len(self.lastExtensions) == 0:
                                        self.lastExtensions.append(ne)
                                    elif len(self.lastExtensions) == 1:
                                        if not DraftGeomUtils.areColinear(ne,self.lastExtensions[0]):
                                            self.lastExtensions.append(self.lastExtensions[0])
                                            self.lastExtensions[0] = ne
                                    else:
                                        if (not DraftGeomUtils.areColinear(ne,self.lastExtensions[0])) and \
                                            (not DraftGeomUtils.areColinear(ne,self.lastExtensions[1])):
                                                self.lastExtensions[1] = self.lastExtensions[0]
                                                self.lastExtensions[0] = ne
                                    return np,ne
                        else:
                            if self.is_enabled('Parallel'):
                                if last:
                                    ve = DraftGeomUtils.vec(e)
                                    if not DraftVecUtils.isNull(ve):
                                        de = Part.LineSegment(last,last.add(ve)).toShape()
                                        np = self.get_perpendicular(de,point)
                                        if (np.sub(point)).Length < self.radius:
                                            if self.tracker and not self.selectMode:
                                                self.tracker.setCoords(np)
                                                self.tracker.setMarker(self.mk['parallel'])
                                                self.tracker.on()
                                            self.set_cursor('parallel')
                                            return np,de
        return point,eline

    def snap_to_cross_extensions(self, point):
        """Snap to the intersection of the last 2 extension lines."""
        if self.is_enabled('Extension'):
            if len(self.lastExtensions) == 2:
                np = DraftGeomUtils.findIntersection(self.lastExtensions[0], self.lastExtensions[1], True, True)
                if np:
                    for p in np:
                        dv = point.sub(p)
                        if (self.radius == 0) or (dv.Length <= self.radius):
                            if self.tracker and not self.selectMode:
                                self.tracker.setCoords(p)
                                self.tracker.setMarker(self.mk['intersection'])
                                self.tracker.on()
                            self.set_cursor('intersection')
                            if self.extLine and self.extLine2:
                                if DraftVecUtils.equals(self.extLine.p1(), self.lastExtensions[0].Vertexes[0].Point):
                                    p0 = self.lastExtensions[1].Vertexes[0].Point
                                else:
                                    p0 = self.lastExtensions[0].Vertexes[0].Point
                                if self.snapStyle:
                                    dv = p.sub(p0)
                                    self.extLine2.p1(p0.add(dv.multiply(0.5)))
                                else:
                                    self.extLine2.p1(p0)
                                self.extLine2.p2(p)
                                self.extLine.p2(p)
                                self.extLine2.on()
                            return p
        return None

    def snap_to_polar(self,point,last):
        """Snap to polar lines from the given point."""
        if self.is_enabled('Ortho') and (not self.mask):
            if last:
                vecs = []
                if hasattr(App,"DraftWorkingPlane"):
                    ax = [App.DraftWorkingPlane.u,
                          App.DraftWorkingPlane.v,
                          App.DraftWorkingPlane.axis]
                else:
                    ax = [App.Vector(1, 0, 0),
                          App.Vector(0, 1, 0),
                          App.Vector(0, 0, 1)]
                for a in self.polarAngles:
                    if a == 90:
                        vecs.extend([ax[0], ax[0].negative()])
                        vecs.extend([ax[1], ax[1].negative()])
                    else:
                        v = DraftVecUtils.rotate(ax[0], math.radians(a), ax[2])
                        vecs.extend([v, v.negative()])
                        v = DraftVecUtils.rotate(ax[1], math.radians(a), ax[2])
                        vecs.extend([v, v.negative()])
                for v in vecs:
                    if not DraftVecUtils.isNull(v):
                        try:
                            de = Part.LineSegment(last, last.add(v)).toShape()
                        except Part.OCCError:
                            return point, None
                        np = self.get_perpendicular(de, point)
                        if ((self.radius == 0) and (point.sub(last).getAngle(v) < 0.087)) \
                        or ((np.sub(point)).Length < self.radius):
                            if self.tracker and not self.selectMode:
                                self.tracker.setCoords(np)
                                self.tracker.setMarker(self.mk['parallel'])
                                self.tracker.on()
                                self.set_cursor('ortho')
                            return np,de
        return point, None

    def snap_to_grid(self, point):
        """Return a grid snap point if available."""
        if self.grid:
            if self.grid.Visible:
                if self.is_enabled("Grid"):
                    np = self.grid.getClosestNode(point)
                    if np:
                        dv = point.sub(np)
                        if (self.radius == 0) or (dv.Length <= self.radius):
                            if self.tracker and not self.selectMode:
                                self.tracker.setCoords(np)
                                self.tracker.setMarker(self.mk['grid'])
                                self.tracker.on()
                            self.set_cursor('grid')
                            return np
        return point

    def snap_to_endpoints(self, shape):
        """Return a list of endpoints snap locations."""
        snaps = []
        if self.is_enabled("Endpoint"):
            if hasattr(shape, "Vertexes"):
                for v in shape.Vertexes:
                    snaps.append([v.Point, 'endpoint', self.project_to_working_plane_normal(v.Point)])
            elif hasattr(shape, "Point"):
                snaps.append([shape.Point, 'endpoint', self.project_to_working_plane_normal(shape.Point)])
            elif hasattr(shape, "Points"):
                if len(shape.Points) and hasattr(shape.Points[0], "Vector"):
                    for v in shape.Points:
                        snaps.append([v.Vector, 'endpoint', self.project_to_working_plane_normal(v.Vector)])
                else:
                    for v in shape.Points:
                        snaps.append([v, 'endpoint', self.project_to_working_plane_normal(v)])
        return snaps

    def snap_to_midpoint(self, shape):
        """Return a list of midpoints snap locations."""
        snaps = []
        if self.is_enabled("Midpoint"):
            if isinstance(shape, Part.Edge):
                mp = DraftGeomUtils.findMidpoint(shape)
                if mp:
                    snaps.append([mp, 'midpoint', self.project_to_working_plane_normal(mp)])
        return snaps

    def snap_to_perpendicular(self, shape, last):
        """Return a list of perpendicular snap locations."""
        snaps = []
        if self.is_enabled("Perpendicular"):
            if last:
                if isinstance(shape, Part.Edge):
                    if DraftGeomUtils.geomType(shape) == "Line":
                        np = self.get_perpendicular(shape, last)
                    elif DraftGeomUtils.geomType(shape) == "Circle":
                        dv = last.sub(shape.Curve.Center)
                        dv = DraftVecUtils.scaleTo(dv, shape.Curve.Radius)
                        np = (shape.Curve.Center).add(dv)
                    elif DraftGeomUtils.geomType(shape) == "BSplineCurve":
                        try:
                            pr = shape.Curve.parameter(last)
                            np = shape.Curve.value(pr)
                        except Exception:
                            return snaps
                    else:
                        return snaps
                    snaps.append([np, 'perpendicular', self.project_to_working_plane_normal(np)])
        return snaps

    def snap_to_ortho(self, shape, last, constrain):
        """Return a list of ortho snap locations."""
        snaps = []
        if self.is_enabled("Ortho"):
            if constrain:
                if isinstance(shape, Part.Edge):
                    if last:
                        if DraftGeomUtils.geomType(shape) == "Line":
                            if self.constraintAxis:
                                temp_edge = Part.LineSegment(last, last.add(self.constraintAxis)).toShape()
                                # get the intersection points
                                pt = DraftGeomUtils.findIntersection(temp_edge, shape, True, True)
                                if pt:
                                    for p in pt:
                                        snaps.append([p, 'ortho', self.project_to_working_plane_normal(p)])
        return snaps

    def snap_to_extension_ortho(self, last, constrain, eline):
        """Return an ortho X extension snap location."""
        if self.is_enabled("Extension") and self.is_enabled("Ortho"):
            if constrain and last and self.constraintAxis and self.extLine:
                temp_edge1 = Part.LineSegment(last, last.add(self.constraintAxis)).toShape()
                temp_edge2 = Part.LineSegment(self.extLine.p1(), self.extLine.p2()).toShape()
                # get the intersection points
                pt = DraftGeomUtils.findIntersection(temp_edge1, temp_edge2, True, True)
                if pt:
                    return [pt[0], 'ortho', pt[0]]
            if eline:
                try:
                    temp_edge2 = Part.LineSegment(self.extLine.p1(), self.extLine.p2()).toShape()
                    # get the intersection points
                    pt = DraftGeomUtils.findIntersection(eline, temp_edge2, True, True)
                    if pt:
                        return [pt[0], 'ortho', pt[0]]
                except Exception:
                    return None
        return None

    def snap_to_hold(self, point):
        """Return a snap location that is orthogonal to hold points.

        Or if possible at crossings.
        """
        if not self.holdPoints:
            return None
        if hasattr(App, "DraftWorkingPlane"):
            u = App.DraftWorkingPlane.u
            v = App.DraftWorkingPlane.v
        else:
            u = App.Vector(1, 0, 0)
            v = App.Vector(0, 1, 0)
        if len(self.holdPoints) > 1:
            # first try mid points
            if self.is_enabled("Midpoint"):
                l = list(self.holdPoints)
                for p1, p2 in itertools.combinations(l, 2):
                    p3 = p1.add((p2.sub(p1)).multiply(0.5))
                    if (p3.sub(point)).Length < self.radius:
                        return [p1, 'midpoint', p3]
            # then try int points
            ipoints = []
            l = list(self.holdPoints)
            while len(l) > 1:
                p1 = l.pop()
                for p2 in l:
                    i1 = DraftGeomUtils.findIntersection(p1, p1.add(u), p2, p2.add(v), True, True)
                    if i1:
                        ipoints.append([p1, i1[0]])
                    i2 = DraftGeomUtils.findIntersection(p1, p1.add(v), p2, p2.add(u), True, True)
                    if i2:
                        ipoints.append([p1, i2[0]])
            for p in ipoints:
                if (p[1].sub(point)).Length < self.radius:
                    return [p[0], 'ortho', p[1]]
        # then try to stick to a line
        for p in self.holdPoints:
            d = DraftGeomUtils.findDistance(point, [p, p.add(u)])
            if d:
                if d.Length < self.radius:
                    fp = point.add(d)
                    return [p, 'extension', fp]
            d = DraftGeomUtils.findDistance(point, [p, p.add(v)])
            if d:
                if d.Length < self.radius:
                    fp = point.add(d)
                    return [p, 'extension', fp]
        return None

    def snap_to_extension_perpendicular(self, last):
        """Return a perpendicular X extension snap location."""
        if self.is_enabled("Extension") and self.is_enabled("Perpendicular"):
            if last and self.extLine:
                if self.extLine.p1() != self.extLine.p2():
                    temp_edge = Part.LineSegment(self.extLine.p1(), self.extLine.p2()).toShape()
                    np = self.get_perpendicular(temp_edge, last)
                    return [np, 'perpendicular', np]
        return None

    def snap_to_edge_lines(self, e1, e2):
        """Return a snap at the infinite intersection of the given edges."""
        snaps = []
        if self.is_enabled("Intersection") and self.is_enabled("Extension"):
            if e1 and e2:
                # get the intersection points
                pts = DraftGeomUtils.findIntersection(e1, e2, True, True)
                if pts:
                    for p in pts:
                        snaps.append([p, 'intersection', self.project_to_working_plane_normal(p)])
        return snaps

    def snap_to_angles(self, shape):
        """Return a list of angle snap locations."""
        snaps = []
        if self.is_enabled("Angle"):
            rad = shape.Curve.Radius
            pos = shape.Curve.Center
            for i in (0, 30, 45, 60, 90,
                      120, 135, 150, 180,
                      210, 225, 240, 270,
                      300, 315, 330):
                ang = math.radians(i)
                cur = App.Vector(math.sin(ang) * rad + pos.x,
                                 math.cos(ang) * rad + pos.y,
                                 pos.z)
                snaps.append([cur, 'angle', self.project_to_working_plane_normal(cur)])
        return snaps

    def snap_to_center(self, shape):
        """Return a list of center snap locations."""
        snaps = []
        if self.is_enabled("Center"):
            pos = shape.Curve.Center
            c = self.project_to_working_plane_normal(pos)
            if hasattr(shape.Curve, "Radius"):
                rad = shape.Curve.Radius
                for i in (15, 37.5, 52.5, 75,
                          105, 127.5, 142.5, 165,
                          195, 217.5, 232.5, 255,
                          285, 307.5, 322.5, 345):
                    ang = math.radians(i)
                    cur = App.Vector(math.sin(ang) * rad + pos.x,
                                     math.cos(ang) * rad + pos.y,
                                     pos.z)
                    snaps.append([cur, 'center', c])
            else:
                snaps.append([c, 'center', c])
        return snaps

    def snap_to_face(self, shape):
        """Return a face center snap location."""
        snaps = []
        if self.is_enabled("Center"):
            pos = shape.CenterOfMass
            c = self.project_to_working_plane_normal(pos)
            snaps.append([pos, 'center', c])
        return snaps

    def snap_to_intersection(self, shape):
        """Return a list of intersection snap locations."""
        snaps = []
        if self.is_enabled("Intersection"):
            # get the stored objects to calculate intersections
            if self.lastObj[0]:
                obj = App.ActiveDocument.getObject(self.lastObj[0])
                if obj:
                    if obj.isDerivedFrom("Part::Feature") or (Draft.get_type(obj) == "Axis"):
                        if (not self.maxEdges) or (len(obj.Shape.Edges) <= self.maxEdges):
                            import Part
                            for e in obj.Shape.Edges:
                                # get the intersection points
                                try:
                                    if self.is_enabled("WorkingPlane") and hasattr(e,"Curve") and isinstance(e.Curve,(Part.Line,Part.LineSegment)) and hasattr(shape,"Curve") and isinstance(shape.Curve,(Part.Line,Part.LineSegment)):
                                        # get apparent intersection (lines projected on WP)
                                        p1 = self.project_to_working_plane_normal(e.Vertexes[0].Point)
                                        p2 = self.project_to_working_plane_normal(e.Vertexes[-1].Point)
                                        p3 = self.project_to_working_plane_normal(shape.Vertexes[0].Point)
                                        p4 = self.project_to_working_plane_normal(shape.Vertexes[-1].Point)
                                        pt = DraftGeomUtils.findIntersection(p1, p2, p3, p4, True, True)
                                    else:
                                        pt = DraftGeomUtils.findIntersection(e, shape)
                                    if pt:
                                        for p in pt:
                                            snaps.append([p, 'intersection', self.project_to_working_plane_normal(p)])
                                except:
                                    pass
                                    # some curve types yield an error
                                    # when trying to read their types
        return snaps

    def snap_to_polygon(self, obj):
        """Return a list of polygon center snap locations."""
        snaps = []
        if self.is_enabled("Center"):
            c = obj.Placement.Base
            for edge in obj.Shape.Edges:
                p1 = edge.Vertexes[0].Point
                p2 = edge.Vertexes[-1].Point
                v1 = p1.add((p2 - p1).scale(0.25, 0.25, 0.25))
                v2 = p1.add((p2 - p1).scale(0.75, 0.75, 0.75))
                snaps.append([v1, 'center', self.project_to_working_plane_normal(c)])
                snaps.append([v2, 'center', self.project_to_working_plane_normal(c)])
        return snaps

    def snap_to_vertex(self, info, active=False):
        p = App.Vector(info['x'], info['y'], info['z'])
        if active:
            if self.is_enabled("Near"):
                return [p, 'endpoint', self.project_to_working_plane_normal(p)]
            else:
                return []
        elif self.is_enabled("Near"):
            return [p, 'passive', p]
        else:
            return []

    def snap_to_specials(self, obj, lastpoint=None, eline=None):
        """Return special snap locations, if any."""
        snaps = []
        if self.is_enabled("Special"):

            if (Draft.get_type(obj) == "Wall"):
                # special snapping for wall: snap to its base shape if it is linear
                if obj.Base:
                    if not obj.Base.Shape.Solids:
                        for v in obj.Base.Shape.Vertexes:
                            snaps.append([v.Point, 'special', self.project_to_working_plane_normal(v.Point)])

            elif (Draft.get_type(obj) == "Structure"):
                # special snapping for struct: only to its base point
                if obj.Base:
                    if not obj.Base.Shape.Solids:
                        for v in obj.Base.Shape.Vertexes:
                            snaps.append([v.Point, 'special', self.project_to_working_plane_normal(v.Point)])
                else:
                    b = obj.Placement.Base
                    snaps.append([b, 'special', self.project_to_working_plane_normal(b)])
                if obj.ViewObject.ShowNodes:
                    for edge in obj.Proxy.getNodeEdges(obj):
                        snaps.extend(self.snap_to_endpoints(edge))
                        snaps.extend(self.snap_to_midpoint(edge))
                        snaps.extend(self.snap_to_perpendicular(edge, lastpoint))
                        snaps.extend(self.snap_to_intersection(edge))
                        snaps.extend(self.snap_to_edge_lines(edge, eline))

            elif hasattr(obj, "SnapPoints"):
                for p in obj.SnapPoints:
                    p2 = obj.Placement.multVec(p)
                    snaps.append([p2, 'special', p2])

        return snaps

    def get_screen_distance(self, dist, cursor):
        """Return a distance in 3D space from a screen pixels distance."""
        view = Draft.get3DView()
        p1 = view.getPoint(cursor)
        p2 = view.getPoint((cursor[0] + dist, cursor[1]))
        return (p2.sub(p1)).Length

    def get_perpendicular(self, edge, pt):
        """Return a point on an edge, perpendicular to the given point."""
        dv = pt.sub(edge.Vertexes[0].Point)
        nv = DraftVecUtils.project(dv, DraftGeomUtils.vec(edge))
        np = (edge.Vertexes[0].Point).add(nv)
        return np

    def set_arc_dimensions(self, p1, p2):
        """Show arc dimensions between 2 points."""
        if self.is_enabled("Dimensions"):
            if not self.dim1:
                self.dim1 = trackers.archDimTracker(mode=2)
            if not self.dim2:
                self.dim2 = trackers.archDimTracker(mode=3)
            self.dim1.p1(p1)
            self.dim2.p1(p1)
            self.dim1.p2(p2)
            self.dim2.p2(p2)
            if self.dim1.Distance:
                self.dim1.on()
            if self.dim2.Distance:
                self.dim2.on()

    def set_cursor(self, mode=None):
        """Set or reset the cursor to the given mode or resets."""
        if self.selectMode:
            mw = Gui.getMainWindow()
            for w in mw.findChild(QtGui.QMdiArea).findChildren(QtGui.QWidget):
                if w.metaObject().className() == "SIM::Coin3D::Quarter::QuarterWidget":
                    w.unsetCursor()
            self.cursorMode = None
        elif not mode:
            mw = Gui.getMainWindow()
            for w in mw.findChild(QtGui.QMdiArea).findChildren(QtGui.QWidget):
                if w.metaObject().className() == "SIM::Coin3D::Quarter::QuarterWidget":
                    w.unsetCursor()
            self.cursorMode = None
        else:
            if mode != self.cursorMode:
                baseicon = QtGui.QPixmap(":/icons/Draft_Cursor.svg")
                newicon = QtGui.QPixmap(32, 24)
                newicon.fill(QtCore.Qt.transparent)
                qp = QtGui.QPainter()
                qp.begin(newicon)
                qp.drawPixmap(0, 0, baseicon)
                if not (mode == 'passive'):
                    tp = QtGui.QPixmap(self.cursors[mode]).scaledToWidth(16)
                    qp.drawPixmap(QtCore.QPoint(16, 8), tp)
                qp.end()
                cur = QtGui.QCursor(newicon, 8, 8)
                mw = Gui.getMainWindow()
                for w in mw.findChild(QtGui.QMdiArea).findChildren(QtGui.QWidget):
                    if w.metaObject().className() == "SIM::Coin3D::Quarter::QuarterWidget":
                        w.setCursor(cur)
                self.cursorMode = mode

    def restack(self):
        """Lower the grid tracker so it doesn't obscure other objects."""
        if self.grid:
            self.grid.lowerTracker()

    def off(self, hide_snap_bar=False):
        """Finish snapping."""
        if self.tracker:
            self.tracker.off()
        if self.trackLine:
            self.trackLine.off()
        if self.extLine:
            self.extLine.off()
        if self.extLine2:
            self.extLine2.off()
        if self.radiusTracker:
            self.radiusTracker.off()
        if self.dim1:
            self.dim1.off()
        if self.dim2:
            self.dim2.off()
        if self.grid:
            if not Draft.getParam("alwaysShowGrid", True):
                self.grid.off()
        if self.holdTracker:
            self.holdTracker.clear()
            self.holdTracker.off()
        self.unconstrain()
        self.radius = 0
        self.set_cursor()
        if hide_snap_bar or Draft.getParam("hideSnapBar", False):
            if hasattr(self, "toolbar") and self.toolbar:
                self.toolbar.hide()
        self.mask = None
        self.selectMode = False
        self.running = False
        self.holdPoints = []

    def set_select_mode(self, mode):
        """Set the snapper into select mode (hides snapping temporarily)."""
        self.selectMode = mode
        if not mode:
            self.set_cursor()
        else:
            if self.trackLine:
                self.trackLine.off()

    def set_angle(self, delta=None):
        """Keep the current angle."""
        if delta:
            self.mask = delta
        elif isinstance(self.mask, App.Vector):
            self.mask = None
        elif self.trackLine:
            if self.trackLine.Visible:
                self.mask = self.trackLine.p2().sub(self.trackLine.p1())

    def constrain(self, point, basepoint=None, axis=None):
        """Return a constrained point.

        constrain(point,basepoint=None,axis=None: Returns a
        constrained point. Axis can be "x","y" or "z" or a custom vector. If None,
        the closest working plane axis will be picked.
        Basepoint is the base point used to figure out from where the point
        must be constrained. If no basepoint is given, the current point is
        used as basepoint.
        """
        # without the Draft module fully loaded, no axes system!"
        if not hasattr(App, "DraftWorkingPlane"):
            return point

        point = App.Vector(point)

        # setup trackers if needed
        if not self.constrainLine:
            if self.snapStyle:
                self.constrainLine = trackers.lineTracker(scolor=Gui.draftToolBar.getDefaultColor("snap"))
            else:
                self.constrainLine = trackers.lineTracker(dotted=True)

        # setting basepoint
        if not basepoint:
            if not self.basepoint:
                self.basepoint = point
        else:
            self.basepoint = basepoint
        delta = point.sub(self.basepoint)

        # setting constraint axis
        if self.mask:
            self.affinity = self.mask
        if not self.affinity:
            self.affinity = App.DraftWorkingPlane.getClosestAxis(delta)
        if isinstance(axis, App.Vector):
            self.constraintAxis = axis
        elif axis == "x":
            self.constraintAxis = App.DraftWorkingPlane.u
        elif axis == "y":
            self.constraintAxis = App.DraftWorkingPlane.v
        elif axis == "z":
            self.constraintAxis = App.DraftWorkingPlane.axis
        else:
            if self.affinity == "x":
                self.constraintAxis = App.DraftWorkingPlane.u
            elif self.affinity == "y":
                self.constraintAxis = App.DraftWorkingPlane.v
            elif self.affinity == "z":
                self.constraintAxis = App.DraftWorkingPlane.axis
            elif isinstance(self.affinity, App.Vector):
                self.constraintAxis = self.affinity
            else:
                self.constraintAxis = None

        if not self.constraintAxis:
            return point

        # calculating constrained point
        cdelta = DraftVecUtils.project(delta, self.constraintAxis)
        npoint = self.basepoint.add(cdelta)

        # setting constrain line
        if self.constrainLine:
            if point != npoint:
                self.constrainLine.p1(point)
                self.constrainLine.p2(npoint)
                self.constrainLine.on()
            else:
                self.constrainLine.off()

        return npoint

    def unconstrain(self):
        """Unset the basepoint and the constrain line."""
        self.basepoint = None
        self.affinity = None
        if self.constrainLine:
            self.constrainLine.off()

    def get_point(self, last=None, callback=None, movecallback=None,
                 extradlg=None, title=None, mode="point"):
        """Get a 3D point from the screen.

        getPoint([last],[callback],[movecallback],[extradlg],[title]):
        gets a 3D point from the screen. You can provide an existing point,
        in that case additional snap options and a tracker are available.
        You can also pass a function as callback, which will get called
        with the resulting point as argument, when a point is clicked,
        and optionally another callback which gets called when
        the mouse is moved.

        If the operation gets cancelled (the user pressed Escape),
        no point is returned.

        Example:

        def cb(point):
            if point:
                print "got a 3D point: ",point

        Gui.Snapper.getPoint(callback=cb)

        If the callback function accepts more than one argument,
        it will also receive the last snapped object. Finally, a qt widget
        can be passed as an extra taskbox.
        title is the title of the point task box mode is the dialog box
        you want (default is point, you can also use wire and line)

        If getPoint() is invoked without any argument, nothing is done
        but the callbacks are removed, so it can be used as a cancel function.
        """
        self.pt = None
        self.lastSnappedObject = None
        self.holdPoints = []
        self.ui = Gui.draftToolBar
        self.view = Draft.get3DView()

        # remove any previous leftover callbacks
        if self.callbackClick:
            self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), self.callbackClick)
        if self.callbackMove:
            self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), self.callbackMove)
        self.callbackClick = None
        self.callbackMove = None

        def move(event_cb):
            event = event_cb.getEvent()
            mousepos = event.getPosition()
            ctrl = event.wasCtrlDown()
            shift = event.wasShiftDown()
            self.pt = Gui.Snapper.snap(mousepos, lastpoint=last,
                                       active=ctrl, constrain=shift)
            if hasattr(App, "DraftWorkingPlane"):
                self.ui.displayPoint(self.pt, last,
                                     plane=App.DraftWorkingPlane,
                                     mask=Gui.Snapper.affinity)
            if movecallback:
                movecallback(self.pt, self.snapInfo)

        def getcoords(point, relative=False):
            """Get the global coordinates from a point."""
            self.pt = point
            if relative and last and hasattr(App, "DraftWorkingPlane"):
                v = App.DraftWorkingPlane.getGlobalCoords(point)
                self.pt = last.add(v)
            accept()

        def click(event_cb):
            event = event_cb.getEvent()
            if event.getButton() == 1:
                if event.getState() == coin.SoMouseButtonEvent.DOWN:
                    accept()

        def accept():
            if self.callbackClick:
                self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), self.callbackClick)
            if self.callbackMove:
                self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), self.callbackMove)
            self.callbackClick = None
            self.callbackMove = None
            Gui.Snapper.off()
            self.ui.offUi()
            if callback:
                if len(inspect.getargspec(callback).args) > 1:
                    obj = None
                    if self.snapInfo and ("Object" in self.snapInfo) and self.snapInfo["Object"]:
                        obj = App.ActiveDocument.getObject(self.snapInfo["Object"])
                    callback(self.pt, obj)
                else:
                    callback(self.pt)
            self.pt = None

        def cancel():
            if self.callbackClick:
                self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), self.callbackClick)
            if self.callbackMove:
                self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), self.callbackMove)
            self.callbackClick = None
            self.callbackMove = None
            Gui.Snapper.off()
            self.ui.offUi()
            if callback:
                if len(inspect.getargspec(callback).args) > 1:
                    callback(None, None)
                else:
                    callback(None)

        # adding callback functions
        if mode == "line":
            interface = self.ui.lineUi
        elif mode == "wire":
            interface = self.ui.wireUi
        else:
            interface = self.ui.pointUi
        if callback:
            if title:
                interface(title=title, cancel=cancel, getcoords=getcoords,
                          extra=extradlg, rel=bool(last))
            else:
                interface(cancel=cancel,getcoords=getcoords,extra=extradlg,rel=bool(last))
            self.callbackClick = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),click)
            self.callbackMove = self.view.addEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(),move)

    def make_snap_toolbar(self):
        """Build the Snap toolbar."""
        mw = Gui.getMainWindow()
        self.toolbar = QtGui.QToolBar(mw)
        mw.addToolBar(QtCore.Qt.TopToolBarArea, self.toolbar)
        self.toolbar.setObjectName("Draft Snap")
        self.toolbar.setWindowTitle(QtCore.QCoreApplication.translate("Workbench", "Draft Snap"))

        # make snap buttons
        snap_gui_commands = get_draft_snap_commands()
        self.init_draft_snap_buttons(snap_gui_commands, self.toolbar, "_Button")
        self.restore_snap_buttons_state(self.toolbar,"_Button")

        if not Draft.getParam("showSnapBar",True):
            self.toolbar.hide()

    def init_draft_snap_buttons(self, commands, context, button_suffix):
        """
        Init Draft Snap toolbar buttons.

        Parameters:
        commands        Snap command list,
                        use: get_draft_snap_commands():
        context         The toolbar or action group the buttons have 
                        to be added to    
        button_suffix   The suffix that have to be applied to the command name
                        to define the button name
        """
        for gc in commands:
            if gc == "Separator":
                continue
            if gc == "Draft_ToggleGrid":
                gb = self.init_grid_button(self.toolbar)
                context.addAction(gb)
                QtCore.QObject.connect(gb, QtCore.SIGNAL("triggered()"),
                                    lambda f=Gui.doCommand, 
                                    arg='Gui.runCommand("Draft_ToggleGrid")':f(arg))
                continue
            # setup toolbar buttons
            command = 'Gui.runCommand("' + gc + '")'
            b = QtGui.QAction(context)
            b.setIcon(QtGui.QIcon(':/icons/' + gc[6:] + '.svg'))
            b.setText(QtCore.QCoreApplication.translate("Draft_Snap", "Snap " + gc[11:]))
            b.setToolTip(QtCore.QCoreApplication.translate("Draft_Snap", "Snap " + gc[11:]))
            b.setObjectName(gc + button_suffix)
            b.setWhatsThis("Draft_" + gc[11:].capitalize())
            b.setCheckable(True)
            b.setChecked(True)
            context.addAction(b)
            QtCore.QObject.connect(b,
                                   QtCore.SIGNAL("triggered()"),
                                   lambda f=Gui.doCommand, 
                                   arg=command:f(arg))

        for b in context.actions():
            if len(b.statusTip()) == 0:
                b.setStatusTip(b.toolTip())

    def init_grid_button(self, context):
        """Add grid button to the given toolbar"""
        b = QtGui.QAction(context)
        b.setIcon(QtGui.QIcon.fromTheme("Draft", QtGui.QIcon(":/icons/"
                                                         "Draft_Grid.svg")))        
        b.setText(QtCore.QCoreApplication.translate("Draft_Snap", "Toggles Grid On/Off"))
        b.setToolTip(QtCore.QCoreApplication.translate("Draft_Snap", "Toggle Draft Grid"))
        b.setObjectName("Grid_Button")
        b.setWhatsThis("Draft_ToggleGrid")
        return b

    def restore_snap_buttons_state(self, toolbar, button_suffix):
        """
        Restore toolbar button's checked state according to 
        "snapModes" saved in preferences
        """
        # set status tip where needed
        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        snap_modes = param.GetString("snapModes")

        for button in toolbar.actions():
            if len(button.statusTip()) == 0:
                button.setStatusTip(button.toolTip())

        # restore toolbar buttons state
        if snap_modes:
            for action in toolbar.findChildren(QtGui.QAction):
                snap = action.objectName()[11:].replace(button_suffix, "")
                if snap in Gui.Snapper.snaps:
                    i = Gui.Snapper.snaps.index(snap)
                    state = bool(int(snap_modes[i]))
                    action.setChecked(state)
                    if state:
                        action.setToolTip(action.toolTip() + " (ON)")
                    else:
                        action.setToolTip(action.toolTip() + " (OFF)")

    def get_snap_toolbar(self):
        """Returns snap toolbar object."""
        mw = Gui.getMainWindow()
        if mw:
            toolbar = mw.findChild(QtGui.QToolBar, "Draft Snap")
            if toolbar:
                return toolbar
        return None

    def toggle_grid(self):
        """Toggle FreeCAD Draft Grid."""
        Gui.runCommand("Draft_ToggleGrid")

    def showradius(self):
        """Show the snap radius indicator."""
        self.radius = self.get_screen_distance(Draft.getParam("snapRange", 8),
                                         (400, 300))
        if self.radiusTracker:
            self.radiusTracker.update(self.radius)
            self.radiusTracker.on()

    def is_enabled(self, snap):
        """Returns true if the given snap is on"""
        if "Lock" in self.active_snaps and snap in self.active_snaps:
            return True
        else:
            return False

    def toggle_snap(self, snap, set_to = None):
        """Sets the given snap on/off according to the given parameter"""
        if set_to: # set mode
            if set_to is True:
                if not snap in self.active_snaps:
                    self.active_snaps.append(snap)
                status = True
            elif set_to is False:
                if snap in self.active_snaps:
                    self.active_snaps.remove(snap)
                status = False
        else: # toggle mode, default
            if not snap in self.active_snaps:
                self.active_snaps.append(snap)
                status = True
            elif snap in self.active_snaps:
                self.active_snaps.remove(snap)
                status = False
        self.save_snap_state()
        return status

    def save_snap_state(self):
        """
        Save snap state to user preferences to be restored in next session.
        """
        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        snap_modes = ""
        for snap in self.snaps:
            if snap in self.active_snaps:
                snap_modes += "1"
            else:
                snap_modes += "0"
        param.SetString("snapModes",snap_modes)

    def show(self):
        """Show the toolbar and the grid."""
        if not hasattr(self, "toolbar"):
            self.make_snap_toolbar()
        bt = self.get_snap_toolbar()
        if not bt:
            mw = Gui.getMainWindow()
            mw.addToolBar(self.toolbar)
            self.toolbar.setParent(mw)
        self.toolbar.show()
        self.toolbar.toggleViewAction().setVisible(True)
        if Gui.ActiveDocument:
            self.set_trackers()
            if not App.ActiveDocument.Objects:
                if Gui.ActiveDocument.ActiveView:
                    if Gui.ActiveDocument.ActiveView.getCameraType() == 'Orthographic':
                        c = Gui.ActiveDocument.ActiveView.getCameraNode()
                        if c.orientation.getValue().getValue() == (0.0, 0.0, 0.0, 1.0):
                            p = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
                            h = p.GetInt("defaultCameraHeight",0)
                            if h:
                                c.height.setValue(h)

    def hide(self):
        """Hide the toolbar."""
        if hasattr(self, "toolbar"):
            self.toolbar.hide()
            self.toolbar.toggleViewAction().setVisible(True)

    def set_grid(self):
        """Set the grid, if visible."""
        self.set_trackers()
        if self.grid and (not self.forceGridOff):
            if self.grid.Visible:
                self.grid.set()

    def set_trackers(self):
        """Set the trackers."""
        v = Draft.get3DView()
        if v != self.activeview:
            if v in self.trackers[0]:
                i = self.trackers[0].index(v)
                self.grid = self.trackers[1][i]
                self.tracker = self.trackers[2][i]
                self.extLine = self.trackers[3][i]
                self.radiusTracker = self.trackers[4][i]
                self.dim1 = self.trackers[5][i]
                self.dim2 = self.trackers[6][i]
                self.trackLine = self.trackers[7][i]
                self.extLine2 = self.trackers[8][i]
                self.holdTracker = self.trackers[9][i]
            else:
                if Draft.getParam("grid", True):
                    self.grid = trackers.gridTracker()
                    self.grid.on()
                else:
                    self.grid = None
                self.tracker = trackers.snapTracker()
                self.trackLine = trackers.lineTracker()
                if self.snapStyle:
                    c = Gui.draftToolBar.getDefaultColor("snap")
                    self.extLine = trackers.lineTracker(scolor=c)
                    self.extLine2 = trackers.lineTracker(scolor=c)
                else:
                    self.extLine = trackers.lineTracker(dotted=True)
                    self.extLine2 = trackers.lineTracker(dotted=True)
                self.radiusTracker = trackers.radiusTracker()
                self.dim1 = trackers.archDimTracker(mode=2)
                self.dim2 = trackers.archDimTracker(mode=3)
                self.holdTracker = trackers.snapTracker()
                self.holdTracker.setMarker("cross")
                self.holdTracker.clear()
                self.trackers[0].append(v)
                self.trackers[1].append(self.grid)
                self.trackers[2].append(self.tracker)
                self.trackers[3].append(self.extLine)
                self.trackers[4].append(self.radiusTracker)
                self.trackers[5].append(self.dim1)
                self.trackers[6].append(self.dim2)
                self.trackers[7].append(self.trackLine)
                self.trackers[8].append(self.extLine2)
                self.trackers[9].append(self.holdTracker)
            self.activeview = v
            
        if self.grid and (not self.forceGridOff):
            self.grid.set()

    def add_hold_point(self):
        """Add hold snap point to list of hold points."""
        if self.spoint and self.spoint not in self.holdPoints:
            if self.holdTracker:
                self.holdTracker.addCoords(self.spoint)
                self.holdTracker.on()
            self.holdPoints.append(self.spoint)

## @}
