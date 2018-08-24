# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import Path
import PathScripts.PathLog as PathLog

__title__ = "Setup Sheet for a Job."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Prototype objects to allow extraction of setup sheet values and editing."


if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

class Property(object):
    '''Base class for all prototype properties'''
    def __init__(self, name, category, info):
        self.name = name
        self.category = category
        self.info = info
        self.editorMode = 0
        self.value = None

    def setValue(self, value):
        self.value = value
    def getValue(self):
        return self.value

    def setEditorMode(self, mode):
        self.editorMode = mode

    def toString(self):
        if not self.value is None:
            return str(self.value)
        t = self.typeString()
        p = 'an' if t[0] in ['A', 'E', 'I', 'O', 'U'] else 'a'
        return "%s %s" % (p, t)

    def typeString(self):
        return "Property"

class PropertyEnumeration(Property):
    def typeString(self):
        return "Enumeration"

    def setValue(self, value):
        if list == type(value):
            self.enums = value
        else:
            super(self.__class__, self).setValue(value)

    def getEnumValues(self):
        return self.enums

class PropertyDistance(Property):
    def typeString(self):
        return "Distance"

class PropertyPercent(Property):
    def typeString(self):
        return "Percent"

class PropertyFloat(Property):
    def typeString(self):
        return "Float"

class PropertyBool(Property):
    def typeString(self):
        return "Bool"

class PropertyString(Property):
    def typeString(self):
        return "String"

class OpPrototype(object):

    PropertyType = {
            'App::PropertyBool': PropertyBool,
            'App::PropertyDistance': PropertyDistance,
            'App::PropertyEnumeration': PropertyEnumeration,
            'App::PropertyFloat': PropertyFloat,
            'App::PropertyLink': Property,
            'App::PropertyLinkSubListGlobal': Property,
            'App::PropertyPercent': PropertyPercent,
            'App::PropertyString': PropertyString,
            'App::PropertyVectorDistance': Property,
            'Part::PropertyPartShape': Property,
            }

    def __init__(self):
        self.properties = {}
        self.DoNotSetDefaultValues = True

    def __setattr__(self, name, val):
        if name in ['properties', 'DoNotSetDefaultValues']:
            return super(self.__class__, self).__setattr__(name, val)
        self.properties[name].setValue(val)

    def addProperty(self, typeString, name, category, info = None):
        prop = self.PropertyType[typeString](name, category, info)
        self.properties[name] = prop

    def setEditorMode(self, name, mode):
        self.properties[name].setEditorMode(mode)

    def getProperty(self, name):
        return self.properties[name]

    def setupProperties(self, setup):
        return [p for p in self.properties if p.name in setup]
