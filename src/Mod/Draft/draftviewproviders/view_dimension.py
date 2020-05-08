# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""This module provides the Draft Dimensions view provider classes
"""
## @package dimension
# \ingroup DRAFT
# \brief This module provides the view provider code for Draft Dimensions.


import FreeCAD as App
import DraftVecUtils, DraftGeomUtils
from pivy import coin
from PySide.QtCore import QT_TRANSLATE_NOOP
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
from draftviewproviders.view_draft_annotation import ViewProviderDraftAnnotation

class ViewProviderDimensionBase(ViewProviderDraftAnnotation):
    """
    A View Provider for the Draft Dimension object
    This class is not used directly, but inherited by all dimension
    view providers.

    DIMENSION VIEW PROVIDER NOMENCLATURE:
    
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

    COIN OBJECT STRUCTURE:
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

        super(ViewProviderDimensionBase, self).__init__(vobj)

        # text properties
        vobj.addProperty("App::PropertyFont","FontName",
                         "Text",
                         QT_TRANSLATE_NOOP("App::Property","Font name"))
        vobj.addProperty("App::PropertyLength","FontSize",
                         "Text",
                         QT_TRANSLATE_NOOP("App::Property","Font size"))
        vobj.addProperty("App::PropertyLength","TextSpacing",
                         "Text",
                         QT_TRANSLATE_NOOP("App::Property",
                         "Spacing between text and dimension line"))
        vobj.addProperty("App::PropertyBool","FlipText",
                         "Text",
                         QT_TRANSLATE_NOOP("App::Property",
                         "Rotate the dimension text 180 degrees"))
        vobj.addProperty("App::PropertyVectorDistance","TextPosition",
                         "Text",
                         QT_TRANSLATE_NOOP("App::Property",
                         "Text Position. \n"
                         "Leave (0,0,0) for automatic position"))
        vobj.addProperty("App::PropertyString","Override",
                         "Text",
                         QT_TRANSLATE_NOOP("App::Property",
                         "Text override. \n"
                         "Use $dim to insert the dimension length"))

        # units properties
        vobj.addProperty("App::PropertyInteger","Decimals",
                         "Units",
                         QT_TRANSLATE_NOOP("App::Property",
                         "The number of decimals to show"))
        vobj.addProperty("App::PropertyBool","ShowUnit",
                         "Units",
                         QT_TRANSLATE_NOOP("App::Property",
                         "Show the unit suffix"))
        vobj.addProperty("App::PropertyString","UnitOverride",
                         "Units",
                         QT_TRANSLATE_NOOP("App::Property",
                         "A unit to express the measurement. \n"
                         "Leave blank for system default"))

        # graphics properties
        vobj.addProperty("App::PropertyLength","ArrowSize",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property","Arrow size"))
        vobj.addProperty("App::PropertyEnumeration","ArrowType",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property","Arrow type"))
        vobj.addProperty("App::PropertyBool","FlipArrows",
                        "Graphics",
                        QT_TRANSLATE_NOOP("App::Property",
                        "Rotate the dimension arrows 180 degrees"))        
        vobj.addProperty("App::PropertyDistance","DimOvershoot",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property",
                         "The distance the dimension line is extended\n"
                         "past the extension lines"))        
        vobj.addProperty("App::PropertyDistance","ExtLines",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property",
                         "Length of the extension lines"))
        vobj.addProperty("App::PropertyDistance","ExtOvershoot",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property",
                         "Length of the extension line \n"
                         "above the dimension line"))
        vobj.addProperty("App::PropertyBool","ShowLine",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property",
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



class ViewProviderLinearDimension(ViewProviderDimensionBase):
    """
    A View Provider for the Draft Linear Dimension object
    """
    def __init__(self, vobj):

        super(ViewProviderLinearDimension, self).__init__(vobj)
           
        self.Object = vobj.Object
        vobj.Proxy = self


    def attach(self, vobj):
        '''Setup the scene sub-graph of the view provider'''
        self.Object = vobj.Object
        self.color = coin.SoBaseColor()
        self.font = coin.SoFont()
        self.font3d = coin.SoFont()
        self.text = coin.SoAsciiText()
        self.text3d = coin.SoText2()
        self.text.string = "d" # some versions of coin crash if string is not set
        self.text3d.string = "d"
        self.textpos = coin.SoTransform()
        self.text.justification = self.text3d.justification = coin.SoAsciiText.CENTER
        label = coin.SoSeparator()
        label.addChild(self.textpos)
        label.addChild(self.color)
        label.addChild(self.font)
        label.addChild(self.text)
        label3d = coin.SoSeparator()
        label3d.addChild(self.textpos)
        label3d.addChild(self.color)
        label3d.addChild(self.font3d)
        label3d.addChild(self.text3d)
        self.coord1 = coin.SoCoordinate3()
        self.trans1 = coin.SoTransform()
        self.coord2 = coin.SoCoordinate3()
        self.trans2 = coin.SoTransform()
        self.transDimOvershoot1 = coin.SoTransform()
        self.transDimOvershoot2 = coin.SoTransform()
        self.transExtOvershoot1 = coin.SoTransform()
        self.transExtOvershoot2 = coin.SoTransform()
        self.marks = coin.SoSeparator()
        self.marksDimOvershoot = coin.SoSeparator()
        self.marksExtOvershoot = coin.SoSeparator()
        self.drawstyle = coin.SoDrawStyle()
        self.line = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.coords = coin.SoCoordinate3()
        self.node = coin.SoGroup()
        self.node.addChild(self.color)
        self.node.addChild(self.drawstyle)
        self.lineswitch2 = coin.SoSwitch()
        self.lineswitch2.whichChild = -3
        self.node.addChild(self.lineswitch2)
        self.lineswitch2.addChild(self.coords)
        self.lineswitch2.addChild(self.line)
        self.lineswitch2.addChild(self.marks)
        self.lineswitch2.addChild(self.marksDimOvershoot)
        self.lineswitch2.addChild(self.marksExtOvershoot)
        self.node.addChild(label)
        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.color)
        self.node3d.addChild(self.drawstyle)
        self.lineswitch3 = coin.SoSwitch()
        self.lineswitch3.whichChild = -3
        self.node3d.addChild(self.lineswitch3)
        self.lineswitch3.addChild(self.coords)
        self.lineswitch3.addChild(self.line)
        self.lineswitch3.addChild(self.marks)
        self.lineswitch3.addChild(self.marksDimOvershoot)
        self.lineswitch3.addChild(self.marksExtOvershoot)
        self.node3d.addChild(label3d)
        vobj.addDisplayMode(self.node,"2D")
        vobj.addDisplayMode(self.node3d,"3D")
        self.updateData(vobj.Object,"Start")
        self.onChanged(vobj,"FontSize")
        self.onChanged(vobj,"FontName")
        self.onChanged(vobj,"ArrowType")
        self.onChanged(vobj,"LineColor")
        self.onChanged(vobj,"DimOvershoot")
        self.onChanged(vobj,"ExtOvershoot")

    def updateData(self, obj, prop):
        """called when the base object is changed"""
        import DraftGui
        if prop in ["Start","End","Dimline","Direction"]:

            if obj.Start == obj.End:
                return

            if not hasattr(self,"node"):
                return

            import Part, DraftGeomUtils
            from pivy import coin

            # calculate the 4 points
            self.p1 = obj.Start
            self.p4 = obj.End
            base = None
            if hasattr(obj,"Direction"):
                if not DraftVecUtils.isNull(obj.Direction):
                    v2 = self.p1.sub(obj.Dimline)
                    v3 = self.p4.sub(obj.Dimline)
                    v2 = DraftVecUtils.project(v2,obj.Direction)
                    v3 = DraftVecUtils.project(v3,obj.Direction)
                    self.p2 = obj.Dimline.add(v2)
                    self.p3 = obj.Dimline.add(v3)
                    if DraftVecUtils.equals(self.p2,self.p3):
                        base = None
                        proj = None
                    else:
                        base = Part.LineSegment(self.p2,self.p3).toShape()
                        proj = DraftGeomUtils.findDistance(self.p1,base)
                        if proj:
                            proj = proj.negative()
            if not base:
                if DraftVecUtils.equals(self.p1,self.p4):
                    base = None
                    proj = None
                else:
                    base = Part.LineSegment(self.p1,self.p4).toShape()
                    proj = DraftGeomUtils.findDistance(obj.Dimline,base)
                if proj:
                    self.p2 = self.p1.add(proj.negative())
                    self.p3 = self.p4.add(proj.negative())
                else:
                    self.p2 = self.p1
                    self.p3 = self.p4
            if proj:
                if hasattr(obj.ViewObject,"ExtLines") and hasattr(obj.ViewObject, "ScaleMultiplier"):
                    dmax = obj.ViewObject.ExtLines.Value * obj.ViewObject.ScaleMultiplier
                    if dmax and (proj.Length > dmax):
                        if (dmax > 0):
                            self.p1 = self.p2.add(DraftVecUtils.scaleTo(proj,dmax))
                            self.p4 = self.p3.add(DraftVecUtils.scaleTo(proj,dmax))
                        else:
                            rest = proj.Length + dmax
                            self.p1 = self.p2.add(DraftVecUtils.scaleTo(proj,rest))
                            self.p4 = self.p3.add(DraftVecUtils.scaleTo(proj,rest))
            else:
                proj = (self.p3.sub(self.p2)).cross(App.Vector(0,0,1))

            # calculate the arrows positions
            self.trans1.translation.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.coord1.point.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.trans2.translation.setValue((self.p3.x,self.p3.y,self.p3.z))
            self.coord2.point.setValue((self.p3.x,self.p3.y,self.p3.z))

            # calculate dimension and extension lines overshoots positions
            self.transDimOvershoot1.translation.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.transDimOvershoot2.translation.setValue((self.p3.x,self.p3.y,self.p3.z))
            self.transExtOvershoot1.translation.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.transExtOvershoot2.translation.setValue((self.p3.x,self.p3.y,self.p3.z))

            # calculate the text position and orientation
            if hasattr(obj,"Normal"):
                if DraftVecUtils.isNull(obj.Normal):
                    if proj:
                        norm = (self.p3.sub(self.p2).cross(proj)).negative()
                    else:
                        norm = App.Vector(0,0,1)
                else:
                    norm = App.Vector(obj.Normal)
            else:
                if proj:
                    norm = (self.p3.sub(self.p2).cross(proj)).negative()
                else:
                    norm = App.Vector(0,0,1)
            if not DraftVecUtils.isNull(norm):
                norm.normalize()
            u = self.p3.sub(self.p2)
            u.normalize()
            v1 = norm.cross(u)
            rot1 = App.Placement(DraftVecUtils.getPlaneRotation(u,v1,norm)).Rotation.Q
            self.transDimOvershoot1.rotation.setValue((rot1[0],rot1[1],rot1[2],rot1[3]))
            self.transDimOvershoot2.rotation.setValue((rot1[0],rot1[1],rot1[2],rot1[3]))
            if hasattr(obj.ViewObject,"FlipArrows"):
                if obj.ViewObject.FlipArrows:
                    u = u.negative()
            v2 = norm.cross(u)
            rot2 = App.Placement(DraftVecUtils.getPlaneRotation(u,v2,norm)).Rotation.Q
            self.trans1.rotation.setValue((rot2[0],rot2[1],rot2[2],rot2[3]))
            self.trans2.rotation.setValue((rot2[0],rot2[1],rot2[2],rot2[3]))
            if self.p1 != self.p2:
                u3 = self.p1.sub(self.p2)
                u3.normalize()
                v3 = norm.cross(u3)
                rot3 = App.Placement(DraftVecUtils.getPlaneRotation(u3,v3,norm)).Rotation.Q
                self.transExtOvershoot1.rotation.setValue((rot3[0],rot3[1],rot3[2],rot3[3]))
                self.transExtOvershoot2.rotation.setValue((rot3[0],rot3[1],rot3[2],rot3[3]))
            if hasattr(obj.ViewObject,"TextSpacing")and hasattr(obj.ViewObject, "ScaleMultiplier"):
                ts = obj.ViewObject.TextSpacing.Value * obj.ViewObject.ScaleMultiplier
                offset = DraftVecUtils.scaleTo(v1,ts)
            else:
                offset = DraftVecUtils.scaleTo(v1,0.05)
            rott = rot1
            if hasattr(obj.ViewObject,"FlipText"):
                if obj.ViewObject.FlipText:
                    rott = App.Rotation(*rott).multiply(App.Rotation(norm,180)).Q
                    offset = offset.negative()
            # setting text
            try:
                m = obj.ViewObject.DisplayMode
            except: # swallow all exceptions here since it always fails on first run (Displaymode enum no set yet)
                m = ["2D","3D"][utils.get_param("dimstyle",0)]
            if m == "3D":
                offset = offset.negative()
            self.tbase = (self.p2.add((self.p3.sub(self.p2).multiply(0.5)))).add(offset)
            if hasattr(obj.ViewObject,"TextPosition"):
                if not DraftVecUtils.isNull(obj.ViewObject.TextPosition):
                    self.tbase = obj.ViewObject.TextPosition
            self.textpos.translation.setValue([self.tbase.x,self.tbase.y,self.tbase.z])
            self.textpos.rotation = coin.SbRotation(rott[0],rott[1],rott[2],rott[3])
            su = True
            if hasattr(obj.ViewObject,"ShowUnit"):
                su = obj.ViewObject.ShowUnit
            # set text value
            l = self.p3.sub(self.p2).Length
            unit = None
            if hasattr(obj.ViewObject,"UnitOverride"):
                unit = obj.ViewObject.UnitOverride
            # special representation if "Building US" scheme
            if App.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt("UserSchema",0) == 5:
                s = App.Units.Quantity(l,App.Units.Length).UserString
                self.string = s.replace("' ","'- ")
                self.string = s.replace("+"," ")
            elif hasattr(obj.ViewObject,"Decimals"):
                self.string = DraftGui.displayExternal(l,obj.ViewObject.Decimals,'Length',su,unit)
            else:
                self.string = DraftGui.displayExternal(l,None,'Length',su,unit)
            if hasattr(obj.ViewObject,"Override"):
                if obj.ViewObject.Override:
                    self.string = obj.ViewObject.Override.replace("$dim",\
                            self.string)
            self.text.string = self.text3d.string = utils.string_encode_coin(self.string)

            # set the lines
            if m == "3D":
                # calculate the spacing of the text
                textsize = (len(self.string)*obj.ViewObject.FontSize.Value)/4.0
                spacing = ((self.p3.sub(self.p2)).Length/2.0) - textsize
                self.p2a = self.p2.add(DraftVecUtils.scaleTo(self.p3.sub(self.p2),spacing))
                self.p2b = self.p3.add(DraftVecUtils.scaleTo(self.p2.sub(self.p3),spacing))
                self.coords.point.setValues([[self.p1.x,self.p1.y,self.p1.z],
                                             [self.p2.x,self.p2.y,self.p2.z],
                                             [self.p2a.x,self.p2a.y,self.p2a.z],
                                             [self.p2b.x,self.p2b.y,self.p2b.z],
                                             [self.p3.x,self.p3.y,self.p3.z],
                                             [self.p4.x,self.p4.y,self.p4.z]])
                #self.line.numVertices.setValues([3,3])
                self.line.coordIndex.setValues(0,7,(0,1,2,-1,3,4,5))
            else:
                self.coords.point.setValues([[self.p1.x,self.p1.y,self.p1.z],
                                             [self.p2.x,self.p2.y,self.p2.z],
                                             [self.p3.x,self.p3.y,self.p3.z],
                                             [self.p4.x,self.p4.y,self.p4.z]])
                #self.line.numVertices.setValue(4)
                self.line.coordIndex.setValues(0,4,(0,1,2,3))

    def onChanged(self, vobj, prop):
        """called when a view property has changed"""
        if prop == "ScaleMultiplier" and hasattr(vobj, "ScaleMultiplier"):
            # update all dimension values
            if hasattr(self,"font"):
                self.font.size = vobj.FontSize.Value * vobj.ScaleMultiplier
            if hasattr(self,"font3d"):
                self.font3d.size = vobj.FontSize.Value * 100 * vobj.ScaleMultiplier
            if hasattr(self,"node") and hasattr(self,"p2") and hasattr(vobj,"ArrowSize"):
                self.remove_dim_arrows()
                self.draw_dim_arrows(vobj)
            if hasattr(vobj,"DimOvershoot"):
                self.remove_dim_overshoot()
                self.draw_dim_overshoot(vobj)
            if hasattr(vobj,"ExtOvershoot"):
                self.remove_ext_overshoot()
                self.draw_ext_overshoot(vobj)
            self.updateData(vobj.Object,"Start")
            vobj.Object.touch()
            
        elif (prop == "FontSize") and hasattr(vobj,"FontSize") and hasattr(vobj, "ScaleMultiplier"):
            if hasattr(self,"font"):
                self.font.size = vobj.FontSize.Value * vobj.ScaleMultiplier
            if hasattr(self,"font3d"):
                self.font3d.size = vobj.FontSize.Value * 100 * vobj.ScaleMultiplier
            vobj.Object.touch()
            
        elif (prop == "FontName") and hasattr(vobj,"FontName"):
            if hasattr(self,"font") and hasattr(self,"font3d"):
                self.font.name = self.font3d.name = str(vobj.FontName)
                vobj.Object.touch()
                
        elif (prop == "LineColor") and hasattr(vobj,"LineColor"):
            if hasattr(self,"color"):
                c = vobj.LineColor
                self.color.rgb.setValue(c[0],c[1],c[2])
                
        elif (prop == "LineWidth") and hasattr(vobj,"LineWidth"):
            if hasattr(self,"drawstyle"):
                self.drawstyle.lineWidth = vobj.LineWidth
                
        elif (prop in ["ArrowSize","ArrowType"]) and hasattr(vobj,"ArrowSize") and hasattr(vobj, "ScaleMultiplier"):
            if hasattr(self,"node") and hasattr(self,"p2"):
                self.remove_dim_arrows()
                self.draw_dim_arrows(vobj)
                vobj.Object.touch()
                
        elif (prop == "DimOvershoot") and hasattr(vobj,"DimOvershoot") and hasattr(vobj, "ScaleMultiplier"):
            self.remove_dim_overshoot()
            self.draw_dim_overshoot(vobj)
            vobj.Object.touch()
            
        elif (prop == "ExtOvershoot") and hasattr(vobj,"ExtOvershoot") and hasattr(vobj, "ScaleMultiplier"):
            self.remove_ext_overshoot()
            self.draw_ext_overshoot(vobj)
            vobj.Object.touch()
            
        elif (prop == "ShowLine") and hasattr(vobj,"ShowLine"):
            if vobj.ShowLine:
                self.lineswitch2.whichChild = -3
                self.lineswitch3.whichChild = -3
            else:
                self.lineswitch2.whichChild = -1
                self.lineswitch3.whichChild = -1
        else:
            self.updateData(vobj.Object,"Start")
            
    def remove_dim_arrows(self):
        # remove existing nodes
        self.node.removeChild(self.marks)
        self.node3d.removeChild(self.marks)

    def draw_dim_arrows(self, vobj):
        from pivy import coin

        if not hasattr(vobj,"ArrowType"):
            return

        if self.p3.x < self.p2.x:
            inv = False
        else:
            inv = True

        # set scale
        symbol = utils.ARROW_TYPES.index(vobj.ArrowType)
        s = vobj.ArrowSize.Value * vobj.ScaleMultiplier
        self.trans1.scaleFactor.setValue((s,s,s))
        self.trans2.scaleFactor.setValue((s,s,s))


        # set new nodes
        self.marks = coin.SoSeparator()
        self.marks.addChild(self.color)
        s1 = coin.SoSeparator()
        if symbol == "Circle":
            s1.addChild(self.coord1)
        else:
            s1.addChild(self.trans1)
        s1.addChild(gui_utils.dim_symbol(symbol,invert=not(inv)))
        self.marks.addChild(s1)
        s2 = coin.SoSeparator()
        if symbol == "Circle":
            s2.addChild(self.coord2)
        else:
            s2.addChild(self.trans2)
        s2.addChild(gui_utils.dim_symbol(symbol,invert=inv))
        self.marks.addChild(s2)
        self.node.insertChild(self.marks,2)
        self.node3d.insertChild(self.marks,2)

    def remove_dim_overshoot(self):
        self.node.removeChild(self.marksDimOvershoot)
        self.node3d.removeChild(self.marksDimOvershoot)

    
    def draw_dim_overshoot(self, vobj):
        from pivy import coin

        # set scale
        s = vobj.DimOvershoot.Value * vobj.ScaleMultiplier
        self.transDimOvershoot1.scaleFactor.setValue((s,s,s))
        self.transDimOvershoot2.scaleFactor.setValue((s,s,s))

        # remove existing nodes

        # set new nodes
        self.marksDimOvershoot = coin.SoSeparator()
        if vobj.DimOvershoot.Value:
            self.marksDimOvershoot.addChild(self.color)
            s1 = coin.SoSeparator()
            s1.addChild(self.transDimOvershoot1)
            s1.addChild(gui_utils.dimDash((-1,0,0),(0,0,0)))
            self.marksDimOvershoot.addChild(s1)
            s2 = coin.SoSeparator()
            s2.addChild(self.transDimOvershoot2)
            s2.addChild(gui_utils.dimDash((0,0,0),(1,0,0)))
            self.marksDimOvershoot.addChild(s2)
        self.node.insertChild(self.marksDimOvershoot,2)
        self.node3d.insertChild(self.marksDimOvershoot,2)

    
    def remove_ext_overshoot(self):
        self.node.removeChild(self.marksExtOvershoot)
        self.node3d.removeChild(self.marksExtOvershoot)

    
    def draw_ext_overshoot(self, vobj):
        from pivy import coin

        # set scale
        s = vobj.ExtOvershoot.Value * vobj.ScaleMultiplier
        self.transExtOvershoot1.scaleFactor.setValue((s,s,s))
        self.transExtOvershoot2.scaleFactor.setValue((s,s,s))

        # set new nodes
        self.marksExtOvershoot = coin.SoSeparator()
        if vobj.ExtOvershoot.Value:
            self.marksExtOvershoot.addChild(self.color)
            s1 = coin.SoSeparator()
            s1.addChild(self.transExtOvershoot1)
            s1.addChild(gui_utils.dimDash((0,0,0),(-1,0,0)))
            self.marksExtOvershoot.addChild(s1)
            s2 = coin.SoSeparator()
            s2.addChild(self.transExtOvershoot2)
            s2.addChild(gui_utils.dimDash((0,0,0),(-1,0,0)))
            self.marksExtOvershoot.addChild(s2)
        self.node.insertChild(self.marksExtOvershoot,2)
        self.node3d.insertChild(self.marksExtOvershoot,2)

    def is_linked_to_circle(self):
        _obj = self.Object
        if _obj.LinkedGeometry and len(_obj.LinkedGeometry) == 1:
            lobj = _obj.LinkedGeometry[0][0]
            lsub = _obj.LinkedGeometry[0][1]
            if len(lsub) == 1 and "Edge" in lsub[0]:
                n = int(lsub[0][4:]) - 1
                edge = lobj.Shape.Edges[n]
                if DraftGeomUtils.geomType(edge) == "Circle":
                    return True
        return False

    def getIcon(self):
        if self.is_linked_to_circle():
            return ":/icons/Draft_DimensionRadius.svg"
        return ":/icons/Draft_Dimension_Tree.svg"


