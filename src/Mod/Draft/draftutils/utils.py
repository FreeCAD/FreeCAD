# -*- coding: utf-8 -*-
# ***************************************************************************
# *   (c) 2009, 2010                                                        *
# *   Yorik van Havre <yorik@uncreated.net>, Ken Cline <cline@frii.com>     *
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
"""Provides general utility functions used throughout the workbench.

This module contains auxiliary functions which can be used
in other modules of the workbench, and which don't require
the graphical user interface (GUI).
"""
## @package utils
# \ingroup draftutils
# \brief Provides general utility functions used throughout the workbench.

## \addtogroup draftutils
# @{
import os
import PySide.QtCore as QtCore

import FreeCAD as App

from draftutils.messages import _msg, _wrn, _err, _log
from draftutils.translate import translate

# TODO: move the functions that require the graphical interface
# This module should not import any graphical commands; those should be
# in gui_utils
if App.GuiUp:
    import FreeCADGui as Gui
    import Draft_rc

    # The module is used to prevent complaints from code checkers (flake8)
    True if Draft_rc else False

ARROW_TYPES = ["Dot", "Circle", "Arrow", "Tick", "Tick-2"]
arrowtypes = ARROW_TYPES

param_draft = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
param_view  = App.ParamGet("User parameter:BaseApp/Preferences/View")

ANNOTATION_STYLE = {
    "ArrowSize":       ("float", param_draft.GetFloat("arrowsize", 20)),
    "ArrowType":       ("index", param_draft.GetInt("dimsymbol", 0)),
    "Decimals":        ("int",   param_draft.GetInt("dimPrecision", 2)),
    "DimOvershoot":    ("float", param_draft.GetFloat("dimovershoot", 20)),
    "ExtLines":        ("float", param_draft.GetFloat("extlines", 300)),
    "ExtOvershoot":    ("float", param_draft.GetFloat("extovershoot", 20)),
    "FontName":        ("font",  param_draft.GetString("textfont", "Sans")),
    "FontSize":        ("float", param_draft.GetFloat("textheight", 100)),
    "LineColor":       ("color", param_view.GetUnsigned("DefaultShapeLineColor", 255)),
    "LineSpacing":     ("float", param_draft.GetFloat("LineSpacing", 1)),
    "LineWidth":       ("int",   param_view.GetInt("DefaultShapeLineWidth", 1)),
    "ScaleMultiplier": ("float", 1),
    "ShowLine":        ("bool",  True),
    "ShowUnit":        ("bool",  param_draft.GetBool("showUnit", True)),
    "TextColor":       ("color", param_draft.GetUnsigned("DefaultTextColor", 255)),
    "TextSpacing":     ("float", param_draft.GetFloat("dimspacing", 20)),
    "UnitOverride":    ("str",   param_draft.GetString("overrideUnit", "")),
}


def string_encode_coin(ustr):
    """Encode a unicode object to be used as a string in coin.

    Parameters
    ----------
    ustr : str
        A string to be encoded

    Returns
    -------
    str
        Encoded string. If the coin version is >= 4
        it will encode the string to `'utf-8'`, otherwise
        it will encode it to `'latin-1'`.
    """
    try:
        from pivy import coin
        coin4 = coin.COIN_MAJOR_VERSION >= 4
    except (ImportError, AttributeError):
        coin4 = False
    if coin4:
        return ustr.encode('utf-8')
    else:
        return ustr.encode('latin1')


stringencodecoin = string_encode_coin


