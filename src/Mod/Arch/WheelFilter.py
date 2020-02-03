#***************************************************************************
#*   Copyright (c) 2020 WandererFan <wandererfan@gmail.com>                *
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

from PySide import QtCore

class WheelFilter(QtCore.QObject):
    '''WheelFilter(widget) - prevent widget from changing value when cursor/mouse wheel scrolls through widget's
    screen area.  Usage:  myWidget = QtGui.QComboBox() , WheelFilter(myWidget)'''

    def __init__(self, widget):
        super().__init__(widget)
        self.widget = widget
        self.savePolicy = self.widget.focusPolicy()
        self.widget.installEventFilter(self)
        self.widget.setFocusPolicy(QtCore.Qt.StrongFocus)

    def eventFilter(self, object, event):
        if event.type() == QtCore.QEvent.Type.Wheel:
            if object == self.widget:
                if not self.widget.hasFocus():
                    return True;                     #skip this event
        return super(WheelFilter, self).eventFilter(object, event)

    def removeFilter(self):
        '''clear the event filter from a QObject(QWidget).  Usage:  WheelFilter.removeFilter(myWidget)'''
        print("removing filter!")
        self.widget.removeEventFilter(self)
        self.widget.setFocusPolicy(self.savePolicy) 
