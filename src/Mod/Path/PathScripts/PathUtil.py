# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

'''
The purpose of this file is to collect some handy functions. The reason they
are not in PathUtils (and there is this confusing naming going on) is that
PathUtils depends on PathJob. Which makes it impossible to use the functions
and classes defined there in PathJob.

So if you add to this file and think about importing anything from PathScripts
other than PathLog, then it probably doesn't belong here.
'''

import six
import PathScripts.PathLog as PathLog
import PySide

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

def translate(context, text, disambig=None):
    return PySide.QtCore.QCoreApplication.translate(context, text, disambig)

def _getProperty(obj, prop):
    o = obj
    attr = obj
    name = None
    for name in prop.split('.'):
        o = attr
        if not hasattr(o, name):
            break
        attr = getattr(o, name)

    if o == attr:
        PathLog.warning(translate('PathGui', "%s has no property %s (%s))") % (obj.Label, prop, name))
        return (None, None, None)

    #PathLog.debug("found property %s of %s (%s: %s)" % (prop, obj.Label, name, attr))
    return(o, attr, name)

def getProperty(obj, prop):
    '''getProperty(obj, prop) ... answer obj's property defined by its canonical name.'''
    o, attr, name = _getProperty(obj, prop) # pylint: disable=unused-variable
    return attr

def getPropertyValueString(obj, prop):
    '''getPropertyValueString(obj, prop) ... answer a string representation of an object's property's value.'''
    attr = getProperty(obj, prop)
    if hasattr(attr, 'UserString'):
        return attr.UserString
    return str(attr)

def setProperty(obj, prop, value):
    '''setProperty(obj, prop, value) ... set the property value of obj's property defined by its canonical name.'''
    o, attr, name = _getProperty(obj, prop) # pylint: disable=unused-variable
    if o and name:
        setattr(o, name, value)

# NotValidBaseTypeIds = ['Sketcher::SketchObject']
NotValidBaseTypeIds = []


def isValidBaseObject(obj):
    '''isValidBaseObject(obj) ... returns true if the object can be used as a base for a job.'''
    if hasattr(obj, 'getParentGeoFeatureGroup') and obj.getParentGeoFeatureGroup():
        # Can't link to anything inside a geo feature group anymore
        PathLog.debug("%s is inside a geo feature group" % obj.Label)
        return False
    if hasattr(obj, 'BitBody') and hasattr(obj, 'BitShape'):
        # ToolBit's are not valid base objects
        return False
    if obj.TypeId in NotValidBaseTypeIds:
        PathLog.debug("%s is blacklisted (%s)" % (obj.Label, obj.TypeId))
        return False
    if hasattr(obj, 'Sheets') or hasattr(obj, 'TagText'): # Arch.Panels and Arch.PanelCut
        PathLog.debug("%s is not an Arch.Panel" % (obj.Label))
        return False
    import Part
    return not Part.getShape(obj).isNull()

def isSolid(obj):
    '''isSolid(obj) ... return True if the object is a valid solid.'''
    import Part
    shape = Part.getShape(obj)
    return not shape.isNull() and shape.Volume and shape.isClosed()

def opProperty(op, prop):
    '''opProperty(op, prop) ... return the value of property prop of the underlying operation (or None if prop does not exist)'''
    if hasattr(op, prop):
        return getattr(op, prop)
    if hasattr(op, 'Base'):
        return opProperty(op.Base, prop)
    return None

def toolControllerForOp(op):
    '''toolControllerForOp(op) ... return the tool controller used by the op.
    If the op doesn't have its own tool controller but has a Base object, return its tool controller.
    Otherwise return None.'''
    return opProperty(op, 'ToolController')

def getPublicObject(obj):
    '''getPublicObject(obj) ... returns the object which should be used to reference a feature of the given object.'''
    if hasattr(obj, 'getParentGeoFeatureGroup'):
        body = obj.getParentGeoFeatureGroup()
        if body:
            return getPublicObject(body)
    return obj

def clearExpressionEngine(obj):
    '''clearExpressionEngine(obj) ... removes all expressions from obj.

There is currently a bug that invalidates the DAG if an object
is deleted that still has one or more expressions attached to it.
Use this function to remove all expressions before deletion.'''
    if hasattr(obj, 'ExpressionEngine'):
        for attr, expr in obj.ExpressionEngine: # pylint: disable=unused-variable
            obj.setExpression(attr, None)

def toUnicode(string):
    '''toUnicode(string) ... returns a unicode version of string regardless of the python version.'''
    return six.text_type(string)

def isString(string):
    '''isString(string) ... return True if string is a string, regardless of string type and python version.'''
    return isinstance(string, six.string_types)

def keyValueIter(dictionary):
    '''keyValueIter(dict) ... return iterable object over dictionary's (key,value) tuples.'''
    return six.iteritems(dictionary)