def type_check(args_and_types, name="?"):
    """Check that the arguments are instances of certain types.

    Parameters
    ----------
    args_and_types : list
        A list of tuples. The first element of a tuple is tested as being
        an instance of the second element.
        ::
            args_and_types = [(a, Type), (b, Type2), ...]

        Then
        ::
            isinstance(a, Type)
            isinstance(b, Type2)

        A `Type` can also be a tuple of many types, in which case
        the check is done for any of them.
        ::
            args_and_types = [(a, (Type3, int, float)), ...]

            isinstance(a, (Type3, int, float))

    name : str, optional
        Defaults to `'?'`. The name of the check.

    Raises
    ------
    TypeError
        If the first element in the tuple is not an instance of the second
        element, it raises `Draft.name`.
    """
    for v, t in args_and_types:
        if not isinstance(v, t):
            w = "typecheck[{}]: '{}' is not {}".format(name, v, t)
            _wrn(w)
            raise TypeError("Draft." + str(name))


typecheck = type_check


def get_param_type(param):
    """Return the type of the parameter entered.

    Parameters
    ----------
    param : str
        A string that indicates a parameter in the parameter database.

    Returns
    -------
    str or None
        The returned string could be `'int'`, `'string'`, `'float'`,
        `'bool'`, `'unsigned'`, depending on the parameter.
        It returns `None` for unhandled situations.
    """
    if param in ("dimsymbol", "dimPrecision", "dimorientation",
                 "precision", "defaultWP", "snapRange", "gridEvery",
                 "linewidth", "modconstrain", "modsnap",
                 "maxSnapEdges", "modalt", "HatchPatternResolution",
                 "snapStyle", "dimstyle", "gridSize", "gridTransparency"):
        return "int"
    elif param in ("constructiongroupname", "textfont",
                   "patternFile", "snapModes",
                   "FontFile", "ClonePrefix", "overrideUnit",
                   "labeltype", "gridSpacing") or "inCommandShortcut" in param:
        return "string"
    elif param in ("textheight", "tolerance",
                   "arrowsize", "extlines", "dimspacing",
                   "dimovershoot", "extovershoot", "HatchPatternSize"):
        return "float"
    elif param in ("selectBaseObjects", "alwaysSnap", "grid",
                   "fillmode", "saveonexit", "maxSnap",
                   "SvgLinesBlack", "dxfStdSize", "showSnapBar",
                   "hideSnapBar", "alwaysShowGrid", "renderPolylineWidth",
                   "showPlaneTracker", "UsePartPrimitives",
                   "DiscretizeEllipses", "showUnit", "coloredGridAxes",
                   "Draft_array_fuse", "Draft_array_Link", "gridBorder"):
        return "bool"
    elif param in ("color", "constructioncolor",
                   "snapcolor", "gridColor"):
        return "unsigned"
    else:
        return None


getParamType = get_param_type


def get_param(param, default=None):
    """Return a parameter value from the current parameter database.

    The parameter database is located in the tree
    ::
        'User parameter:BaseApp/Preferences/Mod/Draft'

    In the case that `param` is `'linewidth'` or `'color'` it will get
    the values from the View parameters
    ::
        'User parameter:BaseApp/Preferences/View/DefaultShapeLineWidth'
        'User parameter:BaseApp/Preferences/View/DefaultShapeLineColor'

    Parameters
    ----------
    param : str
        A string that indicates a parameter in the parameter database.

    default : optional
        It indicates the default value of the given parameter.
        It defaults to `None`, in which case it will use a specific
        value depending on the type of parameter determined
        with `get_param_type`.

    Returns
    -------
    int, or str, or float, or bool
        Depending on `param` and its type, by returning `ParameterGrp.GetInt`,
        `ParameterGrp.GetString`, `ParameterGrp.GetFloat`,
        `ParameterGrp.GetBool`, or `ParameterGrp.GetUnsinged`.
    """
    draft_params = "User parameter:BaseApp/Preferences/Mod/Draft"
    view_params = "User parameter:BaseApp/Preferences/View"

    p = App.ParamGet(draft_params)
    v = App.ParamGet(view_params)
    t = getParamType(param)
    # print("getting param ",param, " of type ",t, " default: ",str(default))
    if t == "int":
        if default is None:
            default = 0
        if param == "linewidth":
            return v.GetInt("DefaultShapeLineWidth", default)
        return p.GetInt(param, default)
    elif t == "string":
        if default is None:
            default = ""
        return p.GetString(param, default)
    elif t == "float":
        if default is None:
            default = 0
        return p.GetFloat(param, default)
    elif t == "bool":
        if default is None:
            default = False
        return p.GetBool(param, default)
    elif t == "unsigned":
        if default is None:
            default = 0
        if param == "color":
            return v.GetUnsigned("DefaultShapeLineColor", default)
        return p.GetUnsigned(param, default)
    else:
        return None


