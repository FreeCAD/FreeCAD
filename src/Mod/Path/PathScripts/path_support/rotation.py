# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Russell Johnson (russ4262) <russ4262@gmail.com>    *
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

from __future__ import print_function

__title__ = "Rotation Support"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Support functions for rotation applications."
__contributors__ = ""

# Standard
import math
# Third-party
from PySide import QtCore
# FreeCAD
import FreeCAD
import PathScripts.PathLog as PathLog
# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')
if FreeCAD.GuiUp:
    FreeCADGui = LazyLoader('FreeCADGui', globals(), 'FreeCADGui')


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


# Rotation-related methods
def opDetermineRotationRadii(self, obj):
    '''opDetermineRotationRadii(obj) ...
    Determine rotational radii for 4th-axis rotations,
    for clearance/safe heights.'''

    parentJob = self.job
    xlim = 0.0
    ylim = 0.0

    # Determine boundbox radius based upon xzy limits data
    if math.fabs(self.stockBB.ZMin) > math.fabs(self.stockBB.ZMax):
        zlim = self.stockBB.ZMin
    else:
        zlim = self.stockBB.ZMax

    if obj.EnableRotation != 'B(y)':
        # Rotation is around X-axis, cutter moves along same axis
        if math.fabs(self.stockBB.YMin) > math.fabs(self.stockBB.YMax):
            ylim = self.stockBB.YMin
        else:
            ylim = self.stockBB.YMax

    if obj.EnableRotation != 'A(x)':
        # Rotation is around Y-axis, cutter moves along same axis
        if math.fabs(self.stockBB.XMin) > math.fabs(self.stockBB.XMax):
            xlim = self.stockBB.XMin
        else:
            xlim = self.stockBB.XMax

    xRotRad = math.sqrt(ylim**2 + zlim**2)
    yRotRad = math.sqrt(xlim**2 + zlim**2)
    zRotRad = math.sqrt(xlim**2 + ylim**2)

    clrOfst = parentJob.SetupSheet.ClearanceHeightOffset.Value
    safOfst = parentJob.SetupSheet.SafeHeightOffset.Value

    return [(xRotRad, yRotRad, zRotRad), (clrOfst, safOfst)]


