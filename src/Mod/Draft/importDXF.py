# -*- coding: utf8 -*-
# Check code with
# flake8 --ignore=E226,E266,E401,W503

# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
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

__title__ = "FreeCAD Draft Workbench - DXF importer/exporter"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "https://www.freecad.org"

## @package importDXF
#  \ingroup DRAFT
#  \brief DXF file importer & exporter
#
# This module provides support for importing and exporting Autodesk DXF files

"""
This script uses a DXF-parsing library created by Stani,
Kitsu and Migius for Blender

imports:
line, polylines, lwpolylines, arcs, circles, texts,
mtexts, layers (as groups), colors

exports:
lines, polylines, lwpolylines, circles, arcs,
texts, colors,layers (from groups)
"""
# scaling factor between autocad font sizes and coin font sizes
TEXTSCALING = 1.35
# the minimum version of the dxfLibrary needed to run
CURRENTDXFLIB = 1.41

import sys
import os
import math
import re
import FreeCAD
import Part
import Draft
import Mesh
import DraftVecUtils
import DraftGeomUtils
import WorkingPlane
from Draft import _Dimension
from FreeCAD import Vector
from FreeCAD import Console as FCC

# sets the default working plane if Draft hasn't been started yet
if not hasattr(FreeCAD, "DraftWorkingPlane"):
    plane = WorkingPlane.plane()
    FreeCAD.DraftWorkingPlane = plane

gui = FreeCAD.GuiUp
draftui = None
if gui:
    import FreeCADGui
    try:
        draftui = FreeCADGui.draftToolBar
    except (AttributeError, NameError):
        draftui = None
    from draftutils.translate import translate
    from PySide import QtGui
else:
    def translate(context, txt):
        return txt

dxfReader = None
dxfColorMap = None
dxfLibrary = None

# Save the native open function to avoid collisions
# with the function declared here
if open.__module__ in ['__builtin__', 'io']:
    pythonopen = open


def errorDXFLib(gui):
    """Download the files required to convert DXF files.

    It checks the parameter `'dxfAllowDownload'` to decide whether it
    has access to download the required DXF libraries.

    Parameters
    ----------
    gui : bool
        If `True` it will display error messages in graphical
        text boxes; otherwise it will display the messages in the terminal.

    To do
    -----
    Use local variables, not global variables.
    """
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    dxfAllowDownload = p.GetBool("dxfAllowDownload", False)
    if dxfAllowDownload:
        files = ['dxfColorMap.py', 'dxfImportObjects.py',
                 'dxfLibrary.py', 'dxfReader.py']

        baseurl = 'https://raw.githubusercontent.com/yorikvanhavre/'
        baseurl += 'Draft-dxf-importer/master/'
        import ArchCommands
        from FreeCAD import Base
        progressbar = Base.ProgressIndicator()
        progressbar.start("Downloading files...", 4)
        for f in files:
            progressbar.next()
            p = None
            p = ArchCommands.download(baseurl + f, force=True)
            if not p:
                if gui:
                    message = translate("Draft", """Download of dxf libraries failed.
Please install the dxf Library addon manually
from menu Tools -> Addon Manager""")
                    QtGui.QMessageBox.information(None, "", message)
                else:
                    FCC.PrintWarning("The DXF import/export libraries needed by FreeCAD to handle the DXF format are not installed.\n")
                    FCC.PrintWarning("Please install the dxf Library addon from Tools -> Addon Manager\n")
                break
        progressbar.stop()
        sys.path.append(FreeCAD.ConfigGet("UserAppData"))
    else:
        if gui:
            message = translate('draft', """The DXF import/export libraries needed by FreeCAD to handle
the DXF format were not found on this system.
Please either enable FreeCAD to download these libraries:
  1 - Load Draft workbench
  2 - Menu Edit > Preferences > Import-Export > DXF > Enable downloads
Or download these libraries manually, as explained on
https://github.com/yorikvanhavre/Draft-dxf-importer
To enabled FreeCAD to download these libraries, answer Yes.""")
            reply = QtGui.QMessageBox.question(None, "", message,
                                               QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                                               QtGui.QMessageBox.No)
            if reply == QtGui.QMessageBox.Yes:
                p.SetBool("dxfAllowDownload", True)
                errorDXFLib(gui)
            if reply == QtGui.QMessageBox.No:
                pass
        else:
            FCC.PrintWarning("The DXF import/export libraries needed by FreeCAD to handle the DXF format are not installed.\n")
            _ver = FreeCAD.Version()
            _maj = _ver[0]
            _min = _ver[1]
            if float(_maj + "." + _min) >= 0.17:
                FCC.PrintWarning("Please install the dxf Library addon from Tools -> Addon Manager\n")
            else:
                FCC.PrintWarning("Please check https://github.com/yorikvanhavre/Draft-dxf-importer\n")


def getDXFlibs():
    """Load the DXF Python libraries.

    It tries loading the global libraries for use in the system
    `dxfLibrary`, `dxfColorMap`, `dxfReader`,
    If they are not present, they are downloaded.

    To do
    -----
    Use local variables, not global variables.
    """
    try:
        if FreeCAD.ConfigGet("UserAppData") not in sys.path:
            sys.path.append(FreeCAD.ConfigGet("UserAppData"))
        global dxfLibrary, dxfColorMap, dxfReader
        import dxfLibrary
        import dxfColorMap
        try:
            import dxfReader
        except Exception:
            libsok = False
    except ImportError:
        libsok = False
        FCC.PrintWarning("DXF libraries not found. Trying to download...\n")
    else:
        if float(dxfLibrary.__version__[1:5]) >= CURRENTDXFLIB:
            libsok = True
        else:
            FCC.PrintWarning("DXF libraries need to be updated. "
                             "Trying to download...\n")
            libsok = False
    if not libsok:
        errorDXFLib(gui)
        try:
            import dxfColorMap, dxfLibrary, dxfReader
        except ImportError:
            dxfReader = None
            dxfLibrary = None
            FCC.PrintWarning("DXF libraries not available. Aborting.\n")


def prec():
    """Return the current Draft precision level."""
    return Draft.getParam("precision", 6)


def deformat(text):
    """Remove weird formats in texts and wipes UTF characters.

    It removes `{}`, html codes, \\(U...) characters,

    Parameters
    ----------
    text : str
        The input string.

    Results
    -------
    str
        The deformatted string.
    """
    # remove ACAD string formatation
    # t = re.sub('{([^!}]([^}]|\n)*)}', '', text)
    # print("input text: ",text)
    t = text.strip("{}")
    t = re.sub("\\\.*?;", "", t)
    # replace UTF codes by utf chars
    sts = re.split("\\\\(U\+....)", t)
    t = u"".join(sts)
    # replace degrees, diameters chars
    t = re.sub('%%d', u'°', t)
    t = re.sub('%%c', u'Ø', t)
    t = re.sub('%%D', u'°', t)
    t = re.sub('%%C', u'Ø', t)
    # print("output text: ", t)
    return t


def locateLayer(wantedLayer, color=None, drawstyle=None):
    """Return layer group and create it if needed.

    This function iterates over a global list named `layers`, which is
    defined in `processdxf`.

    If no layers are found it looks for the global `dxfUseDraftVisGroup`
    variable defined in `readPreferences`, and creates a new `Draft Layer`
    with the specified color.

    Otherwise it creates a group (`App::DocumentObjectGroup`)
    to use as a layer container.

    Parameters
    ----------
    wantedLayer : str
        The name of a layer to search in the global `layers` list.

    color : tuple of four floats, optional
        It defaults to `None`.
        A tuple with color information `(r,g,b,a)`, where each value
        is a float between 0 and 1.

    Returns
    -------
    App::FeaturePython or App::DocumentObjectGroup
        If the `wantedLayer` is found in the global list of layers,
        it is returned.
        Otherwise, a new layer or group is created and returned.

        If the global variable `dxfUseDraftVisGroup` is set,
        it creates a `Draft Layer` (`App::FeaturePython`).
        Otherwise, it creates a simple group (`App::DocumentObjectGroup`).

    See also
    --------
    Draft.make_layer

    To do
    -----
    Use local variables, not global variables.
    """
    # layers is a global variable.
    # It should probably be passed as an argument.
    if wantedLayer is None:
        wantedLayer = '0'
    for layer in layers:
        if layer.Label == wantedLayer:
            return layer
    if dxfUseDraftVisGroups:
        newLayer = Draft.make_layer(name=wantedLayer,
                                    line_color=(0.0,0.0,0.0) if not color else color,
                                    draw_style="Solid" if not drawstyle else drawstyle)
    else:
        newLayer = doc.addObject("App::DocumentObjectGroup", wantedLayer)
    newLayer.Label = wantedLayer
    layers.append(newLayer)
    return newLayer


def getdimheight(style):
    """Return the dimension text height from the given dimstyle.

    It searches the global variable `drawing.tables.data`,
    created in `processdxf`, for a `dimstyle`; then iterates on the data,
    and if a `dimstyle` is found, it compares if its raw value with DXF code 2
    (Name) is equal to `style`.

    Parameters
    ---------
    style : str
        A raw value of DXF code 3 (other text or name value).

    Returns
    -------
    float
        The data of DXF code 140 (DIMSTYLE setting),
        or just 1 if no `dimstyle` was found in `drawing.tables.data`.

    To do
    -----
    Use local variables, not global variables.
    """
    for t in drawing.tables.data:
        if t.name == 'dimstyle':
            for a in t.data:
                if hasattr(a, "type"):
                    if a.type == "dimstyle":
                        if rawValue(a, 2) == style:
                            return rawValue(a, 140)
    return 1


def calcBulge(v1, bulge, v2):
    """Calculate intermediary vertex for a curved segment.

    Considering an arc of a circle, it can be defined by two vertices `v1`
    and `v2`, and a `bulge` value that indicates how curved the arc is.
    A `bulge` of 0 is a straight line, while a `bulge` of 1 is the maximum
    curvature, or a semicircle.

    A vertex that is in the curve, equidistant to the two vertices,
    can be found by finding the sagitta of the arc, that is,
    the perpendicular to the chord that goes from `v1` to `v2`.

    It uses the algorithm from http://www.afralisp.net/lisp/Bulges1.htm

    Parameters
    ----------
    v1 : Base::Vector3
        The first point.
    bulge : float
        The bulge is the tangent of 1/4 of the included angle for the arc
        between `v1` and `v2`. A negative `bulge` indicates that the arc
        goes clockwise from `v1` to `v2`. A `bulge` of 0 indicates
        a straight segment, and a `bulge` of 1 is a semicircle.
    v2 : Base::Vector3
        The second point.

    Returns
    -------
    Base::Vector3
        The new point between `v1` and `v2`.
    """
    chord = v2.sub(v1)
    sagitta = (bulge * chord.Length)/2
    perp = chord.cross(Vector(0, 0, 1))
    startpoint = v1.add(chord.multiply(0.5))
    if not DraftVecUtils.isNull(perp):
        perp.normalize()
    endpoint = perp.multiply(sagitta)
    return startpoint.add(endpoint)


def getGroup(ob):
    """Get the name of the group or Draft layer that contains the object.

    It looks for the global `dxfUseDraftVisGroup` variable defined
    in `readPreferences`. Then searches all objects of type "Layer"
    for the one that contains `ob`.

    Otherwise, it searches all objects derived from
    `App::DocumentObjectGroup` for the one that contains `ob`.

    Parameters
    ----------
    ob : App::DocumentObject
        Any object to test as belonging to a layer or group.

    Returns
    -------
    str
        The label of the layer, or of the group, if it contains `ob`.
        Otherwise, return "0".

    To do
    -----
    Use local variables, not global variables.
    """
    all_objs = FreeCAD.ActiveDocument.Objects
    if dxfUseDraftVisGroups:
        for layer in [o for o in all_objs if Draft.getType(o) == "Layer"]:
            if ob in layer.Group:
                return layer.Label
    for i in all_objs:
        if i.isDerivedFrom("App::DocumentObjectGroup"):
            for j in i.Group:
                if j == ob:
                    return i.Label
    return "0"


def getACI(ob, text=False):
    """Get the AutoCAD color index (ACI) color closest to the object's color.

    This function only works if the graphical interface is loaded,
    as it checks the `ViewObject` attribute of the object
    which only exists when the GUI is available.

    Parameters
    ----------
    ob : App::DocumentObject
        Any object.

    text : bool, optional
        It defaults ot `False`. If `True`, use the `TextColor`
        instead of the `LineColor` of the object.

    Returns
    -------
    int
        The numerical value of the AutoCAD color index (ACI) color,
        which goes from 0 to 255.
        It returns 0 (black) if no graphical interface is loaded.
        It returns 256 (`BYLAYER`) if `ob` is inside a Draft Layer,
        and the layer's `OverrideChildren` view property is `True`.
    """
    if not gui:
        return 0
    else:
        # detect if we need to set "BYLAYER"
        for parent in ob.InList:
            if Draft.getType(parent) == "Layer":
                if ob in parent.Group:
                    if hasattr(parent, "ViewObject") and hasattr(parent.ViewObject, "OverrideChildren"):
                        if parent.ViewObject.OverrideChildren:
                            return 256  # BYLAYER
        if text:
            col = ob.ViewObject.TextColor
        else:
            col = ob.ViewObject.LineColor
        aci = [0, 442]
        for i in range(255, -1, -1):
            ref = dxfColorMap.color_map[i]
            dist = ((ref[0]-col[0])**2
                    + (ref[1]-col[1])**2
                    + (ref[2]-col[2])**2)
            if dist <= aci[1]:
                aci = [i, dist]
        return aci[0]


def rawValue(entity, code):
    """Return the value of a DXF code in an entity section.

    Parameters
    ----------
    entity : drawing.entities
        A DXF entity in the `drawing` data obtained from `processdxf`.
    code : int
        A numerical value of the code.

    Returns
    -------
    float or str
        The value corresponding to the code. It may be numeric or a string.
    """
    value = None
    for pair in entity.data:
        if pair[0] == code:
            value = pair[1]
    return value


def getMultiplePoints(entity):
    """Scan the given entity (paths, leaders, etc.) for multiple points.

    Parameters
    ----------
    entity : drawing.entities
        A DXF entity in the `drawing` data obtained from `processdxf`.

    Returns
    -------
    list of Base::Vector3
        The list of points (vectors).
        Each point has three coordinates `(X,Y,Z)`.
        If the original point only had two, the third coordinate
        is set to zero `(X,Y,0)`.
    """
    pts = []
    for d in entity.data:
        if d[0] == 10:
            pts.append([d[1]])
        elif d[0] in [20, 30]:
            pts[-1].append(d[1])
    pts.reverse()
    points = []
    for p in pts:
        if len(p) == 3:
            points.append(Vector(p[0], p[1], p[2]))
        else:
            points.append(Vector(p[0], p[1], 0))
    return points


def isBrightBackground():
    """Check if the current viewport's background is a bright color.

    It considers the values of `BackgroundColor` for a solid background,
    or a combination of `BackgroundColor2` and `BackgroundColor3`
    for a gradient background from the parameter database.

    Returns
    -------
    bool
        Returns `True` if the value of the color is larger than 128,
        which is considered light; otherwise it is considered dark
        and returns `False`.
    """
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
    if p.GetBool("Gradient"):
        c1 = p.GetUnsigned("BackgroundColor2")
        c2 = p.GetUnsigned("BackgroundColor3")
        r1 = float((c1 >> 24) & 0xFF)
        g1 = float((c1 >> 16) & 0xFF)
        b1 = float((c1 >> 8) & 0xFF)
        r2 = float((c2 >> 24) & 0xFF)
        g2 = float((c2 >> 16) & 0xFF)
        b2 = float((c2 >> 8) & 0xFF)
        v1 = Vector(r1, g1, b1)
        v2 = Vector(r2, g2, b2)
        v = v2.sub(v1)
        v.multiply(0.5)
        cv = v1.add(v)
    else:
        c1 = p.GetUnsigned("BackgroundColor")
        r1 = float((c1 >> 24) & 0xFF)
        g1 = float((c1 >> 16) & 0xFF)
        b1 = float((c1 >> 8) & 0xFF)
        cv = Vector(r1, g1, b1)
    value = cv.x*.3 + cv.y*.59 + cv.z*.11
    if value < 128:
        return False
    else:
        return True


def getGroupColor(dxfobj, index=False):
    """Get the color of the layer.

    It searches the global variable `drawing.tables`,
    created in `processdxf`, for a `layer`; then iterates on the data,
    and if the layer name matches the layer of `dxfobj`, it will try
    to return the color of its layer.

    It searches the global variable `dxfBrightBackground` to determine
    if it should return black, or a color from the global
    `dxfColorMap.color_map` dictionary.

    Parameters
    ----------
    dxfobj : Part::Feature
        An imported DXF object.

    index : bool, optional
        It defaults to `False`. If it is `True` it will return the layer's
        color; otherwise it will check the global variable
        `dxfBrightBackground`, and return black or a mapped color.

    Returns
    -------
    list of 3 floats
        The layer's color as a list `[r, g, b]`, black `[0, 0, 0]`
        or the mapped color `dxfColorMap.color_map[color]`.

    To do
    -----
    Use local variables, not global variables.
    """
    name = dxfobj.layer
    for table in drawing.tables.get_type("table"):
        if table.name == "layer":
            for l in table.get_type("layer"):
                if l.name == name:
                    if index:
                        return l.color
                    else:
                        if (l.color == 7) and dxfBrightBackground:
                            return [0.0, 0.0, 0.0]
                        else:
                            if isinstance(l.color, int):
                                if l.color > 0:
                                    return dxfColorMap.color_map[l.color]
    return [0.0, 0.0, 0.0]