getParam = get_param


def set_param(param, value):
    """Set a Draft parameter with the given value.

    The parameter database is located in the tree
    ::
        'User parameter:BaseApp/Preferences/Mod/Draft'

    In the case that `param` is `'linewidth'` or `'color'` it will set
    the View parameters
    ::
        'User parameter:BaseApp/Preferences/View/DefaultShapeLineWidth'
        'User parameter:BaseApp/Preferences/View/DefaultShapeLineColor'

    Parameters
    ----------
    param : str
        A string that indicates a parameter in the parameter database.

    value : int, or str, or float, or bool
        The appropriate value of the parameter.
        Depending on `param` and its type, determined with `get_param_type`,
        it sets the appropriate value by calling `ParameterGrp.SetInt`,
        `ParameterGrp.SetString`, `ParameterGrp.SetFloat`,
        `ParameterGrp.SetBool`, or `ParameterGrp.SetUnsinged`.
    """
    draft_params = "User parameter:BaseApp/Preferences/Mod/Draft"
    view_params = "User parameter:BaseApp/Preferences/View"

    p = App.ParamGet(draft_params)
    v = App.ParamGet(view_params)
    t = getParamType(param)

    if t == "int":
        if param == "linewidth":
            v.SetInt("DefaultShapeLineWidth", value)
        else:
            p.SetInt(param, value)
    elif t == "string":
        p.SetString(param, value)
    elif t == "float":
        p.SetFloat(param, value)
    elif t == "bool":
        p.SetBool(param, value)
    elif t == "unsigned":
        if param == "color":
            v.SetUnsigned("DefaultShapeLineColor", value)
        else:
            p.SetUnsigned(param, value)


setParam = set_param


def precision():
    """Return the precision value from the parameter database.

    It is the number of decimal places that a float will have.
    Example
    ::
        precision=6, 0.123456
        precision=5, 0.12345
        precision=4, 0.1234

    Due to floating point operations there may be rounding errors.
    Therefore, this precision number is used to round up values
    so that all operations are consistent.
    By default the precision is 6 decimal places.

    Returns
    -------
    int
        get_param("precision", 6)
    """
    return getParam("precision", 6)


def tolerance():
    """Return the tolerance value from the parameter database.

    This specifies a tolerance around a quantity.
    ::
        value + tolerance
        value - tolerance

    By default the tolerance is 0.05.

    Returns
    -------
    float
        get_param("tolerance", 0.05)
    """
    return getParam("tolerance", 0.05)


def epsilon():
    """Return a small number based on the tolerance for use in comparisons.

    The epsilon value is used in floating point comparisons. Use with caution.
    ::
        denom = 10**tolerance
        num = 1
        epsilon = num/denom

    Returns
    -------
    float
        1/(10**tolerance)
    """
    return 1.0/(10.0**tolerance())


def get_real_name(name):
    """Strip the trailing numbers from a string to get only the letters.

    Parameters
    ----------
    name : str
        A string that may have a number at the end, `Line001`.

    Returns
    -------
    str
        A string without the numbers at the end, `Line`.
        The returned string cannot be empty; it will have
        at least one letter.
    """
    for i in range(1, len(name) + 1):
        if name[-i] not in '1234567890':
            return name[:len(name) - (i - 1)]
    return name


getRealName = get_real_name


