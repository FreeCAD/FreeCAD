# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2019 Zheng, Lei (realthunder)<realthunder.dev@gmail.com>*
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides the base viewprovider code for the Link objects."""
## @package view_draftlink
# \ingroup draftviewproviders
# \brief Provides the base viewprovider code for the Link objects.

## \addtogroup draftviewproviders
# @{
import FreeCAD
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
from draftviewproviders.view_base import ViewProviderDraft

class ViewProviderDraftLink(ViewProviderDraft):
    """ A view provider for link type object.
    """

    def __init__(self,vobj):
        super(ViewProviderDraftLink, self).__init__(vobj)

    def setupContextMenu(self,vobj,menu):
        if hasattr(vobj.Object, 'Fuse'):
            action = QtGui.QAction(QT_TRANSLATE_NOOP('Draft', 'Toggle fuse'), menu)
            QtCore.QObject.connect(action,
                                    QtCore.SIGNAL("triggered()"),
                                    self.toggleFuse)
            menu.addAction(action)

            if vobj.Object.Fuse:
                if vobj.DefaultMode == 0:
                    action = QtGui.QAction(QT_TRANSLATE_NOOP('Draft', 'Show fused shape'), menu)
                else:
                    action = QtGui.QAction(QT_TRANSLATE_NOOP('Draft', 'Show array shapes'), menu)
                QtCore.QObject.connect(action,
                                    QtCore.SIGNAL("triggered()"),
                                    self.toggleFusedShape)
                menu.addAction(action)
                return

        if getattr(vobj.Object, 'BuildShape', True):
            action = QtGui.QAction(QT_TRANSLATE_NOOP('Draft', 'Disable build shape'), menu)
        else:
            action = QtGui.QAction(QT_TRANSLATE_NOOP('Draft', 'Enable build shape'), menu)
        action.setToolTip(QT_TRANSLATE_NOOP('Draft',
            "Toggle whether to build compound shape for this array.\n" \
            "It is recommended to disable shape building to speed up\n" \
            "recomputation for complex element shape, if you are not\n" \
            "using the array for further modeling."))

        QtCore.QObject.connect(action,
                               QtCore.SIGNAL("triggered()"),
                               self.toggleBuildShape)
        menu.addAction(action)

    def toggleFusedShape(self):
        FreeCAD.setActiveTransaction(QT_TRANSLATE_NOOP('Draft', 'Toggle fused shape'))
        vobj = self.Object.ViewObject
        if vobj.DefaultMode == 1:
            mode = 0
        else:
            mode = 1
            if not vobj.ChildViewProvider:
                vobj.ChildViewProvider = 'PartGui::ViewProviderPartExt'
        vobj.DefaultMode = mode

    def toggleFuse(self):
        FreeCAD.setActiveTransaction(QT_TRANSLATE_NOOP('Draft', 'Toggle fuse array'))
        self.Object.Fuse = not self.Object.Fuse
        FreeCAD.ActiveDocument.recompute()

    def toggleBuildShape(self):
        FreeCAD.setActiveTransaction(QT_TRANSLATE_NOOP('Draft', 'Toggle build shape'))
        self.Object.BuildShape = not self.Object.BuildShape
        FreeCAD.ActiveDocument.recompute()

    def setEdit(self, _vobj, _mode=0):
        # skip ViewPRoviderDraft.setEdit and let ViewProviderLink handle it
        raise NotImplementedError

    def unsetEdit(self, _vobj, _mode=0):
        # skip ViewPRoviderDraft.unsetEdit and let ViewProviderLink handle it
        raise NotImplementedError

    def getIcon(self):
        tp = self.Object.Proxy.Type
        if tp == 'Array':
            if self.Object.ArrayType == 'ortho':
                return ":/icons/Draft_LinkArray.svg"
            elif self.Object.ArrayType == 'polar':
                return ":/icons/Draft_PolarLinkArray.svg"
            elif self.Object.ArrayType == 'circular':
                return ":/icons/Draft_CircularLinkArray.svg"
        elif tp == 'PathArray':
            return ":/icons/Draft_PathLinkArray.svg"
        elif tp == 'PathTwistedArray':
            return ":/icons/Draft_PathTwistedLinkArray.svg"
        elif tp == 'PointArray':
            return ":/icons/Draft_PointLinkArray.svg"

    def claimChildren(self):
        obj = self.Object
        if hasattr(obj,'ExpandArray'):
            expand = obj.ExpandArray
        else:
            expand = obj.ShowElement
        if not expand:
            return super(ViewProviderDraftLink, self).claimChildren()
        else:
            return obj.ElementList

    def updateData(self, obj, prop):
        if prop == 'Fuse':
            if (obj.Fuse and obj.ViewObject.DefaultMode == 0) \
                    or (not obj.Fuse and obj.ViewObject.DefaultMode == 1):
                self.toggleFusedShape()
        super(ViewProviderDraftLink, self).updateData(obj, prop)


# Alias for compatibility with old versions of v0.19
_ViewProviderDraftLink = ViewProviderDraftLink

## @}
