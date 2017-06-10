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

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())

def isSolid(obj):
    '''isSolid(obj) ... returns true if an object represents a solid.'''

    if hasattr(obj, 'Tip'):
        return isSolid(obj.Tip)
    if hasattr(obj, 'Shape'):
        if obj.Shape.ShapeType == 'Solid' and obj.Shape.isClosed():
            return True
        if obj.Shape.ShapeType == 'Compound':
            if hasattr(obj, 'Base') and hasattr(obj, 'Tool'):
                return isSolid(obj.Base) and isSolid(obj.Tool)
    return False