def get_type(obj):
    """Return a string indicating the type of the given object.

    Parameters
    ----------
    obj : App::DocumentObject
        Any type of scripted object created with Draft,
        or any other workbench.

    Returns
    -------
    str
        If `obj` has a `Proxy`, it will return the value of `obj.Proxy.Type`.

        * If `obj` is a `Part.Shape`, returns `'Shape'`

        * If `obj` has a `TypeId`, returns `obj.TypeId`

        In other cases, it will return `'Unknown'`,
        or `None` if `obj` is `None`.
    """
    import Part
    if not obj:
        return None
    if isinstance(obj, Part.Shape):
        return "Shape"
    if hasattr(obj, 'Proxy') and hasattr(obj.Proxy, "Type"):
        return obj.Proxy.Type
    if hasattr(obj, 'TypeId'):
        return obj.TypeId
    return "Unknown"


getType = get_type


def get_objects_of_type(objects, typ):
    """Return only the objects that match the type in the list of objects.

    Parameters
    ----------
    objects : list of App::DocumentObject
        A list of objects which will be tested.

    typ : str
        A string that indicates a type. This should be one of the types
        that can be returned by `get_type`.

    Returns
    -------
    list of objects
        Only the objects that match `typ` will be added to the output list.
    """
    objs = []
    for o in objects:
        if getType(o) == typ:
            objs.append(o)
    return objs


getObjectsOfType = get_objects_of_type


def is_clone(obj, objtype=None, recursive=False):
    """Return True if the given object is a clone of a certain type.

    A clone is of type `'Clone'`, and has a reference
    to the original object inside its `Objects` attribute,
    which is an `'App::PropertyLinkListGlobal'`.

    The `Objects` attribute can point to another `'Clone'` object.
    If `recursive` is `True`, the function will be called recursively
    to further test this clone, until the type of the original object
    can be compared to `objtype`.

    Parameters
    ----------
    obj : App::DocumentObject
        The clone object that will be tested for a certain type.

    objtype : str or list of str
        A type string such as one obtained from `get_type`.
        Or a list of such types.

    recursive : bool, optional
        It defaults to `False`.
        If it is `True`, this same function will be called recursively
        with `obj.Object[0]` as input.

        This option only works if `obj.Object[0]` is of type `'Clone'`,
        that is, if `obj` is a clone of a clone.

    Returns
    -------
    bool
        Returns `True` if `obj` is of type `'Clone'`,
        and `obj.Object[0]` is of type `objtype`.

        If `objtype` is a list, then `obj.Objects[0]`
        will be tested against each of the elements in the list,
        and it will return `True` if at least one element matches the type.

        If `obj` isn't of type `'Clone'` but has the `CloneOf` attribute,
        it will also return `True`.

        It returns `False` otherwise, for example,
        if `obj` is not even a clone.
    """
    if isinstance(objtype, list):
        return any([is_clone(obj, t, recursive) for t in objtype])
    if getType(obj) == "Clone":
        if len(obj.Objects) == 1:
            if objtype:
                if getType(obj.Objects[0]) == objtype:
                    return True
            elif recursive and (getType(obj.Objects[0]) == "Clone"):
                return is_clone(obj.Objects[0], objtype, recursive)
    elif hasattr(obj, "CloneOf"):
        if obj.CloneOf:
            if objtype:
                if getType(obj.CloneOf) == objtype:
                    return True
            else:
                return True
    return False


isClone = is_clone


def get_clone_base(obj, strict=False, recursive=True):
    """Return the object cloned by this object, if any.

    Parameters
    ----------
    obj: App::DocumentObject
        Any type of object.

    strict: bool, optional
        It defaults to `False`.
        If it is `True`, and this object is not a clone,
        this function will return `False`.

    recursive: bool, optional
        It defaults to `True`
        If it is `True`, it call recursively to itself to
        get base object and if it is `False` then it just
        return base object, not call recursively to find
        base object.

    Returns
    -------
    App::DocumentObject
        It `obj` is a `Draft Clone`, it will return the first object
        that is in its `Objects` property.

        If `obj` has a `CloneOf` property, it will search iteratively
        inside the object pointed to by this property.

    obj
        If `obj` is not a `Draft Clone`, nor it has a `CloneOf` property,
        it will return the same `obj`, as long as `strict` is `False`.

    False
        It will return `False` if `obj` is not a clone,
        and `strict` is `True`.
    """
    if hasattr(obj, "CloneOf") and obj.CloneOf:
        if recursive:
            return get_clone_base(obj.CloneOf)
        return obj.CloneOf
    if get_type(obj) == "Clone" and obj.Objects:
        if recursive:
            return get_clone_base(obj.Objects[0])
        return obj.Objects[0]
    if strict:
        return False
    return obj


