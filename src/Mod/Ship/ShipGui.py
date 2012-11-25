#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

from PyQt4 import QtCore, QtGui
import FreeCAD, FreeCADGui, os

# Load resources
import Ship_rc
FreeCADGui.addLanguagePath(":/Ship/translations")
FreeCADGui.addIconPath(":/Ship/icons")

class LoadExample: 
    def Activated(self):
        import shipLoadExample
        shipLoadExample.load()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_LoadExample', 'Load an example ship geometry')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_LoadExample', 'Load an example ship geometry able to be converted into a ship.')
        return {'Pixmap' : 'LoadIco', 'MenuText': MenuText, 'ToolTip': ToolTip} 

class CreateShip: 
    def Activated(self):
        import shipCreateShip
        shipCreateShip.load()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_CreateShip', 'Create a new ship')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_CreateShip', 'Create a new ship in order to work with them')
        return {'Pixmap' : 'Ico', 'MenuText': MenuText, 'ToolTip': ToolTip} 

class OutlineDraw: 
    def Activated(self):
        import shipOutlineDraw
        shipOutlineDraw.load()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_OutlineDraw', 'Outline draw')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_OutlineDraw', 'Plot ship outline draw')
        return {'Pixmap' : 'OutlineDrawIco', 'MenuText': MenuText, 'ToolTip': ToolTip} 

class AreasCurve: 
    def Activated(self):
        import shipAreasCurve
        shipAreasCurve.load()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_AreasCurve', 'Areas curve')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_AreasCurve', 'Plot transversal areas curve')
        return {'Pixmap' : 'AreaCurveIco', 'MenuText': MenuText, 'ToolTip': ToolTip} 

class Hydrostatics: 
    def Activated(self):
        import shipHydrostatics
        shipHydrostatics.load()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_Hydrostatics', 'Hydrostatics')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_Hydrostatics', 'Plot ship hydrostatics')
        return {'Pixmap' : 'HydrostaticsIco', 'MenuText': MenuText, 'ToolTip': ToolTip} 
      
class SetWeights: 
    def Activated(self):
        import tankWeights
        tankWeights.load()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_Weights', 'Set ship weights')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_Weights', 'Set ship weights, tanks must be added later')
        return {'Pixmap' : 'Weight', 'MenuText': MenuText, 'ToolTip': ToolTip} 

class CreateTank: 
    def Activated(self):
        import tankCreateTank
        tankCreateTank.load()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_CreateTank', 'Create a new tank')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_CreateTank', 'Create a new ship tank')
        return {'Pixmap' : 'Tank', 'MenuText': MenuText, 'ToolTip': ToolTip} 

class GZ: 
    def Activated(self):
        import tankGZ
        tankGZ.load()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_GZ', 'GZ curve')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_GZ', 'Transversal stability GZ curve computation')
        return {'Pixmap' : 'HydrostaticsIco', 'MenuText': MenuText, 'ToolTip': ToolTip} 

class CreateSim: 
    def Activated(self):
        import simCreate
        simCreate.load()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_CreateSim', 'Create a new simulation')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_CreateSim', 'Create a new simulation in order to process later')
        return {'Pixmap' : 'SimCreateIco', 'MenuText': MenuText, 'ToolTip': ToolTip} 

class RunSim: 
    def Activated(self):
        import simRun
        simRun.load()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_RunSim', 'Run a simulation')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_RunSim', 'Run a simulation')
        return {'Pixmap' : 'SimRunIco', 'MenuText': MenuText, 'ToolTip': ToolTip} 

class StopSim: 
    def Activated(self):
        import simRun
        simRun.stop()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_StopSim', 'Stop active simulation')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_StopSim', 'Stop active simulation')
        return {'Pixmap' : 'SimStopIco', 'MenuText': MenuText, 'ToolTip': ToolTip} 

class TrackSim: 
    def Activated(self):
        import simPost
        simPost.load()

    def GetResources(self):
        MenuText = QtCore.QT_TRANSLATE_NOOP('Ship_TrackSim', 'Track simulation')
        ToolTip  = QtCore.QT_TRANSLATE_NOOP('Ship_TrackSim', 'Track simulation')
        return {'Pixmap' : 'SimPostIco', 'MenuText': MenuText, 'ToolTip': ToolTip} 


FreeCADGui.addCommand('Ship_LoadExample', LoadExample())
FreeCADGui.addCommand('Ship_CreateShip', CreateShip())
FreeCADGui.addCommand('Ship_OutlineDraw', OutlineDraw())
FreeCADGui.addCommand('Ship_AreasCurve', AreasCurve())
FreeCADGui.addCommand('Ship_Hydrostatics', Hydrostatics())
FreeCADGui.addCommand('Ship_Weights', SetWeights())
FreeCADGui.addCommand('Ship_CreateTank', CreateTank())
FreeCADGui.addCommand('Ship_GZ', GZ())
FreeCADGui.addCommand('Ship_CreateSim', CreateSim())
FreeCADGui.addCommand('Ship_RunSim', RunSim())
FreeCADGui.addCommand('Ship_StopSim', StopSim())
FreeCADGui.addCommand('Ship_TrackSim', TrackSim())
