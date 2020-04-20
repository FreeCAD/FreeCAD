# ***************************************************************************
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
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""This module provides the Draft Annotations view provider base class
"""
## @package annotation
# \ingroup DRAFT
# \brief This module provides the Draft Annotations view provider base class


import FreeCAD as App
from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui


class ViewProviderDraftAnnotation(object):
    """
    The base class for Draft Annotation Viewproviders
    This class is not used directly, but inherited by all annotation
    view providers.
    """

    def __init__(self, vobj):
        #vobj.Proxy = self
        #self.Object = vobj.Object

        # annotation properties
        vobj.addProperty("App::PropertyFloat","ScaleMultiplier",
                        "Annotation",QT_TRANSLATE_NOOP("App::Property",
                        "Dimension size overall multiplier"))

        # graphics properties
        vobj.addProperty("App::PropertyFloat","LineWidth",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property","Line width"))
        vobj.addProperty("App::PropertyColor","LineColor",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property","Line color"))  

        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        annotation_scale = param.GetFloat("DraftAnnotationScale", 1.0)
        if annotation_scale != 0:
            vobj.ScaleMultiplier = 1 / annotation_scale


    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def updateData(self, obj, prop):
        return

    def getDisplayModes(self, vobj):
        modes=[]
        return modes

    def setDisplayMode(self, mode):
        return mode

    def onChanged(self, vobj, prop):
        return

    def execute(self,vobj):
        return

    def setEdit(self,vobj,mode=0):
        if mode == 0:
            Gui.runCommand("Draft_Edit")
            return True
        return False

    def unsetEdit(self,vobj,mode=0):
        if App.activeDraftCommand:
            App.activeDraftCommand.finish()
        Gui.Control.closeDialog()
        return False

    def getIcon(self):
        return ":/icons/Draft_Draft.svg"

    def claimChildren(self):
        """perhaps this is not useful???"""
        objs = []
        if hasattr(self.Object,"Base"):
            objs.append(self.Object.Base)
        if hasattr(self.Object,"Objects"):
            objs.extend(self.Object.Objects)
        if hasattr(self.Object,"Components"):
            objs.extend(self.Object.Components)
        if hasattr(self.Object,"Group"):
            objs.extend(self.Object.Group)
        return objs


