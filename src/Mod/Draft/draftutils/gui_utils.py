# ***************************************************************************
# *   (c) 2009, 2010                                                        *
# *   Yorik van Havre <yorik@uncreated.net>, Ken Cline <cline@frii.com>     *
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *   (c) 2020 Carlo Pavan <carlopa@gmail.com>                              *
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
"""Provides utility functions that deal with GUI interactions.

This module contains auxiliary functions which can be used
in other modules of the workbench, and which require
the graphical user interface (GUI), as they access the view providers
of the objects or the 3D view.
"""
## @package gui_utils
# \ingroup draftutils
# \brief Provides utility functions that deal with GUI interactions.

## \addtogroup draftutils
# @{
import importlib
import math
import os

import FreeCAD as App
from draftutils import params
from draftutils import utils
from draftutils.messages import _err, _wrn
from draftutils.translate import translate

if App.GuiUp:
    import FreeCADGui as Gui
    from pivy import coin
    from PySide import QtCore
    from PySide import QtGui
    # from PySide import QtSvg  # for load_texture


def get_3d_view():
    """Return the current 3D view.

    Returns
    -------
    Gui::View3DInventor
        The Active 3D View or `None`.
    """
    if not App.GuiUp:
        return None

    # FIXME The following two imports were added as part of PR4926
    # Also see discussion https://forum.freecadweb.org/viewtopic.php?f=3&t=60251
    import FreeCADGui as Gui
    from pivy import coin

    mw = Gui.getMainWindow()
    view = mw.getActiveWindow()
    if view is None:
        return None
    if not hasattr(view, "getSceneGraph"):
        return None
    return view


get3DView = get_3d_view


def autogroup(obj):
    """Add a given object to the defined Draft autogroup, if applicable.

    This function only works if the graphical interface is available.
    It checks that the `App.draftToolBar` class is available,
    which contains the group to use to automatically store
    new created objects.

    Originally, it worked with standard groups (`App::DocumentObjectGroup`),
    and Arch Workbench containers like `'Site'`, `'Building'`, `'Floor'`,
    and `'BuildingPart'`.

    Now it works with Draft Layers.

    Parameters
    ----------
    obj: App::DocumentObject
        Any type of object that will be stored in the group.
    """

    # check for required conditions for autogroup to work
    if not App.GuiUp:
        return
    if not hasattr(Gui,"draftToolBar"):
        return
    if not hasattr(Gui.draftToolBar,"autogroup"):
        return
    if Gui.draftToolBar.isConstructionMode():
        return

    # check first for objects that do autogroup themselves
    # at the moment only Arch_BuildingPart, which is an App::GeometryPython
    for par in App.ActiveDocument.findObjects(Type="App::GeometryPython"):
        if hasattr(par.Proxy,"autogroup"):
            if par.Proxy.autogroup(par,obj):
                return

    # autogroup code
    active_group = None
    if Gui.draftToolBar.autogroup is not None:
        active_group = App.ActiveDocument.getObject(Gui.draftToolBar.autogroup)
        if active_group:
            gr = active_group.Group
            if not obj in gr:
                gr.append(obj)
                active_group.Group = gr

    if Gui.ActiveDocument.ActiveView.getActiveObject("NativeIFC") is not None:
        # NativeIFC handling
        try:
            from nativeifc import ifc_tools
            parent = Gui.ActiveDocument.ActiveView.getActiveObject("NativeIFC")
            if parent != active_group:
                ifc_tools.aggregate(obj, parent)
        except:
            pass

    elif Gui.ActiveDocument.ActiveView.getActiveObject("Arch") is not None:
        # add object to active Arch Container
        active_arch_obj = Gui.ActiveDocument.ActiveView.getActiveObject("Arch")
        if active_arch_obj != active_group:
            if obj in active_arch_obj.InListRecursive:
                # do not autogroup if obj points to active_arch_obj to prevent cyclic references
                return
            active_arch_obj.addObject(obj)

    elif Gui.ActiveDocument.ActiveView.getActiveObject("part") is not None:
        # add object to active part and change it's placement accordingly
        # so object does not jump to different position, works with App::Link
        # if not scaled. Modified accordingly to realthunder suggestions
        active_part, parent, sub = Gui.ActiveDocument.ActiveView.getActiveObject("part", False)
        if active_part != active_group:
            if obj in active_part.InListRecursive:
                # do not autogroup if obj points to active_part to prevent cyclic references
                return
            matrix = parent.getSubObject(sub, retType=4)
            if matrix.hasScale() == App.ScaleType.Uniform:
                err = translate("draft",
                                "Unable to insert new object into "
                                "a scaled part")
                App.Console.PrintMessage(err)
                return
            inverse_placement = App.Placement(matrix.inverse())
            if utils.get_type(obj) == 'Point':
                point_vector = App.Vector(obj.X, obj.Y, obj.Z)
                real_point = inverse_placement.multVec(point_vector)
                obj.X = real_point.x
                obj.Y = real_point.y
                obj.Z = real_point.z
            elif utils.get_type(obj) in ["Dimension", "LinearDimension"]:
                obj.Start = inverse_placement.multVec(obj.Start)
                obj.End = inverse_placement.multVec(obj.End)
                obj.Dimline = inverse_placement.multVec(obj.Dimline)
                obj.Normal = inverse_placement.Rotation.multVec(obj.Normal)
                obj.Direction = inverse_placement.Rotation.multVec(obj.Direction)
            elif utils.get_type(obj) in ["Label"]:
                obj.Placement = App.Placement(inverse_placement.multiply(obj.Placement))
                obj.TargetPoint = inverse_placement.multVec(obj.TargetPoint)
            elif hasattr(obj,"Placement"):
                # every object that have a placement is processed here
                obj.Placement = App.Placement(inverse_placement.multiply(obj.Placement))

            active_part.addObject(obj)