def faceRotationAnalysis(self, obj, norm, surf):
    '''faceRotationAnalysis(obj, norm, surf) ...
    Determine X and Y independent rotation necessary
    to make normalAt = Z=1 (0,0,1).'''
    PathLog.track()

    praInfo = "faceRotationAnalysis()"
    rtn = True
    orientation = 'X'
    angle = 500.0
    precision = 6

    for i in range(0, 13):
        if PathGeom.Tolerance * (i * 10) == 1.0:
            precision = i
            break

    def roundRoughValues(precision, val):
        # Convert VALxe-15 numbers to zero
        if PathGeom.isRoughly(0.0, val) is True:
            return 0.0
        # Convert VAL.99999999 to next integer
        elif math.fabs(val % 1) > 1.0 - PathGeom.Tolerance:
            return round(val)
        else:
            return round(val, precision)

    nX = roundRoughValues(precision, norm.x)
    nY = roundRoughValues(precision, norm.y)
    nZ = roundRoughValues(precision, norm.z)
    praInfo += "\n -normalAt(0,0): " + str(nX) + ", "
    praInfo += str(nY) + ", " + str(nZ)

    saX = roundRoughValues(precision, surf.x)
    saY = roundRoughValues(precision, surf.y)
    saZ = roundRoughValues(precision, surf.z)
    praInfo += "\n -Surface.Axis: " + str(saX) + ", "
    praInfo += str(saY) + ", " + str(saZ)

    # Determine rotation needed and current orientation
    if saX == 0.0:
        if saY == 0.0:
            orientation = "Z"
            if saZ == 1.0:
                angle = 0.0
            elif saZ == -1.0:
                angle = -180.0
            else:
                praInfo += "_else_X" + str(saZ)
        elif saY == 1.0:
            orientation = "Y"
            angle = 90.0
        elif saY == -1.0:
            orientation = "Y"
            angle = -90.0
        else:
            if saZ != 0.0:
                angle = math.degrees(math.atan(saY / saZ))
                orientation = "Y"
    elif saY == 0.0:
        if saZ == 0.0:
            orientation = "X"
            if saX == 1.0:
                angle = -90.0
            elif saX == -1.0:
                angle = 90.0
            else:
                praInfo += "_else_X" + str(saX)
        else:
            orientation = "X"
            ratio = saX / saZ
            angle = math.degrees(math.atan(ratio))
            if ratio < 0.0:
                praInfo += " NEG-ratio"
                # angle -= 90
            else:
                praInfo += " POS-ratio"
                angle = -1 * angle
                if saX < 0.0:
                    angle = angle + 180.0
    elif saZ == 0.0:
        # if saY != 0.0:
        angle = math.degrees(math.atan(saX / saY))
        orientation = "Y"

    if saX + nX == 0.0:
        angle = -1 * angle
    if saY + nY == 0.0:
        angle = -1 * angle
    if saZ + nZ == 0.0:
        angle = -1 * angle

    if saY == -1.0 or saY == 1.0:
        if nX != 0.0:
            angle = -1 * angle

    # Enforce enabled rotation in settings
    praInfo += "\n -Initial orientation:  {}".format(orientation)
    if orientation == 'Y':
        axis = 'X'
        if obj.EnableRotation == 'B(y)':  # Required axis disabled
            if angle == 180.0 or angle == -180.0:
                axis = 'Y'
            else:
                rtn = False
    elif orientation == 'X':
        axis = 'Y'
        if obj.EnableRotation == 'A(x)':  # Required axis disabled
            if angle == 180.0 or angle == -180.0:
                axis = 'X'
            else:
                rtn = False
    elif orientation == 'Z':
        axis = 'X'

    if math.fabs(angle) == 0.0:
        angle = 0.0
        rtn = False

    if angle == 500.0:
        angle = 0.0
        rtn = False

    if rtn is False:
        if (orientation == 'Z' and angle == 0.0 and
                obj.ReverseDirection is True):
            if obj.EnableRotation == 'B(y)':
                axis = 'Y'
            rtn = True

    if rtn is True:
        self.rotateFlag = True
        if obj.ReverseDirection is True:
            if angle < 180.0:
                angle = angle + 180.0
            else:
                angle = angle - 180.0
        angle = round(angle, precision)

    praInfo += "\n -Rotation analysis:  angle: " + str(angle)
    praInfo += ",   axis: " + str(axis)
    if rtn is True:
        praInfo += "\n - ... rotation triggered"
    else:
        praInfo += "\n - ... NO rotation triggered"

    PathLog.debug("\n" + str(praInfo))

    return (rtn, angle, axis, praInfo)


def visualAxis(self):
    '''visualAxis() ...
    Create visual X & Y axis for use in orientation of rotational operations
    Triggered only when the log level is set to DEBUG.'''

    if not FreeCAD.ActiveDocument.getObject('xAxCyl'):
        xAx = 'xAxCyl'
        yAx = 'yAxCyl'
        # zAx = 'zAxCyl'
        VA = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup",
                                              "visualAxis")
        if FreeCAD.GuiUp:
            FreeCADGui.ActiveDocument.getObject('visualAxis').Visibility = \
                False
        vaGrp = FreeCAD.ActiveDocument.getObject("visualAxis")

        FreeCAD.ActiveDocument.addObject("Part::Cylinder", xAx)
        cyl = FreeCAD.ActiveDocument.getObject(xAx)
        cyl.Label = xAx
        cyl.Radius = self.xRotRad
        cyl.Height = 0.01
        fcadRot = FreeCAD.Rotation(FreeCAD.Vector(0, 1, 0), 90)
        cyl.Placement = FreeCAD.Placement(FreeCAD.Vector(0, 0, 0), fcadRot)
        cyl.purgeTouched()
        if FreeCAD.GuiUp:
            cylGui = FreeCADGui.ActiveDocument.getObject(xAx)
            cylGui.ShapeColor = (0.667, 0.000, 0.000)
            cylGui.Transparency = 85
            cylGui.Visibility = False
        vaGrp.addObject(cyl)

        FreeCAD.ActiveDocument.addObject("Part::Cylinder", yAx)
        cyl = FreeCAD.ActiveDocument.getObject(yAx)
        cyl.Label = yAx
        cyl.Radius = self.yRotRad
        cyl.Height = 0.01
        fcadRot = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90)
        cyl.Placement = FreeCAD.Placement(FreeCAD.Vector(0, 0, 0), fcadRot)
        cyl.purgeTouched()
        if FreeCAD.GuiUp:
            cylGui = FreeCADGui.ActiveDocument.getObject(yAx)
            cylGui.ShapeColor = (0.000, 0.667, 0.000)
            cylGui.Transparency = 85
            cylGui.Visibility = False
        vaGrp.addObject(cyl)
        VA.purgeTouched()


