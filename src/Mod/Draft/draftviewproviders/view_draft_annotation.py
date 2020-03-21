# ***************************************************************************
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
"""This module provides the Draft Annotations view provider base classes
"""
## @package polararray
# \ingroup DRAFT
# \brief This module provides the view provider code for Draft PolarArray.


import FreeCAD as App
import FreeCADGui as Gui
from PySide.QtCore import QT_TRANSLATE_NOOP
import draftutils.utils as utils


class ViewProviderDraftAnnotation:
    """The base class for Draft Annotation Viewproviders"""

    def __init__(self, vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

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


class ViewProviderDimensionBase(ViewProviderDraftAnnotation):
    """
    A View Provider for the Draft Dimension object
    
    DIMENSION VIEW PROVIDER:
    
        |              txt               |       e
    ----o--------------------------------o-----
        |                                |
        |                                |       d  
        |                                |

     a  b               c                b  a
    
    a = DimOvershoot (vobj)
    b = Arrows (vobj)
    c = Dimline (obj)
    d = ExtLines (vobj)
    e = ExtOvershoot (vobj)
    txt = label (vobj)

    STRUCTURE:
    vobj.node.color
             .drawstyle
             .lineswitch1.coords
                         .line
                         .marks
                         .marksDimOvershoot
                         .marksExtOvershoot
             .label.textpos
                   .color
                   .font
                   .text
             
    vobj.node3d.color
               .drawstyle
               .lineswitch3.coords
                           .line
                           .marks
                           .marksDimOvershoot
                           .marksExtOvershoot
               .label3d.textpos
                       .color
                       .font3d
                       .text3d
    
    """
    def __init__(self, vobj): 
        # text properties
        vobj.addProperty("App::PropertyFont","FontName",
                         "Text",QT_TRANSLATE_NOOP("App::Property","Font name"))
        vobj.addProperty("App::PropertyLength","FontSize",
                         "Text",QT_TRANSLATE_NOOP("App::Property","Font size"))
        vobj.addProperty("App::PropertyLength","TextSpacing",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "The spacing between the text and the dimension line"))
        vobj.addProperty("App::PropertyBool","FlipText",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "Rotate the dimension text 180 degrees"))
        vobj.addProperty("App::PropertyVectorDistance","TextPosition",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "The position of the text. Leave (0,0,0) for automatic position"))
        vobj.addProperty("App::PropertyString","Override",
                         "Text",QT_TRANSLATE_NOOP("App::Property",
                         "Text override. Use $dim to insert the dimension length"))
        # units properties
        vobj.addProperty("App::PropertyInteger","Decimals",
                         "Units",QT_TRANSLATE_NOOP("App::Property",
                         "The number of decimals to show"))
        vobj.addProperty("App::PropertyBool","ShowUnit",
                         "Units",QT_TRANSLATE_NOOP("App::Property",
                         "Show the unit suffix"))
        vobj.addProperty("App::PropertyString","UnitOverride",
                         "Units",QT_TRANSLATE_NOOP("App::Property",
                         "A unit to express the measurement. Leave blank for system default"))
        # graphics properties
        vobj.addProperty("App::PropertyLength","ArrowSize",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property","Arrow size"))
        vobj.addProperty("App::PropertyEnumeration","ArrowType",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property","Arrow type"))
        vobj.addProperty("App::PropertyBool","FlipArrows",
                        "Graphics",QT_TRANSLATE_NOOP("App::Property",
                        "Rotate the dimension arrows 180 degrees"))        
        vobj.addProperty("App::PropertyDistance","DimOvershoot",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property",
                         "The distance the dimension line is extended past the extension lines"))        
        vobj.addProperty("App::PropertyDistance","ExtLines",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property",
                         "Length of the extension lines"))
        vobj.addProperty("App::PropertyDistance","ExtOvershoot",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property",
                         "Length of the extension line above the dimension line"))
        vobj.addProperty("App::PropertyBool","ShowLine",
                         "Graphics",QT_TRANSLATE_NOOP("App::Property",
                         "Shows the dimension line and arrows"))
        
        vobj.FontSize = utils.get_param("textheight",0.20)
        vobj.TextSpacing = utils.get_param("dimspacing",0.05)
        vobj.FontName = utils.get_param("textfont","")
        vobj.ArrowSize = utils.get_param("arrowsize",0.1)
        vobj.ArrowType = utils.ARROW_TYPES
        vobj.ArrowType = utils.ARROW_TYPES[utils.get_param("dimsymbol",0)]
        vobj.ExtLines = utils.get_param("extlines",0.3)
        vobj.DimOvershoot = utils.get_param("dimovershoot",0)
        vobj.ExtOvershoot = utils.get_param("extovershoot",0)
        vobj.Decimals = utils.get_param("dimPrecision",2)
        vobj.ShowUnit = utils.get_param("showUnit",True)
        vobj.ShowLine = True
        ViewProviderDraftAnnotation.__init__(self,vobj)

    def attach(self, vobj):
        """called on object creation"""
        return

    def updateData(self, obj, prop):
        """called when the base object is changed"""
        return

    def onChanged(self, vobj, prop):
        """called when a view property has changed"""
        return

    def doubleClicked(self,vobj):
        self.setEdit(vobj)

    def getDisplayModes(self,vobj):
        return ["2D","3D"]

    def getDefaultDisplayMode(self):
        if hasattr(self,"defaultmode"):
            return self.defaultmode
        else:
            return ["2D","3D"][utils.get_param("dimstyle",0)]

    def setDisplayMode(self,mode):
        return mode

    def getIcon(self):
        if self.is_linked_to_circle():
            return ":/icons/Draft_DimensionRadius.svg"
        return ":/icons/Draft_Dimension_Tree.svg"

    def __getstate__(self):
        return self.Object.ViewObject.DisplayMode

    def __setstate__(self,state):
        if state:
            self.defaultmode = state
            self.setDisplayMode(state)