def dim_symbol(symbol=None, invert=False):
    """Return the specified dimension symbol.

    Parameters
    ----------
    symbol: int, optional
        It defaults to `None`, in which it gets the value from the parameter
        database, `get_param("dimsymbol")`.

        A numerical value defines different markers
         * 0, `SoSphere`
         * 1, `SoSeparator` with a `SoLineSet`, a circle (in fact a 24 sided polygon)
         * 2, `SoSeparator` with a `soCone`
         * 3, `SoSeparator` with a `SoFaceSet`
         * 4, `SoSeparator` with a `SoLineSet`, calling `dim_dash`
         * Otherwise, `SoSphere`

    invert: bool, optional
        It defaults to `False`.
        If it is `True` and `symbol=2`, the cone will be rotated
        -90 degrees around the Z axis, otherwise the rotation is positive,
        +90 degrees.

    Returns
    -------
    Coin.SoNode
        A `Coin.SoSphere`, or `Coin.SoSeparator` (circle, cone, face, line)
        that will be used as a dimension symbol.
    """
    if symbol is None:
        symbol = params.get_param("dimsymbol")

    if symbol == 0:
        # marker = coin.SoMarkerSet()
        # marker.markerIndex = 80

        # Returning a sphere means that the bounding box will
        # be 3-dimensional; a marker will always be planar seen from any
        # orientation but it currently doesn't work correctly
        marker = coin.SoSphere()
        return marker
    elif symbol == 1:
        marker = coin.SoSeparator()
        v = coin.SoVertexProperty()
        for i in range(25):
            ang = math.radians(i * 15)
            v.vertex.set1Value(i, (math.sin(ang), math.cos(ang), 0))
        p = coin.SoLineSet()
        p.vertexProperty = v
        marker.addChild(p)
        return marker
    elif symbol == 2:
        marker = coin.SoSeparator()
        t = coin.SoTransform()
        t.translation.setValue((0, -2, 0))
        t.center.setValue((0, 2, 0))
        if invert:
            t.rotation.setValue(coin.SbVec3f((0, 0, 1)), -math.pi/2)
        else:
            t.rotation.setValue(coin.SbVec3f((0, 0, 1)), math.pi/2)
        c = coin.SoCone()
        c.height.setValue(4)
        marker.addChild(t)
        marker.addChild(c)
        return marker
    elif symbol == 3:
        marker = coin.SoSeparator()
        # hints are required otherwise only the bottom of the face is colored
        h = coin.SoShapeHints()
        h.vertexOrdering = h.COUNTERCLOCKWISE
        c = coin.SoCoordinate3()
        c.point.setValues([(-1, -2, 0), (0, 2, 0),
                           (1, 2, 0), (0, -2, 0)])
        f = coin.SoFaceSet()
        marker.addChild(h)
        marker.addChild(c)
        marker.addChild(f)
        return marker
    elif symbol == 4:
        return dim_dash((-1.5, -1.5, 0), (1.5, 1.5, 0))
    else:
        _wrn(translate("draft", "Symbol not implemented. Using a default symbol."))
        return coin.SoSphere()