def getColor():
    """Get the Draft color defined in the Draft toolbar or preferences.

    Returns
    -------
    tuple of 4 floats
        Return the `(r, g, b, 0.0)` tuple with the colors defined
        in the Draft toolbar, if the graphical user interface is active.
        Otherwise, return the tuple with the color
        of the `DefaultShapeLineColor` in the parameter database.
    """
    if gui and draftui:
        r = float(draftui.color.red()/255.0)
        g = float(draftui.color.green()/255.0)
        b = float(draftui.color.blue()/255.0)
        return (r, g, b, 0.0)
    else:
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        c = p.GetUnsigned("DefaultShapeLineColor", 0)
        r = float(((c >> 24) & 0xFF)/255)
        g = float(((c >> 16) & 0xFF)/255)
        b = float(((c >> 8) & 0xFF)/255)
        return (r, g, b, 0.0)


def formatObject(obj, dxfobj=None):
    """Apply text and line color to an object from a DXF object.

    This function only works when the graphical user interface is loaded
    as it needs access to the `ViewObject` attribute of the objects.

    If `dxfobj` and the global variable `dxfGetColors` exist
    the `TextColor` and `LineColor` of `obj` will be set to the color
    indicated by the global dictionary
    `dxfColorMap.color_map[dxfobj.color_index]`.

    If the global `dxfBrightBackground` is set, it will set the `LineColor`
    to black.

    If no `dxfobj` is given, `TextColor` and `LineColor`
    are set to the global variable `dxfDefaultColor`.

    Parameters
    ----------
    obj : App::DocumentObject
        Object that will use the DXF color.

    dxfobj : drawing.entities, optional
        It defaults to `None`. DXF object from which the color will be taken.

    To do
    -----
    Use local variables, not global variables.
    """
    if dxfGetColors and dxfobj and hasattr(dxfobj, "color_index"):
        if hasattr(obj.ViewObject, "TextColor"):
            if dxfobj.color_index == 256:
                cm = getGroupColor(dxfobj)[:3]
            else:
                cm = dxfColorMap.color_map[dxfobj.color_index]
            obj.ViewObject.TextColor = (cm[0], cm[1], cm[2])
        elif hasattr(obj.ViewObject, "LineColor"):
            if dxfobj.color_index == 256:
                cm = getGroupColor(dxfobj)
            elif (dxfobj.color_index == 7) and dxfBrightBackground:
                cm = [0.0, 0.0, 0.0]
            else:
                cm = dxfColorMap.color_map[dxfobj.color_index]
            obj.ViewObject.LineColor = (cm[0], cm[1], cm[2], 0.0)
    else:
        if hasattr(obj.ViewObject, "TextColor"):
            obj.ViewObject.TextColor = dxfDefaultColor
        elif hasattr(obj.ViewObject, "LineColor"):
            obj.ViewObject.LineColor = dxfDefaultColor


def vec(pt):
    """Return a rounded and scaled Vector from a DXF point.

    Parameters
    ----------
    pt : Base::Vector3, or list of three numerical values, or float, or int
        A point with three coordinates `(x, y, z)`,
        or just a single numerical value.

    Returns
    -------
    Base::Vector3 or float
        Each of the components of the vector, or the single numerical value,
        is rounded to the precision defined by `prec`,
        and scaled by the amount of the global variable `dxfScaling`.

    To do
    -----
    Use local variables, not global variables.
    """
    if isinstance(pt, (int, float)):
        v = round(pt, prec())
        if dxfScaling != 1:
            v = v * dxfScaling
    else:
        v = Vector(round(pt[0], prec()),
                   round(pt[1], prec()),
                   round(pt[2], prec()))
        if dxfScaling != 1:
            v.multiply(dxfScaling)
    return v


def placementFromDXFOCS(ent):
    """Return the placement of an object from AutoCAD's OCS.

    In AutoCAD DXF's the points of each entity are expressed in terms
    of the entity's object coordinate system (OCS).
    Then to determine the entity's position in 3D space,
    what is needed is a 3D vector defining the Z axis of the OCS,
    and the elevation value over it.

    It uses `WorkingPlane.alignToPointAndAxis()` to align the working plane
    to the origin and to `ent.extrusion` (the plane's `axis`).
    Then it gets the global coordinates of the entity
    by using `WorkingPlane.getGlobalCoords()`
    and either `ent.elevation` (Z coordinate) or `ent.loc` a `(x,y,z)` tuple.

    Parameters
    ----------
    ent : A DXF entity
        It could be of several types, like `lwpolyline`, `polyline`,
        and others, and with `ent.extrusion`, `ent.elevation`
        or `ent.loc` attributes.

    Returns
    -------
    Base::Placement
        A placement, comprised of a `Base` (`Base::Vector3`),
        and a `Rotation` (`Base::Rotation`).

    See also
    --------
    WorkingPlane.alignToPointAndAxis, WorkingPlane.getGlobalCoords
    """
    draftWPlane = FreeCAD.DraftWorkingPlane
    draftWPlane.alignToPointAndAxis(Vector(0.0, 0.0, 0.0),
                                    vec(ent.extrusion), 0.0)
    # Object Coordinate Systems (OCS)
    # http://docs.autodesk.com/ACD/2011/ENU/filesDXF/WS1a9193826455f5ff18cb41610ec0a2e719-7941.htm
    # Arbitrary Axis Algorithm
    # http://docs.autodesk.com/ACD/2011/ENU/filesDXF/WS1a9193826455f5ff18cb41610ec0a2e719-793d.htm#WSc30cd3d5faa8f6d81cb25f1ffb755717d-7ff5
    # Riferimenti dell'algoritmo dell'asse arbitrario in italiano
    # http://docs.autodesk.com/ACD/2011/ITA/filesDXF/WS1a9193826455f5ff18cb41610ec0a2e719-7941.htm
    # http://docs.autodesk.com/ACD/2011/ITA/filesDXF/WS1a9193826455f5ff18cb41610ec0a2e719-793d.htm#WSc30cd3d5faa8f6d81cb25f1ffb755717d-7ff5
    if (draftWPlane.axis == FreeCAD.Vector(1.0, 0.0, 0.0)):
        draftWPlane.u = FreeCAD.Vector(0.0, 1.0, 0.0)
        draftWPlane.v = FreeCAD.Vector(0.0, 0.0, 1.0)
    elif (draftWPlane.axis == FreeCAD.Vector(-1.0, 0.0, 0.0)):
        draftWPlane.u = FreeCAD.Vector(0.0, -1.0, 0.0)
        draftWPlane.v = FreeCAD.Vector(0.0, 0.0, 1.0)
    else:
        if ((abs(ent.extrusion[0]) < (1.0 / 64.0)) and (abs(ent.extrusion[1]) < (1.0 / 64.0))):
            draftWPlane.u = FreeCAD.Vector(0.0, 1.0, 0.0).cross(draftWPlane.axis)
        else:
            draftWPlane.u = FreeCAD.Vector(0.0, 0.0, 1.0).cross(draftWPlane.axis)
        draftWPlane.u.normalize()
        draftWPlane.v = draftWPlane.axis.cross(draftWPlane.u)
        draftWPlane.v.normalize()
        draftWPlane.position = Vector(0.0, 0.0, 0.0)
        draftWPlane.weak = False

    pl = FreeCAD.Placement()
    pl = draftWPlane.getPlacement()
    if ((ent.type == "lwpolyline") or (ent.type == "polyline")):
        pl.Base = draftWPlane.getGlobalCoords(vec([0.0, 0.0, ent.elevation]))
    else:
        pl.Base = draftWPlane.getGlobalCoords(vec(ent.loc))
    return pl


def drawLine(line, forceShape=False):
    """Return a Part shape (Wire or Edge) from a DXF line.

    Parameters
    ----------
    line : drawing.entities
        The DXF object of type `'line'`.

    forceShape : bool, optional
        It defaults to `False`. If it is `True` it will produce a `Part.Edge`,
        otherwise it produces a `Draft Wire`.

    Returns
    -------
    Part::Part2DObject or Part::TopoShape ('Edge')
        The returned object is normally a `Wire`, if the global
        variables `dxfCreateDraft` or `dxfCreateSketch` are set,
        and `forceShape` is `False`.
        Otherwise it produces a `Part.Edge`.

        It returns `None` if it fails.

    See also
    --------
    drawBlock

    To do
    -----
    Use local variables, not global variables.
    """
    if len(line.points) > 1:
        v1 = vec(line.points[0])
        v2 = vec(line.points[1])
        if not DraftVecUtils.equals(v1, v2):
            try:
                if (dxfCreateDraft or dxfCreateSketch) and (not forceShape):
                    return Draft.make_wire([v1, v2])
                else:
                    return Part.LineSegment(v1, v2).toShape()
            except Part.OCCError:
                warn(line)
    return None


def drawPolyline(polyline, forceShape=False, num=None):
    """Return a Part shape (Wire, Face, or Shell) from a DXF polyline.

    It traverses the points of the polyline checking for straight edges,
    and for curvatures (bulges) between two points.
    Then it produces `Part.Edges` and `Part.Arcs`, and decides what to output
    at the end based on the options.

    Parameters
    ----------
    polyline : drawing.entities
        The DXF object of type `'polyline'` or `'lwpolyline'`.

    forceShape : bool, optional
        It defaults to `False`. If it is `True` it will try to produce
        a `Part.Wire`, otherwise it try to produce a `Draft Wire`.

    num : float, optional
        It defaults to `None`. A simple number that identifies this polyline.

    Returns
    -------
    Part::Part2DObject or Part::TopoShape ('Wire', 'Face', 'Shell')
        It returns `None` if it fails producing a shape.

    If the polyline has a `width` and the global variable
    `dxfRenderPolylineWidth` is set, it will try to return a face simulating
    a thick line. If the polyline is closed, it will cut the interior loop
    to produce the a shell.

    If the polyline doesn't have curvatures, and the global variables
    `dxfCreateDraft` or `dxfCreateSketch` are set, and `forceShape` is `False`
    it creates a straight `Draft Wire`.

    If the polyline is closed, and the global variable `dxfFillMode`
    is set, it will return a `Part.Face`, otherwise it will return
    a `Part.Wire`.

    See also
    --------
    drawBlock

    To do
    -----
    Use local variables, not global variables.
    """
    if len(polyline.points) > 1:
        edges = []
        curves = False
        verts = []
        for p in range(len(polyline.points)-1):
            p1 = polyline.points[p]
            p2 = polyline.points[p+1]
            v1 = vec(p1)
            v2 = vec(p2)
            verts.append(v1)
            if not DraftVecUtils.equals(v1, v2):
                if polyline.points[p].bulge:
                    curves = True
                    cv = calcBulge(v1, polyline.points[p].bulge, v2)
                    if DraftVecUtils.isColinear([v1, cv, v2]):
                        try:
                            edges.append(Part.LineSegment(v1, v2).toShape())
                        except Part.OCCError:
                            warn(polyline, num)
                    else:
                        try:
                            edges.append(Part.Arc(v1, cv, v2).toShape())
                        except Part.OCCError:
                            warn(polyline, num)
                else:
                    try:
                        edges.append(Part.LineSegment(v1, v2).toShape())
                    except Part.OCCError:
                        warn(polyline, num)
        verts.append(v2)
        if polyline.closed:
            p1 = polyline.points[len(polyline.points)-1]
            p2 = polyline.points[0]
            v1 = vec(p1)
            v2 = vec(p2)
            cv = calcBulge(v1, polyline.points[-1].bulge, v2)
            if not DraftVecUtils.equals(v1, v2):
                if DraftVecUtils.isColinear([v1, cv, v2]):
                    try:
                        edges.append(Part.LineSegment(v1, v2).toShape())
                    except Part.OCCError:
                        warn(polyline, num)
                else:
                    try:
                        edges.append(Part.Arc(v1, cv, v2).toShape())
                    except Part.OCCError:
                        warn(polyline, num)
        if edges:
            try:
                width = rawValue(polyline, 43)
                if width and dxfRenderPolylineWidth:
                    w = Part.Wire(edges)
                    w1 = w.makeOffset(width/2)
                    if polyline.closed:
                        w2 = w.makeOffset(-width/2)
                        w1 = Part.Face(w1)
                        w2 = Part.Face(w2)
                        if w1.BoundBox.DiagonalLength > w2.BoundBox.DiagonalLength:
                            return w1.cut(w2)
                        else:
                            return w2.cut(w1)
                    else:
                        return Part.Face(w1)
                elif (dxfCreateDraft or dxfCreateSketch) and (not curves) and (not forceShape):
                    ob = Draft.make_wire(verts)
                    ob.Closed = polyline.closed
                    ob.Placement = placementFromDXFOCS(polyline)
                    return ob
                else:
                    if polyline.closed and dxfFillMode:
                        w = Part.Wire(edges)
                        w.Placement = placementFromDXFOCS(polyline)
                        return Part.Face(w)
                    else:
                        w = Part.Wire(edges)
                        w.Placement = placementFromDXFOCS(polyline)
                        return w
            except Part.OCCError:
                warn(polyline, num)
    return None


def drawArc(arc, forceShape=False):
    """Return a Part shape (Arc, Edge) from a DXF arc.

    Parameters
    ----------
    arc : drawing.entities
        The DXF object of type `'arc'`. The `'arc'` object is different from
        a `'circle'` because it has different start and end angles.

    forceShape : bool, optional
        It defaults to `False`. If it is `True` it will try to produce
        a `Part.Edge`, otherwise it tries to produce a `Draft Arc`.

    Returns
    -------
    Part::Part2DObject or Part::TopoShape ('Edge')
        The returned object is normally a `Draft Arc` with no face,
        if the global variables `dxfCreateDraft` or `dxfCreateSketch` are set,
        and `forceShape` is `False`.
        Otherwise it produces a `Part.Edge`.

        It returns `None` if it fails producing a shape.

    See also
    --------
    drawCircle, drawBlock

    To do
    -----
    Use local variables, not global variables.
    """
    v = vec(arc.loc)
    firstangle = round(arc.start_angle, prec())
    lastangle = round(arc.end_angle, prec())
    circle = Part.Circle()
    circle.Center = v
    circle.Radius = vec(arc.radius)
    try:
        if (dxfCreateDraft or dxfCreateSketch) and (not forceShape):
            pl = placementFromDXFOCS(arc)
            return Draft.make_circle(circle.Radius, pl, face=False,
                                     startangle=firstangle,
                                     endangle=lastangle)
        else:
            return circle.toShape(math.radians(firstangle),
                                  math.radians(lastangle))
    except Part.OCCError:
        warn(arc)
    return None


def drawCircle(circle, forceShape=False):
    """Return a Part shape (Circle, Edge) from a DXF circle.

    Parameters
    ----------
    circle : drawing.entities
        The DXF object of type `'circle'`. The `'circle'` object is different
        from an `'arc'` because the circle forms a full circumference.

    forceShape : bool, optional
        It defaults to `False`. If it is `True` it will try to produce
        a `Part.Edge`, otherwise it tries to produce a `Draft Circle`.

    Returns
    -------
    Part::Part2DObject or Part::TopoShape ('Edge')
        The returned object is normally a `Draft Circle` with no face,
        if the global variables `dxfCreateDraft` or `dxfCreateSketch` are set,
        and `forceShape` is `False`.
        Otherwise it produces a `Part.Edge`.

        It returns `None` if it fails producing a shape.

    See also
    --------
    drawArc, drawBlock

    To do
    -----
    Use local variables, not global variables.
    """
    v = vec(circle.loc)
    curve = Part.Circle()
    curve.Radius = vec(circle.radius)
    curve.Center = v
    try:
        if (dxfCreateDraft or dxfCreateSketch) and (not forceShape):
            pl = placementFromDXFOCS(circle)
            return Draft.make_circle(circle.radius, pl)
        else:
            return curve.toShape()
    except Part.OCCError:
        warn(circle)
    return None