def useTempJobClones(self, cloneName):
    '''useTempJobClones(cloneName) ...
    Manage use of temporary model clones for rotational operation calculations.
    Clones are stored in 'rotJobClones' group.
    '''
    fcad = FreeCAD.ActiveDocument
    if fcad.getObject('rotJobClones'):
        if cloneName == 'Start':
            if PathLog.getLevel(PathLog.thisModule()) < 4:
                for cln in fcad.getObject('rotJobClones').Group:
                    fcad.removeObject(cln.Name)
        elif cloneName == 'Delete':
            if PathLog.getLevel(PathLog.thisModule()) < 4:
                for cln in fcad.getObject('rotJobClones').Group:
                    fcad.removeObject(cln.Name)
                fcad.removeObject('rotJobClones')
            else:
                fcad.getObject('rotJobClones').purgeTouched()
    else:
        fcad.addObject("App::DocumentObjectGroup", "rotJobClones")
        if FreeCAD.GuiUp:
            FreeCADGui.ActiveDocument.getObject('rotJobClones').Visibility = \
                False

    if cloneName != 'Start' and cloneName != 'Delete':
        fcad.getObject('rotJobClones').addObject(fcad.getObject(cloneName))
        if FreeCAD.GuiUp:
            FreeCADGui.ActiveDocument.getObject(cloneName).Visibility = False


def cloneBaseAndStock(self, obj, base, angle, axis, subCount):
    '''cloneBaseAndStock(obj, base, angle, axis, subCount) ...
    Method called to create a temporary clone of the base and parent Job stock.
    Clones are destroyed after usage for calculations related to
    rotational operations.
    '''
    # Create a temporary clone and stock of model for rotational use.
    fcad = FreeCAD.ActiveDocument
    rndAng = round(angle, 8)
    if rndAng < 0.0:  # neg sign is converted to underscore in clone name.
        tag = axis + '_' + axis + '_'
        tag += str(math.fabs(rndAng)).replace('.', '_')
    else:
        tag = axis + str(rndAng).replace('.', '_')
    clnNm = obj.Name + '_base_' + '_' + str(subCount) + '_' + tag
    stckClnNm = obj.Name + '_stock_' + '_' + str(subCount) + '_' + tag
    if clnNm not in self.cloneNames:
        self.cloneNames.append(clnNm)
        self.cloneNames.append(stckClnNm)
        if fcad.getObject(clnNm):
            fcad.getObject(clnNm).Shape = base.Shape
        else:
            fcad.addObject('Part::Feature', clnNm).Shape = base.Shape
            self.useTempJobClones(clnNm)
        if fcad.getObject(stckClnNm):
            fcad.getObject(stckClnNm).Shape = self.job.Stock.Shape
        else:
            fcad.addObject('Part::Feature', stckClnNm).Shape = \
                self.job.Stock.Shape
            self.useTempJobClones(stckClnNm)
        if FreeCAD.GuiUp:
            fcadGui = FreeCADGui.ActiveDocument
            fcadGui.getObject(stckClnNm).Transparency = 90
            fcadGui.getObject(clnNm).ShapeColor = (1.000, 0.667, 0.000)
    clnBase = fcad.getObject(clnNm)
    clnStock = fcad.getObject(stckClnNm)
    tag = base.Name + '_' + tag
    return (clnBase, clnStock, tag)