dimSymbol = dim_symbol


def dim_dash(p1, p2):
    """Return a SoSeparator with a line used to make dimension dashes.

    It is used by `dim_symbol` to create line end symbols
    like `'Tick-2'`, `'DimOvershoot'`, and `'ExtOvershoot'` dashes.

    Parameters
    ----------
    p1: tuple of three floats or Base::Vector3
        A point to define a line vertex.

    p2: tuple of three floats or Base::Vector3
        A point to define a line vertex.

    Returns
    -------
    Coin.SoSeparator
        A Coin object with a `SoLineSet` created from `p1` and `p2`
        as vertices.
    """
    dash = coin.SoSeparator()
    v = coin.SoVertexProperty()
    v.vertex.set1Value(0, p1)
    v.vertex.set1Value(1, p2)
    line = coin.SoLineSet()
    line.vertexProperty = v
    dash.addChild(line)
    return dash


dimDash = dim_dash


def remove_hidden(objectslist):
    """Return only the visible objects in the list.

    This function only works if the graphical interface is available
    as the `Visibility` attribute is a property of the view provider
    (`obj.ViewObject`).

    Parameters
    ----------
    objectslist: list of App::DocumentObject
        List of any type of object.

    Returns
    -------
    list
        Return a copy of the input list without those objects
        for which `obj.ViewObject.Visibility` is `False`.

        If the graphical interface is not loaded
        the returned list is just a copy of the input list.
    """
    newlist = objectslist[:]
    for obj in objectslist:
        if obj.ViewObject:
            if not obj.ViewObject.isVisible():
                newlist.remove(obj)
    return newlist


removeHidden = remove_hidden


def get_diffuse_color(objs):
    """Get a (cumulative) diffuse color from one or more objects.

    If all tuples in the result are identical a list with a single tuple is
    returned. In theory all faces of an object can be set to the same diffuse
    color that is different from its shape color, but that seems rare. The
    function does not take that into account.

    Parameters
    ----------
    objs: a single object or an iterable with objects.

    Returns
    -------
    list of tuples
        The list will be empty if no valid object is found.
    """
    def _get_color(obj):
        if hasattr(obj, "ColoredElements"):
            if hasattr(obj, "Count") or hasattr(obj, "ElementCount"):
                # Link and Link array.
                if hasattr(obj, "Count"):
                    count = obj.Count
                    base = obj.Base
                else:
                    count = obj.ElementCount if obj.ElementCount > 0 else 1
                    base = obj.LinkedObject
                if base is None:
                    return []
                cols = _get_color(base) * count
                if obj.ColoredElements is None:
                    return cols
                face_num = len(base.Shape.Faces)
                for elm, overide in zip(obj.ColoredElements[1], obj.ViewObject.OverrideColorList):
                    if "Face" in elm: # Examples: "Face3" and "1.Face6". Int before "." is zero-based, other int is 1-based.
                        if "." in elm:
                            elm0, elm1 = elm.split(".")
                            i = (int(elm0) * face_num) + int(elm1[4:]) - 1
                            cols[i] = overide
                        else:
                            i = int(elm[4:]) - 1
                            for j in range(count):
                                cols[(j * face_num) + i] = overide
                return cols
            elif hasattr(obj, "ElementList"):
                # LinkGroup
                cols = []
                for sub in obj.ElementList:
                    sub_cols = _get_color(sub)
                    if obj.ColoredElements is None:
                        cols += sub_cols
                    else:
                        for elm, overide in zip(obj.ColoredElements[1], obj.ViewObject.OverrideColorList):
                            if sub.Name + ".Face" in elm:
                                i = int(elm[(len(sub.Name) + 5):]) - 1
                                sub_cols[i] = overide
                        cols += sub_cols
                return cols
            else:
                return []
        elif hasattr(obj.ViewObject, "DiffuseColor"):
            if len(obj.ViewObject.DiffuseColor) == len(obj.Shape.Faces):
                return obj.ViewObject.DiffuseColor
            else:
                col = obj.ViewObject.ShapeColor
                col = (col[0], col[1], col[2], 1.0 - obj.ViewObject.Transparency / 100.0)
                return [col] * len(obj.Shape.Faces)
        elif obj.hasExtension("App::GeoFeatureGroupExtension"):
            cols = []
            for sub in obj.Group:
                cols += _get_color(sub)
            return cols
        else:
            return []

    if not isinstance(objs, list):
        # Quick check to avoid processing a single object:
        obj = objs
        if not hasattr(obj, "ColoredElements") \
                and hasattr(obj.ViewObject, "DiffuseColor") \
                and (len(obj.ViewObject.DiffuseColor) == 1 \
                        or len(obj.ViewObject.DiffuseColor) == len(obj.Shape.Faces)):
            return obj.ViewObject.DiffuseColor
        # Create a list for further processing:
        objs = [objs]

    colors = []
    for obj in objs:
        colors += _get_color(obj)

    if len(colors) > 1:
        first = colors[0]
        for next in colors[1:]:
            if next != first:
                break
        else:
            colors = [first]
    return colors


