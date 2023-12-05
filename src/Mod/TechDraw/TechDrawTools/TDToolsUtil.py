# ***************************************************************************
# *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>               *
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
"""Provides utility functions for TD Tools."""

import FreeCAD as App
import FreeCADGui as Gui
from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtGui
from PySide.QtGui import QMessageBox

def havePage():
    objs = App.ActiveDocument.Objects
    for o in objs:
        if o.isDerivedFrom("TechDraw::DrawPage"):
            return True
    return False

def haveView():
    objs = App.ActiveDocument.Objects
    for o in objs:
        if o.isDerivedFrom("TechDraw::DrawView"):
            return True
    return False

def displayMessage(title,message):
    '''
    displayMessage('Title','Messagetext')
    '''
    msgBox = QtGui.QMessageBox()
    msgBox.setText(message)
    msgBox.setWindowTitle(title)
    msgBox.exec_()


def getSelView():
    '''
    view = getSelView()
    Return selected view, otherwise return False
    '''
    if not Gui.Selection.getSelection():
        displayMessage('TechDraw_Utils','No view selected')
    else:
        view = Gui.Selection.getSelection()[0]
        return view

def getSelVertexes(nVertex=1):
    '''
    vertexes = getSelVertexes(nVertex)
    nVertex ... min. number of selected vertexes
    Return a list of selected vertexes if at least nVertex vertexes are selected, otherwise return False
    '''
    if getSelView():
        view = getSelView()
    else:
        return False
    if not Gui.Selection.getSelectionEx():
        displayMessage('TechDraw_Utils',
                        QT_TRANSLATE_NOOP('TechDraw_Utils','No vertex selected'))
        return False
    objectList = Gui.Selection.getSelectionEx()[0].SubElementNames

    vertexes = []
    for objectString in objectList:
        if objectString[0:6] == 'Vertex':
            vertexes.append(view.getVertexBySelection(objectString))

    if (len(vertexes) < nVertex):
        displayMessage('TechDraw_Utils',
                        QT_TRANSLATE_NOOP('TechDraw_Utils','Select at least ')+
                        str(nVertex)+
                        QT_TRANSLATE_NOOP('TechDraw_Utils',' vertexes'))
        return False
    else:
        return vertexes