def drawEllipse(ellipse, forceShape=False):
    """Return a Part shape (Ellipse, Edge) from a DXF ellipse.

    Parameters
    ----------
    ellipse : drawing.entities
        The DXF object of type `'ellipse'`. The ellipse can be a full ellipse
        or an elliptical arc.

    forceShape : bool, optional
        It defaults to `False`. If it is `True` it will try to produce
        a `Part.Edge`, otherwise it tries to produce a `Draft Ellipse`.

    Returns
    -------
    Part::Part2DObject or Part::TopoShape ('Edge')
        The returned object is normally a `Draft Ellipse` with a face,
        if the global variables `dxfCreateDraft` or `dxfCreateSketch` are set,
        and `forceShape` is `False`.
        Otherwise it produces a `Part.Edge`.

        It returns `None` if it fails producing a shape.

    See also
    --------
    drawArc, drawCircle

    To do
    -----
    Use local variables, not global variables.
    """
    try:
        c = vec(ellipse.loc)
        start = round(ellipse.start_angle, prec())
        end = round(ellipse.end_angle, prec())
        majv = vec(ellipse.major)
        majr = majv.Length
        minr = majr*ellipse.ratio
        el = Part.Ellipse(vec((0, 0, 0)), majr, minr)
        x = majv.normalize()
        z = vec(ellipse.extrusion).normalize()
        y = z.cross(x)
        m = DraftVecUtils.getPlaneRotation(x, y)
        pl = FreeCAD.Placement(m)
        pl.move(c)
        if (dxfCreateDraft or dxfCreateSketch) and (not forceShape):
            if (start != 0.0) or ((end != 0.0) or (end != round(math.pi/2, prec()))):
                shape = el.toShape(start, end)
                shape.Placement = pl
                return shape
            else:
                return Draft.make_ellipse(majr, minr, pl)
        else:
            shape = el.toShape(start, end)
            shape.Placement = pl
            return shape
    except Part.OCCError:
        warn(arc)
    return None


def drawFace(face):
    """Return a Part face (filled) from a list of points.

    It takes the points in a `face` and places them in a list,
    then appends the first point again to the end.
    Only in this way the shape returned appears filled.

    Parameters
    ----------
    face : drawing.entities
        The DXF object of type `'3dface'`.

    Returns
    -------
    Part::TopoShape ('Face')
        The returned object is a `Part.Face`.
        It returns `None` if it fails producing a shape.
    """
    pl = []
    for p in face.points:
        pl.append(vec(p))
    p1 = face.points[0]
    pl.append(vec(p1))
    try:
        pol = Part.makePolygon(pl)
        return Part.Face(pol)
    except Part.OCCError:
        warn(face)
    return None


def drawMesh(mesh, forceShape=False):
    """Return a Mesh (Mesh, Shell) from a DXF mesh.

    Parameters
    ----------
    mesh : drawing.entities
        The DXF object of type `'polyline'` or `'lwpolyline'`
        with `flags` of 16 (3D polygon mesh) or 64 (polyface mesh).

    forceShape : bool, optional
        It defaults to `False`. If it is `True` it will try to produce
        a `Part.Shape` of type `'Shell'`,
        otherwise it tries to produce a `Mesh::MeshObject`.

    Returns
    -------
    Mesh::MeshObject or Part::TopoShape ('Shell')
        The returned object is normally a `Mesh` if `forceShape` is `False`.
        Otherwise it produces a `Part.Shape` of type `'Shell'`.

        It returns `None` if it fails producing a shape.

    See also
    --------
    drawBlock
    """
    md = []
    if mesh.flags == 16:
        pts = mesh.points
        udim = rawValue(mesh, 71)
        vdim = rawValue(mesh, 72)
        for u in range(udim-1):
            for v in range(vdim-1):
                b = u+v*udim
                p1 = pts[b]
                p2 = pts[b+1]
                p3 = pts[b+udim]
                p4 = pts[b+udim+1]
                md.append([p1, p2, p4])
                md.append([p1, p4, p3])
    elif mesh.flags == 64:
        pts = []
        fcs = []
        for p in mesh.points:
            if p.flags == 192:
                pts.append(p)
            elif p.flags == 128:
                fcs.append(p)
        # print("Creating polyface with", len(pts),
        #       "points and", len(fcs), "facets")
        for f in fcs:
            p1 = pts[abs(rawValue(f, 71)) - 1]
            p2 = pts[abs(rawValue(f, 72)) - 1]
            p3 = pts[abs(rawValue(f, 73)) - 1]
            md.append([p1, p2, p3])
            if rawValue(f, 74) is not None:
                p4 = pts[abs(rawValue(f, 74)) - 1]
                md.append([p1, p3, p4])
    try:
        m = Mesh.Mesh(md)
        if forceShape:
            s = Part.Shape()
            s.makeShapeFromMesh(m.Topology, 1)
            s = s.removeSplitter()
            m = s
    except FreeCAD.Base.FreeCADError:
        warn(mesh)
    else:
        return m
    return None


def drawSolid(solid):
    """Return a Part shape (Face) from a DXF solid.

    It takes three or four points from a `solid`, if possible.
    It adds the first point again to the end of the points list, and creates
    a polygon, which is then used to create a face.

    Parameters
    ----------
    solid : drawing.entities
        The DXF object of type `'solid'`.

    Returns
    -------
    Part::TopoShape ('Face')
        The returned object is a `Part.Face`.
        It returns `None` if it fails producing a shape.

    See also
    --------
    drawBlock
    """
    p4 = None
    p1x = rawValue(solid, 10)
    p1y = rawValue(solid, 20)
    p1z = rawValue(solid, 30) or 0
    p2x = rawValue(solid, 11)
    p2y = rawValue(solid, 21)
    p2z = rawValue(solid, 31) or p1z
    p3x = rawValue(solid, 12)
    p3y = rawValue(solid, 22)
    p3z = rawValue(solid, 32) or p1z
    p4x = rawValue(solid, 13)
    p4y = rawValue(solid, 23)
    p4z = rawValue(solid, 33) or p1z
    p1 = Vector(p1x, p1y, p1z)
    p2 = Vector(p2x, p2y, p2z)
    p3 = Vector(p3x, p3y, p3z)
    if p4x is not None:
        p4 = Vector(p4x, p4y, p4z)
    if p4 and (p4 != p3) and (p4 != p2) and (p4 != p1):
        try:
            return Part.Face(Part.makePolygon([p1, p2, p4, p3, p1]))
        except Part.OCCError:
            warn(solid)
    else:
        try:
            return Part.Face(Part.makePolygon([p1, p2, p3, p1]))
        except Part.OCCError:
            warn(solid)
    return None


def drawSplineIterpolation(verts, closed=False, forceShape=False,
                           alwaysDiscretize=False):
    """Return a wire or spline, opened or closed.

    Parameters
    ----------
    verts : Base::Vector3
        A list of points.

    closed : bool, optional
        It defaults to `False`. If it is `True` it will create a closed
        Wire, closed BSpline, or a filled Face.

    forceShape : bool, optional
        It defaults to `False`. If it is `True` it will try to produce
        a `Part.Shape` of type `'Edge'` or `'Face'`.
        Otherwise it tries to produce a `Draft Wire` or `Draft BSpline`.

    alwaysDiscretize : bool, optional
        It defaults to `False`. If it is `True` it will try to produce
        straight lines (Wires, Edges).
        Otherwise it will try to produce BSplines.

    Returns
    -------
    Part::Part2DObject or Part::TopoShape ('Edge', 'Face')
        The returned object is normally a `Draft Wire` or `Draft BSpline`,
        if the global variables `dxfCreateDraft` or `dxfCreateSketch` are set,
        and `forceShape` is `False`.
        It is a `Draft Wire` if the global variables
        `dxfDiscretizeCurves` or `alwaysDiscretize` are `True`,
        and a `Draft BSpline` otherwise.

        Otherwise it tries producing a `Part.Edge`
        (`dxfDiscretizeCurves` or `alwaysDiscretize` are `True`)
        or `Part.Face`
        if `closed` and the global variable `dxfFillMode` are `True`.

    To do
    -----
    Use local variables, not global variables.
    """
    if (dxfCreateDraft or dxfCreateSketch) and (not forceShape):
        if dxfDiscretizeCurves or alwaysDiscretize:
            ob = Draft.make_wire(verts)
        else:
            ob = Draft.make_bspline(verts)
        ob.Closed = closed
        return ob
    else:
        if dxfDiscretizeCurves or alwaysDiscretize:
            sh = Part.makePolygon(verts+[verts[0]])
        else:
            sp = Part.BSplineCurve()
            # print(knots)
            sp.interpolate(verts)
            sh = Part.Wire(sp.toShape())
        if closed and dxfFillMode:
            return Part.Face(sh)
        else:
            return sh


def drawSplineOld(spline, forceShape=False):
    """Return a Part Shape from a DXF spline. DEPRECATED.

    It takes the vertices from the spline data,
    considers the value from code 70 to know if the spline
    is closed or not, and then calls
    `drawSplineIterpolation(verts, closed, forceShape)`.

    Parameters
    ----------
    spline : drawing.entities
        The DXF object of type `'spline'`.

    forceShape : bool, optional
        It defaults to `False`. If it is `True` it will try to produce
        a `Part.Shape` of type `'Edge'` or `'Face'`.
        Otherwise it tries to produce a `Draft Wire` or `Draft BSpline`.

    Returns
    -------
    Part::Part2DObject or Part::TopoShape ('Edge', 'Face')
        The returned object is normally a `Draft Wire` or `Draft BSpline`
        as returned from `drawSplineIterpolation()`.

        It returns `None` if it fails producing a shape.

    See also
    --------
    drawSplineIterpolation
    """
    flag = rawValue(spline, 70)
    if flag == 1:
        closed = True
    else:
        closed = False
    verts = []
    knots = []
    for dline in spline.data:
        if dline[0] == 10:
            cp = [dline[1]]
        elif dline[0] == 20:
            cp.append(dline[1])
        elif dline[0] == 30:
            cp.append(dline[1])
            pt = vec(cp)
            if verts:
                if pt != verts[-1]:
                    verts.append(pt)
            else:
                verts.append(pt)
        elif dline[0] == 40:
            knots.append(dline[1])
    try:
        return drawSplineIterpolation(verts, closed, forceShape)
    except Part.OCCError:
        warn(spline)
    return None


def drawSpline(spline, forceShape=False):
    """Return a Part Shape (BSpline, Wire) from a DXF spline.

    A BSpline may be defined in several ways, by knots,
    control points, fit points, and weights.
    The function searches all values to determine the best way
    of building the BSpline with Draft or Part tools.

    Parameters
    ----------
    spline : drawing.entities
        The DXF object of type `'spline'`.

    forceShape : bool, optional
        It defaults to `False`. If it is `True` it will try to produce
        a `Part.Shape` of type `'Wire'`.
        Otherwise it tries to produce a `Draft BSpline`.

    Returns
    -------
    Part::Part2DObject or Part::TopoShape ('Edge', 'Face')
        The returned object is normally a `Draft BezCurve`
        created with `Draft.make_bezcurve(controlpoints, Degree=degree)`,
        if `forceShape` is `False` and there are no weights.

        Otherwise it tries to return a `Part.Shape` of type `'Wire'`,
        by first creating a Bezier curve with `Part.BezierCurve()`.

        If it's impossible to create the BSpline in this way,
        it will try to create an interpolated BSpline with
        `drawSplineIterpolation(controlpoints)`.

        If fit points exist and control points do not,
        it will try to create an interpolated BSpline with
        `drawSplineIterpolation(fitpoints)`.

        In other cases it will try to create a `Part.Shape`
        from a BSpline, using the available control points,
        multiplicity vector, the kot vector, the degree,
        the periodic data, and the weights.

        It returns `None` if it fails producing a shape.

    Raises
    ------
    ValueError
        If there are wrong number of knots, wrong number of control points,
        wrong number of fit points, an inconsistent rational flag, or wrong
        number of weights.

    See also
    --------
    drawBlock, Draft.make_bezcurve, Part.BezierCurve, drawSplineIterpolation,
    Part.BSplineCurve.buildFromPolesMultsKnots

    To do
    ----
    As there is currently no Draft primitive to handle splines
    the result is a non-parametric curve.

    **2019:** There is a `Draft BSpline` now, but it's not used.
    """
    flags = rawValue(spline, 70)
    closed = (flags & 1) != 0
    periodic = (flags & 2) != 0 and False  # workaround
    rational = (flags & 4) != 0
    planar = (flags & 8) != 0
    linear = (flags & 16) != 0
    degree = rawValue(spline, 71)
    nbknots = rawValue(spline, 72) or 0
    nbcontrolp = rawValue(spline, 73) or 0
    nbfitp = rawValue(spline, 74) or 0
    knots = []
    weights = []
    controlpoints = []
    fitpoints = []
    # parse the knots and points
    dataremain = spline.data[:]
    while len(dataremain) > 0:
        groupnumber = dataremain[0][0]
        if groupnumber == 40:  # knot
            knots.append(dataremain[0][1])
            dataremain = dataremain[1:]
        elif groupnumber == 41:  # weight
            weights.append(dataremain[0][1])
            dataremain = dataremain[1:]
        elif groupnumber in (10, 11):  # control or fit point
            x = dataremain[0][1]
            if dataremain[1][0] in (20, 21):
                y = dataremain[1][1]
                if dataremain[2][0] in (30, 31):
                    z = dataremain[2][1]
                    dataremain = dataremain[3:]
                else:
                    z = 0.0
                    dataremain = dataremain[2:]
            else:
                y = 0.0
                dataremain = dataremain[1:]
            v = vec([x, y, z])
            if groupnumber == 10:
                controlpoints.append(v)
            elif groupnumber == 11:
                fitpoints.append(v)
        else:
            dataremain = dataremain[1:]
            # print(groupnumber)

    if nbknots != len(knots):
        raise ValueError('Wrong number of knots')
    if nbcontrolp != len(controlpoints):
        raise ValueError('Wrong number of control points')
    if nbfitp != len(fitpoints):
        raise ValueError('Wrong number of fit points')
    if rational == all((w == 1.0 or w is None) for w in weights):
        raise ValueError('inconsistant rational flag')
    if len(weights) == 0:
        weights = None
    elif len(weights) != len(controlpoints):
        raise ValueError('Wrong number of weights')

    # build knotvector and multvector
    # this means to remove duplicate knots
    multvector = []
    knotvector = []
    mult = 0
    previousknot = None
    for knotvalue in knots:
        if knotvalue == previousknot:
            mult += 1
        else:
            if mult > 0:
                multvector.append(mult)
            mult = 1
            previousknot = knotvalue
            knotvector.append(knotvalue)
    multvector.append(mult)
    # check if the multiplicities are valid
    innermults = multvector[:] if periodic else multvector[1:-1]
    if any(m > degree for m in innermults):  # invalid
        if all(m == degree+1 for m in multvector):
            if not forceShape and weights is None:
                points = controlpoints[:]
                del points[degree+1::degree+1]
                return Draft.make_bezcurve(points, Degree=degree)
            else:
                poles = controlpoints[:]
                edges = []
                while len(poles) >= degree+1:
                    # bezier segments
                    bzseg = Part.BezierCurve()
                    bzseg.increase(degree)
                    bzseg.setPoles(poles[0:degree+1])
                    poles = poles[degree+1:]
                    if weights is not None:
                        bzseg.setWeights(weights[0:degree+1])
                        weights = weights[degree+1:]
                    edges.append(bzseg.toShape())
                return Part.Wire(edges)
        else:
            warn('polygon fallback on %s' % spline)
            return drawSplineIterpolation(controlpoints, closed=closed,
                                          forceShape=forceShape,
                                          alwaysDiscretize=True)
    if fitpoints and not controlpoints:
        return drawSplineIterpolation(fitpoints, closed=closed,
                                      forceShape=forceShape)
    try:
        bspline = Part.BSplineCurve()
        bspline.buildFromPolesMultsKnots(poles=controlpoints,
                                         mults=multvector,
                                         knots=knotvector,
                                         degree=degree,
                                         periodic=periodic,
                                         weights=weights)
        return bspline.toShape()
    except Part.OCCError:
        warn(spline)
    return None


