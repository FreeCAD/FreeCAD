# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
# *   Copyright (c) 2018 - Bernd Hahnebach <bernd@bimstatik.org>            *
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


__title__ = "FEM Utilities"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import FreeCAD as App


def createObject(doc, name, proxy, viewProxy):
    obj = doc.addObject(proxy.BaseType, name)
    proxy(obj)
    if App.GuiUp:
        viewProxy(obj.ViewObject)
    return obj


def findAnalysisOfMember(member):
    if member is None:
        raise ValueError("Member must not be None")
    for obj in member.Document.Objects:
        if obj.isDerivedFrom("Fem::FemAnalysis"):
            if member in obj.Group:
                return obj
            if _searchGroups(member, obj.Group):
                return obj
    return None


def _searchGroups(member, objs):
    for o in objs:
        if o == member:
            return True
        if hasattr(o, "Group"):
            return _searchGroups(member, o.Group)
    return False


def getMember(analysis, t):
    if analysis is None:
        raise ValueError("Analysis must not be None")
    matching = []
    for m in analysis.Group:
        if isDerivedFrom(m, t):
            matching.append(m)
    return matching


def getSingleMember(analysis, t):
    objs = getMember(analysis, t)
    return objs[0] if objs else None


def typeOfObj(obj):
    '''returns objects TypeId (C++ objects) or Proxy.Type (Python objects)'''
    if hasattr(obj, "Proxy") and hasattr(obj.Proxy, "Type"):
        return obj.Proxy.Type
    return obj.TypeId


def isOfTypeNew(obj, ty):
    '''returns if an object is of a given TypeId (C++ objects) or Proxy.Type (Python objects)'''
    if typeOfObj(obj) == ty:
        return True
    else:
        return False


def isOfType(obj, t):
    if hasattr(obj, "Proxy") and hasattr(obj.Proxy, "Type"):
        return obj.Proxy.Type == t
    return obj.TypeId == t


def isDerivedFrom(obj, t):
    if (hasattr(obj, "Proxy") and hasattr(obj.Proxy, "Type") and
            obj.Proxy.Type == t):
        return True
    return obj.isDerivedFrom(t)
