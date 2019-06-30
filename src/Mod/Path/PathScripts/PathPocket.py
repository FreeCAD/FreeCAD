# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
# *                                                                         *
# *   Additional modifications and contributions beginning 2019             *
# *   Focus: improve 3D facial pockets                                      *
# *   by Russell Johnson  <russ4262@gmail.com>                              *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Part
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathPocketBase as PathPocketBase
import PathScripts.PathUtils as PathUtils

from PySide import QtCore

__title__ = "Path 3D Pocket Operation"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of the 3D Pocket operation."
__contributors__ = "russ4262 (Russell Johnson)"
__created__ = "2014"
__scriptVersion__ = "1a testing"
__lastModified__ = "2019-06-28 23:45 CST"

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectPocket(PathPocketBase.ObjectPocket):
    '''Proxy object for Pocket operation.'''

    def pocketOpFeatures(self, obj):
        return PathOp.FeatureNoFinalDepth

    def initPocketOp(self, obj):
        '''initPocketOp(obj) ... setup receiver'''
        if not hasattr(obj, 'HandleMultipleFeatures'):
            obj.addProperty('App::PropertyEnumeration', 'HandleMultipleFeatures', 'Pocket', QtCore.QT_TRANSLATE_NOOP('PathPocket', 'Choose how to process multiple Base Geometry features.'))
        obj.HandleMultipleFeatures = ['Collectively', 'Individually']
        pass

    def opOnDocumentRestored(self, obj):
        '''opOnDocumentRestored(obj) ... adds the properties if they doesn't exist.'''
        self.initPocketOp(obj)

    def pocketInvertExtraOffset(self):
        return False

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... return shapes representing the solids to be removed.'''
        PathLog.track()
        PathLog.info("----- areaOpShapes() in PathPocket.py")

        removalshapes = []
        if obj.Base:
            PathLog.debug("base items exist.  Processing...")
            for base in obj.Base:
                PathLog.debug("Base item: {}".format(base))

                # Check if all subs are faces
                allFaceSubs = True
                Faces = []
                for sub in base[1]:
                    if "Face" in sub:
                        Faces.append(getattr(base[0].Shape, sub))
                    else:
                        allFaceSubs = False
                        break

                if allFaceSubs is True and obj.HandleMultipleFeatures == 'Collectively':
                    shape = Part.makeCompound(Faces)
                    env = PathUtils.getEnvelope(base[0].Shape, subshape=shape, depthparams=self.depthparams)
                    obj.removalshape = env.cut(base[0].Shape)
                    obj.removalshape.tessellate(0.1)
                    removalshapes.append((obj.removalshape, False))
                else:
                    for sub in base[1]:
                        if "Face" in sub:
                            shape = Part.makeCompound([getattr(base[0].Shape, sub)])
                        else:
                            edges = [getattr(base[0].Shape, sub) for sub in base[1]]
                            shape = Part.makeFace(edges, 'Part::FaceMakerSimple')

                        env = PathUtils.getEnvelope(base[0].Shape, subshape=shape, depthparams=self.depthparams)
                        obj.removalshape = env.cut(base[0].Shape)
                        obj.removalshape.tessellate(0.1)
                        removalshapes.append((obj.removalshape, False))

        else:  # process the job base object as a whole
            PathLog.debug("processing the whole job base object")
            for base in self.model:
                env = PathUtils.getEnvelope(base.Shape, subshape=None, depthparams=self.depthparams)
                obj.removalshape = env.cut(base.Shape)
                obj.removalshape.tessellate(0.1)
                removalshapes.append((obj.removalshape, False))
        return removalshapes

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... set default values'''
        obj.StepOver = 100
        obj.ZigZagAngle = 45
        obj.HandleMultipleFeatures = 'Collectively'


def SetupProperties():
    return PathPocketBase.SetupProperties().append("HandleMultipleFeatures")


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Pocket operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectPocket(obj, name)
    return obj
