# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 Lorenz Hüdepohl <dev@stellardeath.org>             *
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

import PathScripts.PathCircularHoleBase as PathCircularHoleBase
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp

from PathScripts.PathUtils import fmt
from PySide import QtCore

__doc__ = "Class and implementation of Helix Drill operation"

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectHelix(PathCircularHoleBase.ObjectOp):
    '''Proxy class for Helix operations.'''

    def circularHoleFeatures(self, obj):
        '''circularHoleFeatures(obj) ... enable features supported by Helix.'''
        return PathOp.FeatureStepDown | PathOp.FeatureBaseEdges | PathOp.FeatureBaseFaces | PathOp.FeatureBasePanels

    def initCircularHoleOperation(self, obj):
        '''initCircularHoleOperation(obj) ... create helix specific properties.'''
        obj.addProperty("App::PropertyEnumeration", "Direction", "Helix Drill", translate("PathHelix", "The direction of the circular cuts, ClockWise (CW), or CounterClockWise (CCW)"))
        obj.Direction = ['CW', 'CCW']

        obj.addProperty("App::PropertyEnumeration", "StartSide", "Helix Drill", translate("PathHelix", "Start cutting from the inside or outside"))
        obj.StartSide = ['Inside', 'Outside']

        obj.addProperty("App::PropertyLength", "StepOver", "Helix Drill", translate("PathHelix", "Radius increment (must be smaller than tool diameter)"))

    def circularHoleExecute(self, obj, holes):
        '''circularHoleExecute(obj, holes) ... generate helix commands for each hole in holes'''
        PathLog.track()
        self.commandlist.append(Path.Command('(helix cut operation)'))

        self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))

        zsafe = max(baseobj.Shape.BoundBox.ZMax for baseobj, features in obj.Base) + obj.ClearanceHeight.Value
        output = ''
        output += "G0 Z" + fmt(zsafe)

        for hole in holes:
            output += self.helix_cut(obj, hole['x'], hole['y'], hole['r'] / 2, 0.0, (float(obj.StepOver.Value)/50.0) * self.radius)
        PathLog.debug(output)

    def helix_cut(self, obj, x0, y0, r_out, r_in, dr):
        '''helix_cut(obj, x0, y0, r_out, r_in, dr) ... generate helix commands for specified hole.
            x0, y0: coordinates of center
            r_out, r_in: outer and inner radius of the hole
            dr: step over radius value'''
        from numpy import ceil, linspace

        if (obj.StartDepth.Value <= obj.FinalDepth.Value):
            return ""

        out = "(helix_cut <{0}, {1}>, {2})".format(x0, y0,
                    ", ".join(map(str, (r_out, r_in, dr, obj.StartDepth.Value, obj.FinalDepth.Value, obj.StepDown.Value, obj.SafeHeight.Value,
                                        self.radius, self.vertFeed, self.horizFeed, obj.Direction, obj.StartSide))))

        nz = max(int(ceil((obj.StartDepth.Value - obj.FinalDepth.Value)/obj.StepDown.Value)), 2)
        zi = linspace(obj.StartDepth.Value, obj.FinalDepth.Value, 2 * nz + 1)

        def xyz(x=None, y=None, z=None):
            out = ""
            if x is not None:
                out += " X" + fmt(x)
            if y is not None:
                out += " Y" + fmt(y)
            if z is not None:
                out += " Z" + fmt(z)
            return out

        def rapid(x=None, y=None, z=None):
            return "G0" + xyz(x, y, z) + "\n"

        def F(f=None):
            return (" F" + fmt(f) if f else "")

        def feed(x=None, y=None, z=None, f=None):
            return "G1" + xyz(x, y, z) + F(f) + "\n"

        def arc(x, y, i, j, z, f):
            if obj.Direction == "CW":
                code = "G2"
            elif obj.Direction == "CCW":
                code = "G3"
            return code + " I" + fmt(i) + " J" + fmt(j) + " X" + fmt(x) + " Y" + fmt(y) + " Z" + fmt(z) + F(f) + "\n"

        def helix_cut_r(r):
            arc_cmd = 'G2' if obj.Direction == 'CW' else 'G3'
            out = ""
            out += rapid(x=x0+r, y=y0)
            self.commandlist.append(Path.Command('G0', {'X': x0 + r, 'Y': y0, 'F': self.horizRapid}))
            out += rapid(z=obj.StartDepth.Value + 2*self.radius)
            self.commandlist.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
            out += feed(z=obj.StartDepth.Value, f=self.vertFeed)
            self.commandlist.append(Path.Command('G1', {'Z': obj.StartDepth.Value, 'F': self.vertFeed}))
            # z = obj.FinalDepth.Value
            for i in range(1, nz+1):
                out += arc(x0-r, y0, i=-r, j=0.0, z=zi[2*i-1], f=self.horizFeed)
                self.commandlist.append(Path.Command(arc_cmd, {'X': x0-r, 'Y': y0, 'Z': zi[2*i-1], 'I': -r, 'J': 0.0, 'F': self.horizFeed}))
                out += arc(x0+r, y0, i= r, j=0.0, z=zi[2*i],   f=self.horizFeed)
                self.commandlist.append(Path.Command(arc_cmd, {'X': x0+r, 'Y': y0, 'Z': zi[2*i],   'I':  r, 'J': 0.0, 'F': self.horizFeed}))
            out += arc(x0-r, y0, i=-r, j=0.0, z=obj.FinalDepth.Value, f=self.horizFeed)
            self.commandlist.append(Path.Command(arc_cmd, {'X': x0-r, 'Y': y0, 'Z': obj.FinalDepth.Value,   'I': -r, 'J': 0.0, 'F': self.horizFeed}))
            out += arc(x0+r, y0, i=r,  j=0.0, z=obj.FinalDepth.Value, f=self.horizFeed)
            self.commandlist.append(Path.Command(arc_cmd, {'X': x0+r, 'Y': y0, 'Z': obj.FinalDepth.Value,   'I':  r, 'J': 0.0, 'F': self.horizFeed}))
            out += feed(z=obj.StartDepth.Value + 2*self.radius, f=self.vertFeed)
            out += rapid(z=obj.SafeHeight.Value)
            self.commandlist.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
            return out

        assert(r_out > 0.0)
        assert(r_in >= 0.0)

        msg = None
        if r_out < 0.0:
            msg = "r_out < 0"
        elif r_in > 0 and r_out - r_in < 2*self.radius:
            msg = "r_out - r_in = {0} is < tool diameter of {1}".format(r_out - r_in, 2*self.radius)
        elif r_in == 0.0 and not r_out > self.radius/2.:
            msg = "Cannot drill a hole of diameter {0} with a tool of diameter {1}".format(2 * r_out, 2*self.radius)
        elif obj.StartSide not in ["Inside", "Outside"]:
            msg = "Invalid value for parameter 'obj.StartSide'"

        if msg:
            out += "(ERROR: Hole at {0}:".format((x0, y0, obj.StartDepth.Value)) + msg + ")\n"
            PathLog.error("PathHelix: Hole at {0}:".format((x0, y0, obj.StartDepth.Value)) + msg + "\n")
            return out

        if r_in > 0:
            out += "(annulus mode)\n"
            r_out = r_out - self.radius
            r_in = r_in + self.radius
            if abs((r_out - r_in) / dr) < 1e-5:
                radii = [(r_out + r_in)/2]
            else:
                nr = max(int(ceil((r_out - r_in)/dr)), 2)
                radii = linspace(r_out, r_in, nr)
        elif r_out <= 2 * dr:
            out += "(single helix mode)\n"
            radii = [r_out - self.radius]
            assert(radii[0] > 0)
        else:
            out += "(full hole mode)\n"
            r_out = r_out - self.radius
            r_in = dr/2

            nr = max(1 + int(ceil((r_out - r_in)/dr)), 2)
            radii = linspace(r_out, r_in, nr)
            assert(all(radii > 0))

        if obj.StartSide == "Inside":
            radii = radii[::-1]

        for r in radii:
            out += "(radius {0})\n".format(r)
            out += helix_cut_r(r)

        return out

    def opSetDefaultValues(self, obj, job):
        obj.Direction = "CW"
        obj.StartSide = "Inside"
        obj.StepOver = 100

def SetupProperties():
    setup = []
    setup.append("Direction")
    setup.append("StartSide")
    setup.append("StepOver")
    return setup

def Create(name, obj = None):
    '''Create(name) ... Creates and returns a Helix operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectHelix(obj, name)
    if obj.Proxy:
        proxy.findAllHoles(obj)
    return obj
