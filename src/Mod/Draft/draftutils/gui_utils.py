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
"""Provides GUI utility functions for the Draft Workbench.

This module contains auxiliary functions which can be used
in other modules of the workbench, and which require
the graphical user interface (GUI), as they access the view providers
of the objects or the 3D view.
"""
## @package gui_utils
# \ingroup DRAFT
# \brief This module provides GUI utility functions for the Draft Workbench

import math
import os
import six

import FreeCAD as App
from draftutils.messages import _msg, _wrn
from draftutils.utils import getParam
from draftutils.utils import get_type
from draftutils.translate import _tr, translate

if App.GuiUp:
    import FreeCADGui as Gui
    from pivy import coin
    from PySide import QtGui
    # from PySide import QtSvg  # for load_texture


def get_3d_view():
    """Return the current 3D view.

    Returns
    -------
    Gui::View3DInventor
        Return the current `ActiveView` in the active document,
        or the first `Gui::View3DInventor` view found.

        Return `None` if the graphical interface is not available.
    """
    if App.GuiUp:
        v = Gui.ActiveDocument.ActiveView
        if "View3DInventor" in str(type(v)):
            return v

        # print("Debug: Draft: Warning, not working in active view")
        v = Gui.ActiveDocument.mdiViewsOfType("Gui::View3DInventor")
        if v:
            return v[0]

    _wrn(_tr("No graphical interface"))
    return None


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
    
    # autogroup code
    if Gui.draftToolBar.autogroup is not None:
        active_group = App.ActiveDocument.getObject(Gui.draftToolBar.autogroup)
        if active_group:
            found = False
            for o in active_group.Group:
                if o.Name == obj.Name:
                    found = True
            if not found:
                gr = active_group.Group
                gr.append(obj)
                active_group.Group = gr
                
    else:

        if Gui.ActiveDocument.ActiveView.getActiveObject("Arch"):
            # add object to active Arch Container
            Gui.ActiveDocument.ActiveView.getActiveObject("Arch").addObject(obj)
            
        elif Gui.ActiveDocument.ActiveView.getActiveObject("part", False) is not None:
            # add object to active part and change it's placement accordingly
            # so object does not jump to different position, works with App::Link
            # if not scaled. Modified accordingly to realthunder suggestions
            p, parent, sub = Gui.ActiveDocument.ActiveView.getActiveObject("part", False)
            matrix = parent.getSubObject(sub, retType=4)
            if matrix.hasScale() == 1:
                err = translate("Draft",
                                "Unable to insert new object into "
                                "a scaled part")
                App.Console.PrintMessage(err)
                return
            inverse_placement = App.Placement(matrix.inverse())
            if get_type(obj) == 'Point':
                point_vector = App.Vector(obj.X, obj.Y, obj.Z)
                real_point = inverse_placement.multVec(point_vector)
                obj.X = real_point.x
                obj.Y = real_point.y
                obj.Z = real_point.z
            elif get_type(obj) in ["Dimension"]:
                obj.Start = inverse_placement.multVec(obj.Start)
                obj.End = inverse_placement.multVec(obj.End)
                obj.Dimline = inverse_placement.multVec(obj.Dimline)
                obj.Normal = inverse_placement.Rotation.multVec(obj.Normal)
                obj.Direction = inverse_placement.Rotation.multVec(obj.Direction)
            elif get_type(obj) in ["Label"]:
                obj.Placement = App.Placement(inverse_placement.multiply(obj.Placement))
                obj.TargetPoint = inverse_placement.multVec(obj.TargetPoint)
            elif hasattr(obj,"Placement"):
                # every object that have a placement is processed here
                obj.Placement = App.Placement(inverse_placement.multiply(obj.Placement))
            p.addObject(obj)


def dim_symbol(symbol=None, invert=False):
    """Return the specified dimension symbol.

    Parameters
    ----------
    symbol: int, optional
        It defaults to `None`, in which it gets the value from the parameter
        database, `get_param("dimsymbol", 0)`.

        A numerical value defines different markers
         * 0, `SoSphere`
         * 1, `SoMarkerSet` with a circle
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
        A `Coin.SoSphere`, or `Coin.SoMarkerSet` (circle),
        or `Coin.SoSeparator` (cone, face, line)
        that will be used as a dimension symbol.
    """
    if symbol is None:
        symbol = getParam("dimsymbol", 0)

    if symbol == 0:
        return coin.SoSphere()
    elif symbol == 1:
        marker = coin.SoMarkerSet()
        marker.markerIndex = Gui.getMarkerIndex("circle", 9)
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
        c = coin.SoCoordinate3()
        c.point.setValues([(-1, -2, 0), (0, 2, 0),
                           (1, 2, 0), (0, -2, 0)])
        f = coin.SoFaceSet()
        marker.addChild(c)
        marker.addChild(f)
        return marker
    elif symbol == 4:
        return dimDash((-1.5, -1.5, 0), (1.5, 1.5, 0))
    else:
        _wrn(_tr("Symbol not implemented. Use a default symbol."))
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
                _msg(_tr("Visibility off; removed from list: ") + obj.Label)
    return newlist


removeHidden = remove_hidden


