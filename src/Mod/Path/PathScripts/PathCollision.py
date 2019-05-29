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

import FreeCAD
import PathScripts.PathLog as PathLog
from PySide import QtCore
from PathScripts.PathUtils import waiting_effects

LOG_MODULE = 'PathCollision'
PathLog.setLevel(PathLog.Level.DEBUG, LOG_MODULE)
PathLog.trackModule('PathCollision')
FreeCAD.setLogLevel('Path.Area', 0)

if FreeCAD.GuiUp:
    import FreeCADGui
    # from PySide import QtGui


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

__title__ = "Path Collision Utility"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path Collision object and FreeCAD command"""


class _CollisionSim:
    def __init__(self, obj):
        #obj.addProperty("App::PropertyLink", "Original", "reference", QtCore.QT_TRANSLATE_NOOP("App::Property", "The base object this collision refers to"))
        obj.Proxy = self

    def execute(self, fp):
        "'''Do something when doing a recomputation, this method is mandatory'''"
        pass


class _ViewProviderCollisionSim:
    def __init__(self, vobj):
        vobj.Proxy = self
        vobj.addProperty("App::PropertyLink", "Original", "reference", QtCore.QT_TRANSLATE_NOOP("App::Property", "The base object this collision refers to"))

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def setEdit(self, vobj, mode=0):
        return True

    def getIcon(self):
        return ":/icons/Path-Contour.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onDelete(self, feature, subelements):
        feature.Original.ViewObject.Visibility = True
        return True


def __compareBBSpace(bb1, bb2):
    if (bb1.XMin == bb2.XMin and
            bb1.XMax == bb2.XMax and
            bb1.YMin == bb2.YMin and
            bb1.YMax == bb2.YMax and
            bb1.ZMin == bb2.ZMin and
            bb1.ZMax == bb2.ZMax):
        return True
    return False


@waiting_effects
def getCollisionObject(baseobject, simobject):
    result = None
    cVol = baseobject.Shape.common(simobject)
    if cVol.Volume > 1e-12:
        baseColor = (0.800000011920929, 0.800000011920929, 0.800000011920929, 00.0)
        intersecColor = (1.0, 0.0, 0.0, 0.0)
        colorassignment = []
        gougedShape = baseobject.Shape.cut(simobject)

        for idx, i in enumerate(gougedShape.Faces):
            match = False
            for jdx, j in enumerate(cVol.Faces):
                if __compareBBSpace(i.BoundBox, j.BoundBox):
                    match = True
            if match is True:
                # print ("Need to highlight Face{}".format(idx+1))
                colorassignment.append(intersecColor)
            else:
                colorassignment.append(baseColor)

        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Collision")
        obj.Shape = gougedShape
        _CollisionSim(obj)
        _ViewProviderCollisionSim(obj.ViewObject)

        obj.ViewObject.DiffuseColor = colorassignment
        FreeCAD.ActiveDocument.recompute()
        baseobject.ViewObject.Visibility = False
        obj.ViewObject.Original = baseobject

    return result



