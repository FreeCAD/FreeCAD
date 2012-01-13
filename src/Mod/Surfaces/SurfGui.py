#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercós Pita <jlcercos@gmail.com>                            *  
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

class IsoCurve: 
    def Activated(self):
        import surfISOCurve
        surfISOCurve.load() 

    def GetResources(self):
        from surfUtils import Paths, Translator
        IconPath = Paths.iconsPath() + "/IsoCurveIco.png"
        MenuText = str(Translator.translate('Get ISO curve'))
        ToolTip  = str(Translator.translate('Get ISO curve from surface'))
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

class SliceCurve: 
    def Activated(self):
        import surfSlice
        surfSlice.load() 

    def GetResources(self):
        from surfUtils import Paths, Translator
        IconPath = Paths.iconsPath() + "/SliceIco.png"
        MenuText = str(Translator.translate('Get surface slice'))
        ToolTip  = str(Translator.translate('Get surface intersection with coordinates planes'))
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

class Border: 
    def Activated(self):
        import surfBorder
        surfBorder.load()

    def GetResources(self):
        from surfUtils import Paths, Translator
        IconPath = Paths.iconsPath() + "/BorderIco.png"
        MenuText = str(Translator.translate('Get border'))
        ToolTip  = str(Translator.translate('Get edges from objects'))
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

class Convert: 
    def Activated(self):
        import surfConvert
        surfConvert.load()

    def GetResources(self):
        from surfUtils import Paths, Translator
        IconPath = Paths.iconsPath() + "/ConvertIco.png"
        MenuText = str(Translator.translate('Convert to 4 sides surface'))
        ToolTip  = str(Translator.translate('Convert a surface (or couple of them) into 4 sides surface'))
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 
      
FreeCADGui.addCommand('Surf_IsoCurve', IsoCurve())
FreeCADGui.addCommand('Surf_SliceCurve', SliceCurve())
FreeCADGui.addCommand('Surf_Border', Border())
FreeCADGui.addCommand('Surf_Convert', Convert())
