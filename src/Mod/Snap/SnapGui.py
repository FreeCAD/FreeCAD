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

def showWidget():
    """ Show snap tools widget.
    @return True if widget has been showed, False otherwise
    """
    flag = False
    # Loop over widgets
    widgets = QtGui.qApp.allWidgets()
    for i in range(0, len(widgets)):
        widget = widgets[i]
        # Show the widget if exist
        if widget.property(QtCore.QString("snapToolWidget")).toString() == "Valid":
            widget.show()
            flag = True
    return flag

class SnapToolbar:
    def Activated(self):
        import snapWidget
        if not showWidget():
            snapWidget.load() 

    def GetResources(self):
        from snapUtils import Paths, Translator
        IconPath = Paths.iconsPath() + "/Ico.png"
        MenuText = str(Translator.translate('Snap toolbar'))
        ToolTip  = str(Translator.translate('Show snap tool window'))
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

class Test: 
    def Activated(self):
        import snapTest
        snapTest.load() 

    def GetResources(self):
        from snapUtils import Paths, Translator
        IconPath = Paths.iconsPath() + "/Ico.png"
        MenuText = str(Translator.translate('Snap test'))
        ToolTip  = str(Translator.translate('Show test tool'))
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

FreeCADGui.addCommand('Snap_Toolbar', SnapToolbar())
FreeCADGui.addCommand('Snap_Test', Test())
