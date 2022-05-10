#! python
# -*- coding: utf-8 -*-
# ShapeContent.py
# 2021, by Mark Ganson <TheMarkster>
# LGPL 2.1 or later

# this file is called from c++ in TaskCheckGeometry.cpp
# from buildShapeContent()

import FreeCAD as App
import Part

def roundVector(v,dec):
    return str([round(v[0],dec), round(v[1],dec), round(v[2],dec)])

def buildShapeContent(objArg, decimals=2, advancedShapeContent=True):
    linkName = ""
    if objArg.isDerivedFrom("App::Link"):
        linkName = "<" + objArg.Name + "> "

    obj = objArg
    shp = Part.getShape(objArg)
    typeStr = str(shp.ShapeType)
    lbl = '' if obj.Name == obj.Label else '(' + obj.Label + ')'
    result = linkName + obj.Name + lbl + '\n'
    result += 'Shape type:  '+typeStr+'\n'
    result += 'Vertices:  '+str(len(shp.Vertexes))+'\n'
    result += 'Edges:  '+str(len(shp.Edges))+'\n'
    result += 'Wires:  '+str(len(shp.Wires))+'\n'
    result += 'Faces:  '+str(len(shp.Faces))+'\n'
    result += 'Shells:  '+str(len(shp.Shells))+'\n'
    result += 'Solids:  '+str(len(shp.Solids))+'\n'
    result += 'CompSolids:  '+str(len(shp.CompSolids))+'\n'
    result += 'Compounds:  '+str(len(shp.Compounds))+'\n'
    result += 'Shapes:  '+str(len(shp.Vertexes+shp.Edges+shp.Wires+shp.Faces+shp.Shells+shp.Solids+shp.CompSolids+shp.Compounds))+'\n'
    if advancedShapeContent:
        result += '----------\n'
        if hasattr(shp,'Area') and not 'Wire' in typeStr and not 'Edge' in typeStr and not 'Vertex' in typeStr:
            result += 'Area:  '+str(round(shp.Area, decimals))+'\n'
        if hasattr(shp,'Volume') and not 'Wire' in typeStr and not 'Edge' in typeStr and not 'Vertex' in typeStr and not 'Face' in typeStr:
            result += 'Volume:  '+str(round(shp.Volume, decimals))+'\n'
        if hasattr(shp,'Mass'):
            result += 'Mass:  '+str(round(shp.Mass, decimals))+'\n'
        if hasattr(shp,'Length'):
            result += 'Length:  '+str(round(shp.Length, decimals))+'\n'
        if hasattr(shp,'Curve') and hasattr(shp.Curve,'Radius'):
            result += 'Radius:  '+str(round(shp.Curve.Radius, decimals))+'\n'
        if hasattr(shp,'Curve') and hasattr(shp.Curve,'Center'):
            result += 'Curve center:  '+str([round(vv,decimals) for vv in shp.Curve.Center])+'\n'
        if hasattr(shp,'Curve') and hasattr(shp.Curve,'Continuity'):
            result += 'Continuity:  '+str(shp.Curve.Continuity)+'\n'
        if hasattr(shp,'CenterOfMass'):
            result += 'CenterOfMass:  '+roundVector(shp.CenterOfMass,decimals)+'\n'
        if hasattr(shp,'normalAt'):
            try:
                result += 'normalAt(0):  '+str([round(vv,decimals) for vv in shp.normalAt(0)]) +'\n'
            except Exception:
                try:
                    result += 'normalAt(0,0):  '+str([round(vv,decimals) for vv in shp.normalAt(0,0)]) +'\n'
                except Exception:
                    pass
        if hasattr(shp, 'isClosed') and ('Wire' in typeStr or 'Edge' in typeStr):
            result += 'isClosed:  '+str(shp.isClosed())+'\n'
        if hasattr(shp, 'Orientation'):
            result += 'Orientation:  '+str(shp.Orientation)+'\n'
        if hasattr(shp, 'PrincipalProperties'):
            props = shp.PrincipalProperties
            for p in props:
                if isinstance(props[p], App.Vector) or isinstance(props[p], tuple):
                    result += str(p)+':  '+roundVector(props[p],decimals) +'\n'
                else:
                    result += str(p)+':  '+str(props[p])+'\n'
        if hasattr(obj,"getGlobalPlacement"):
            if obj.getGlobalPlacement() != obj.Placement:
                rpl = obj.getGlobalPlacement() * obj.Placement.inverse()
                rot = rpl.Rotation
                if hasattr(shp, 'CenterOfMass'):
                    result += 'Global CenterOfMass:  '+roundVector(rpl.multVec(shp.CenterOfMass),decimals)+'\n'
                if hasattr(shp, 'PrincipalProperties'):
                    props = shp.PrincipalProperties
                    for p in props:
                        if 'AxisOfInertia' in p:
                            result += 'Global ' + str(p)+':  '+roundVector(rot.multVec(props[p]),decimals) +'\n'
            else:
                result += 'Global Placement = Placement'
    return result
