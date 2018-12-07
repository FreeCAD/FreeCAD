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

import PathScripts.PathLog as PathLog
import sys

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# NotValidBaseTypeIds = ['Sketcher::SketchObject']
NotValidBaseTypeIds = []


def isValidBaseObject(obj):
    '''isValidBaseObject(obj) ... returns true if the object can be used as a base for a job.'''
    if hasattr(obj, 'getParentGeoFeatureGroup') and obj.getParentGeoFeatureGroup():
        # Can't link to anything inside a geo feature group anymore
        PathLog.debug("%s is inside a geo feature group" % obj.Label)
        return False
    if hasattr(obj, 'TypeId') and 'App::Part' == obj.TypeId:
        return obj.Group and any(hasattr(o, 'Shape') for o in obj.Group)
    if not hasattr(obj, 'Shape'):
        PathLog.debug("%s has no shape" % obj.Label)
        return False
    if obj.TypeId in NotValidBaseTypeIds:
        PathLog.debug("%s is blacklisted (%s)" % (obj.Label, obj.TypeId))
        return False
    if hasattr(obj, 'Sheets') or hasattr(obj, 'TagText'): # Arch.Panels and Arch.PanelCut
        PathLog.debug("%s is not an Arch.Panel" % (obj.Label))
        return False
    return True

def isSolid(obj):
    '''isSolid(obj) ... return True if the object is a valid solid.'''
    if hasattr(obj, 'Tip'):
        return isSolid(obj.Tip)
    if hasattr(obj, 'Shape'):
        if obj.Shape.Volume > 0.0 and obj.Shape.isClosed():
            return True
    if hasattr(obj, 'TypeId') and 'App::Part' == obj.TypeId:
        if not obj.Group or any(hasattr(o, 'Shape') and not isSolid(o) for o in obj.Group):
            return False
        return True
    return False

def toolControllerForOp(op):
    '''toolControllerForOp(op) ... return the tool controller used by the op.
    If the op doesn't have its own tool controller but has a Base object, return its tool controller.
    Otherwise return None.'''
    if hasattr(op, 'ToolController'):
        return op.ToolController
    if hasattr(op, 'Base'):
        return toolControllerForOp(op.Base)
    return None

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
        for attr,expr in obj.ExpressionEngine:
            obj.setExpression(attr, None)

def toUnicode(string):
    '''toUnicode(string) ... returns a unicode version of string regardless of the python version.'''
    if sys.version_info.major < 3:
        return unicode(string)
    return string

def isString(string):
    '''isString(string) ... return True if string is a string, regardless of string type and python version.'''
    if type(string) == str:
        return True
    if sys.version_info.major < 3 and type(string) == unicode:
        return True
    return False

def keyValueIter(dictionary):
    '''keyValueIter(dict) ... return iterable object over dictionary's (key,value) tuples.'''
    if sys.version_info.major < 3:
        return dictionary.items()
    return dictionary.items()
