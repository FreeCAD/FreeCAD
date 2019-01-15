# ***************************************************************************
# *   (c) Sliptonic (shopinthewoods@gmail.com)  2016                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/

'''This file has utilities for checking and catching common errors in FreeCAD
Path projects.  Ideally, the user could execute these utilities from an icon
to make sure tools are selected and configured and defaults have been revised'''

from __future__ import print_function
from PySide import QtCore
import FreeCAD
import FreeCADGui
import PathScripts
import PathScripts.PathLog as PathLog
# import PathScripts.PathCollision as PC
# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

LOG_MODULE = 'PathSanity'
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)
#PathLog.trackModule('PathSanity')


class CommandPathSanity:
    baseobj=None

    def GetResources(self):
        return {'Pixmap'  : 'Path-Sanity',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Sanity","Check the Path project for common errors"),
                'Accel': "P, S",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Sanity","Check the Path Project for common errors")}

    def IsActive(self):
        obj = FreeCADGui.Selection.getSelectionEx()[0].Object
        if hasattr(obj, 'Operations') and hasattr(obj, 'ToolController'):
            return True
        return False

    def Activated(self):
        # if everything is ok, execute
        obj = FreeCADGui.Selection.getSelectionEx()[0].Object
        self.baseobj = obj.Base
        if self.baseobj is None:
            FreeCAD.Console.PrintWarning(translate("Path_Sanity", "The Job has no selected Base object.")+"\n")
            return
        self.__review(obj)

    def __review(self, obj):
        "checks the selected job for common errors"
        clean = True

        # if obj.X_Max == obj.X_Min or obj.Y_Max == obj.Y_Min:
        #     FreeCAD.Console.PrintWarning(translate("Path_Sanity", "It appears the machine limits haven't been set.  Not able to check path extents.")+"\n")

        if obj.PostProcessor == '':
            FreeCAD.Console.PrintWarning(translate("Path_Sanity", "A Postprocessor has not been selected.")+"\n")
            clean = False

        if obj.PostProcessorOutputFile == '':
            FreeCAD.Console.PrintWarning(translate("Path_Sanity", "No output file is named. You'll be prompted during postprocessing.")+"\n")
            clean = False

        for tc in obj.ToolController:
            PathLog.info("Checking: {}.{}".format(obj.Label, tc.Label))
            clean &= self.__checkTC(tc)

        for op in obj.Operations.Group:
            PathLog.info("Checking: {}.{}".format(obj.Label, op.Label))

            if isinstance(op.Proxy, PathScripts.PathProfileContour.ObjectContour):
                if op.Active:
                    # simobj = op.Proxy.execute(op, getsim=True)
                    # if simobj is not None:
                    #     print ('collision detected')
                    #     PC.getCollisionObject(self.baseobj, simobj)
                    #     clean = False
                    pass

            if isinstance(op.Proxy, PathScripts.PathProfileFaces.ObjectProfile):
                if op.Active:
                    # simobj = op.Proxy.execute(op, getsim=True)
                    # if simobj is not None:
                    #     print ('collision detected')
                    #     PC.getCollisionObject(self.baseobj, simobj)
                    #     clean = False
                    pass

            if isinstance(op.Proxy, PathScripts.PathProfileEdges.ObjectProfile):
                if op.Active:
                    # simobj = op.Proxy.execute(op, getsim=True)
                    # if simobj is not None:
                    #     print ('collision detected')
                    #     PC.getCollisionObject(self.baseobj, simobj)
                    #     clean = False
                    pass

            if isinstance(op.Proxy, PathScripts.PathPocket.ObjectPocket):
                if op.Active:
                    # simobj = op.Proxy.execute(op, getsim=True)
                    # if simobj is not None:
                    #     print ('collision detected')
                    #     PC.getCollisionObject(self.baseobj, simobj)
                    #     clean = False
                    pass

            if isinstance(op.Proxy, PathScripts.PathPocketShape.ObjectPocket):
                if op.Active:
                    # simobj = op.Proxy.execute(op, getsim=True)
                    # if simobj is not None:
                    #     print ('collision detected')
                    #     PC.getCollisionObject(self.baseobj, simobj)
                    #     clean = False
                    pass

        if not any(op.Active for op in obj.Operations.Group): #no active operations
            FreeCAD.Console.PrintWarning(translate("Path_Sanity", "No active operations was found. Post processing will not result in any tooling."))
            clean = False

        if len(obj.ToolController) == 0: #need at least one active TC
            FreeCAD.Console.PrintWarning(translate("Path_Sanity", "A Tool Controller was not found. Default values are used which is dangerous. Please add a Tool Controller.")+"\n")
            clean = False

        if clean:
            FreeCAD.Console.PrintMessage(translate("Path_Sanity", "No issues detected, {} has passed basic sanity check.").format(obj.Label))

    def __checkTC(self, tc):
        clean = True
        if tc.ToolNumber == 0:
            FreeCAD.Console.PrintWarning(translate("Path_Sanity", "Tool Controller: " + str(tc.Label) + " is using ID 0 which the undefined default. Please set a real tool.")+"\n")
            clean = False
        if tc.HorizFeed == 0:
            FreeCAD.Console.PrintWarning(translate("Path_Sanity", "Tool Controller: " + str(tc.Label) + " has a 0 value for the Horizontal feed rate")+"\n")
            clean = False
        if tc.VertFeed == 0:
            FreeCAD.Console.PrintWarning(translate("Path_Sanity", "Tool Controller: " + str(tc.Label) + " has a 0 value for the Vertical feed rate")+"\n")
            clean = False
        if tc.SpindleSpeed == 0:
            FreeCAD.Console.PrintWarning(translate("Path_Sanity", "Tool Controller: " + str(tc.Label) + " has a 0 value for the spindle speed")+"\n")
            clean = False
        return clean

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Sanity',CommandPathSanity())
