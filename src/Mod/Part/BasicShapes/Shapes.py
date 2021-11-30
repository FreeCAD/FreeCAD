# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

__title__ = "BasicShapes.Shapes"
__author__ = "Werner Mayer"
__url__ = "http://www.freecadweb.org"
__doc__ = "Basic shapes"


import FreeCAD
import Part


def makeTube(outerRadius, innerRadius, height):
    outer_cylinder = Part.makeCylinder(outerRadius, height)
    shape = outer_cylinder
    if innerRadius > 0 and innerRadius < outerRadius:
        inner_cylinder = Part.makeCylinder(innerRadius, height)
        shape = outer_cylinder.cut(inner_cylinder)
    return shape


class TubeFeature:
    def __init__(self, obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyLength","OuterRadius","Tube","Outer radius").OuterRadius = 5.0
        obj.addProperty("App::PropertyLength","InnerRadius","Tube","Inner radius").InnerRadius = 2.0
        obj.addProperty("App::PropertyLength","Height","Tube", "Height of the tube").Height = 10.0
        obj.addExtension("Part::AttachExtensionPython")

    def execute(self, fp):
        if fp.InnerRadius >= fp.OuterRadius:
            raise ValueError("Inner radius must be smaller than outer radius")
        fp.Shape = makeTube(fp.OuterRadius, fp.InnerRadius, fp.Height)

def addTube(doc, name="Tube"):
    """addTube(document, [name]): adds a tube object"""

    obj = doc.addObject("Part::FeaturePython", name)
    TubeFeature(obj)
    if FreeCAD.GuiUp:
        from . import ViewProviderShapes
        ViewProviderShapes.ViewProviderTube(obj.ViewObject)
    return obj

_ParamUnits = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units");
_ParamDecimals = None
_Decimals = None

class _ParamUnitsObserver:
    def __init__(self):
        _ParamUnits.Attach(self)
        self.update();

    def onChange(self, _grp, param):
        if param == 'Decimals':
            self.update()

    def update(self):
        global _ParamDecimals
        _ParamDecimals = _ParamUnits.GetInt('Decimals', 2)

_Observer = _ParamUnitsObserver()

def _ftostr(v):
    if _Decimals:
        res = '%.*f' % (_Decimals, v)
    else:
        res = '%g' % v
    if len(res) > 128: # probably infinite shape
        res = '?'
    return res

def _vec_tostr(v):
    return '(%s, %s, %s)' % (_ftostr(v[0]), _ftostr(v[1]), _ftostr(v[2]))

def _bbox_tostr(v):
    return '(%s, %s, %s), (%s, %s, %s)' % \
            (_ftostr(v.XMin), _ftostr(v.YMin), _ftostr(v.ZMin),
             _ftostr(v.XMax), _ftostr(v.YMax), _ftostr(v.ZMax))

def _pla_tostr(pla):
    angles = pla.Rotation.toEuler()
    return 'Pos%s, Yaw-Pitch-Roll(%s, %s, %s)' \
            % (_vec_tostr(pla.Base),
               _ftostr(angles[0]),
               _ftostr(angles[1]),
               _ftostr(angles[2]))

_presel_maxlen = 75

def _tostr(v):
    if isinstance(v, float):
        return _ftostr(v)
    if isinstance(v, FreeCAD.Vector):
        return _vec_tostr(v)
    if isinstance(v, Part.Vertex):
        return _vec_tostr(v.Point)
    if isinstance(v, FreeCAD.Placement):
        return _pla_tostr(v)
    if isinstance(v, tuple):
        res = ''
        for vv in v:
            if res:
                res += ', ';
            res += _tostr(vv)
            if len(res) > _presel_maxlen:
                res = res[:_presel_maxlen-5] + '...'
                break
        return '(' + res + ')'
    return str(v)

def _setupDecimals():
    global _Decimals
    try:
        from PySide import QtCore, QtGui
        if QtGui.QApplication.queryKeyboardModifiers() == QtCore.Qt.AltModifier:
            _Decimals = None
            return
    except Exception:
        pass
    _Decimals = _ParamDecimals

def showPreselectInfo():
    import FreeCADGui
    sel = FreeCADGui.Selection.getPreselection()
    try:
        if not sel.Object or len(sel.SubElementNames) != 1:
            return
    except Exception:
        return
    shape, _, obj = Part.getShape(sel.Object, sel.SubElementNames[0], retType=1)
    if shape.isNull():
        return
    _setupDecimals()
    txt = FreeCADGui.Selection.getPreselectionText()
    if txt:
        txt += '\n\n'
    point = sel.PickedPoints[0]
    txt += 'Picked point: %s' % _vec_tostr(point)
    maxlen = _presel_maxlen
    try:
        path, mappedName, elementName = Part.splitSubname(sel.SubElementNames[0])
        name = '\nObject: %s' % obj.Label
        if obj.Document == sel.Object.Document:
            if obj.Name != obj.Label:
                name += ' (%s)' % obj.Name
        else:
            name += ' (@%s' % obj.Document.Label
            if obj.Name != obj.Label:
                txt += '#' + obj.Name
            name += ')'
        if len(name) > maxlen:
            if name[-1] == ')':
                name = name[:maxlen-5] + ')'
            else:
                name = name[:maxlen-5]
        txt += name

        if path:
            path = '\nPath: %s.%s' % (sel.Object.Name, path)
            if len(path) > maxlen:
                path = path[:maxlen-5] + '..';
            txt += path
        if elementName:
            txt += '\nElement name: %s' % elementName
            if not mappedName:
                mappedName = shape.getElementMappedName(elementName)
            if mappedName:
                mappedName = '\nMapped name: %s' % mappedName
                if len(mappedName) > maxlen:
                    mappedName = mappedName[:maxlen-5] + '...'
                txt += mappedName
            shape = Part.getShape(obj, elementName, needSubElement=True)
    except Exception:
        shape = Part.getShape(sel.Object, sel.SubElementNames[0], needSubElement=True)

    if shape.isNull():
        return

    txt += '\nPlacement: %s' % _pla_tostr(shape.Placement)
    tol = getattr(shape, 'Tolerance', None)
    if tol is not None:
        txt += '\nTolerance: %g' % tol
    if shape.ShapeType == 'Vertex':
        FreeCADGui.Selection.setPreselectionText(txt)
        return

    if shape.ShapeType != 'Edge':
        elements = ''
        elementTypes = ['Solid', 'Shell', 'Face', 'Wire', 'Edge', 'Vertex']
        if shape.ShapeType == 'Compound':
            elementTypes.insert(0, 'SubShape')
        for element in elementTypes:
            try:
                n = shape.countElement(element)
                if n > 1:
                    if elements:
                        elements += ', '
                    elements += '%s: %d' % (element, n)
            except Exception:
                pass
        if elements:
            txt += '\n' + elements

    try:
        bbox = shape.BoundBox
        txt += '\nBoundBox: %s' % ('Infinite' if shape.isInfinite() else _bbox_tostr(bbox))
        txt += '\nBound center: %s' % _vec_tostr(bbox.Center)
    except Exception:
        pass

    txt = _getGeoAttributes(txt, shape, ('Orientation',
                                         ('Closed', 'isClosed'),
                                         ('FirstVertex', 'firstVertex'),
                                         ('LastVertex', 'lastVertex'),
                                         'ParameterRange',
                                        ))

    geo = getattr(shape, 'Curve', None)
    if geo:
        txt += '\nCurve type: %s' % geo.TypeId
        if point != FreeCAD.Vector():
            try:
                u = geo.parameter(point)
                txt += '\nParameter: %s' % _ftostr(u)
                txt += '\nNormal: %s' % _vec_tostr(geo.normal(u))
                txt += '\nTangent: %s' % _vec_tostr(geo.tangent(u)[0])
                txt += '\nCurvature: %s' % _ftostr(geo.curvature(u))
            except Exception:
                pass
    else:
        geo = getattr(shape, 'Surface', None)
        if geo:
            txt += '\nSurface type: %s' % geo.TypeId
            if point != FreeCAD.Vector():
                try:
                    u = geo.parameter(point)
                    txt += '\nParameter: %s' % _tostr(u)
                    txt += '\nNormal: %s' % _vec_tostr(geo.normal(*u))
                except Exception:
                    pass
                try:
                    t = geo.tangent(*u)
                    txt += '\nTangent: %s %s' % (_vec_tostr(t[0]), _vec_tostr(t[1]))
                    txt += '\nCurvature: %s' % _ftostr(geo.curvature(u[0], u[1],'Mean'))
                except Exception:
                    pass
    txt = _getGeoAttributes(txt, geo,
            ('Radius',
             'MajorRadius',
             'MinorRadius',
             'Center',
             'Axis',
             'Continuity',
             ('Planar', 'isPlanar'),
             ('Linear', 'isLinear')))
    FreeCADGui.Selection.setPreselectionText(txt)

def _getGeoAttributes(txt, geo, attrs):
    if not geo:
        return txt

    for attr in attrs:
        try:
            if isinstance(attr, tuple):
                func = getattr(geo, attr[1], None)
                value = func()
                attr = attr[0]
            else:
                value = getattr(geo, attr, None)
            if value is not None:
                if txt:
                    txt += '\n'
                txt += '%s: %s' % (attr, _tostr(value))
        except Exception:
            pass
    return txt

def showPreselectMeasure():
    import FreeCADGui
    try:
        count = FreeCADGui.Selection.countObjectsOfType()
        if count > 3:
            return
    except Exception:
        return
    presel = FreeCADGui.Selection.getPreselection()
    try:
        if not presel.Object or len(presel.SubElementNames) != 1:
            return
    except Exception:
        return
    shape = Part.getShape(presel.Object, presel.SubElementNames[0], needSubElement=True)
    if shape.isNull():
        return
    _setupDecimals()
    txt = ''
    if shape.ShapeType == 'Vertex':
        txt = 'Position: %s' % _vec_tostr(shape.Point)
    try:
        geo = getattr(shape, 'Curve', None)
        if not geo:
            geo = getattr(shape, 'Surface', None)
        txt = _getGeoAttributes(txt, geo, ('Radius',
                                           'MajorRadius',
                                           'MinorRadius',
                                           'Center'))
        import Measure
        m = Measure.Measurement(False)
        m.addReference3D(presel.Object.Name, presel.SubElementNames[0])
        for sel in FreeCADGui.Selection.getSelectionEx('', 0):
            for sub in sel.SubElementNames:
                m.addReference3D(sel.Object.Name, sub)
        txt = _getGeoAttributes(txt, m, (('Length', 'length'),
                                         ('Perimeter', 'perimeter'),
                                         ('Area', 'area'),
                                         ('Volume', 'volume'),
                                         ('Center of mass', 'com'),
                                         ('Angle', 'angle'),
                                         ('Delta', 'delta')))
        if txt:
            msg = FreeCADGui.Selection.getPreselectionText()
            if msg:
                msg += '\n\n' + txt
            else:
                msg = txt
            FreeCADGui.Selection.setPreselectionText(msg)
    except Exception:
        pass
