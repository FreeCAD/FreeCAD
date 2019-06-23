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

import PathScripts.PathLog as PathLog

__title__ = "Setup Sheet for a Job."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Prototype objects to allow extraction of setup sheet values and editing."


LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

class Property(object):
    '''Base class for all prototype properties'''
    def __init__(self, name, propType, category, info):
        self.name = name
        self.propType = propType
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

    def displayString(self):
        if self.value is None:
            t = self.typeString()
            p = 'an' if t[0] in ['A', 'E', 'I', 'O', 'U'] else 'a'
            return "%s %s" % (p, t)
        return self.value

    def typeString(self):
        return "Property"

    def setupProperty(self, obj, name, category, value):
        created = False
        if not hasattr(obj, name):
            obj.addProperty(self.propType, name, category, self.info)
            self.initProperty(obj, name)
            created = True
        setattr(obj, name, value)
        return created

    def initProperty(self, obj, name):
        pass

    def setValueFromString(self, string):
        self.setValue(self.valueFromString(string))

    def valueFromString(self, string):
        return string

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

    def initProperty(self, obj, name):
        setattr(obj, name, self.enums)

class PropertyQuantity(Property):
    def displayString(self):
        if self.value is None:
            return Property.displayString(self)
        return self.value.getUserPreferred()[0]

class PropertyDistance(PropertyQuantity):
    def typeString(self):
        return "Distance"

class PropertyLength(PropertyQuantity):
    def typeString(self):
        return "Length"

class PropertyPercent(Property):
    def typeString(self):
        return "Percent"

class PropertyFloat(Property):
    def typeString(self):
        return "Float"

class PropertyInteger(Property):
    def typeString(self):
        return "Integer"

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
            'App::PropertyFloatConstraint': Property,
            'App::PropertyFloatList': Property,
            'App::PropertyInteger': PropertyInteger,
            'App::PropertyIntegerList': PropertyInteger,
            'App::PropertyLength': PropertyLength,
            'App::PropertyLink': Property,
            'App::PropertyLinkList': Property,
            'App::PropertyLinkSubListGlobal': Property,
            'App::PropertyPercent': PropertyPercent,
            'App::PropertyString': PropertyString,
            'App::PropertyStringList': Property,
            'App::PropertyVectorDistance': Property,
            'App::PropertyVectorList': Property,
            'Part::PropertyPartShape': Property,
            }

    def __init__(self, name):
        self.Label = name
        self.properties = {}
        self.DoNotSetDefaultValues = True
        self.Proxy = None

    def __setattr__(self, name, val):
        if name in ['Label', 'DoNotSetDefaultValues', 'properties', 'Proxy']:
            if name == 'Proxy':
                val = None # make sure the proxy is never set
            return super(self.__class__, self).__setattr__(name, val)
        self.properties[name].setValue(val)

    def addProperty(self, typeString, name, category, info = None):
        prop = self.PropertyType[typeString](name, typeString, category, info)
        self.properties[name] = prop
        return self

    def setEditorMode(self, name, mode):
        self.properties[name].setEditorMode(mode)

    def getProperty(self, name):
        return self.properties[name]

    def setupProperties(self, setup):
        return [p for p in self.properties if p.name in setup]