getCloneBase = get_clone_base


def shapify(obj):
    """Transform a parametric object into a static, non-parametric shape.

    Parameters
    ----------
    obj : App::DocumentObject
        Any type of scripted object.

        This object will be removed, and a non-parametric object
        with the same topological shape (`Part::TopoShape`)
        will be created.

    Returns
    -------
    Part::Feature
        The new object that takes `obj.Shape` as its own.

        Depending on the contents of the Shape, the resulting object
        will be named `'Face'`, `'Solid'`, `'Compound'`,
        `'Shell'`, `'Wire'`, `'Line'`, `'Circle'`,
        or the name returned by `get_real_name(obj.Name)`.

        If there is a problem with `obj.Shape`, it will return `None`,
        and the original object will not be modified.
    """
    try:
        shape = obj.Shape
    except Exception:
        return None

    if len(shape.Faces) == 1:
        name = "Face"
    elif len(shape.Solids) == 1:
        name = "Solid"
    elif len(shape.Solids) > 1:
        name = "Compound"
    elif len(shape.Faces) > 1:
        name = "Shell"
    elif len(shape.Wires) == 1:
        name = "Wire"
    elif len(shape.Edges) == 1:
        import DraftGeomUtils
        if DraftGeomUtils.geomType(shape.Edges[0]) == "Line":
            name = "Line"
        else:
            name = "Circle"
    else:
        name = getRealName(obj.Name)

    App.ActiveDocument.removeObject(obj.Name)
    newobj = App.ActiveDocument.addObject("Part::Feature", name)
    newobj.Shape = shape

    return newobj


def print_shape(shape):
    """Print detailed information of a topological shape.

    Parameters
    ----------
    shape : Part::TopoShape
        Any topological shape in an object, usually obtained from `obj.Shape`.
    """
    _msg(translate("draft", "Solids:") + " {}".format(len(shape.Solids)))
    _msg(translate("draft", "Faces:") + " {}".format(len(shape.Faces)))
    _msg(translate("draft", "Wires:") + " {}".format(len(shape.Wires)))
    _msg(translate("draft", "Edges:") + " {}".format(len(shape.Edges)))
    _msg(translate("draft", "Vertices:") + " {}".format(len(shape.Vertexes)))

    if shape.Faces:
        for f in range(len(shape.Faces)):
            _msg(translate("draft", "Face") + " {}:".format(f))
            for v in shape.Faces[f].Vertexes:
                _msg("    {}".format(v.Point))
    elif shape.Wires:
        for w in range(len(shape.Wires)):
            _msg(translate("draft", "Wire") + " {}:".format(w))
            for v in shape.Wires[w].Vertexes:
                _msg("    {}".format(v.Point))
    else:
        for v in shape.Vertexes:
            _msg("    {}".format(v.Point))


printShape = print_shape