def apply_current_style(objs):
    """Apply the current style to one or more objects.

    Parameters
    ----------
    objs: a single object or an iterable with objects.
    """
    if not isinstance(objs, list):
        objs = [objs]
    anno_style = utils.get_default_annotation_style()
    shape_style = utils.get_default_shape_style()
    for obj in objs:
        if not hasattr(obj, 'ViewObject'):
            continue
        vobj = obj.ViewObject
        props = vobj.PropertiesList
        style = anno_style if ("FontName" in props) else shape_style
        for prop in props:
            if prop in style:
                if style[prop][0] == "index":
                    if style[prop][2] in vobj.getEnumerationsOfProperty(prop):
                        setattr(vobj, prop, style[prop][2])
                elif style[prop][0] == "color":
                    setattr(vobj, prop, style[prop][1] & 0xFFFFFF00)
                else:
                    setattr(vobj, prop, style[prop][1])


def restore_view_object(obj, vp_module, vp_class, format=True, format_ref=None):
    """Restore the ViewObject if the object was saved without the GUI.

    Parameters
    ----------
    obj: App::DocumentObject
        Object whose ViewObject needs to be restored.

    vp_module: string
        View provider module. Must be in the draftviewproviders directory.

    vp_class: string
        View provider class.

    format: bool, optional
        Defaults to `True`.
        If `True` the `format_object` function is called to update the
        properties of the ViewObject.

    format_ref: App::DocumentObject, optional
        Defaults to `None`.
        Reference object to copy ViewObject properties from.
    """
    if not getattr(obj, "ViewObject", None):
        return
    vobj = obj.ViewObject
    if not getattr(vobj, "Proxy", None):
        vp_module = importlib.import_module("draftviewproviders." + vp_module)
        getattr(vp_module, vp_class)(vobj)
        if format:
            format_object(obj, format_ref)