def drawBlock(blockref, num=None, createObject=False):
    """Return a Part Shape (Compound) from a DXF block reference.

    It inspects the `blockref.entities` for objects of types `'line'`,
    `'polyline'`, `'lwpolyline'`, `'arc'`, `'circle'`, `'insert'`,
    `'solid'`, and `'spline'`.
    If they are found they create shapes with `drawLine`,
    `drawMesh` or `drawPolyline`, `drawArc`, `drawCircle`, `drawInsert`,
    `drawSolid`, `drawSpline`, and adds all shapes to a list.
    Then it makes a compound of all those shapes.

    In the case of entities of type `'text'` and `'mtext'`
    it will only process the entities if the global variable
    `dxfImportTexts` exist, and `dxfImportLayouts` exists
    or if the DXF code 67 doesn't indicate an empty space (empty text).
    Then it will use `addText` and add the found text to its proper
    layer.

    Parameters
    ----------
    blockref : drawing.blocks.data
        The DXF block data.

    num : float, optional
        It defaults to `None`. A simple number that identifies
        the given `blockref`.

    createObject : bool, optional
        It defaults to `False`. If it is `True` it will try to produce
        and return a `'Part::Feature'` with the compound
        as its shape attribute.
        Otherwise, just return the `Part.Compound`.

    Returns
    -------
    Part::TopoShape ('Compound') or Part::Feature
        The returned object is normally a `Part.Compound`
        created from the list of all `Part.Shapes` created from
        the `blockref` entities, if `createObject` is `False`.
        Otherwise, it will return a `'Part::Feature'` document object
        with the compound as its shape attribute.

        In the first case, it will add the compound shape
        to the global dictionary `blockshapes`.
        In the latter case, it will add the `'Part::Feature'` object
        to the global dictionary `blockobjects`.

        It returns `None` if the global variable `dxfStarBlocks`
        doesn't exist, if the `blockref.entities.data` is empty,
        or if it fails producing the compound shape.

    See also
    --------
    `drawLine`, `drawMesh`, `drawPolyline`, `drawArc`, `drawCircle`,
    `drawInsert`, `drawSolid`, `drawSpline`, `addText`.

    To do
    -----
    Use local variables, not global variables.
    """
    if not dxfStarBlocks:
        if blockref.name[0] == '*':
            return None
    if len(blockref.entities.data) == 0:
        print("skipping empty block ", blockref.name)
        return None
    # print("creating block ", blockref.name,
    #       " containing ", len(blockref.entities.data), " entities")
    shapes = []
    for line in blockref.entities.get_type('line'):
        s = drawLine(line, forceShape=True)
        if s:
            shapes.append(s)
    for polyline in blockref.entities.get_type('polyline'):
        if hasattr(polyline, "flags") and polyline.flags in [16, 64]:
            s = drawMesh(polyline, forceShape=True)
        else:
            s = drawPolyline(polyline, forceShape=True)
        if s:
            shapes.append(s)
    for polyline in blockref.entities.get_type('lwpolyline'):
        s = drawPolyline(polyline, forceShape=True)
        if s:
            shapes.append(s)
    for arc in blockref.entities.get_type('arc'):
        s = drawArc(arc, forceShape=True)
        if s:
            shapes.append(s)
    for circle in blockref.entities.get_type('circle'):
        s = drawCircle(circle, forceShape=True)
        if s:
            shapes.append(s)
    for insert in blockref.entities.get_type('insert'):
        # print("insert ",insert," in block ",insert.block[0])
        if dxfStarBlocks or insert.block[0] != '*':
            s = drawInsert(insert)
            if s:
                shapes.append(s)
    for solid in blockref.entities.get_type('solid'):
        s = drawSolid(solid)
        if s:
            shapes.append(s)
    for spline in blockref.entities.get_type('spline'):
        s = drawSpline(spline, forceShape=True)
        if s:
            shapes.append(s)
    for text in blockref.entities.get_type('text'):
        if dxfImportTexts:
            if dxfImportLayouts or (not rawValue(text, 67)):
                addText(text)
    for text in blockref.entities.get_type('mtext'):
        if dxfImportTexts:
            if dxfImportLayouts or (not rawValue(text, 67)):
                print("adding block text", text.value, " from ", blockref)
                addText(text)
    try:
        shape = Part.makeCompound(shapes)
    except Part.OCCError:
        warn(blockref)
    if shape:
        blockshapes[blockref.name] = shape
        if createObject:
            newob = doc.addObject("Part::Feature", blockref.name)
            newob.Shape = shape
            blockobjects[blockref.name] = newob
            return newob
        return shape
    return None


def drawInsert(insert, num=None, clone=False):
    """Return a Part Shape (Compound, Clone) from a DXF insert.

    It searches for `insert.block` in `blockobjects`
    or `blockshapes`, and returns a clone or a copy of the compound,
    with transformations applied: rotation, translation (movement),
    and scaling.

    If the global variable `dxfImportTexts` is available
    it will check the attributes of `insert` and add those text attributes
    to their own layers with `addText`.

    Parameters
    ----------
    insert : drawing.entities
        The DXF object of type `'insert'`.

    num : float, optional
        It defaults to `None`. A simple number that identifies
        the given block being drawn, if it is not a clone.

    clone : bool, optional
        It defaults to `False`. If it is `True` it will try to produce
        and return a `Draft Clone` of the `'insert.block'` contained
        in the global dictionary `blockobjects`.

        Otherwise, it will try to return a copy of the shape
        of the `'insert.block'` contained in the global dictionary
        `blockshapes`, or created from the `drawing.blocks.data`
        with `drawBlock()`.

    Returns
    -------
    Part::TopoShape ('Compound') or
    Part::Part2DObject or Part::PartFeature (`Draft Clone`)
        The returned object is normally a copy of the `Part.Compound`
        extracted from `blockshapes` or created with `drawBlock()`.

        If `clone` is `True` then it will try returning
        a `Draft Clone` from the `'insert.block'` contained
        in the global dictionary `blockobjects`.
        It returns `None` if `insert.block` isn't in `blockobjects`.

        In any of these two cases, it will try to apply the
        insert transformations: rotation, translation (movement),
        and scaling.

    See also
    --------
    drawBlock

    To do
    -----
    Use local variables, not global variables.
    """
    if dxfImportTexts:
        attrs = attribs(insert)
        for a in attrs:
            addText(a, attrib=True)
    if clone:
        if insert.block in blockobjects:
            newob = Draft.make_clone(blockobjects[insert.block])
            tsf = FreeCAD.Matrix()
            rot = math.radians(insert.rotation)
            pos = vec(insert.loc)
            tsf.move(pos)
            tsf.rotateZ(rot)
            sc = insert.scale
            sc = vec([sc[0], sc[1], 0])
            newob.Placement = FreeCAD.Placement(tsf)
            newob.Scale = sc
            return newob
        else:
            shape = None
    else:
        if insert in blockshapes:
            shape = blockshapes[insert.block].copy()
        else:
            shape = None
            for b in drawing.blocks.data:
                if b.name == insert.block:
                    shape = drawBlock(b, num)
        if shape:
            pos = vec(insert.loc)
            rot = math.radians(insert.rotation)
            scale = insert.scale
            tsf = FreeCAD.Matrix()
            # for some reason z must be 0 to work
            # tsf.scale(scale[0], scale[1], 0)
            tsf.rotateZ(rot)
            try:
                shape = shape.transformGeometry(tsf)
            except Part.OCCError:
                tsf.scale(scale[0], scale[1], 0)
                try:
                    shape = shape.transformGeometry(tsf)
                except Part.OCCError:
                    print("importDXF: unable to apply insert transform:", tsf)
            shape.translate(pos)
            return shape
    return None


def drawLayerBlock(objlist):
    """Return a Draft Block (compound) from the given object list.

    Parameters
    ----------
    objlist : list
        A list of Draft objects or Part.shapes.

    Returns
    -------
    Part::Part2DObject or Part::TopoShape ('Compound')
        If the global variables `dxfCreateDraft` or `dxfCreateSketch` are set,
        and no element in `objlist` is a `Part.Shape`,
        it will try to return a `Draft Block`.
        Otherwise, it will try to return a `Part.Compound`.

        It returns `None` if it fails producing a shape.

    To do
    -----
    Use local variables, not global variables.
    """
    isObj = True
    for o in objlist:
        if isinstance(o, Part.Shape):
            isObj = False
    obj = None
    if (dxfCreateDraft or dxfCreateSketch) and isObj:
        try:
            obj = Draft.make_block(objlist)
        except Part.OCCError:
            pass
    else:
        try:
            obj = Part.makeCompound(objlist)
        except Part.OCCError:
            pass
    return obj


def attribs(insert):
    """Check if an insert has attributes, and return the values if positive.

    It checks the `drawing.entities.data` for the `insert`,
    and saves the index of the element.
    Then it iterates again looking for entities with an `'attrib'`,
    collecting the entities in a list.

    Parameters
    ----------
    insert : drawing.entities
        The DXF object of type `'insert'`.

    Returns
    -------
    list
        It returns a list with the entities that have `'attrib'` data,
        until `'seqend'` is found.

        It returns an empty list `[]`, if DXF code 66 ("Entities follow")
        is different from 1, or if the `insert` is not found
        in `drawing.entities.data`.
    """
    atts = []
    if rawValue(insert, 66) != 1:
        return []
    index = None
    for i in range(len(drawing.entities.data)):
        if drawing.entities.data[i] == insert:
            index = i
            break
    if index is None:
        return []
    j = index+1
    while True:
        ent = drawing.entities.data[j]
        if str(ent) == 'seqend':
            return atts
        elif str(ent) == 'attrib':
            atts.append(ent)
            j += 1


def addObject(shape, name="Shape", layer=None):
    """Adds a new object to the document, with the given name and layer.

    Parameters
    ----------
    shape : Part.Shape or Part::Feature
        The simple Part.Shape or Draft object previously created
        from an entity in a DXF file.

    name : str, optional
        It defaults to "Shape". The name of the new document object.

    layer : App::FeaturePython or App::DocumentObjectGroup, optional
        It defaults to `None`.
        The `Draft Layer` (`App::FeaturePython`)
        or simple group (`App::DocumentObjectGroup`)
        to which the new object will be added.

    Returns
    -------
    Part::Feature or Part::Part2DObject
        If the `shape` is a simple `Part.Shape`, it will be encapsulated
        inside a `Part::Feature` object and this will be returned.
        Otherwise, it is assumed it is already a Draft object
        (`Part::Part2DObject`) and will just return this.

        It applies the text and line color by calling `formatObject()`
        before returning the new object.
    """
    if isinstance(shape, Part.Shape):
        newob = doc.addObject("Part::Feature", name)
        newob.Shape = shape
    else:
        newob = shape
    if layer:
        lay = locateLayer(layer)
        # For old style layers, which are just groups
        if hasattr(lay, "addObject"):
            lay.addObject(newob)
        # For new Draft Layers
        elif hasattr(lay, "Proxy") and hasattr(lay.Proxy, "addObject"):
            lay.Proxy.addObject(lay, newob)
    formatObject(newob)
    return newob


def addText(text, attrib=False):
    """Add a new Draft Text object to the document.

    It creates a `Draft Text` from the `text` entity,
    and adds the new object to its indicated layer,
    creating it if it doesn't exist.
    It also applies its rotation, position, justification
    ('center' or 'right'), and color.

    If the graphical interface is available, together with the Draft toolbar,
    as well as the global variable `dxfUseStandardSize`, it will
    use the toolbar's indicated font size.
    Otherwise, it will use the text's height scaled by the value of
    the global variable `TEXTSCALING`.

    Parameters
    ----------
    text : drawing.entities
        The DXF object of type `'text'` or `'mtext'`.

    attrib : bool, optional
        It defaults to `False`. If `True` it determines
        the layer name from the DXF code 8, the text value from code 1,
        the position from codes 10, 20, 30, the height from code 40,
        the rotation from code 50, and assigns the name `'Attribute'`.
        Otherwise, it assumes these values from `text`
        and sets the name to `'Text'`.

    See also
    --------
    locateLayer, drawBlock, Draft.make_text

    To do
    -----
    Use local variables, not global variables.
    """
    if attrib:
        lay = locateLayer(rawValue(text, 8))
        val = rawValue(text, 1)
        pos = vec([rawValue(text, 10),
                   rawValue(text, 20),
                   rawValue(text, 30)])
        hgt = vec(rawValue(text, 40))
    else:
        lay = locateLayer(text.layer)
        val = text.value
        pos = vec(text.loc)
        hgt = vec(text.height)
    if val:
        if attrib:
            name = "Attribute"
        else:
            name = "Text"
        val = deformat(val)
        newob = Draft.make_text(val.split("\n"))
        if hasattr(lay, "addObject"):
            lay.addObject(newob)
        elif hasattr(lay, "Proxy") and hasattr(lay.Proxy, "addObject"):
            lay.Proxy.addObject(lay, newob)
        rx = rawValue(text, 11)
        ry = rawValue(text, 21)
        rz = rawValue(text, 31)
        xv = Vector(1, 0, 0)
        ax = Vector(0, 0, 1)
        if rx or ry or rz:
            xv = vec([rx, ry, rz])
            if not DraftVecUtils.isNull(xv):
                ax = (xv.cross(Vector(1, 0, 0))).negative()
                if DraftVecUtils.isNull(ax):
                    ax = Vector(0, 0, 1)
                ang = -math.degrees(DraftVecUtils.angle(xv,
                                                        Vector(1, 0, 0), ax))
                Draft.rotate(newob, ang, axis=ax)
            if ax == Vector(0, 0, -1):
                ax = Vector(0, 0, 1)
        elif hasattr(text, "rotation"):
            if text.rotation:
                Draft.rotate(newob, text.rotation)
        if attrib:
            attrot = rawValue(text, 50)
            if attrot:
                Draft.rotate(newob, attrot)
        if gui and draftui and dxfUseStandardSize:
            fsize = draftui.fontsize
        else:
            fsize = float(hgt) * TEXTSCALING
        if hasattr(text, "alignment"):
            yv = ax.cross(xv)
            if text.alignment in [1, 2, 3]:
                sup = DraftVecUtils.scaleTo(yv, fsize/TEXTSCALING).negative()
                # print(ax, sup)
                pos = pos.add(sup)
            elif text.alignment in [4, 5, 6]:
                sup = DraftVecUtils.scaleTo(yv, fsize/(2*TEXTSCALING)).negative()
                pos = pos.add(sup)
        newob.Placement.Base = pos
        if gui:
            newob.ViewObject.FontSize = fsize
            if hasattr(text, "alignment"):
                if text.alignment in [2, 5, 8]:
                    newob.ViewObject.Justification = "Center"
                elif text.alignment in [3, 6, 9]:
                    newob.ViewObject.Justification = "Right"
            # newob.ViewObject.DisplayMode = "World"
            formatObject(newob, text)


def addToBlock(obj, layer):
    """Add the given object to the layer in the global dictionary.

    It searches for `layer` in the global dictionary `layerBlocks`.
    If found, it appends the `obj` to the `layer`;
    otherwise, it adds the `layer` to `layerBlocks` first,
    and then adds `obj`.

    Parameters
    ----------
    obj : Part.Shape or App::DocumentObject
        Any shape or Draft object previously created from a DXF file.
    layer : str
        The name of a layer to which `obj` is added.

    To do
    -----
    Use local variables, not global variables.
    """
    if layer in layerBlocks:
        layerBlocks[layer].append(obj)
    else:
        layerBlocks[layer] = [obj]