def analyzeFace(self, base, sub, shape, subCount):
    rtn = False
    (norm, surf) = getFaceNormAndSurf(shape)
    (rtn, angle, axis, praInfo) = faceRotationAnalysis(self, norm, surf) # pylint: disable=unused-variable
    PathLog.debug("initial faceRotationAnalysis: {}".format(praInfo))
    if rtn is True:
        (clnBase, angle, clnStock, tag) = applyRotationalAnalysis(self, base, angle, axis, subCount)
        # Verify faces are correctly oriented - InverseAngle might be necessary
        faceIA = getattr(clnBase.Shape, sub)
        (norm, surf) = getFaceNormAndSurf(faceIA)
        (rtn, praAngle, praAxis, praInfo2) = faceRotationAnalysis(self, norm, surf) # pylint: disable=unused-variable
        PathLog.debug("follow-up faceRotationAnalysis: {}".format(praInfo2))

        if abs(praAngle) == 180.0:
            rtn = False
            if isFaceUp(clnBase, faceIA) is False:
                PathLog.debug('isFaceUp 1 is False')
                angle -= 180.0

        if rtn is True:
            PathLog.debug(translate("Path", "Face appears misaligned after initial "))
            if self.obj.InverseAngle is False:
                if self.obj.AttemptInverseAngle is True:
                    (clnBase, clnStock, angle) = applyInverseAngle(self, clnBase, clnStock, axis, angle)
                else:
                    msg = translate("Path", "Consider toggling the 'InverseAngle' property and recomputing.")
                    PathLog.warning(msg)

            if isFaceUp(clnBase, faceIA) is False:
                PathLog.debug('isFaceUp 2 is False')
                angle += 180.0
            else:
                PathLog.debug('  isFaceUp')

        else:
            PathLog.debug("Face appears to be oriented correctly.")

        if angle < 0.0:
            angle += 360.0

        tup = clnBase, sub, tag, angle, axis, clnStock
    else:
        if warnDisabledAxis(self, axis) is False:
            PathLog.debug(str(sub) + ": No rotation used")
        axis = 'X'
        angle = 0.0
        tag = base.Name + '_' + axis + str(angle).replace('.', '_')
        stock = self.job.Stock
        tup = base, sub, tag, angle, axis, stock

    return tup





def getFaceNormAndSurf(face):
    '''getFaceNormAndSurf(face) ...
    Return face.normalAt(0,0) or face.normal(0,0) and
    face.Surface.Axis vectors.
    '''
    norm = FreeCAD.Vector(0.0, 0.0, 0.0)
    surf = FreeCAD.Vector(0.0, 0.0, 0.0)

    if hasattr(face, 'normalAt'):
        n = face.normalAt(0, 0)
    elif hasattr(face, 'normal'):
        n = face.normal(0, 0)
    if hasattr(face.Surface, 'Axis'):
        s = face.Surface.Axis
    else:
        s = n
    norm.x = n.x
    norm.y = n.y
    norm.z = n.z
    surf.x = s.x
    surf.y = s.y
    surf.z = s.z
    return (norm, surf)


