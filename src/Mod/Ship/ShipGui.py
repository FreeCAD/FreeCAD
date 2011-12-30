# ##### BEGIN GPL LICENSE BLOCK #####
#
#  Author: Jose Luis Cercos Pita <jlcercos@gmail.com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

from PyQt4 import QtCore, QtGui
import FreeCAD, FreeCADGui, os

class CreateShip: 
    def Activated(self):
        import shipCreateShip
        shipCreateShip.load()

    def GetResources(self):
        from shipUtils import Paths, Translator
        IconPath = Paths.iconsPath() + "/Ico.png"
        MenuText = str(Translator.translate('Create a new ship'))
        ToolTip  = str(Translator.translate('Create a new ship in order to work with them'))
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

class OutlineDraw: 
    def Activated(self):
        import shipOutlineDraw
        shipOutlineDraw.load()

    def GetResources(self):
        from shipUtils import Paths, Translator
        IconPath = Paths.iconsPath() + "/OutlineDrawIco.png"
        MenuText = str(Translator.translate('Outline draw'))
        ToolTip  = str(Translator.translate('Plot ship outline draw'))
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 
      
FreeCADGui.addCommand('Ship_CreateShip', CreateShip())
FreeCADGui.addCommand('Ship_OutlineDraw', OutlineDraw())
