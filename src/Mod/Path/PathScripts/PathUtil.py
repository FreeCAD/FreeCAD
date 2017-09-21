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

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

NotValidBaseTypeIds = ['Sketcher::SketchObject']

def isValidBaseObject(obj):
    '''isValidBaseObject(obj) ... returns true if the object can be used as a base for a job.'''
    if hasattr(obj, 'getParentGeoFeatureGroup') and obj.getParentGeoFeatureGroup():
        # Can't link to anything inside a geo feature group anymore
        return False
    if hasattr(obj, 'TypeId') and 'App::Part' == obj.TypeId:
        return obj.Group and any(hasattr(o, 'Shape') for o in obj.Group)
    if not hasattr(obj, 'Shape'):
        return False
    if obj.TypeId in NotValidBaseTypeIds:
        return False
    if hasattr(obj, 'Sheets') or hasattr(obj, 'TagText'): # Arch.Panels and Arch.PanelCut
        return False
    return True

def isSolid(obj):
    '''isSolid(obj) ... return True if the object is a valid solid.'''
    if hasattr(obj, 'Tip'):
        return isSolid(obj.Tip)
    if hasattr(obj, 'Shape'):
        if obj.Shape.ShapeType == 'Solid' and obj.Shape.isClosed():
            return True
        if obj.Shape.ShapeType == 'Compound':
            if hasattr(obj, 'Base') and hasattr(obj, 'Tool'):
                return isSolid(obj.Base) and isSolid(obj.Tool)
    if hasattr(obj, 'TypeId') and 'App::Part' == obj.TypeId:
        if not obj.Group or any(hasattr(o, 'Shape') and not isSolid(o) for o in obj.Group):
            return False
        return True
    return False

def toolControllerForOp(op):
    if hasattr(op, 'ToolController'):
        return op.ToolController
    if hasattr(op, 'Base'):
        return toolControllerForOp(op.Base)
    return None