def applyRotationalAnalysis(self, obj, base, angle, axis, subCount):
    '''applyRotationalAnalysis(obj, base, angle, axis, subCount) ...
    Create temp clone and stock and apply rotation to both.
    Return new rotated clones.'''
    if axis == 'X':
        vect = FreeCAD.Vector(1, 0, 0)
    elif axis == 'Y':
        vect = FreeCAD.Vector(0, 1, 0)

    if obj.InverseAngle is True:
        angle = -1 * angle
        if math.fabs(angle) == 0.0:
            angle = 0.0

    # Create a temporary clone of model for rotational use.
    (clnBase, clnStock, tag) = \
        self.cloneBaseAndStock(obj, base, angle, axis, subCount)

    # Rotate base to such that Surface.Axis of pocket bottom is Z=1
    clnBase = Draft.rotate(clnBase, angle,
                           center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect,
                           copy=False)
    clnStock = Draft.rotate(clnStock, angle,
                            center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect,
                            copy=False)

    clnBase.purgeTouched()
    clnStock.purgeTouched()
    return (clnBase, angle, clnStock, tag)


def applyInverseAngle(self, obj, clnBase, clnStock, axis, angle):
    '''applyInverseAngle(obj, clnBase, clnStock, axis, angle) ...
    Apply rotations to incoming base and stock objects.'''
    if axis == 'X':
        vect = FreeCAD.Vector(1, 0, 0)
    elif axis == 'Y':
        vect = FreeCAD.Vector(0, 1, 0)
    # Rotate base to inverse of original angle
    clnBase = Draft.rotate(clnBase, (-2 * angle),
                           center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect,
                           copy=False)
    clnStock = Draft.rotate(clnStock, (-2 * angle),
                            center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect,
                            copy=False)
    clnBase.purgeTouched()
    clnStock.purgeTouched()
    # Update property and angle values
    obj.InverseAngle = True
    obj.AttemptInverseAngle = False
    angle = -1 * angle

    PathLog.debug(translate("Path", "Rotated to inverse angle."))
    return (clnBase, clnStock, angle)


def sortTuplesByIndex(self, TupleList, tagIdx):
    '''sortTuplesByIndex(TupleList, tagIdx) ...
    Sort list of tuples based on tag index provided.
    return (TagList, GroupList).'''
    # Separate elements, regroup by orientation (axis_angle combination)
    TagList = ['X34.2']
    GroupList = [[(2.3, 3.4, 'X')]]
    for tup in TupleList:
        if tup[tagIdx] in TagList:
            # Determine index of found string
            i = 0
            for orn in TagList:
                if orn == tup[4]:
                    break
                i += 1
            GroupList[i].append(tup)
        else:
            TagList.append(tup[4])  # add orientation entry
            GroupList.append([tup])  # add orientation entry
    # Remove temp elements
    TagList.pop(0)
    GroupList.pop(0)
    return (TagList, GroupList)


def warnDisabledAxis(self, obj, axis, sub=''):
    '''warnDisabledAxis(self, obj, axis) ...
    Provide user feedback if required axis is disabled.'''
    if axis == 'X' and obj.EnableRotation == 'B(y)':
        msg = translate('Path',
                        "{}:: {} is inaccessible.".format(obj.Name, sub))
        msg += "  "
        msg += translate('Path',
                         ("Selected feature(s) require 'Enable Rotation:"
                          " A(x)' for access."))
        PathLog.warning(msg)
        return True
    elif axis == 'Y' and obj.EnableRotation == 'A(x)':
        msg = translate('Path',
                        "{}:: {} is inaccessible.".format(obj.Name, sub))
        msg += "  "
        msg += translate('Path',
                         ("Selected feature(s) require 'Enable Rotation:"
                          " B(y)' for access."))
        PathLog.warning(msg)
        return True
    else:
        return False


def isFaceUp(base, face):
    '''isFaceUp(base, face) ...
    Determine if the referenced face is above the referenced
    base, or below it.  Results are dependent upon the orientation
    of the face.'''
    up = face.extrude(FreeCAD.Vector(0.0, 0.0, 5.0))
    dwn = face.extrude(FreeCAD.Vector(0.0, 0.0, -5.0))
    upCmn = base.Shape.common(up)
    dwnCmn = base.Shape.common(dwn)
    if upCmn.Volume == 0.0:
        return True
    elif dwnCmn.Volume == 0.0:
        return False
    if dwnCmn.Volume > upCmn.Volume:
        return True
    return False