def format_object(target, origin=None):
    """Apply visual properties from the Draft toolbar or another object.

    This function only works if the graphical interface is available
    as the visual properties are attributes of the view provider
    (`obj.ViewObject`).

    Parameters
    ----------
    target: App::DocumentObject
        Any type of scripted object.

        This object will adopt the applicable visual properties,
        `FontSize`, `TextColor`, `LineWidth`, `PointColor`, `LineColor`,
        and `ShapeColor`, defined in the Draft toolbar
        (`Gui.draftToolBar`) or will adopt
        the properties from the `origin` object.

        The `target` is also placed in the construction group
        if the construction mode in the Draft toolbar is active.

    origin: App::DocumentObject, optional
        It defaults to `None`.
        If it exists, it will provide the visual properties to assign
        to `target`, with the exception of `BoundingBox`, `Proxy`,
        `RootNode` and `Visibility`.
    """
    if not target:
        return
    obrep = target.ViewObject
    if not obrep:
        return
    ui = None
    if App.GuiUp:
        if hasattr(Gui, "draftToolBar"):
            ui = Gui.draftToolBar
    if ui:
        doc = App.ActiveDocument
        if ui.isConstructionMode():
            col = fcol = ui.getDefaultColor("constr")
            gname = getParam("constructiongroupname", "Construction")
            grp = doc.getObject(gname)
            if not grp:
                grp = doc.addObject("App::DocumentObjectGroup", gname)
            grp.addObject(target)
            if hasattr(obrep, "Transparency"):
                obrep.Transparency = 80
        else:
            col = ui.getDefaultColor("ui")
            fcol = ui.getDefaultColor("face")
        col = (float(col[0]), float(col[1]), float(col[2]), 0.0)
        fcol = (float(fcol[0]), float(fcol[1]), float(fcol[2]), 0.0)
        lw = ui.linewidth
        fs = ui.fontsize
        if not origin or not hasattr(origin, 'ViewObject'):
            if "FontSize" in obrep.PropertiesList:
                obrep.FontSize = fs
            if "TextColor" in obrep.PropertiesList:
                obrep.TextColor = col
            if "LineWidth" in obrep.PropertiesList:
                obrep.LineWidth = lw
            if "PointColor" in obrep.PropertiesList:
                obrep.PointColor = col
            if "LineColor" in obrep.PropertiesList:
                obrep.LineColor = col
            if "ShapeColor" in obrep.PropertiesList:
                obrep.ShapeColor = fcol
        else:
            matchrep = origin.ViewObject
            for p in matchrep.PropertiesList:
                if p not in ("DisplayMode", "BoundingBox",
                             "Proxy", "RootNode", "Visibility"):
                    if p in obrep.PropertiesList:
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
            if (hasattr(matchrep, "DiffuseColor")
                    and hasattr(obrep, "DiffuseColor")):
                obrep.DiffuseColor = matchrep.DiffuseColor


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
                _wrn("load_texture: " + _tr("image is Null"))

                if not os.path.exists(filename):
                    raise FileNotFoundError(-1,
                                            _tr("filename does not exist "
                                                "on the system or "
                                                "on the resource file"),
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
            buffersize = p.byteCount()
            width = size[0]
            height = size[1]
            numcomponents = int(float(buffersize) / (width * height))

            img = coin.SoSFImage()
            byteList = []
            # isPy2 = sys.version_info.major < 3
            isPy2 = six.PY2

            # The SoSFImage needs to be filled with bytes.
            # The pixel information is converted into a Qt color, gray,
            # red, green, blue, or transparency (alpha),
            # depending on the input image.
            #
            # If Python 2 is used, the color is turned into a character,
            # which is of type 'byte', and added to the byte list.
            # If Python 3 is used, characters are unicode strings,
            # so they need to be encoded into 'latin-1'
            # to produce the correct bytes for the list.
            for y in range(height):
                # line = width*numcomponents*(height-(y));
                for x in range(width):
                    rgb = p.pixel(x, y)
                    if numcomponents == 1 or numcomponents == 2:
                        gray = chr(QtGui.qGray(rgb))
                        if isPy2:
                            byteList.append(gray)
                        else:
                            byteList.append(gray.encode('latin-1'))

                        if numcomponents == 2:
                            alpha = chr(QtGui.qAlpha(rgb))
                            if isPy2:
                                byteList.append(alpha)
                            else:
                                byteList.append(alpha.encode('latin-1'))
                    elif numcomponents == 3 or numcomponents == 4:
                        red = chr(QtGui.qRed(rgb))
                        green = chr(QtGui.qGreen(rgb))
                        blue = chr(QtGui.qBlue(rgb))

                        if isPy2:
                            byteList.append(red)
                            byteList.append(green)
                            byteList.append(blue)
                        else:
                            byteList.append(red.encode('latin-1'))
                            byteList.append(green.encode('latin-1'))
                            byteList.append(blue.encode('latin-1'))

                        if numcomponents == 4:
                            alpha = chr(QtGui.qAlpha(rgb))
                            if isPy2:
                                byteList.append(alpha)
                            else:
                                byteList.append(alpha.encode('latin-1'))
                    # line += numcomponents

            _bytes = b"".join(byteList)
            img.setValue(size, numcomponents, _bytes)
        except FileNotFoundError as exc:
            _wrn("load_texture: {0}, {1}".format(exc.strerror,
                                                 exc.filename))
            return None
        except Exception as exc:
            _wrn(str(exc))
            _wrn("load_texture: " + _tr("unable to load texture"))
            return None
        else:
            return img
    return None


loadTexture = load_texture