def format_object(target, origin=None):
    """Apply visual properties to an object.

    This function only works if the graphical interface is available.

    If origin is `None` and target is not an annotation, the DefaultDrawStyle
    and DefaultDisplayMode preferences are applied. Else, the properties of
    origin are applied to target.

    If construction mode is active target is then placed in the construction
    group and the `constr` color is applied to its applicable color properties:
    TextColor, PointColor, LineColor, and ShapeColor.

    Parameters
    ----------
    target: App::DocumentObject

    origin: App::DocumentObject, optional
        Defaults to `None`.
        If construction mode is not active, its visual properties are assigned
        to `target`, with the exception of `BoundingBox`, `Proxy`, `RootNode`
        and `Visibility`.
    """
    if not target:
        return
    if not App.GuiUp:
        return
    if not hasattr(Gui, "draftToolBar"):
        return
    if not hasattr(target, 'ViewObject'):
        return
    obrep = target.ViewObject
    obprops = obrep.PropertiesList
    if origin and hasattr(origin, 'ViewObject'):
        matchrep = origin.ViewObject
        for p in matchrep.PropertiesList:
            if p not in ("DisplayMode", "BoundingBox",
                         "Proxy", "RootNode", "Visibility"):
                if p in obprops:
                    if not obrep.getEditorMode(p):
                        if hasattr(getattr(matchrep, p), "Value"):
                            val = getattr(matchrep, p).Value
                        else:
                            val = getattr(matchrep, p)
                        try:
                            setattr(obrep, p, val)
                        except Exception:
                            pass
        if matchrep.DisplayMode in obrep.listDisplayModes():
            obrep.DisplayMode = matchrep.DisplayMode
        if hasattr(obrep, "DiffuseColor"):
            difcol = get_diffuse_color(origin)
            if difcol:
                obrep.DiffuseColor = difcol
    elif "FontName" not in obprops:
        # Apply 2 Draft style preferences, other style preferences are applied by Core.
        if "DrawStyle" in obprops:
            obrep.DrawStyle = utils.DRAW_STYLES[params.get_param("DefaultDrawStyle")]
        if "DisplayMode" in obprops:
            dm = utils.DISPLAY_MODES[params.get_param("DefaultDisplayMode")]
            if dm in obrep.listDisplayModes():
                obrep.DisplayMode = dm
    if Gui.draftToolBar.isConstructionMode():
        doc = App.ActiveDocument
        col = params.get_param("constructioncolor") & 0xFFFFFF00
        grp = doc.getObject("Draft_Construction")
        if not grp:
            grp = doc.addObject("App::DocumentObjectGroup", "Draft_Construction")
            grp.Label = params.get_param("constructiongroupname")
        grp.addObject(target)
        if "TextColor" in obprops:
            obrep.TextColor = col
        if "PointColor" in obprops:
            obrep.PointColor = col
        if "LineColor" in obprops:
            obrep.LineColor = col
        if "ShapeColor" in obprops:
            obrep.ShapeColor = col
        if hasattr(obrep, "Transparency"):
            obrep.Transparency = 80


formatObject = format_object


def get_selection(gui=App.GuiUp):
    """Return the current selected objects.

    This function only works if the graphical interface is available
    as the selection module only works on the 3D view.

    It wraps around `Gui.Selection.getSelection`

    Parameters
    ----------
    gui: bool, optional
        It defaults to the value of `App.GuiUp`, which is `True`
        when the interface exists, and `False` otherwise.

        This value can be set to `False` to simulate
        when the interface is not available.

    Returns
    -------
    list of App::DocumentObject
        Returns a list of objects in the current selection.
        It can be an empty list if no object is selected.

        If the interface is not available, it returns `None`.
    """
    if gui:
        return Gui.Selection.getSelection()
    return None


getSelection = get_selection


def get_selection_ex(gui=App.GuiUp):
    """Return the current selected objects together with their subelements.

    This function only works if the graphical interface is available
    as the selection module only works on the 3D view.

    It wraps around `Gui.Selection.getSelectionEx`

    Parameters
    ----------
    gui: bool, optional
        It defaults to the value of `App.GuiUp`, which is `True`
        when the interface exists, and `False` otherwise.

        This value can be set to `False` to simulate
        when the interface is not available.

    Returns
    -------
    list of Gui::SelectionObject
        Returns a list of `Gui::SelectionObject` in the current selection.
        It can be an empty list if no object is selected.

        If the interface is not available, it returns `None`.

    Selection objects
    -----------------
    One `Gui::SelectionObject` has attributes that indicate which specific
    subelements, that is, vertices, wires, and faces, were selected.
    This can be useful to operate on the subelements themselves.
    If `G` is a `Gui::SelectionObject`
     * `G.Object` is the selected object
     * `G.ObjectName` is the name of the selected object
     * `G.HasSubObjects` is `True` if there are subelements in the selection
     * `G.SubObjects` is a tuple of the subelements' shapes
     * `G.SubElementNames` is a tuple of the subelements' names

    `SubObjects` and `SubElementNames` should be empty tuples
    if `HasSubObjects` is `False`.
    """
    if gui:
        return Gui.Selection.getSelectionEx()
    return None