class ViewProviderAngularDimension(ViewProviderDimensionBase):
    """A View Provider for the Draft Angular Dimension object"""
    def __init__(self, vobj):

        super(ViewProviderAngularDimension, self).__init__(vobj)

        vobj.addProperty("App::PropertyBool","FlipArrows",
                        "Graphics",QT_TRANSLATE_NOOP("App::Property",
                        "Rotate the dimension arrows 180 degrees"))
        
        self.Object = vobj.Object
        vobj.Proxy = self

    def attach(self, vobj):
        '''Setup the scene sub-graph of the view provider'''
        from pivy import coin
        self.Object = vobj.Object
        self.color = coin.SoBaseColor()
        if hasattr(vobj,"LineColor"):
            self.color.rgb.setValue(vobj.LineColor[0],vobj.LineColor[1],vobj.LineColor[2])
        self.font = coin.SoFont()
        self.font3d = coin.SoFont()
        self.text = coin.SoAsciiText()
        self.text3d = coin.SoText2()
        self.text.string = "d" # some versions of coin crash if string is not set
        self.text3d.string = "d"
        self.text.justification = self.text3d.justification = coin.SoAsciiText.CENTER
        self.textpos = coin.SoTransform()
        label = coin.SoSeparator()
        label.addChild(self.textpos)
        label.addChild(self.color)
        label.addChild(self.font)
        label.addChild(self.text)
        label3d = coin.SoSeparator()
        label3d.addChild(self.textpos)
        label3d.addChild(self.color)
        label3d.addChild(self.font3d)
        label3d.addChild(self.text3d)
        self.coord1 = coin.SoCoordinate3()
        self.trans1 = coin.SoTransform()
        self.coord2 = coin.SoCoordinate3()
        self.trans2 = coin.SoTransform()
        self.marks = coin.SoSeparator()
        self.drawstyle = coin.SoDrawStyle()
        self.coords = coin.SoCoordinate3()
        self.arc = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.node = coin.SoGroup()
        self.node.addChild(self.color)
        self.node.addChild(self.drawstyle)
        self.node.addChild(self.coords)
        self.node.addChild(self.arc)
        self.node.addChild(self.marks)
        self.node.addChild(label)
        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.color)
        self.node3d.addChild(self.drawstyle)
        self.node3d.addChild(self.coords)
        self.node3d.addChild(self.arc)
        self.node3d.addChild(self.marks)
        self.node3d.addChild(label3d)
        vobj.addDisplayMode(self.node,"2D")
        vobj.addDisplayMode(self.node3d,"3D")
        self.updateData(vobj.Object,None)
        self.onChanged(vobj,"FontSize")
        self.onChanged(vobj,"FontName")
        self.onChanged(vobj,"ArrowType")
        self.onChanged(vobj,"LineColor")

    def updateData(self, obj, prop):
        if hasattr(self,"arc"):
            from pivy import coin
            import Part, DraftGeomUtils
            import DraftGui
            arcsegs = 24

            # calculate the arc data
            if DraftVecUtils.isNull(obj.Normal):
                norm = App.Vector(0,0,1)
            else:
                norm = obj.Normal
            radius = (obj.Dimline.sub(obj.Center)).Length
            self.circle = Part.makeCircle(radius,obj.Center,norm,obj.FirstAngle.Value,obj.LastAngle.Value)
            self.p2 = self.circle.Vertexes[0].Point
            self.p3 = self.circle.Vertexes[-1].Point
            mp = DraftGeomUtils.findMidpoint(self.circle.Edges[0])
            ray = mp.sub(obj.Center)

            # set text value
            if obj.LastAngle.Value > obj.FirstAngle.Value:
                a = obj.LastAngle.Value - obj.FirstAngle.Value
            else:
                a = (360 - obj.FirstAngle.Value) + obj.LastAngle.Value
            su = True
            if hasattr(obj.ViewObject,"ShowUnit"):
                su = obj.ViewObject.ShowUnit
            if hasattr(obj.ViewObject,"Decimals"):
                self.string = DraftGui.displayExternal(a,obj.ViewObject.Decimals,'Angle',su)
            else:
                self.string = DraftGui.displayExternal(a,None,'Angle',su)
            if obj.ViewObject.Override:
                self.string = obj.ViewObject.Override.replace("$dim",\
                    self.string)
            self.text.string = self.text3d.string = utils.string_encode_coin(self.string)

            # check display mode
            try:
                m = obj.ViewObject.DisplayMode
            except: # swallow all exceptions here since it always fails on first run (Displaymode enum no set yet)
                m = ["2D","3D"][utils.get_param("dimstyle",0)]

            # set the arc
            if m == "3D":
                # calculate the spacing of the text
                spacing = (len(self.string)*obj.ViewObject.FontSize.Value)/8.0
                pts1 = []
                cut = None
                pts2 = []
                for i in range(arcsegs+1):
                    p = self.circle.valueAt(self.circle.FirstParameter+((self.circle.LastParameter-self.circle.FirstParameter)/arcsegs)*i)
                    if (p.sub(mp)).Length <= spacing:
                        if cut is None:
                            cut = i
                    else:
                        if cut is None:
                            pts1.append([p.x,p.y,p.z])
                        else:
                            pts2.append([p.x,p.y,p.z])
                self.coords.point.setValues(pts1+pts2)
                i1 = len(pts1)
                i2 = i1+len(pts2)
                self.arc.coordIndex.setValues(0,len(pts1)+len(pts2)+1,list(range(len(pts1)))+[-1]+list(range(i1,i2)))
                if (len(pts1) >= 3) and (len(pts2) >= 3):
                    self.circle1 = Part.Arc(App.Vector(pts1[0][0],pts1[0][1],pts1[0][2]),App.Vector(pts1[1][0],pts1[1][1],pts1[1][2]),App.Vector(pts1[-1][0],pts1[-1][1],pts1[-1][2])).toShape()
                    self.circle2 = Part.Arc(App.Vector(pts2[0][0],pts2[0][1],pts2[0][2]),App.Vector(pts2[1][0],pts2[1][1],pts2[1][2]),App.Vector(pts2[-1][0],pts2[-1][1],pts2[-1][2])).toShape()
            else:
                pts = []
                for i in range(arcsegs+1):
                    p = self.circle.valueAt(self.circle.FirstParameter+((self.circle.LastParameter-self.circle.FirstParameter)/arcsegs)*i)
                    pts.append([p.x,p.y,p.z])
                self.coords.point.setValues(pts)
                self.arc.coordIndex.setValues(0,arcsegs+1,list(range(arcsegs+1)))

            # set the arrow coords and rotation
            self.trans1.translation.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.coord1.point.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.trans2.translation.setValue((self.p3.x,self.p3.y,self.p3.z))
            self.coord2.point.setValue((self.p3.x,self.p3.y,self.p3.z))
            # calculate small chords to make arrows look better
            arrowlength = 4*obj.ViewObject.ArrowSize.Value
            u1 = (self.circle.valueAt(self.circle.FirstParameter+arrowlength)).sub(self.circle.valueAt(self.circle.FirstParameter)).normalize()
            u2 = (self.circle.valueAt(self.circle.LastParameter)).sub(self.circle.valueAt(self.circle.LastParameter-arrowlength)).normalize()
            if hasattr(obj.ViewObject,"FlipArrows"):
                if obj.ViewObject.FlipArrows:
                    u1 = u1.negative()
                    u2 = u2.negative()
            w2 = self.circle.Curve.Axis
            w1 = w2.negative()
            v1 = w1.cross(u1)
            v2 = w2.cross(u2)
            q1 = App.Placement(DraftVecUtils.getPlaneRotation(u1,v1,w1)).Rotation.Q
            q2 = App.Placement(DraftVecUtils.getPlaneRotation(u2,v2,w2)).Rotation.Q
            self.trans1.rotation.setValue((q1[0],q1[1],q1[2],q1[3]))
            self.trans2.rotation.setValue((q2[0],q2[1],q2[2],q2[3]))

            # setting text pos & rot
            self.tbase = mp
            if hasattr(obj.ViewObject,"TextPosition"):
                if not DraftVecUtils.isNull(obj.ViewObject.TextPosition):
                    self.tbase = obj.ViewObject.TextPosition

            u3 = ray.cross(norm).normalize()
            v3 = norm.cross(u3)
            r = App.Placement(DraftVecUtils.getPlaneRotation(u3,v3,norm)).Rotation
            offset = r.multVec(App.Vector(0,1,0))

            if hasattr(obj.ViewObject,"TextSpacing"):
                offset = DraftVecUtils.scaleTo(offset,obj.ViewObject.TextSpacing.Value)
            else:
                offset = DraftVecUtils.scaleTo(offset,0.05)
            if m == "3D":
                offset = offset.negative()
            self.tbase = self.tbase.add(offset)
            q = r.Q
            self.textpos.translation.setValue([self.tbase.x,self.tbase.y,self.tbase.z])
            self.textpos.rotation = coin.SbRotation(q[0],q[1],q[2],q[3])

            # set the angle property
            if round(obj.Angle,utils.precision()) != round(a,utils.precision()):
                obj.Angle = a

    def onChanged(self, vobj, prop):
        if hasattr(vobj, "ScaleMultiplier"):
            if vobj.ScaleMultiplier == 0:
                return
        if prop == "ScaleMultiplier" and hasattr(vobj, "ScaleMultiplier"):
            if hasattr(self,"font"):
                self.font.size = vobj.FontSize.Value * vobj.ScaleMultiplier
            if hasattr(self,"font3d"):
                self.font3d.size = vobj.FontSize.Value * 100 * vobj.ScaleMultiplier
            if hasattr(self,"node") and hasattr(self,"p2") and hasattr(vobj,"ArrowSize"):
                self.remove_dim_arrows()
                self.draw_dim_arrows(vobj)
            self.updateData(vobj.Object,"Start")
            vobj.Object.touch()
        elif prop == "FontSize" and hasattr(vobj, "ScaleMultiplier"):
            if hasattr(self,"font"):
                self.font.size = vobj.FontSize.Value * vobj.ScaleMultiplier
            if hasattr(self,"font3d"):
                self.font3d.size = vobj.FontSize.Value * 100 * vobj.ScaleMultiplier
            vobj.Object.touch()
        elif prop == "FontName":
            if hasattr(self,"font") and hasattr(self,"font3d"):
                self.font.name = self.font3d.name = str(vobj.FontName)
                vobj.Object.touch()
        elif prop == "LineColor":
            if hasattr(self,"color") and hasattr(vobj,"LineColor"):
                c = vobj.LineColor
                self.color.rgb.setValue(c[0],c[1],c[2])
        elif prop == "LineWidth":
            if hasattr(self,"drawstyle"):
                self.drawstyle.lineWidth = vobj.LineWidth
        elif prop in ["ArrowSize","ArrowType"] and hasattr(vobj, "ScaleMultiplier"):
            if hasattr(self,"node") and hasattr(self,"p2"):
                self.remove_dim_arrows()
                self.draw_dim_arrows(vobj)
                vobj.Object.touch()
        else:
            self.updateData(vobj.Object, None)

    def remove_dim_arrows(self):
        # remove existing nodes
        self.node.removeChild(self.marks)
        self.node3d.removeChild(self.marks)

    def draw_dim_arrows(self, vobj):
        from pivy import coin

        if not hasattr(vobj,"ArrowType"):
            return

        # set scale
        symbol = utils.ARROW_TYPES.index(vobj.ArrowType)
        s = vobj.ArrowSize.Value * vobj.ScaleMultiplier
        self.trans1.scaleFactor.setValue((s,s,s))
        self.trans2.scaleFactor.setValue((s,s,s))

        # set new nodes
        self.marks = coin.SoSeparator()
        self.marks.addChild(self.color)
        s1 = coin.SoSeparator()
        if symbol == "Circle":
            s1.addChild(self.coord1)
        else:
            s1.addChild(self.trans1)
        s1.addChild(gui_utils.dim_symbol(symbol,invert=False))
        self.marks.addChild(s1)
        s2 = coin.SoSeparator()
        if symbol == "Circle":
            s2.addChild(self.coord2)
        else:
            s2.addChild(self.trans2)
        s2.addChild(gui_utils.dim_symbol(symbol,invert=True))
        self.marks.addChild(s2)
        self.node.insertChild(self.marks,2)
        self.node3d.insertChild(self.marks,2)

    def getIcon(self):
        return ":/icons/Draft_DimensionAngular.svg"