def compare_objects(obj1, obj2):
    """Print the differences between 2 objects.

    The two objects are compared through their `TypeId` attribute,
    or by using the `get_type` function.

    If they are the same type their properties are compared
    looking for differences.

    Neither `Shape` nor `Label` attributes are compared.

    Parameters
    ----------
    obj1 : App::DocumentObject
        Any type of scripted object.
    obj2 : App::DocumentObject
        Any type of scripted object.
    """
    if obj1.TypeId != obj2.TypeId:
        _msg("'{0}' ({1}), '{2}' ({3}): ".format(obj1.Name, obj1.TypeId,
                                                 obj2.Name, obj2.TypeId)
             + translate("draft", "different types") + " (TypeId)")
    elif getType(obj1) != getType(obj2):
        _msg("'{0}' ({1}), '{2}' ({3}): ".format(obj1.Name, get_type(obj1),
                                                 obj2.Name, get_type(obj2))
             + translate("draft", "different types") + " (Proxy.Type)")
    else:
        for p in obj1.PropertiesList:
            if p in obj2.PropertiesList:
                if p in ("Shape", "Label"):
                    pass
                elif p == "Placement":
                    delta = obj1.Placement.Base.sub(obj2.Placement.Base)
                    text = translate("draft", "Objects have different placements. "
                                              "Distance between the two base points: ")
                    _msg(text + str(delta.Length))
                else:
                    if getattr(obj1, p) != getattr(obj2, p):
                        _msg("'{}' ".format(p) + translate("draft", "has a different value"))
            else:
                _msg("{} ".format(p)
                     + translate("draft", "doesn't exist in one of the objects"))


compareObjects = compare_objects


def load_svg_patterns():
    """Load the default Draft SVG patterns and user defined patterns.

    The SVG patterns are added as a dictionary to the `App.svgpatterns`
    attribute.
    """
    import importSVG
    App.svgpatterns = {}

    # Get default patterns in the resource file
    patfiles = QtCore.QDir(":/patterns").entryList()
    for fn in patfiles:
        file = ":/patterns/" + str(fn)
        f = QtCore.QFile(file)
        f.open(QtCore.QIODevice.ReadOnly)
        p = importSVG.getContents(str(f.readAll()), 'pattern', True)
        if p:
            for k in p:
                p[k] = [p[k], file]
            App.svgpatterns.update(p)

    # Get patterns in a user defined file
    altpat = getParam("patternFile", "")
    if os.path.isdir(altpat):
        for f in os.listdir(altpat):
            if f[-4:].upper() == ".SVG":
                file = os.path.join(altpat, f)
                p = importSVG.getContents(file, 'pattern')
                if p:
                    for k in p:
                        p[k] = [p[k], file]
                    App.svgpatterns.update(p)

    # Get TechDraw patterns
    altpat = os.path.join(App.getResourceDir(),"Mod","TechDraw","Patterns")
    if os.path.isdir(altpat):
        for f in os.listdir(altpat):
            if f[-4:].upper() == ".SVG":
                file = os.path.join(altpat, f)
                p = importSVG.getContents(file, 'pattern')
                if p:
                    for k in p:
                        p[k] = [p[k], file]
                else:
                    # some TD pattern files have no <pattern> definition but can still be used by Draft
                    p = {f[:-4]:["<pattern></pattern>",file]}
                    App.svgpatterns.update(p)


loadSvgPatterns = load_svg_patterns


def svg_patterns():
    """Return a dictionary with installed SVG patterns.

    Returns
    -------
    dict
        Returns `App.svgpatterns` if it exists.
        Otherwise it calls `load_svg_patterns` to create it
        before returning it.
    """
    if hasattr(App, "svgpatterns"):
        return App.svgpatterns
    else:
        loadSvgPatterns()
        if hasattr(App, "svgpatterns"):
            return App.svgpatterns
    return {}


svgpatterns = svg_patterns


def get_rgb(color, testbw=True):
    """Return an RRGGBB value #000000 from a FreeCAD color.

    Parameters
    ----------
    color : list or tuple with RGB values
        The values must be in the 0.0-1.0 range.
    testwb : bool (default = True)
        Pure white will be converted into pure black.
    """
    r = str(hex(int(color[0]*255)))[2:].zfill(2)
    g = str(hex(int(color[1]*255)))[2:].zfill(2)
    b = str(hex(int(color[2]*255)))[2:].zfill(2)
    col = "#"+r+g+b
    if testbw:
        if col == "#ffffff":
            # print(getParam('SvgLinesBlack'))
            if getParam('SvgLinesBlack', True):
                col = "#000000"
    return col