getSelectionEx = get_selection_ex


def select(objs=None, gui=App.GuiUp):
    """Unselects everything and selects only the given list of objects.

    This function only works if the graphical interface is available
    as the selection module only works on the 3D view.

    Parameters
    ----------
    objs: list of App::DocumentObject, optional
        It defaults to `None`.
        Any type of scripted object.
        It may be a list of objects or a single object.

    gui: bool, optional
        It defaults to the value of `App.GuiUp`, which is `True`
        when the interface exists, and `False` otherwise.

        This value can be set to `False` to simulate
        when the interface is not available.
    """
    if gui:
        Gui.Selection.clearSelection()
        if objs:
            if not isinstance(objs, list):
                objs = [objs]
            for obj in objs:
                if obj:
                    Gui.Selection.addSelection(obj)


def load_texture(filename, size=None, gui=App.GuiUp):
    """Return a Coin.SoSFImage to use as a texture for a 2D plane.

    This function only works if the graphical interface is available
    as the visual properties that can be applied to a shape
    are attributes of the view provider (`obj.ViewObject`).

    Parameters
    ----------
    filename: str
        A path to a pixel image file (PNG) that can be used as a texture
        on the face of an object.

    size: tuple of two int, or a single int, optional
        It defaults to `None`.
        If a tuple is given, the two values define the width and height
        in pixels to which the loaded image will be scaled.
        If it is a single value, it is used for both dimensions.

        If it is `None`, the size will be determined from the `QImage`
        created from `filename`.

        CURRENTLY the input `size` parameter IS NOT USED.
        It always uses the `QImage` to determine this information.

    gui: bool, optional
        It defaults to the value of `App.GuiUp`, which is `True`
        when the interface exists, and `False` otherwise.

        This value can be set to `False` to simulate
        when the interface is not available.

    Returns
    -------
    coin.SoSFImage
        An image object with the appropriate size, number of components
        (grayscale, grayscale and transparency, color,
        color and transparency), and byte data.

        It returns `None` if the interface is not available,
        or if there is a problem creating the image.
    """
    if gui:
        # from pivy import coin
        # from PySide import QtGui, QtSvg
        try:
            p = QtGui.QImage(filename)

            if p.isNull():
                _wrn("load_texture: " + translate("draft", "image is Null"))

                if not os.path.exists(filename):
                    raise FileNotFoundError(-1,
                                            translate("draft", "filename does not exist "
                                                               "on the system or "
                                                               "in the resource file"),
                                            filename)

            # This is buggy so it was de-activated.
            #
            # TODO: allow SVGs to use resolutions
            # if size and (".svg" in filename.lower()):
            #    # this is a pattern, not a texture
            #    if isinstance(size, int):
            #        size = (size, size)
            #    svgr = QtSvg.QSvgRenderer(filename)
            #    p = QtGui.QImage(size[0], size[1],
            #                     QtGui.QImage.Format_ARGB32)
            #    pa = QtGui.QPainter()
            #    pa.begin(p)
            #    svgr.render(pa)
            #    pa.end()
            # else:
            #    p = QtGui.QImage(filename)
            size = coin.SbVec2s(p.width(), p.height())
            buffersize = p.sizeInBytes()
            width = size[0]
            height = size[1]
            numcomponents = int(buffersize / (width * height))

            img = coin.SoSFImage()
            byteList = bytearray()

            # The SoSFImage needs to be filled with bytes.
            # The pixel information is converted into a Qt color, gray,
            # red, green, blue, or transparency (alpha),
            # depending on the input image.
            for y in range(height):
                # line = width*numcomponents*(height-(y));
                for x in range(width):
                    rgba = p.pixel(x, y)
                    if numcomponents <= 2:
                        byteList.append(QtGui.qGray(rgba))

                        if numcomponents == 2:
                            byteList.append(QtGui.qAlpha(rgba))

                    elif numcomponents <= 4:
                        byteList.append(QtGui.qRed(rgba))
                        byteList.append(QtGui.qGreen(rgba))
                        byteList.append(QtGui.qBlue(rgba))

                        if numcomponents == 4:
                            byteList.append(QtGui.qAlpha(rgba))
                    # line += numcomponents

            _bytes = bytes(byteList)
            img.setValue(size, numcomponents, _bytes)
        except FileNotFoundError as exc:
            _wrn("load_texture: {0}, {1}".format(exc.strerror,
                                                 exc.filename))
            return None
        except Exception as exc:
            _wrn(str(exc))
            _wrn("load_texture: " + translate("draft", "unable to load texture"))
            return None
        else:
            return img
    return None


