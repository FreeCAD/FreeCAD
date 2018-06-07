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
import Part
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp

from PySide import QtCore

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectChamfer(PathOp.ObjectOp):
    '''Proxy class for Chamfer operation.'''

    def opFeatures(self, obj):
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureStepDown | PathOp.FeatureBaseEdges | PathOp.FeatureBaseFaces;

    def initOperation(self, obj):
        obj.addProperty("App::PropertyDistance", "Width",      "Chamfer", QtCore.QT_TRANSLATE_NOOP("PathChamfer", "The desired width of the chamfer"))
        obj.addProperty("App::PropertyDistance", "ExtraDepth", "Chamfer", QtCore.QT_TRANSLATE_NOOP("PathChamfer", "The additional depth of the tool path"))

    def opExecute(self, obj):
        pass

    def opSetDefaultValues(self, obj):
        obj.Width = '1 mm'
        obj.ExtraDepth = '0.1 mm'

def Create(name):
    '''Create(name) ... Creates and returns a Chamfer operation.'''
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectChamfer(obj)
    return obj