getrgb = get_rgb


def argb_to_rgba(color):
    """Change byte order of a 4 byte color int from ARGB (Qt) to RGBA (FreeCAD).

    Alpha in both integers is always 255.
    Alpha in color properties, although ignored, is always zero however.

    Usage:

        qt_int = self.form.ShapeColor.property("color").rgba() # Note: returns ARGB int
        qt_int = self.form.ShapeColor.property("color").rgb()  # Note: returns ARGB int
        fc_int = argb_to_rgba(qt_int)

        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")\
            .SetUnsigned("DefaultShapeColor", fc_int)

        obj.ViewObject.ShapeColor = fc_int & 0xFFFFFF00

    Related:

        getRgbF() returns an RGBA tuple. 4 floats in the range 0.0 - 1.0. Alpha is always 1.
        Alpha should be set to zero or removed before using the tuple to change a color property:

        obj.ViewObject.ShapeColor = self.form.ShapeColor.property("color").getRgbF()[:3]
    """
    return ((color & 0xFFFFFF) << 8) + ((color & 0xFF000000) >> 24)


def rgba_to_argb(color):
    """Change byte order of a 4 byte color int from RGBA (FreeCAD) to ARGB (Qt).
    """
    return ((color & 0xFFFFFF00) >> 8) + ((color & 0xFF) << 24)


def filter_objects_for_modifiers(objects, isCopied=False):
    filteredObjects = []
    for obj in objects:
        if hasattr(obj, "MoveBase") and obj.MoveBase and obj.Base:
            parents = []
            for parent in obj.Base.InList:
                if parent.isDerivedFrom("Part::Feature"):
                    parents.append(parent.Name)
            if len(parents) > 1:
                warningMessage = translate("draft", "%s shares a base with %d other objects. Please check if you want to modify this.") % (obj.Name,len(parents) - 1)
                App.Console.PrintError(warningMessage)
                if App.GuiUp:
                    Gui.getMainWindow().showMessage(warningMessage, 0)
            filteredObjects.append(obj.Base)
        elif hasattr(obj,"Placement") and obj.getEditorMode("Placement") == ["ReadOnly"] and not isCopied:
            App.Console.PrintError(translate("draft", "%s cannot be modified because its placement is readonly.") % obj.Name)
            continue
        else:
            filteredObjects.append(obj)
    return filteredObjects


filterObjectsForModifiers = filter_objects_for_modifiers


def is_closed_edge(edge_index, object):
    return edge_index + 1 >= len(object.Points)


isClosedEdge = is_closed_edge


def utf8_decode(text):
    r"""Decode the input string and return a unicode string.

    Python 2:
    ::
        str -> unicode
        unicode -> unicode

    Python 3:
    ::
        str -> str
        bytes -> str

    It runs
    ::
        try:
            return text.decode("utf-8")
        except AttributeError:
            return text

    Parameters
    ----------
    text : str, unicode or bytes
        A str, unicode, or bytes object that may have unicode characters
        like accented characters.

        In Python 2, a `bytes` object can include accented characters,
        but in Python 3 it must only contain ASCII literal characters.

    Returns
    -------
    unicode or str
        In Python 2 it will try decoding the `bytes` string
        and return a `'utf-8'` decoded string.

        >>> "Aá".decode("utf-8")
        >>> b"Aá".decode("utf-8")
        u'A\xe1'

        In Python 2 the unicode string is prefixed with `u`,
        and unicode characters are replaced by their two-digit hexadecimal
        representation, or four digit unicode escape.

        >>> "AáBẃCñ".decode("utf-8")
        u'A\xe1B\u1e83C\xf1'

        In Python 2 it will always return a `unicode` object.

        In Python 3 a regular string is already unicode encoded,
        so strings have no `decode` method. In this case, `text`
        will be returned as is.

        In Python 3, if `text` is a `bytes` object, then it will be converted
        to `str`; in this case, the `bytes` object cannot have accents,
        it must only contain ASCII literal characters.

        >>> b"ABC".decode("utf-8")
        'ABC'

        In Python 3 it will always return a `str` object, with no prefix.
    """
    try:
        return text.decode("utf-8")
    except AttributeError:
        return text