loadTexture = load_texture


def migrate_text_display_mode(obj_type="Text", mode="3D text", doc=None):
    """Migrate the display mode of objects of certain type."""
    if not doc:
        doc = App.activeDocument()

    for obj in doc.Objects:
        if utils.get_type(obj) == obj_type:
            obj.ViewObject.DisplayMode = mode


def get_bbox(obj, debug=False):
    """Return a BoundBox from any object that has a Coin RootNode.

    Normally the bounding box of an object can be taken
    from its `Part::TopoShape`.
    ::
        >>> print(obj.Shape.BoundBox)

    However, for objects without a `Shape`, such as those
    derived from `App::FeaturePython` like `Draft Text` and `Draft Dimension`,
    the bounding box can be calculated from the `RootNode` of the viewprovider.

    Parameters
    ----------
    obj: App::DocumentObject
        Any object that has a `ViewObject.RootNode`.

    Returns
    -------
    Base::BoundBox
        It returns a `BoundBox` object which has information like
        minimum and maximum values of X, Y, and Z, as well as bounding box
        center.

    None
        If there is a problem it will return `None`.
    """
    _name = "get_bbox"

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft", "No active document. Aborting."))
        return None

    if isinstance(obj, str):
        obj_str = obj

    found, obj = utils.find_object(obj, doc)
    if not found:
        _err(translate("draft", "Wrong input: object {} not in document.").format(obj_str))
        return None

    if (not hasattr(obj, "ViewObject")
            or not obj.ViewObject
            or not hasattr(obj.ViewObject, "RootNode")):
        _err(translate("draft", "Does not have 'ViewObject.RootNode'."))

    # For Draft Dimensions
    # node = obj.ViewObject.Proxy.node
    node = obj.ViewObject.RootNode

    view = Gui.ActiveDocument.ActiveView
    region = view.getViewer().getSoRenderManager().getViewportRegion()
    action = coin.SoGetBoundingBoxAction(region)

    node.getBoundingBox(action)
    bb = action.getBoundingBox()

    # xlength, ylength, zlength = bb.getSize().getValue()
    xmin, ymin, zmin = bb.getMin().getValue()
    xmax, ymax, zmax = bb.getMax().getValue()

    return App.BoundBox(xmin, ymin, zmin, xmax, ymax, zmax)


# Code by Yorik van Havre.
def find_coin_node(parent, nodetype):
    for i in range(parent.getNumChildren()):
        if isinstance(parent.getChild(i), nodetype):
            return parent.getChild(i)
    return None


# Code by Chris Hennes (chennes).
# See https://forum.freecadweb.org/viewtopic.php?p=656362#p656362.
# Used to fix https://github.com/FreeCAD/FreeCAD/issues/10469.
def end_all_events():
    view = get_3d_view()
    if view is None:
        return
    if view.getNavigationType() in (
            "Gui::GestureNavigationStyle", "Gui::MayaGestureNavigationStyle"
    ):
        return

    class DelayEnder:
        def __init__(self):
            self.delay_is_done = False
        def stop(self):
            self.delay_is_done = True
    ender = DelayEnder()
    timer = QtCore.QTimer()
    timer.timeout.connect(ender.stop)
    timer.setSingleShot(True)
    timer.start(100)  # 100ms (50ms is too short) timer guarantees the loop below runs at least that long
    while not ender.delay_is_done:
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)

## @}