def processdxf(document, filename, getShapes=False, reComputeFlag=True):
    """Process the DXF file, creating Part objects in the document.

    If the `dxfReader` module is not available run `getDXFlibs()`
    to get the required libraries and `readPreferences()`.

    It defines the global variables `drawing`, `layers`, `doc`,
    `blockshapes`, `blockobjects`, `badobjects`, `layerBlocks`.
    The read data is placed in the object `drawing`.

    It iterates over `drawing.tables` to find tables of type `'layer'`,
    and adds them to the document considering its color and drawing style.
    Then it iterates over the `drawing.entities` processing the most common
    drawing types, that include `'line'`, `'lwpolyline'`, `'polyline'`,
    `'arc'`, `'circle'`, `'solid'`, `'spline'`, `'ellipse'`, `'mtext'`,
    `'text'`, and `'3dface'`.
    If `getShapes` is `False` it will additionally process the types
    `'dimension'`, `'point'`, `'leader'`, `'hatch'`, and `'insert'`.

    Parameters
    ----------
    document : App::Document
        A document object opened in which to create the new Part shapes.

    filename : str
        The path to the DXF file to process.

    getShapes : bool, optional
        It defaults to `False`. If it is `True` it will try creating
        simple `Part Shapes` instead of Draft objects,
        and will immediately return the list of the most common shapes
        without processing the entities of types `'dimension'`, `'point'`,
        `'leader'`, `'hatch'`, and `'insert'`.

    reComputeFlag : bool, optional
        It defaults to `True`, in which case it recomputes the document
        after finishing processing of the entities.
        Otherwise, it skips the recompute.

        The recompute causes OpenSCAD import to loop, so this flag
        can be set to `False` to prevent this.

    Returns
    -------
    list of `Part.Shapes`
        It returns `None` if the edges (lines, polylines, arcs)
        are above 100, and the user decides to interrupt (graphically)
        the process of joining them.

    To do
    -----
    Use local variables, not global variables.
    """
    # for debugging the drawing variable is global so it is still accessible
    # after running the script
    global drawing
    if not dxfReader:
        getDXFlibs()
        readPreferences()
    FCC.PrintMessage("opening " + filename + "...\n")
    drawing = dxfReader.readDXF(filename)
    global layers
    layers = []
    global doc
    doc = document
    global blockshapes
    blockshapes = {}
    global blockobjects
    blockobjects = {}
    global badobjects
    badobjects = []
    global layerBlocks
    layerBlocks = {}
    sketch = None
    shapes = []

    # Create layers
    if hasattr(drawing, "tables"):
        for table in drawing.tables.get_type("table"):
            for layer in table.get_type("layer"):
                name = layer.name
                color = tuple(dxfColorMap.color_map[layer.color])
                drawstyle = "Solid"
                lt = rawValue(layer, 6)
                if "DASHED" in lt.upper():
                    drawstyle = "Dashed"
                elif "HIDDEN" in lt.upper():
                    drawstyle = "Dotted"
                if ("DASHDOT" in lt.upper()) or ("CENTER" in lt.upper()):
                    drawstyle = "Dashdot"
                locateLayer(name, color, drawstyle)
    else:
        locateLayer("0", (0.0, 0.0, 0.0), "Solid")

     # Draw lines
    lines = drawing.entities.get_type("line")
    if lines:
        FCC.PrintMessage("drawing " + str(len(lines)) + " lines...\n")
    for line in lines:
        if dxfImportLayouts or (not rawValue(line, 67)):
            shape = drawLine(line)
            if shape:
                if dxfCreateSketch:
                    FreeCAD.ActiveDocument.recompute()
                    if dxfMakeBlocks or dxfJoin:
                        if sketch:
                            shape = Draft.make_sketch(shape,
                                                      autoconstraints=True,
                                                      addTo=sketch)
                        else:
                            shape = Draft.make_sketch(shape,
                                                      autoconstraints=True)
                            sketch = shape
                    else:
                        shape = Draft.make_sketch(shape,
                                                  autoconstraints=True)
                elif dxfJoin or getShapes:
                    if isinstance(shape, Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                elif dxfMakeBlocks:
                    addToBlock(shape, line.layer)
                else:
                    newob = addObject(shape, "Line", line.layer)
                    if gui:
                        formatObject(newob, line)

    # Draw polylines
    pls = drawing.entities.get_type("lwpolyline")
    pls.extend(drawing.entities.get_type("polyline"))
    polylines = []
    meshes = []
    for p in pls:
        if hasattr(p, "flags"):
            if p.flags in [16, 64]:
                meshes.append(p)
            else:
                polylines.append(p)
        else:
            polylines.append(p)
    if polylines:
        FCC.PrintMessage("drawing " + str(len(polylines)) + " polylines...\n")
    num = 0
    for polyline in polylines:
        if dxfImportLayouts or (not rawValue(polyline, 67)):
            shape = drawPolyline(polyline, num)
            if shape:
                if dxfCreateSketch:
                    if isinstance(shape, Part.Shape):
                        t = FreeCAD.ActiveDocument.addObject("Part::Feature",
                                                             "Shape")
                        t.Shape = shape
                        shape = t
                    FreeCAD.ActiveDocument.recompute()
                    if dxfMakeBlocks or dxfJoin:
                        if sketch:
                            shape = Draft.make_sketch(shape,
                                                      autoconstraints=True,
                                                      addTo=sketch)
                        else:
                            shape = Draft.make_sketch(shape,
                                                      autoconstraints=True)
                            sketch = shape
                    else:
                        shape = Draft.make_sketch(shape,
                                                  autoconstraints=True)
                elif dxfJoin or getShapes:
                    if isinstance(shape, Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                elif dxfMakeBlocks:
                    addToBlock(shape, polyline.layer)
                else:
                    newob = addObject(shape, "Polyline", polyline.layer)
                    if gui:
                        formatObject(newob, polyline)
            num += 1

    # Draw arcs
    arcs = drawing.entities.get_type("arc")
    if arcs:
        FCC.PrintMessage("drawing " + str(len(arcs)) + " arcs...\n")
    for arc in arcs:
        if dxfImportLayouts or (not rawValue(arc, 67)):
            shape = drawArc(arc)
            if shape:
                if dxfCreateSketch:
                    FreeCAD.ActiveDocument.recompute()
                    if dxfMakeBlocks or dxfJoin:
                        if sketch:
                            shape = Draft.make_sketch(shape,
                                                      autoconstraints=True,
                                                      addTo=sketch)
                        else:
                            shape = Draft.make_sketch(shape,
                                                      autoconstraints=True)
                            sketch = shape
                    else:
                        shape = Draft.make_sketch(shape,
                                                  autoconstraints=True)
                elif dxfJoin or getShapes:
                    if isinstance(shape, Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                elif dxfMakeBlocks:
                    addToBlock(shape, arc.layer)
                else:
                    newob = addObject(shape, "Arc", arc.layer)
                    if gui:
                        formatObject(newob, arc)

    # Join lines, polylines and arcs if needed
    if dxfJoin and shapes:
        FCC.PrintMessage("Joining geometry...\n")
        edges = []
        for s in shapes:
            edges.extend(s.Edges)
        if len(edges) > (100):
            FCC.PrintMessage(str(len(edges)) + " edges to join\n")
            if FreeCAD.GuiUp:
                d = QtGui.QMessageBox()
                d.setText("Warning: High number of entities to join (>100)")
                d.setInformativeText("This might take a long time "
                                     "or even freeze your computer. "
                                     "Are you sure? You can also disable "
                                     "the 'join geometry' setting in DXF "
                                     "import preferences")
                d.setStandardButtons(QtGui.QMessageBox.Ok
                                     | QtGui.QMessageBox.Cancel)
                d.setDefaultButton(QtGui.QMessageBox.Cancel)
                res = d.exec_()
                if res == QtGui.QMessageBox.Cancel:
                    FCC.PrintMessage("Aborted\n")
                    return
        shapes = DraftGeomUtils.findWires(edges)
        for s in shapes:
            newob = addObject(s)

    # Draw circles
    circles = drawing.entities.get_type("circle")
    if circles:
        FCC.PrintMessage("drawing " + str(len(circles))+" circles...\n")
    for circle in circles:
        if dxfImportLayouts or (not rawValue(circle, 67)):
            shape = drawCircle(circle)
            if shape:
                if dxfCreateSketch:
                    FreeCAD.ActiveDocument.recompute()
                    if dxfMakeBlocks or dxfJoin:
                        if sketch:
                            shape = Draft.make_sketch(shape,
                                                      autoconstraints=True,
                                                      addTo=sketch)
                        else:
                            shape = Draft.make_sketch(shape,
                                                      autoconstraints=True)
                            sketch = shape
                    else:
                        shape = Draft.make_sketch(shape,
                                                  autoconstraints=True)
                elif dxfMakeBlocks:
                    addToBlock(shape, circle.layer)
                elif getShapes:
                    if isinstance(shape, Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                else:
                    newob = addObject(shape, "Circle", circle.layer)
                    if gui:
                        formatObject(newob, circle)

    # Draw solids
    solids = drawing.entities.get_type("solid")
    if solids:
        FCC.PrintMessage("drawing " + str(len(solids)) + " solids...\n")
    for solid in solids:
        lay = rawValue(solid, 8)
        if dxfImportLayouts or (not rawValue(solid, 67)):
            shape = drawSolid(solid)
            if shape:
                if dxfMakeBlocks:
                    addToBlock(shape, lay)
                elif getShapes:
                    if isinstance(shape, Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                else:
                    newob = addObject(shape, "Solid", lay)
                    if gui:
                        formatObject(newob, solid)

    # Draw splines
    splines = drawing.entities.get_type("spline")
    if splines:
        FCC.PrintMessage("drawing " + str(len(splines)) + " splines...\n")
    for spline in splines:
        lay = rawValue(spline, 8)
        if dxfImportLayouts or (not rawValue(spline, 67)):
            shape = drawSpline(spline)
            if shape:
                if dxfMakeBlocks:
                    addToBlock(shape, lay)
                elif getShapes:
                    if isinstance(shape, Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                else:
                    newob = addObject(shape, "Spline", lay)
                    if gui:
                        formatObject(newob, spline)

    # Draw ellipses
    ellipses = drawing.entities.get_type("ellipse")
    if ellipses:
        FCC.PrintMessage("drawing " + str(len(ellipses)) + " ellipses...\n")
    for ellipse in ellipses:
        lay = rawValue(ellipse, 8)
        if dxfImportLayouts or (not rawValue(ellipse, 67)):
            shape = drawEllipse(ellipse)
            if shape:
                if dxfMakeBlocks:
                    addToBlock(shape, lay)
                elif getShapes:
                    if isinstance(shape, Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                else:
                    newob = addObject(shape, "Ellipse", lay)
                    if gui:
                        formatObject(newob, ellipse)

    # Draw texts
    if dxfImportTexts:
        texts = drawing.entities.get_type("mtext")
        texts.extend(drawing.entities.get_type("text"))
        if texts:
            FCC.PrintMessage("drawing " + str(len(texts)) + " texts...\n")
        for text in texts:
            if dxfImportLayouts or (not rawValue(text, 67)):
                addText(text)
    else:
        FCC.PrintMessage("skipping texts...\n")

    # Draw 3D objects
    faces3d = drawing.entities.get_type("3dface")
    if faces3d:
        FCC.PrintMessage("drawing " + str(len(faces3d)) + " 3dfaces...\n")
    for face3d in faces3d:
        shape = drawFace(face3d)
        if shape:
            if getShapes:
                if isinstance(shape, Part.Shape):
                    shapes.append(shape)
                else:
                    shapes.append(shape.Shape)
            else:
                newob = addObject(shape, "Face", face3d.layer)
                if gui:
                    formatObject(newob, face3d)
    if meshes:
        FCC.PrintMessage("drawing " + str(len(meshes)) + " 3dmeshes...\n")
    for mesh in meshes:
        me = drawMesh(mesh)
        if me:
            newob = doc.addObject("Mesh::Feature", "Mesh")
            lay = locateLayer(rawValue(mesh, 8))
            lay.addObject(newob)
            newob.Mesh = me
            if gui:
                formatObject(newob, mesh)

    # End of shape-based objects, return if we are just getting shapes
    if getShapes and shapes:
        return shapes

    # Draw dimensions
    if dxfImportTexts:
        dims = drawing.entities.get_type("dimension")
        FCC.PrintMessage("drawing " + str(len(dims)) + " dimensions...\n")
        for dim in dims:
            if dxfImportLayouts or (not rawValue(dim, 67)):
                try:
                    layer = rawValue(dim, 8)
                    if rawValue(dim, 15) is not None:
                        # this is a radial or diameter dimension
                        # x1 = float(rawValue(dim,11))
                        # y1 = float(rawValue(dim,21))
                        # z1 = float(rawValue(dim,31))
                        x2 = float(rawValue(dim, 10))
                        y2 = float(rawValue(dim, 20))
                        z2 = float(rawValue(dim, 30))
                        x3 = float(rawValue(dim, 15))
                        y3 = float(rawValue(dim, 25))
                        z3 = float(rawValue(dim, 35))
                        x1 = x2
                        y1 = y2
                        z1 = z2
                    else:
                        x1 = float(rawValue(dim, 10))
                        y1 = float(rawValue(dim, 20))
                        z1 = float(rawValue(dim, 30))
                        x2 = float(rawValue(dim, 13))
                        y2 = float(rawValue(dim, 23))
                        z2 = float(rawValue(dim, 33))
                        x3 = float(rawValue(dim, 14))
                        y3 = float(rawValue(dim, 24))
                        z3 = float(rawValue(dim, 34))
                    d = rawValue(dim, 70)
                    if d:
                        align = int(d)
                    else:
                        align = 0
                    d = rawValue(dim, 50)
                    if d:
                        angle = float(d)
                    else:
                        angle = 0
                except (ValueError, TypeError):
                    warn(dim)
                else:
                    lay = locateLayer(layer)
                    pt = vec([x1, y1, z1])
                    p1 = vec([x2, y2, z2])
                    p2 = vec([x3, y3, z3])
                    if align >= 128:
                        align -= 128
                    elif align >= 64:
                        align -= 64
                    elif align >= 32:
                        align -= 32
                    if align == 0:
                        if angle in [0, 180]:
                            p2 = vec([x3, y2, z2])
                        elif angle in [90, 270]:
                            p2 = vec([x2, y3, z2])
                    newob = doc.addObject("App::FeaturePython", "Dimension")
                    lay.addObject(newob)
                    _Dimension(newob)
                    if FreeCAD.GuiUp:
                        from Draft import _ViewProviderDimension
                        _ViewProviderDimension(newob.ViewObject)
                    newob.Start = p1
                    newob.End = p2
                    newob.Dimline = pt
                    if gui:
                        dim.layer = layer
                        dim.color_index = 256
                        formatObject(newob, dim)
                        if dxfUseStandardSize and draftui:
                            newob.ViewObject.FontSize = draftui.fontsize
                        else:
                            st = rawValue(dim, 3)
                            size = getdimheight(st) or 1
                            newob.ViewObject.FontSize = float(size)*TEXTSCALING
    else:
        FCC.PrintMessage("skipping dimensions...\n")

    # Draw points
    if dxfImportPoints:
        points = drawing.entities.get_type("point")
        if points:
            FCC.PrintMessage("drawing " + str(len(points)) + " points...\n")
        for point in points:
            x = vec(rawValue(point, 10))
            y = vec(rawValue(point, 20))
            z = vec(rawValue(point, 30))
            lay = rawValue(point, 8)
            if dxfImportLayouts or (not rawValue(point, 67)):
                if dxfMakeBlocks:
                    shape = Part.Vertex(x, y, z)
                    addToBlock(shape, lay)
                else:
                    newob = Draft.make_point(x, y, z)
                    lay = locateLayer(lay)
                    lay.addObject(newob)
                    if gui:
                        formatObject(newob, point)
    else:
        FCC.PrintMessage("skipping points...\n")

    # Draw leaders
    if dxfImportTexts:
        leaders = drawing.entities.get_type("leader")
        if leaders:
            FCC.PrintMessage("drawing " + str(len(leaders)) + " leaders...\n")
        for leader in leaders:
            if dxfImportLayouts or (not rawValue(leader, 67)):
                points = getMultiplePoints(leader)
                newob = Draft.make_wire(points)
                lay = locateLayer(rawValue(leader, 8))
                lay.addObject(newob)
                if gui:
                    newob.ViewObject.EndArrow = True
                    formatObject(newob, leader)
    else:
        FCC.PrintMessage("skipping leaders...\n")

    # Draw hatches
    if dxfImportHatches:
        hatches = drawing.entities.get_type("hatch")
        if hatches:
            FCC.PrintMessage("drawing " + str(len(hatches)) + " hatches...\n")
        for hatch in hatches:
            if dxfImportLayouts or (not rawValue(hatch, 67)):
                points = getMultiplePoints(hatch)
                if len(points) > 1:
                    lay = rawValue(hatch, 8)
                    points = points[:-1]
                    newob = None
                    if dxfCreatePart or dxfMakeBlocks:
                        points.append(points[0])
                        s = Part.makePolygon(points)
                        if dxfMakeBlocks:
                            addToBlock(s, lay)
                        else:
                            newob = addObject(s, "Hatch", lay)
                            if gui:
                                formatObject(newob, hatch)
                    else:
                        newob = Draft.make_wire(points)
                        locateLayer(lay).addObject(newob)
                        if gui:
                            formatObject(newob, hatch)
    else:
        FCC.PrintMessage("skipping hatches...\n")

    # Draw blocks
    inserts = drawing.entities.get_type("insert")
    if not dxfStarBlocks:
        FCC.PrintMessage("skipping *blocks...\n")
        newinserts = []
        for i in inserts:
            if dxfImportLayouts or (not rawValue(i, 67)):
                if i.block[0] != '*':
                    newinserts.append(i)
        inserts = newinserts
    if inserts:
        FCC.PrintMessage("drawing " + str(len(inserts)) + " blocks...\n")
        blockrefs = drawing.blocks.data
        for ref in blockrefs:
            if dxfCreateDraft or dxfCreateSketch:
                drawBlock(ref, createObject=True)
            else:
                drawBlock(ref, createObject=False)
        num = 0
        for insert in inserts:
            if (dxfCreateDraft or dxfCreateSketch) and not dxfMakeBlocks:
                shape = drawInsert(insert, num, clone=True)
            else:
                shape = drawInsert(insert, num)
            if shape:
                if dxfMakeBlocks:
                    addToBlock(shape, insert.layer)
                else:
                    newob = addObject(shape, "Block." + insert.block,
                                      insert.layer)
                    if gui:
                        formatObject(newob, insert)
            num += 1

    # Make blocks, if any
    if dxfMakeBlocks:
        print("creating layerblocks...")
        for k, l in layerBlocks.items():
            shape = drawLayerBlock(l)
            if shape:
                newob = addObject(shape, k)
    del layerBlocks

    # Hide block objects, if any
    for k, o in blockobjects.items():
        if o.ViewObject:
            o.ViewObject.hide()
    del blockobjects

    # Finishing
    print("done processing")

    if reComputeFlag:
        doc.recompute()
        print("recompute done")

    FCC.PrintMessage("successfully imported " + filename + "\n")
    if badobjects:
        print("dxf: ", len(badobjects), " objects were not imported")
    del doc


def warn(dxfobject, num=None):
    """Print a warning that the DXF object couldn't be imported.

    Also add the object to the global list `badobjects`.

    Parameters
    ----------
    dxfobject : drawing.entities
        The DXF object that couldn't be imported.

    num : float, optional
        It defaults to `None`. A simple number that identifies
        the given `dxfobject`.

    To do
    -----
    Use local variables, not global variables.
    """
    print("dxf: couldn't import ", dxfobject, " (", num, ")")
    badobjects.append(dxfobject)


def open(filename):
    """Open a file and return a new document.

    If the global variable `dxfUseLegacyImporter` exists,
    it will process `filename` with `processdxf`.
    Otherwise, it will use the `Import` module, `Import.readDXF(filename)`.

    Parameters
    ----------
    filename : str
        The path to the file to open.

    Returns
    -------
    App::Document
        The new document object with objects and shapes built from `filename`.

    To do
    -----
    Use local variables, not global variables.
    """
    readPreferences()
    if dxfUseLegacyImporter:
        getDXFlibs()
        if dxfReader:
            docname = os.path.splitext(os.path.basename(filename))[0]
            doc = FreeCAD.newDocument(docname)
            doc.Label = docname
            processdxf(doc, filename)
            return doc
        else:
            errorDXFLib(gui)
    else:
        docname = os.path.splitext(os.path.basename(filename))[0]
        doc = FreeCAD.newDocument(docname)
        doc.Label = docname
        FreeCAD.setActiveDocument(doc.Name)
        import Import
        Import.readDXF(filename)
        Draft.convert_draft_texts() # convert annotations to Draft texts
        doc.recompute()


def insert(filename, docname):
    """Import a file into the specified document.

    Parameters
    ----------
    filename : str
        The path to the file to import.

    docname : str
        The name of an `App::Document` instance into which
        the objects and shapes from `filename` will be imported.

        If the document doesn't exist, it is created
        and set as the active document.

    To do
    -----
    Use local variables, not global variables.
    """
    readPreferences()
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.setActiveDocument(docname)
    if dxfUseLegacyImporter:
        getDXFlibs()
        if dxfReader:
            groupname = os.path.splitext(os.path.basename(filename))[0]
            importgroup = doc.addObject("App::DocumentObjectGroup", groupname)
            processdxf(doc, filename)
            for l in layers:
                importgroup.addObject(l)
        else:
            errorDXFLib(gui)
    else:
        import Import
        Import.readDXF(filename)
        Draft.convert_draft_texts() # convert annotations to Draft texts
        doc.recompute()

def getShapes(filename):
    """Read a DXF file, and return a list of shapes from its contents.

    This is an auxiliary function that processes the DXF file to list its
    contents but doesn't open or create a new document.

    Parameters
    ----------
    filename : str
        The path to the file to read.

    Returns
    -------
    list of `Part.Shapes`
        It returns `None` if the edges (lines, polylines, arcs)
        are above 100, and the user decides to interrupt (graphically)
        the process of joining them.

    See also
    --------
    open, insert
    """
    if dxfReader:
        return processdxf(None, filename, getShapes=True)


# EXPORT ######################################################################

def projectShape(shape, direction, tess=None):
    """Project shape in a given direction.

    It uses `TechDraw.projectEx(shape, direction)`
    to return a list with all the parts of the projection.
    The first five elements are added to a list of edges,
    which are then put in a `Part.Compound`.

    Parameters
    ----------
    shape : Part.Shape
        Any shape previously created from a DXF file.

    direction : Base::Vector3
        The direction of the projection.

    tess : list, optional
        It defaults to `None`. If it is available, it is a list with
        two elements, `[True, segment_length]` which are used by
        `DraftGeomUtils.cleanProjection(compound, tess[0], tess[1])`
        to create a valid compound of edges.

        Otherwise, a simple `Part.Compound` is produced.

    Returns
    -------
    Part::TopoShape ('Compound')
        A `Part.Compound` of edges.

        It returns the original `shape` if it fails producing the projection
        in the given `direction`.

    See also
    --------
    TechDraw.projectEx, DraftGeomUtils.cleanProjection
    """
    import TechDraw
    edges = []
    try:
        groups = TechDraw.projectEx(shape, direction)
    except Part.OCCError:
        print("unable to project shape on direction ", direction)
        return shape
    else:
        for g in groups[0:5]:
            if g:
                edges.append(g)
        # return DraftGeomUtils.cleanProjection(Part.makeCompound(edges))
        if tess:
            return DraftGeomUtils.cleanProjection(Part.makeCompound(edges),
                                                  tess[0], tess[1])
        else:
            return Part.makeCompound(edges)
            # return DraftGeomUtils.cleanProjection(Part.makeCompound(edges))


def getArcData(edge):
    """Return center, radius, start, and end angles of a circle-based edge.

    Parameters
    ----------
    edge : Part::TopoShape ('Edge')
        An edge representing a circular arc, either open or closed.

    Returns
    -------
    (tuple, float, float, float)
        It returns a tuple of four values; the first value is a tuple
        with the coordinates of the center `(x, y, z)`;
        the other three represent the magnitude of the radius,
        and the start and end angles in degrees that define the arc.

    (tuple, float, 0, 0)
        If the number of vertices in the `edge` is only one, only the center
        point exists, so it's a full circumference; in this case, both
        angles are zero.
    """
    ce = edge.Curve.Center
    radius = edge.Curve.Radius
    if len(edge.Vertexes) == 1:
        # closed circle
        return DraftVecUtils.tup(ce), radius, 0, 0
    else:
        # new method: recalculate ourselves as we cannot trust edge.Curve.Axis
        # or XAxis
        p1 = edge.Vertexes[0].Point
        p2 = edge.Vertexes[-1].Point
        v1 = p1.sub(ce)
        v2 = p2.sub(ce)
        # print(v1.cross(v2))
        # print(edge.Curve.Axis)
        # print(p1)
        # print(p2)
        # we can use Z check since arcs getting here will ALWAYS be in XY plane
        # Z can be 0 if the arc is 180 deg
        # if (v1.cross(v2).z >= 0) or (edge.Curve.Axis.z > 0):
        # Calculates the angles of the first and last points
        # in the circular arc, with respect to the global X axis.
        if edge.Curve.Axis.z > 0:
            # clockwise
            ang1 = -DraftVecUtils.angle(v1)
            ang2 = -DraftVecUtils.angle(v2)
        else:
            # counterclockwise
            ang2 = -DraftVecUtils.angle(v1)
            ang1 = -DraftVecUtils.angle(v2)

        # obsolete method - fails a lot
        # if round(edge.Curve.Axis.dot(Vector(0, 0, 1))) == 1:
        #    ang1, ang2 = edge.ParameterRange
        # else:
        #    ang2, ang1 = edge.ParameterRange
        # if edge.Curve.XAxis != Vector(1, 0, 0):
        #    ang1 -= DraftVecUtils.angle(edge.Curve.XAxis)
        #    ang2 -= DraftVecUtils.angle(edge.Curve.XAxis)

        return (DraftVecUtils.tup(ce), radius,
                math.degrees(ang1), math.degrees(ang2))


def getSplineSegs(edge):
    """Return a list of points from an edge that is a spline or bezier curve.

    Parameters
    ----------
    edge : Part::TopoShape ('Edge')
        An edge representing a spline or bezier curve.

    Returns
    -------
    list of Base::Vector3
        It returns a list with the points that form the curve.
        It returns the point in `edge.FirstParameter`,
        all the intermediate points, and the point in `edge.LastParameter`.

        If the `segmentlength` variable is zero in the parameters database,
        then it only returns the first and the last point of the `edge`.
    """
    params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    seglength = params.GetFloat("maxsegmentlength", 5.0)
    points = []
    if seglength == 0:
        points.append(edge.Vertexes[0].Point)
        points.append(edge.Vertexes[-1].Point)
    else:
        points.append(edge.valueAt(edge.FirstParameter))
        if edge.Length > seglength:
            nbsegs = int(math.ceil(edge.Length/seglength))
            step = (edge.LastParameter-edge.FirstParameter)/nbsegs
            for nv in range(1, nbsegs):
                # print("value at", nv*step, "=", edge.valueAt(nv*step))
                v = edge.valueAt(edge.FirstParameter+(nv*step))
                points.append(v)
        points.append(edge.valueAt(edge.LastParameter))
    return points


def getWire(wire, nospline=False, lw=True, asis=False):
    """Return a list of DXF ready points and bulges from a wire.

    It builds a list of points from the edges of a `wire`.
    If the edges are circular arcs, the "bulge" of that edge is calculated,
    for other cases, the bulge is considered zero.

    Parameters
    ----------
    wire : Part::TopoShape ('Wire')
        A shape representing a wire.

    nospline : bool, optional
        It defaults to `False`.
        If it is `True`, the edges of the wire are not considered as
        being one of `'BSplineCurve'`, `'BezierCurve'`, or `'Ellipse'`,
        and a simple point is added to the list.
        Otherwise, `getSplineSegs(edge)` is used to extract
        the points and add them to the list.

    lw : bool, optional
        It defaults to `True`. If it is `True` it assumes the `wire`
        is a `'lwpolyline'`.
        Otherwise, it assumes it is a `'polyline'`.

    asis : bool, optional
        It defaults to `False`. If it is `True`, it just returns
        the points of the vertices of the `wire`, and considers the bulge
        is zero.

        Otherwise, it processes the edges of the `wire` and calculates
        the bulge of the edges if they are of type `'Circle'`.
        For types of edges that are `'BSplineCurve'`, `'BezierCurve'`,
        or `'Ellipse'`, the bulge is zero

    Returns
    -------
    list of tuples
        It returns a list of tuples ``[(...), (...), ...]``
        where each tuple indicates a point with additional information
        besides the coordinates.
        Two types of tuples may be returned.

    [(float, float, float, None, None, float), ...]
        When `lw` is `True` (`'lwpolyline'`)
        the first three values represent the coordinates of the point,
        the next two are `None`, and the last value is the bulge.

    [((float, float, float), None, [None, None], float), ...]
        When `lw` is `False` (`'polyline'`)
        the first element is a tuple of three values that indicate
        the coordinates of the point, the next element is `None`,
        the next element is a list of two `None` values,
        and the last element is the value of the bulge.

    See also
    --------
    calcBulge
    """
    def fmt(v, b=0.0):
        if lw:
            # LWpolyline format
            return (v.x, v.y, v.z, None, None, b)
        else:
            # Polyline format
            return ((v.x, v.y, v.z), None, [None, None], b)
    points = []
    if asis:
        points = [fmt(v.Point) for v in wire.OrderedVertexes]
    else:
        edges = Part.__sortEdges__(wire.Edges)
        # print("processing wire ",wire.Edges)
        for edge in edges:
            v1 = edge.Vertexes[0].Point
            if DraftGeomUtils.geomType(edge) == "Circle":
                # polyline bulge -> negative makes the arc go clockwise
                angle = edge.LastParameter-edge.FirstParameter
                bul = math.tan(angle/4)
                # if cross1[2] < 0:
                #     # polyline bulge -> negative makes the arc go clockwise
                #     bul = -bul
                if edge.Curve.Axis.dot(Vector(0, 0, 1)) < 0:
                    bul = -bul
                points.append(fmt(v1, bul))
            elif (DraftGeomUtils.geomType(edge) in ["BSplineCurve",
                                                    "BezierCurve",
                                                    "Ellipse"]) and (not nospline):
                spline = getSplineSegs(edge)
                spline.pop()
                for p in spline:
                    points.append(fmt(p))
            else:
                points.append(fmt(v1))
        if not DraftGeomUtils.isReallyClosed(wire):
            v = edges[-1].Vertexes[-1].Point
            points.append(fmt(v))
        # print("wire verts: ",points)
    return points


def getBlock(sh, obj, lwPoly=False):
    """Return a DXF block with the contents of the object.

    It creates a `block` object using `dxfLibrary.Block`,
    and then writes the given shape with
    `writeShape(sh, obj, block, lwPoly)`.

    Parameters
    ----------
    sh : Part::TopoShape
        Any shape in the document.

    obj : App::DocumentObject
        Any object in the document.

    lwPoly : bool, optional
        It defaults to `False`. If it is `True` it will write
        a `'lwpolyline'`.
        Otherwise, it will be a `'polyline'`.

    Returns
    -------
    dxfLibrary.Block
        The block of data with the given `sh` shape and `obj` object.
    """
    block = dxfLibrary.Block(name=obj.Name, layer=getStrGroup(obj))
    writeShape(sh, obj, block, lwPoly)
    return block


def writeShape(sh, ob, dxfobject, nospline=False, lwPoly=False,
               layer=None, color=None, asis=False):
    """Write the object's shape contents in the given DXF object.

    Iterates over the wires (polylines) and lone edges of `sh`.
    Then it creates DXF object depending of the type of wire,
    and adds those objects to the `dxfobject` list.

    If the wire only has one edge and it is of type `'Circle'`
    it will create an object of type `dxfLibrary.Circle` or `dxfLibrary.Arc`.
    In other cases, it will try creating objects of type
    `dxfLibrary.LwPolyLine` or `dxfLibrary.PolyLine`.

    When parsing lone edges it will approximate single closed edges of type
    `'BSplineCurve'` or `'BezierCurve'` with a `dxfLibrary.Circle`.
    In the case of edges of type `Ellipse`, it can approximate
    the edge as a `dxfLibrary.PolyLine`, depending on the value
    of `'DiscretizeEllipses'` in the parameter database.
    Otherwise it creates an object of type `dxfLibrary.Ellipse`.

    For other lone edges, they are treated as lines,
    so they create an object of type `linesdxfLibrary.Line`.

    Parameters
    ----------
    sh : Part::TopoShape
        Any shape in the document.

    ob : App::DocumentObject
        Any object in the document.

    dxfobject : dxfLibrary.Drawing
        An object which will be populated with DXF objects created
        from `sh.Wires` and `sh.Edges`.

    nospline : bool, optional
        It defaults to `False`.
        If it is `True`, the edges of the wire are not considered as
        being one of `'BSplineCurve'`, `'BezierCurve'`, or `'Ellipse'`,
        and simple points are used to build the new object with
        `getWire(wire, nospline=True, asis=asis)`.

    lwPoly : bool, optional
        It defaults to `False`. If it is `True` it will try producing
        a `dxfLibrary.LwPolyLine`, instead of a `dxfLibrary.PolyLine`.

    layer : str, optional
        It defaults to `None`. It is the name of the layer or group where `ob`
        is contained. If it is `None`, `getStrGroup(ob)` is called to search
        for the layer's name that contains `ob`.
        The created object is placed in this layer.

    color : int, optional
        It defaults to `None`. It is the AutoCAD color index (ACI)
        closest to `ob`'s color obtained with `getACI(ob)`.
        The created object uses this color.

    asis : bool, optional
        It defaults to `False`. If it is `True`, it just extracts
        the edges of the wire as is, and creates the `'lwpolyline'`
        or `'polyline'` with the simple points returned by
        `getWire(wire, nospline, asis=True)`.

        Otherwise, the edges are sorted, and then creates
        more complex shapes with `getWire(wire, nospline, asis=False)`.

    See also
    --------
    getWire, getStrGroup, getACI, dxfLibrary.Circle, dxfLibrary.Arc,
    dxfLibrary.LwPolyLine, dxfLibrary.PolyLine, dxfLibrary.Ellipse,
    dxfLibrary.Line
    """
    processededges = []
    if not layer:
        layer = getStrGroup(ob)
    if not color:
        color = getACI(ob)
    for wire in sh.Wires:  # polylines
        if asis:
            edges = wire.Edges
        else:
            edges = Part.__sortEdges__(wire.Edges)
        for e in edges:
            processededges.append(e.hashCode())
        if (len(wire.Edges) == 1) and (DraftGeomUtils.geomType(wire.Edges[0]) == "Circle"):
            center, radius, ang1, ang2 = getArcData(wire.Edges[0])
            if center is not None:
                if len(wire.Edges[0].Vertexes) == 1:  # circle
                    dxfobject.append(dxfLibrary.Circle(center, radius,
                                                       color=color,
                                                       layer=layer))
                else:  # arc
                    dxfobject.append(dxfLibrary.Arc(center, radius,
                                                    ang1, ang2, color=color,
                                                    layer=layer))
        else:
            if lwPoly:
                if hasattr(dxfLibrary, "LwPolyLine"):
                    dxfobject.append(dxfLibrary.LwPolyLine(getWire(wire, nospline, asis=asis),
                                                           [0.0, 0.0],
                                                           int(DraftGeomUtils.isReallyClosed(wire)),
                                                           color=color,
                                                           layer=layer))
                else:
                    FCC.PrintWarning("LwPolyLine support not found. "
                                     "Please delete dxfLibrary.py "
                                     "from your FreeCAD user directory "
                                     "to force auto-update\n")
            else:
                dxfobject.append(dxfLibrary.PolyLine(getWire(wire, nospline, lw=False, asis=asis),
                                                     [0.0, 0.0, 0.0],
                                                     int(DraftGeomUtils.isReallyClosed(wire)),
                                                     color=color,
                                                     layer=layer))
    if len(processededges) < len(sh.Edges):  # lone edges
        loneedges = []
        for e in sh.Edges:
            if e.hashCode() not in processededges:
                loneedges.append(e)
        # print("lone edges ", loneedges)
        for edge in loneedges:
            # splines
            if (DraftGeomUtils.geomType(edge) in ["BSplineCurve",
                                                  "BezierCurve"]):
                if (len(edge.Vertexes) == 1) and (edge.Curve.isClosed()) and (edge.Area > 0):
                    # special case: 1-vert closed spline, approximate as a circle
                    c = DraftGeomUtils.getCircleFromSpline(edge)
                    if c:
                        dxfobject.append(dxfLibrary.Circle(DraftVecUtils.tup(c.Curve.Center),
                                                           c.Curve.Radius,
                                                           color=color,
                                                           layer=layer))
                else:
                    points = []
                    spline = getSplineSegs(edge)
                    for p in spline:
                        points.append(((p.x, p.y, p.z), None, [None, None], 0.0))
                    dxfobject.append(dxfLibrary.PolyLine(points,
                                                         [0.0, 0.0, 0.0],
                                                         0, color=color,
                                                         layer=layer))
            elif DraftGeomUtils.geomType(edge) == "Circle":  # curves
                center, radius, ang1, ang2 = getArcData(edge)
                if center is not None:
                    if not isinstance(center, tuple):
                        center = DraftVecUtils.tup(center)
                    if len(edge.Vertexes) == 1:  # circles
                        dxfobject.append(dxfLibrary.Circle(center,
                                                           radius,
                                                           color=color,
                                                           layer=layer))
                    else:  # arcs
                        dxfobject.append(dxfLibrary.Arc(center,
                                                        radius,
                                                        ang1, ang2,
                                                        color=getACI(ob),
                                                        layer=layer))
            elif DraftGeomUtils.geomType(edge) == "Ellipse":  # ellipses:
                if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("DiscretizeEllipses", True):
                    points = []
                    spline = getSplineSegs(edge)
                    for p in spline:
                        points.append(((p.x, p.y, p.z), None, [None, None], 0.0))
                    dxfobject.append(dxfLibrary.PolyLine(points,
                                                         [0.0, 0.0, 0.0],
                                                         0, color=color,
                                                         layer=layer))
                else:
                    if hasattr(dxfLibrary, "Ellipse"):
                        center = DraftVecUtils.tup(edge.Curve.Center)
                        norm = DraftVecUtils.tup(edge.Curve.Axis)
                        start = edge.FirstParameter
                        end = edge.LastParameter
                        ax = edge.Curve.Focus1.sub(edge.Curve.Center)
                        major = DraftVecUtils.tup(DraftVecUtils.scaleTo(ax, edge.Curve.MajorRadius))
                        minor = edge.Curve.MinorRadius/edge.Curve.MajorRadius
                        # print("exporting ellipse: ", center, norm,
                        #       start, end, major, minor)
                        dxfobject.append(dxfLibrary.Ellipse(center=center,
                                                            majorAxis=major,
                                                            normalAxis=norm,
                                                            minorAxisRatio=minor,
                                                            startParameter=start,
                                                            endParameter=end,
                                                            color=color,
                                                            layer=layer))
                    else:
                        FCC.PrintWarning("Ellipses support not found. "
                                         "Please delete dxfLibrary.py "
                                         "from your FreeCAD user directory "
                                         "to force auto-update\n")
            else:  # anything else is treated as lines
                if len(edge.Vertexes) > 1:
                    ve1 = edge.Vertexes[0].Point
                    ve2 = edge.Vertexes[1].Point
                    dxfobject.append(dxfLibrary.Line([DraftVecUtils.tup(ve1),
                                                      DraftVecUtils.tup(ve2)],
                                                     color=color,
                                                     layer=layer))


def writeMesh(ob, dxf):
    """Write an object's shape as a polyface mesh in the given DXF list.

    It tessellates the `ob.Shape` with a tolerance of 0.5,
    to produce mesh data, that is, lists of vertices and face indices:
    ``([ point1, point2, ...], [(face1 indices), (face2 indices), ...])``

    The points and faces are extracted, and used with
    `dxfLibrary.PolyLine` to produce a polyface mesh, that is added
    to the `dxf` object.

    Parameters
    ----------
    ob : App::DocumentObject
        Any object in the document.

    dxf : dxfLibrary.Drawing
        An object which will be populated with a DXF polyface mesh
        created from `ob.Shape`.

    See also
    --------
    dxfLibrary.Drawing, dxfLibrary.PolyLine, Part.Shape.tessellate
    """
    meshdata = ob.Shape.tessellate(0.5)
    # print(meshdata)
    points = []
    faces = []
    for p in meshdata[0]:
        points.append([p.x, p.y, p.z])
    for f in meshdata[1]:
        faces.append([f[0] + 1, f[1] + 1, f[2] + 1])
    # print(len(points),len(faces))
    dxf.append(dxfLibrary.PolyLine([points, faces],
                                   [0.0, 0.0, 0.0],
                                   64, color=getACI(ob),
                                   layer=getGroup(ob)))


def writePanelCut(ob, dxf, nospline, lwPoly, parent=None):
    """Create an object's outline and add it to the given DXF list.

    Given an object `ob` that contains an outline in its proxy object,
    it tries obtaining the outline `outl`, the inline `inl`, and a `tag`.
    Then tries creating each shape using the `parent` object as base
    (or `ob` itself), and placing the result in the `dxf` list.

    For `outl` it places the result in an `'Outlines'` layer of color index 5
    (blue).
    For `intl`, if it exists, it places the result in a `'Cuts'` layer
    of color index 4 (light blue).
    For `tag`, if it exists, it places the result in a `'Tags'` layer
    of color index 2 (yellow).
    ::
        writeShape(outl, parent, dxf, nospline, lwPoly, ...)
        writeShape(inl, parent, dxf, nospline, lwPoly, ...)
        writeShape(tag, parent, dxf, nospline, lwPoly, ...)

    Parameters
    ----------
    ob : App::DocumentObject
        Any object in the document.

    dxf : dxfLibrary.Drawing
        An object which will be populated with a DXF object created
        from `writeShape()`.

    nospline : bool
        If it is `True`, the edges of the wire are not considered as
        being one of `'BSplineCurve'`, `'BezierCurve'`, or `'Ellipse'`,
        and simple points are used to build the new shape with
        `writeShape()`.

    lwPoly : bool
        If it is `True` it will try producing
        a `dxfLibrary.LwPolyLine`, instead of a `dxfLibrary.PolyLine`,
        by using `writeShape()`.

    parent : App::DocumentObject, optional
        It defaults to `None`.
        If it exists, its `Base::Placement` is used to modify the
        Placement of the output object and its tag.
        Otherwise, `ob` is also used as the `parent`.

    See also
    --------
    writeShape
    """
    if not hasattr(ob.Proxy, "outline"):
        ob.Proxy.execute(ob)
    if hasattr(ob.Proxy, "outline"):
        outl = ob.Proxy.outline.copy()
        tag = None
        if hasattr(ob.Proxy, "tag"):
            tag = ob.Proxy.tag
        if tag:
            tag = tag.copy()
            tag.Placement = ob.Placement.multiply(tag.Placement)
            if parent:
                tag.Placement = parent.Placement.multiply(tag.Placement)
        outl.Placement = ob.Placement.multiply(outl.Placement)
        if parent:
            outl.Placement = parent.Placement.multiply(outl.Placement)
        else:
            parent = ob
        if len(outl.Wires) > 1:
            # separate outline
            d = 0
            ow = None
            for w in outl.Wires:
                if w.BoundBox.DiagonalLength > d:
                    d = w.BoundBox.DiagonalLength
                    ow = w
            if ow:
                inl = Part.Compound([w for w in outl.Wires if w.hashCode() != ow.hashCode()])
                outl = ow
        else:
            inl = None
            outl = outl.Wires[0]

        writeShape(outl, parent, dxf, nospline, lwPoly,
                   layer="Outlines", color=5)
        if inl:
            writeShape(inl, parent, dxf, nospline, lwPoly,
                       layer="Cuts", color=4)
        if tag:
            writeShape(tag, parent, dxf, nospline, lwPoly,
                       layer="Tags", color=2, asis=True)
            # sticky fonts can render very odd wires...
            # for w in tag.Edges:
            #    pts = [(v.X, v.Y, v.Z) for v in w.Vertexes]
            #    dxf.append(dxfLibrary.Line(pts, color=getACI(ob),
            #                               layer="Tags"))


def getStrGroup(ob):
    """Get a string version of the group or layer that contains the object.

    Parameters
    ----------
    ob : App::DocumentObject
        Any object in the document.

    Returns
    -------
    str
        The name of the layer in capital letters,
        as the DXF R12 format seems to favor this style.
    """
    return getGroup(ob).upper()


def export(objectslist, filename, nospline=False, lwPoly=False):
    """Export a DXF file into the specified filename.

    If will read the preferences. If the global variable
    `dxfUseLegacyExporter` exists, it will try using the `Import` module
    to write the DXF file.
    ::
        Import.writeDXFObject(objectslist, filename, version, lwPoly)

    Where `version` is 14, or 12 if `nospline` is `True`.

    Otherwise it will try to use the DXF export libraries
    by running `getDXFlibs()`.

    Iterating over all objects it writes shapes individually
    with `writeShape()`, looking for types `'PanelSheet'`, `'PanelCut'`,
    `'Axis'`, `'Annotation'`, `'DraftText'`, `'Dimension'`.
    For objects derived from `'Part::Feature'` it may use `writeMesh()`
    depending on the parameter `'dxfmesh'`, or it may project the object
    in the camera view, depending on the parameter `'dxfproject'`.

    Parameters
    ----------
    objectslist : list of App::DocumentObject
        A list with all objects that will be exported.
        If any object of the given list is a group, its contents are appended
        to the export list.

        If the list only contains an `'ArchSectionView'` object
        it will use its `getDXF()` method to provide the DXF information
        to write into `filename`.

        If the list only contains a `'TechDraw::DrawPage'` object it will use
        `exportPage()` to produce the DXF file.

    filename : str
        The path of the new DXF file.

    nospline : bool, optional
        It defaults to `False`.
        If it is `True`, the BSplines are exported as straight segments,
        when passing the objects to `writeShape()`.

    lwPoly : bool, optional.
        It defaults to `False`.
        If it is `True` it will try producing
        a `dxfLibrary.LwPolyLine`, instead of a `dxfLibrary.PolyLine`,
        by using `writeShape()`.
        This is required to produce an OpenSCAD DXF.

    Returns
    -------
    It returns `None` if the export is successful.

    See also
    --------
    dxfLibrary.Drawing, readPreferences, getDXFlibs, errorDXFLib,
    writeShape, writeMesh, Import.writeDXFObject

    To do
    -----
    Use local variables, not global variables.
    """
    readPreferences()
    if not dxfUseLegacyExporter:
        import Import
        version = 14
        if nospline:
            version = 12
        Import.writeDXFObject(objectslist, filename, version, lwPoly)
        return
    getDXFlibs()
    if dxfLibrary:
        global exportList
        exportList = objectslist
        exportList = Draft.get_group_contents(exportList)

        nlist = []
        exportLayers = []
        for ob in exportList:
            t = Draft.getType(ob)
            if t == "AxisSystem":
                nlist.extend(ob.Axes)
            elif t == "Layer":
                exportLayers.append(ob)
                for child in ob.Group:
                    if child not in nlist:
                        nlist.append(child)
            else:
                if ob not in nlist:
                    nlist.append(ob)
        exportList = nlist

        if (len(exportList) == 1) and (Draft.getType(exportList[0]) == "ArchSectionView"):
            # arch view: export it "as is"
            dxf = exportList[0].Proxy.getDXF()
            if dxf:
                f = pythonopen(filename, "w")
                f.write(dxf)
                f.close()

        elif (len(exportList) == 1) and (exportList[0].isDerivedFrom("TechDraw::DrawPage")):
            # page: special hack-export! (see below)
            exportPage(exportList[0], filename)

        else:
            # other cases, treat objects one by one
            dxf = dxfLibrary.Drawing()
            # add global variables
            if hasattr(dxf,"header"):
                dxf.header.append("  9\n$DIMTXT\n 40\n"+str(Draft.getParam("textheight", 20))+"\n")
                dxf.header.append("  9\n$INSUNITS\n 70\n4\n")
            for ob in exportLayers:
                if ob.Label != "0":  # dxflibrary already creates it
                    ltype = 'continuous'
                    if ob.ViewObject:
                        if ob.ViewObject.DrawStyle == "Dashed":
                            ltype = 'DASHED'
                        elif ob.ViewObject.DrawStyle == "Dotted":
                            ltype = 'HIDDEN'
                        elif ob.ViewObject.DrawStyle == "Dashdot":
                            ltype = 'DASHDOT'
                    # print("exporting layer:", ob.Label,
                    #       getACI(ob), ltype)
                    dxf.layers.append(dxfLibrary.Layer(name=ob.Label,
                                                       color=getACI(ob),
                                                       lineType=ltype))

            for ob in exportList:
                obtype = Draft.getType(ob)
                # print("processing " + str(ob.Name))
                if obtype == "PanelSheet":
                    if not hasattr(ob.Proxy, "sheetborder"):
                        ob.Proxy.execute(ob)
                    sb = ob.Proxy.sheetborder
                    if sb:
                        sb.Placement = ob.Placement
                        writeShape(sb, ob, dxf, nospline, lwPoly,
                                   layer="Sheets", color=1)
                    ss = ob.Proxy.sheettag
                    if ss:
                        ss.Placement = ob.Placement.multiply(ss.Placement)
                        writeShape(ss, ob, dxf, nospline, lwPoly,
                                   layer="SheetTags", color=1)
                    for subob in ob.Group:
                        if Draft.getType(subob) == "PanelCut":
                            writePanelCut(subob, dxf, nospline, lwPoly,
                                          parent=ob)
                        elif subob.isDerivedFrom("Part::Feature"):
                            shp = subob.Shape.copy()
                            shp.Placement = ob.Placement.multiply(shp.Placement)
                            writeShape(shp, ob, dxf, nospline, lwPoly,
                                       layer="Outlines", color=5)

                elif obtype == "PanelCut":
                    writePanelCut(ob, dxf, nospline, lwPoly)

                elif obtype == "Space":
                    vobj = ob.ViewObject
                    c = utils.get_rgb(vobj.TextColor)
                    n = vobj.FontName
                    a = 0
                    if rotation != 0:
                        a = math.radians(rotation)
                    t1 = "".join(vobj.Proxy.text1.string.getValues())
                    t2 = "".join(vobj.Proxy.text2.string.getValues())
                    scale = vobj.FirstLine.Value/vobj.FontSize.Value
                    f1 = fontsize * scale
                    if round(FreeCAD.DraftWorkingPlane.axis.getAngle(App.Vector(0,0,1)),2) not in [0,3.14]:
                        # if not in XY view, place the label at center
                        p2 = obj.Shape.CenterOfMass
                    else:
                        _v = vobj.Proxy.coords.translation.getValue().getValue()
                        p2 = obj.Placement.multVec(App.Vector(_v))
                    _h = vobj.Proxy.header.translation.getValue().getValue()
                    lspc = FreeCAD.Vector(_h)
                    p1 = p2 + lspc
                    dxf.append(dxfLibrary.Text(t1, p1, height=f1,
                                               color=getACI(ob, text=True),
                                               style='STANDARD',
                                               layer=getStrGroup(ob)))
                    if t2:
                        ofs = FreeCAD.Vector(0, -lspc.Length, 0)
                        if a:
                            Z = FreeCAD.Vector(0, 0, 1)
                            ofs = FreeCAD.Rotation(Z, -rotation).multVec(ofs)
                        dxf.append(dxfLibrary.Text(t2, p1.add(ofs), height=f1,
                                                   color=getACI(ob, text=True),
                                                   style='STANDARD',
                                                   layer=getStrGroup(ob)))

                elif obtype == "Axis":
                    axes = ob.Proxy.getAxisData(ob)
                    if not axes:
                        continue
                    for ax in axes:
                        dxf.append(dxfLibrary.Line([ax[0],
                                                    ax[1]],
                                                    color=getACI(ob),
                                                    layer=getStrGroup(ob)))
                    h = 1
                    if FreeCAD.GuiUp:
                        vobj = ob.ViewObject
                        h = float(ob.ViewObject.FontSize)
                        for text in vobj.Proxy.getTextData():
                            pos = text[1].add(FreeCAD.Vector(-h/2,-h/2,0))
                            dxf.append(dxfLibrary.Text(text[0],
                                                       pos,
                                                       height=h,
                                                       color=getACI(ob),
                                                       style='STANDARD',
                                                       layer=getStrGroup(ob)))
                        for shape in vobj.Proxy.getShapeData():
                            if hasattr(shape,"Curve") and isinstance(shape.Curve,Part.Circle):
                                dxf.append(dxfLibrary.Circle(shape.Curve.Center,
                                                             shape.Curve.Radius,
                                                             color=getACI(ob),
                                                             layer=getStrGroup(ob)))
                            else:
                                if lwPoly:
                                    points = [(v.Point.x, v.Point.y, v.Point.z, None, None, 0.0) for v in shape.Vertexes]
                                    dxf.append(dxfLibrary.LwPolyLine(points,
                                                                     [0.0, 0.0],
                                                                     1,
                                                                     color=getACI(ob),
                                                                     layer=getGroup(ob)))
                                else:
                                    points = [((v.Point.x, v.Point.y, v.Point.z), None, [None, None], 0.0) for v in shape.Vertexes]
                                    dxf.append(dxfLibrary.PolyLine(points,
                                                                   [0.0, 0.0, 0.0],
                                                                   1,
                                                                   color=getACI(ob),
                                                                   layer=getGroup(ob)))

                elif ob.isDerivedFrom("Part::Feature"):
                    tess = None
                    if hasattr(ob, "Tessellation"):
                        if ob.Tessellation:
                            tess = [ob.Tessellation, ob.SegmentLength]
                    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("dxfmesh"):
                        sh = None
                        if not ob.Shape.isNull():
                            writeMesh(ob, dxf)
                    elif gui and FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("dxfproject"):
                        _view = FreeCADGui.ActiveDocument.ActiveView
                        direction = _view.getViewDirection().multiply(-1)
                        sh = projectShape(ob.Shape, direction, tess)
                    else:
                        if ob.Shape.Volume > 0:
                            sh = projectShape(ob.Shape, Vector(0, 0, 1), tess)
                        else:
                            sh = ob.Shape
                    if sh:
                        if not sh.isNull():
                            if sh.ShapeType == 'Compound':
                                if len(sh.Wires) == 1:
                                    # only one wire in this compound,
                                    # no lone edge -> polyline
                                    if len(sh.Wires[0].Edges) == len(sh.Edges):
                                        writeShape(sh, ob, dxf,
                                                   nospline, lwPoly)
                                    else:
                                        # 1 wire + lone edges -> block
                                        block = getBlock(sh, ob, lwPoly)
                                        dxf.blocks.append(block)
                                        dxf.append(dxfLibrary.Insert(name=ob.Name.upper(),
                                                                     color=getACI(ob),
                                                                     layer=getStrGroup(ob)))
                                else:
                                    # all other cases: block
                                    block = getBlock(sh, ob, lwPoly)
                                    dxf.blocks.append(block)
                                    dxf.append(dxfLibrary.Insert(name=ob.Name.upper(),
                                                                 color=getACI(ob),
                                                                 layer=getStrGroup(ob)))
                            else:
                                writeShape(sh, ob, dxf, nospline, lwPoly)

                elif obtype == "Annotation":
                    # old-style texts
                    # temporary - as dxfLibrary doesn't support mtexts well,
                    # we use several single-line texts
                    # well, anyway, at the moment, Draft only writes
                    # single-line texts, so...
                    for text in ob.LabelText:
                        point = DraftVecUtils.tup(Vector(ob.Position.x,
                                                         ob.Position.y - ob.LabelText.index(text),
                                                         ob.Position.z))
                        if gui:
                            height = float(ob.ViewObject.FontSize)
                        else:
                            height = 1
                        dxf.append(dxfLibrary.Text(text, point, height=height,
                                                   color=getACI(ob, text=True),
                                                   style='STANDARD',
                                                   layer=getStrGroup(ob)))

                elif obtype in ("DraftText","Text"):
                    # texts
                    if gui:
                        height = float(ob.ViewObject.FontSize)
                    else:
                        height = 1
                    for text in ob.Text:
                        point = DraftVecUtils.tup(Vector(ob.Placement.Base.x,
                                                         ob.Placement.Base.y - (height * 1.2 * ob.Text.index(text)),
                                                         ob.Placement.Base.z))
                        rotation = math.degrees(ob.Placement.Rotation.Angle)
                        dxf.append(dxfLibrary.Text(text,
                                                   point,
                                                   height=height * 0.8,
                                                   rotation=rotation,
                                                   color=getACI(ob, text=True),
                                                   style='STANDARD',
                                                   layer=getStrGroup(ob)))

                elif obtype in ["Dimension","LinearDimension"]:
                    p1 = DraftVecUtils.tup(ob.Start)
                    p2 = DraftVecUtils.tup(ob.End)
                    base = Part.LineSegment(ob.Start, ob.End).toShape()
                    proj = DraftGeomUtils.findDistance(ob.Dimline, base)
                    if not proj:
                        pbase = DraftVecUtils.tup(ob.End)
                    else:
                        pbase = DraftVecUtils.tup(ob.End.add(proj.negative()))
                    dxf.append(dxfLibrary.Dimension(pbase,
                                                    p1, p2,
                                                    color=getACI(ob),
                                                    layer=getStrGroup(ob)))

            dxf.saveas(filename)

        FCC.PrintMessage("successfully exported" + " " + filename + "\n")

    else:
        errorDXFLib(gui)


class dxfcounter:
    """DXF counter class to count the number of entities.
    """
    def __init__(self):
        # this leaves 10000 entities for the template
        self.count = 10000

    def incr(self, matchobj):
        self.count += 1
        # print(format(self.count, '02x'))
        return format(self.count, '02x')


def exportPage(page, filename):
    """Export a page created with Drawing or TechDraw workbenches.

    The template is extracted from the page.
    If the template exists in the system, it will be searched
    for editable text fields, and replaced with their text values.
    If no template is found a dummy default DXF template is used.

    For TechDraw pages their templates are not supported currently,
    so the dummy template will be used.

    It considers all views or groups in the page,
    and tries to get the blocks and entities with `getViewDXF(view)`.
    It also increments the counter by using the `dxfcounter` class.

    The blocks and entities are added to the template, and finally
    this template is written into the `filename`.

    Parameters
    ----------
    page : object derived from 'TechDraw::DrawPage'
        A TechDraw page to export.

    filename : str
        The path of the new DXF file.
    """
    if hasattr(page.Template, "Template"):  # techdraw
        template = ""  # not supported for now...
        views = page.Views
    else:  # drawing
        template = os.path.splitext(page.Template)[0] + ".dxf"
        views = page.Group
    if os.path.exists(template):
        f = pythonopen(template, "U")
        template = f.read()
        f.close()
        # find & replace editable texts
        f = pythonopen(page.Template, "rb")
        svgtemplate = f.read()
        f.close()
        editables = re.findall("freecad:editable=\"(.*?)\"", svgtemplate)
        values = page.EditableTexts
        for i in range(len(editables)):
            if len(values) > i:
                template = template.replace(editables[i], values[i])
    else:
        # dummy default template
        print("DXF version of the template not found. "
              "Creating a default empty template.")
        _v = FreeCAD.Version()
        _version = _v[0] + "." + _v[1] + "-" + _v[2]
        template = "999\nFreeCAD DXF exporter v" + _version + "\n"
        template += "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n"
        template += "0\nSECTION\n2\nBLOCKS\n999\n$blocks\n0\nENDSEC\n"
        template += "0\nSECTION\n2\nENTITIES\n999\n$entities\n0\nENDSEC\n"
        template += "0\nEOF"
    blocks = ""
    entities = ""
    r12 = False
    ver = re.findall("\$ACADVER\n.*?\n(.*?)\n", template)
    if ver:
        # at the moment this is not used.
        # TODO: if r12, do not print ellipses or splines
        if ver[0].upper() in ["AC1009", "AC1010", "AC1011",
                              "AC1012", "AC1013"]:
            r12 = True
    for view in views:
        b, e = getViewDXF(view)
        blocks += b
        entities += e
    if blocks:
        template = template.replace("999\n$blocks", blocks[:-1])
    if entities:
        template = template.replace("999\n$entities", entities[:-1])
    c = dxfcounter()
    pat = re.compile("(_handle_)")
    template = pat.sub(c.incr, template)
    f = pythonopen(filename, "w")
    f.write(template)
    f.close()


def getViewBlock(geom, view, blockcount):
    """Get a view block.

    It iterates over all `geom` objects.
    If the global variable `dxfExportBlocks` exists, it will create
    the appropriate strings for `BLOCK` and `INSERT` sections,
    and increment the `blockcount`.
    Otherwise, it will just create an insert by changing the layer,
    and setting a handle.

    Parameters
    ----------
    geom : list of str
        A list string objects or a single object, returned by
        the `getDXF()` method of the `view`.

    view : page view
        A TechDraw view which may be of different types
        depending on the objects being projected:
        `'TechDraw::DrawViewDraft'`, or `'TechDraw::DrawViewArch'`.

    blockcount : int
        A counter that increments by one each time an insert and block
        are added to the output strings, if the global variable
        `dxfExportBlocks` exists.

    Returns
    -------
    str, str, int
        A tuple containing the strings for blocks, inserts,
        and the final value of `blockcount`.

    To do
    -----
    Use local variables, not global variables.
    """
    insert = ""
    block = ""
    r = view.Rotation
    if r != 0:
        r = -r  # fix rotation direction
    if not isinstance(geom, list):
        geom = [geom]
    for g in geom:  # getDXF returns a list of entities
        if dxfExportBlocks:
            # change layer and set color and ltype to BYBLOCK (0)
            g = g.replace("sheet_layer\n",
                          "0\n6\nBYBLOCK\n62\n0\n5\n_handle_\n")
            block += "0\nBLOCK\n5\n_handle_\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockBegin\n2\n"
            block += view.Name + str(blockcount)
            block += "\n70\n0\n10\n0\n20\n0\n3\n"
            block += view.Name + str(blockcount) + "\n1\n\n"
            block += g
            block += "0\nENDBLK\n5\n_handle_\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockEnd\n"
            insert += "0\nINSERT\n5\n_handle_\n8\n0\n6\nBYLAYER\n62\n256\n2\n"
            insert += view.Name + str(blockcount)
            insert += "\n10\n" + str(view.X) + "\n20\n" + str(view.Y)
            insert += "\n30\n0\n41\n" + str(view.Scale) + "\n42\n" + str(view.Scale) + "\n43\n" + str(view.Scale)
            insert += "\n50\n" + str(r) + "\n"
            blockcount += 1
        else:
            # change layer, add handle
            g = g.replace("sheet_layer\n", "0\n5\n_handle_\n")
            insert += g
    return block, insert, blockcount


def getViewDXF(view):
    """Return a DXF fragment from a TechDraw view.

    Depending on the type of page view, it will try
    obtaining `geom`, the DXF representation of `view`,
    and then extract the block and insert strings
    with `getViewBlock(geom, view, blockcount)`,
    starting with a `blockcount` of 1.

    If the `view` is `'TechDraw::DrawViewPart'`,
    and if the global variable `dxfExportBlocks` exists, it will create
    the appropriate strings for `BLOCK` and `INSERT` sections,
    and increment the `blockcount`.
    Otherwise, it will just create an insert by changing the layer,
    and setting a handle

    Parameters
    ----------
    view : App::DocumentObjectGroup or page view
        A TechDraw view which may be of different types
        depending on the objects being projected:
        `'TechDraw::DrawViewDraft'`, `'TechDraw::DrawViewArch'`,
        `'TechDraw::DrawViewPart'`, `'TechDraw::DrawViewAnnotation'`

    Returns
    -------
    str, str
        It returns the two strings for DXF blocks and inserts.

    To do
    -----
    Use local variables, not global variables.
    """
    block = ""
    insert = ""
    blockcount = 1

    if view.isDerivedFrom("TechDraw::DrawViewDraft"):
        geom = Draft.get_dxf(view)
        block, insert, blockcount = getViewBlock(geom, view, blockcount)

    elif view.isDerivedFrom("TechDraw::DrawViewArch"):
        import ArchSectionPlane
        geom = ArchSectionPlane.getDXF(view)
        block, insert, blockcount = getViewBlock(geom, view, blockcount)

    elif view.isDerivedFrom("TechDraw::DrawViewPart"):
        import TechDraw
        for obj in view.Source:
            proj = TechDraw.projectToDXF(obj.Shape, view.Direction)
            if dxfExportBlocks:
                # change layer and set color and ltype to BYBLOCK (0)
                proj = proj.replace("sheet_layer\n",
                                    "0\n6\nBYBLOCK\n62\n0\n5\n_handle_\n")
                block += "0\nBLOCK\n5\n_handle_\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockBegin\n2\n"
                block += view.Name + str(blockcount)
                block += "\n70\n0\n10\n0\n20\n0\n3\n" + view.Name + str(blockcount)
                block += "\n1\n\n"
                block += proj
                block += "0\nENDBLK\n5\n_handle_\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockEnd\n"
                insert += "0\nINSERT\n5\n_handle_\n8\n0\n6\nBYLAYER\n62\n256\n2\n"
                insert += view.Name + str(blockcount)
                insert += "\n10\n" + str(view.X) + "\n20\n" + str(view.Y)
                insert += "\n30\n0\n41\n" + str(view.Scale)
                insert += "\n42\n" + str(view.Scale) + "\n43\n" + str(view.Scale)
                insert += "\n50\n" + str(view.Rotation) + "\n"
                blockcount += 1
            else:
                proj = proj.replace("sheet_layer\n", "0\n5\n_handle_\n")
                insert += proj # view.Rotation is ignored

    elif view.isDerivedFrom("TechDraw::DrawViewAnnotation"):
        insert = "0\nTEXT\n5\n_handle_\n8\n0\n100\nAcDbEntity\n100\nAcDbText\n5\n_handle_"
        insert += "\n10\n" + str(view.X) + "\n20\n" + str(view.Y)
        insert += "\n30\n0\n40\n" + str(view.Scale/2)
        insert += "\n50\n" + str(view.Rotation)
        insert += "\n1\n" + view.Text[0] + "\n"

    else:
        print("Unable to get DXF representation from view: ", view.Label)
    return block, insert


def readPreferences():
    """Read the preferences of the this module from the parameter database.

    It creates and sets the global variables:
    `dxfCreatePart`, `dxfCreateDraft`, `dxfCreateSketch`,
    `dxfDiscretizeCurves`, `dxfStarBlocks`, `dxfMakeBlocks`, `dxfJoin`,
    `dxfRenderPolylineWidth`, `dxfImportTexts`, `dxfImportLayouts`,
    `dxfImportPoints`, `dxfImportHatches`, `dxfUseStandardSize`,
    `dxfGetColors`, `dxfUseDraftVisGroups`, `dxfFillMode`,
    `dxfBrightBackground`, `dxfDefaultColor`, `dxfUseLegacyImporter`,
    `dxfExportBlocks`, `dxfScaling`, `dxfUseLegacyExporter`

    The parameter path is ``User parameter:BaseApp/Preferences/Mod/Draft``

    See also
    --------
    FreeCAD.ParamGet, FreeCAD.ParamGet.GetBool

    To do
    -----
    Use local variables, not global variables.
    """
    # reading parameters
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    if FreeCAD.GuiUp and p.GetBool("dxfShowDialog", False):
        FreeCADGui.showPreferences("Import-Export", 3)
    global dxfCreatePart, dxfCreateDraft, dxfCreateSketch
    global dxfDiscretizeCurves, dxfStarBlocks
    global dxfMakeBlocks, dxfJoin, dxfRenderPolylineWidth
    global dxfImportTexts, dxfImportLayouts
    global dxfImportPoints, dxfImportHatches, dxfUseStandardSize
    global dxfGetColors, dxfUseDraftVisGroups
    global dxfFillMode, dxfBrightBackground, dxfDefaultColor
    global dxfUseLegacyImporter, dxfExportBlocks, dxfScaling
    global dxfUseLegacyExporter
    dxfCreatePart = p.GetBool("dxfCreatePart", True)
    dxfCreateDraft = p.GetBool("dxfCreateDraft", False)
    dxfCreateSketch = p.GetBool("dxfCreateSketch", False)
    dxfDiscretizeCurves = p.GetBool("DiscretizeEllipses", True)
    dxfStarBlocks = p.GetBool("dxfstarblocks", False)
    dxfMakeBlocks = p.GetBool("groupLayers", False)
    dxfJoin = p.GetBool("joingeometry", False)
    dxfRenderPolylineWidth = p.GetBool("renderPolylineWidth", False)
    dxfImportTexts = p.GetBool("dxftext", False)
    dxfImportLayouts = p.GetBool("dxflayouts", False)
    dxfImportPoints = p.GetBool("dxfImportPoints", False)
    dxfImportHatches = p.GetBool("importDxfHatches", False)
    dxfUseStandardSize = p.GetBool("dxfStdSize", False)
    dxfGetColors = p.GetBool("dxfGetOriginalColors", False)
    dxfUseDraftVisGroups = p.GetBool("dxfUseDraftVisGroups", True)
    dxfFillMode = p.GetBool("fillmode", True)
    dxfUseLegacyImporter = p.GetBool("dxfUseLegacyImporter", False)
    dxfUseLegacyExporter = p.GetBool("dxfUseLegacyExporter", False)
    dxfBrightBackground = isBrightBackground()
    dxfDefaultColor = getColor()
    dxfExportBlocks = p.GetBool("dxfExportBlocks", True)
    dxfScaling = p.GetFloat("dxfScaling", 1.0)