def print_header(name, description, debug=True):
    """Print a line to the console when something is called, and log it.

    Parameters
    ----------
    name: str
        The name of the function or class that is being called.
        This `name` will be logged in the log file, so if there are problems
        the log file can be investigated for clues.

    description: str
        Arbitrary text that will be printed to the console
        when the function or class is called.

    debug: bool, optional
        It defaults to `True`.
        If it is `False` the `description` will not be printed
        to the console.
        On the other hand the `name` will always be logged.
    """
    _log(name)
    if debug:
        _msg(16 * "-")
        _msg(description)


def find_doc(doc=None):
    """Return the active document or find a document by name.

    Parameters
    ----------
    doc: App::Document or str, optional
        The document that will be searched in the session.
        It defaults to `None`, in which case it tries to find
        the active document.
        If `doc` is a string, it will try to get the document by `Name`.

    Returns
    -------
    bool, App::Document
        A tuple containing the information on whether the search
        was successful. In this case, the boolean is `True`,
        and the second value is the document instance.

    False, None
        If there is no active document, or the string in `doc`
        doesn't correspond to an open document in the session.
    """
    FOUND = True

    if not doc:
        doc = App.activeDocument()
    if not doc:
        return not FOUND, None

    if isinstance(doc, str):
        try:
            doc = App.getDocument(doc)
        except NameError:
            _msg("document: {}".format(doc))
            _err(translate("draft", "Wrong input: unknown document."))
            return not FOUND, None

    return FOUND, doc


def find_object(obj, doc=None):
    """Find object in the document, inclusive by Label.

    Parameters
    ----------
    obj: App::DocumentObject or str
        The object to search in `doc`.
        Or if the `obj` is a string, it will search the object by `Label`.
        Since Labels are not guaranteed to be unique, it will get the first
        object with that label in the document.

    doc: App::Document or str, optional
        The document in which the object will be searched.
        It defaults to `None`, in which case it tries to search in the
        active document.
        If `doc` is a string, it will search the document by `Name`.

    Returns
    -------
    bool, App::DocumentObject
        A tuple containing the information on whether the search
        was successful. In this case, the boolean is `True`,
        and the second value is the object found.

    False, None
        If the object doesn't exist in the document.
    """
    FOUND = True

    found, doc = find_doc(doc)
    if not found:
        _err(translate("draft", "No active document. Aborting."))
        return not FOUND, None

    if isinstance(obj, str):
        try:
            obj = doc.getObjectsByLabel(obj)[0]
        except IndexError:
            return not FOUND, None

    if obj not in doc.Objects:
        return not FOUND, None

    return FOUND, obj


def use_instead(function, version=""):
    """Print a deprecation message and suggest another function.

    This function must be used inside the definition of a function
    that has been considered for deprecation, so we must provide
    an alternative.
    ::
        def old_function():
            use_instead('new_function', 1.0)

        def someFunction():
            use_instead('some_function')

    Parameters
    ----------
    function: str
        The name of the function to use instead of the current one.

    version: float or str, optional
        It defaults to the empty string `''`.
        The version where this command is to be deprecated, if it is known.
        If we don't know when this command will be deprecated
        then we should not give a version.
    """
    if version:
        _wrn(translate("draft", "This function will be deprecated in ")
             + "{}. ".format(version)
             + translate("draft", "Please use ") + "'{}'.".format(function))
    else:
        _wrn(translate("draft", "This function will be deprecated. ")
             + translate("draft", "Please use ") + "'{}'.".format(function))

## @}
