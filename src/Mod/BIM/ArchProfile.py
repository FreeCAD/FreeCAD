# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__  = "FreeCAD Profile"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

## @package ArchProfile
#  \ingroup ARCH
#  \brief Profile tools for ArchStructure
#
#  This module provides tools to build base profiles
#  for Arch Structure elements

import csv
import os

import FreeCAD
import Draft

from FreeCAD import Vector
from draftutils import params

if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond


# Presets in the form: Class, Name, Profile type, [profile data]
# Search for profiles.csv in data/Mod/Arch/Presets and in the same folder as this file
# and in the user path
profilefiles = [os.path.join(FreeCAD.getResourceDir(),"Mod","BIM","Presets","profiles.csv"),
                os.path.join(os.path.dirname(__file__),"Presets","profiles.csv"),
                os.path.join(FreeCAD.getUserAppDataDir(),"BIM","profiles.csv")]


def readPresets():

    Presets=[]
    bid = 1 #Unique index
    for profilefile in profilefiles:
        if os.path.exists(profilefile):
            try:
                with open(profilefile, "r") as csvfile:
                    beamreader = csv.reader(csvfile)
                    for row in beamreader:
                        if (not row) or row[0].startswith("#"):
                            continue
                        try:
                            r=[bid, row[0], row[1], row[2]]
                            for i in range(3,len(row)):
                                r=r+[float(row[i])]
                            if not r in Presets:
                                Presets.append(r)
                            bid=bid+1
                        except ValueError:
                            print("Skipping bad line: "+str(row))
            except IOError:
                print("Could not open ",profilefile)
    return Presets



class _Profile(Draft._DraftObject):

    '''Superclass for Profile classes'''

    def __init__(self,obj, profile):
        self.Profile = profile
        Draft._DraftObject.__init__(self,obj,"Profile")

    def dumps(self):
        if hasattr(self,"Profile"):
            return self.Profile

    def loads(self,state):
        if isinstance(state,list):
            self.Profile = state
        self.Type = "Profile"

    def cleanProperties(self, obj):

        '''Remove all Profile properties'''

        for prop in [
            "Width", "Height", "WebThickness", "FlangeThickness","OutDiameter", "Thickness"
        ]:
            if hasattr(obj, prop):
                obj.setPropertyStatus(prop, "-LockDynamic")
                obj.removeProperty(prop)


class _ProfileC(_Profile):

    '''A parametric circular tubeprofile. Profile data: [Outside diameter, Wall thickness]'''

    def __init__(self,obj, profile):
        self.cleanProperties(obj)
        obj.addProperty("App::PropertyLength","OutDiameter","Draft",QT_TRANSLATE_NOOP("App::Property","Outside Diameter"), locked=True).OutDiameter = profile[4]
        obj.addProperty("App::PropertyLength","Thickness","Draft",QT_TRANSLATE_NOOP("App::Property","Wall thickness"), locked=True).Thickness = profile[5]
        _Profile.__init__(self,obj,profile)

    def execute(self,obj):
        import Part
        pl = obj.Placement
        c1=Part.Circle()
        c1.Radius=obj.OutDiameter.Value/2
        c2=Part.Circle()
        c2.Radius=obj.OutDiameter.Value/2-obj.Thickness.Value
        cs1=c1.toShape()
        cs2=c2.toShape()
        p=Part.makeRuledSurface(cs2,cs1)
        p.reverse()
        obj.Shape = p
        obj.Placement = pl


class _ProfileH(_Profile):

    '''A parametric H or I beam profile. Profile data: [width, height, web thickness, flange thickness] (see http://en.wikipedia.org/wiki/I-beam for reference)'''

    def __init__(self,obj, profile):
        self.cleanProperties(obj)
        obj.addProperty("App::PropertyLength","Width","Draft",QT_TRANSLATE_NOOP("App::Property","Width of the beam"), locked=True).Width = profile[4]
        obj.addProperty("App::PropertyLength","Height","Draft",QT_TRANSLATE_NOOP("App::Property","Height of the beam"), locked=True).Height = profile[5]
        obj.addProperty("App::PropertyLength","WebThickness","Draft",QT_TRANSLATE_NOOP("App::Property","Thickness of the web"), locked=True).WebThickness = profile[6]
        obj.addProperty("App::PropertyLength","FlangeThickness","Draft",QT_TRANSLATE_NOOP("App::Property","Thickness of the flanges"), locked=True).FlangeThickness = profile[7]
        _Profile.__init__(self,obj,profile)

    def execute(self,obj):
        import Part
        pl = obj.Placement
        p1 = Vector(-obj.Width.Value/2,-obj.Height.Value/2,0)
        p2 = Vector(obj.Width.Value/2,-obj.Height.Value/2,0)
        p3 = Vector(obj.Width.Value/2,(-obj.Height.Value/2)+obj.FlangeThickness.Value,0)
        p4 = Vector(obj.WebThickness.Value/2,(-obj.Height.Value/2)+obj.FlangeThickness.Value,0)
        p5 = Vector(obj.WebThickness.Value/2,obj.Height.Value/2-obj.FlangeThickness.Value,0)
        p6 = Vector(obj.Width.Value/2,obj.Height.Value/2-obj.FlangeThickness.Value,0)
        p7 = Vector(obj.Width.Value/2,obj.Height.Value/2,0)
        p8 = Vector(-obj.Width.Value/2,obj.Height.Value/2,0)
        p9 = Vector(-obj.Width.Value/2,obj.Height.Value/2-obj.FlangeThickness.Value,0)
        p10 = Vector(-obj.WebThickness.Value/2,obj.Height.Value/2-obj.FlangeThickness.Value,0)
        p11 = Vector(-obj.WebThickness.Value/2,(-obj.Height.Value/2)+obj.FlangeThickness.Value,0)
        p12 = Vector(-obj.Width.Value/2,(-obj.Height.Value/2)+obj.FlangeThickness.Value,0)
        p = Part.makePolygon([p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p1])
        p = Part.Face(p)
        #p.reverse()
        obj.Shape = p
        obj.Placement = pl


class _ProfileR(_Profile):

    '''A parametric rectangular beam profile based on [Width, Height]'''

    def __init__(self,obj, profile):
        self.cleanProperties(obj)
        obj.addProperty("App::PropertyLength","Width","Draft",QT_TRANSLATE_NOOP("App::Property","Width of the beam"), locked=True).Width = profile[4]
        obj.addProperty("App::PropertyLength","Height","Draft",QT_TRANSLATE_NOOP("App::Property","Height of the beam"), locked=True).Height = profile[5]
        _Profile.__init__(self,obj,profile)

    def execute(self,obj):
        import Part
        pl = obj.Placement
        p1 = Vector(-obj.Width.Value/2,-obj.Height.Value/2,0)
        p2 = Vector(obj.Width.Value/2,-obj.Height.Value/2,0)
        p3 = Vector(obj.Width.Value/2,obj.Height.Value/2,0)
        p4 = Vector(-obj.Width.Value/2,obj.Height.Value/2,0)
        p = Part.makePolygon([p1,p2,p3,p4,p1])
        p = Part.Face(p)
        #p.reverse()
        obj.Shape = p
        obj.Placement = pl


class _ProfileRH(_Profile):

    '''A parametric Rectangular hollow beam profile. Profile data: [width, height, thickness]'''

    def __init__(self,obj, profile):
        self.cleanProperties(obj)
        obj.addProperty("App::PropertyLength","Width","Draft",QT_TRANSLATE_NOOP("App::Property","Width of the beam"), locked=True).Width = profile[4]
        obj.addProperty("App::PropertyLength","Height","Draft",QT_TRANSLATE_NOOP("App::Property","Height of the beam"), locked=True).Height = profile[5]
        obj.addProperty("App::PropertyLength","Thickness","Draft",QT_TRANSLATE_NOOP("App::Property","Thickness of the sides"), locked=True).Thickness = profile[6]
        _Profile.__init__(self,obj,profile)

    def execute(self,obj):
        import Part
        pl = obj.Placement
        p1 = Vector(-obj.Width.Value/2,-obj.Height.Value/2,0)
        p2 = Vector(obj.Width.Value/2,-obj.Height.Value/2,0)
        p3 = Vector(obj.Width.Value/2,obj.Height.Value/2,0)
        p4 = Vector(-obj.Width.Value/2,obj.Height.Value/2,0)
        q1 = Vector(-obj.Width.Value/2+obj.Thickness.Value,-obj.Height.Value/2+obj.Thickness.Value,0)
        q2 = Vector(obj.Width.Value/2-obj.Thickness.Value,-obj.Height.Value/2+obj.Thickness.Value,0)
        q3 = Vector(obj.Width.Value/2-obj.Thickness.Value,obj.Height.Value/2-obj.Thickness.Value,0)
        q4 = Vector(-obj.Width.Value/2+obj.Thickness.Value,obj.Height.Value/2-obj.Thickness.Value,0)
        p = Part.makePolygon([p1,p2,p3,p4,p1])
        q = Part.makePolygon([q1,q2,q3,q4,q1])
        #r = Part.Face([p,q])
        #r.reverse()
        p = Part.Face(p)
        q = Part.Face(q)
        r = p.cut(q)
        obj.Shape = r
        obj.Placement = pl


class _ProfileU(_Profile):

    '''A parametric U profile. Profile data: [width, height, web thickness, flange thickness] (see  https://en.wikipedia.org/wiki/Structural_channel for reference)'''

    def __init__(self,obj, profile):
        self.cleanProperties(obj)
        obj.addProperty("App::PropertyLength","Width","Draft",QT_TRANSLATE_NOOP("App::Property","Width of the beam"), locked=True).Width = profile[4]
        obj.addProperty("App::PropertyLength","Height","Draft",QT_TRANSLATE_NOOP("App::Property","Height of the beam"), locked=True).Height = profile[5]
        obj.addProperty("App::PropertyLength","WebThickness","Draft",QT_TRANSLATE_NOOP("App::Property","Thickness of the webs"), locked=True).WebThickness = profile[6]
        obj.addProperty("App::PropertyLength","FlangeThickness","Draft",QT_TRANSLATE_NOOP("App::Property","Thickness of the flange"), locked=True).FlangeThickness = profile[7]
        _Profile.__init__(self,obj,profile)

    def execute(self,obj):
        import Part
        pl = obj.Placement
        p1 = Vector(-obj.Width.Value/2,-obj.Height.Value/2,0)
        p2 = Vector(obj.Width.Value/2,-obj.Height.Value/2,0)
        p3 = Vector(obj.Width.Value/2,obj.Height.Value/2,0)
        p4 = Vector(obj.Width.Value/2-obj.FlangeThickness.Value,obj.Height.Value/2,0)
        p5 = Vector(obj.Width.Value/2-obj.FlangeThickness.Value,obj.WebThickness.Value-obj.Height.Value/2,0)
        p6 = Vector(-obj.Width.Value/2+obj.FlangeThickness.Value,obj.WebThickness.Value-obj.Height.Value/2,0)
        p7 = Vector(-obj.Width.Value/2+obj.FlangeThickness.Value,obj.Height.Value/2,0)
        p8 = Vector(-obj.Width.Value/2,obj.Height.Value/2,0)
        p = Part.makePolygon([p1,p2,p3,p4,p5,p6,p7,p8,p1])
        p = Part.Face(p)
        #p.reverse()
        obj.Shape = p
        obj.Placement = pl


class _ProfileL(_Profile):

    '''A parametric L profile. Profile data: [width, height, thickness]'''

    def __init__(self, obj, profile):
        self.cleanProperties(obj)
        obj.addProperty("App::PropertyLength","Width","Draft",QT_TRANSLATE_NOOP("App::Property","Width of the beam"), locked=True).Width = profile[4]
        obj.addProperty("App::PropertyLength","Height","Draft",QT_TRANSLATE_NOOP("App::Property","Height of the beam"), locked=True).Height = profile[5]
        obj.addProperty("App::PropertyLength","Thickness","Draft",QT_TRANSLATE_NOOP("App::Property","Thickness of the legs"), locked=True).Thickness = profile[6]
        _Profile.__init__(self,obj,profile)

    def execute(self,obj):
        import Part
        pl = obj.Placement
        p1 = Vector(-obj.Width.Value/2, obj.Height.Value/2, 0)
        p2 = Vector(-obj.Width.Value/2, -obj.Height.Value/2, 0)
        p3 = Vector(obj.Width.Value/2, -obj.Height.Value/2, 0)
        p4 = Vector(obj.Width.Value/2, -obj.Height.Value/2+obj.Thickness.Value, 0)
        p5 = Vector(-obj.Width.Value/2+obj.Thickness.Value, -obj.Height.Value/2+obj.Thickness.Value, 0)
        p6 = Vector(-obj.Width.Value/2+obj.Thickness.Value, obj.Height.Value/2, 0)
        p = Part.makePolygon([p1,p2,p3,p4,p5,p6,p1])
        p = Part.Face(p)
        #p.reverse()
        obj.Shape = p
        obj.Placement = pl


class _ProfileT(_Profile):

    '''A parametric T profile. Profile data: [width, height, web thickness, flange thickness]'''

    def __init__(self, obj, profile):
        self.cleanProperties(obj)
        obj.addProperty("App::PropertyLength","Width","Draft",QT_TRANSLATE_NOOP("App::Property","Width of the beam"), locked=True).Width = profile[4]
        obj.addProperty("App::PropertyLength","Height","Draft",QT_TRANSLATE_NOOP("App::Property","Height of the beam"), locked=True).Height = profile[5]
        obj.addProperty("App::PropertyLength","WebThickness","Draft",QT_TRANSLATE_NOOP("App::Property","Thickness of the web"), locked=True).WebThickness = profile[6]
        obj.addProperty("App::PropertyLength","FlangeThickness","Draft",QT_TRANSLATE_NOOP("App::Property","Thickness of the flanges"), locked=True).FlangeThickness = profile[7]
        _Profile.__init__(self,obj,profile)

    def execute(self,obj):
        import Part
        pl = obj.Placement
        p1 = Vector(obj.WebThickness.Value/2, -obj.Height.Value/2, 0)
        p2 = Vector(obj.WebThickness.Value/2, obj.Height.Value/2-obj.FlangeThickness.Value, 0)
        p3 = Vector(obj.Width.Value/2, obj.Height.Value/2-obj.FlangeThickness.Value, 0)
        p4 = Vector(obj.Width.Value/2, obj.Height.Value/2, 0)
        p5 = Vector(-obj.Width.Value/2, obj.Height.Value/2, 0)
        p6 = Vector(-obj.Width.Value/2, obj.Height.Value/2-obj.FlangeThickness.Value, 0)
        p7 = Vector(-obj.WebThickness.Value/2, obj.Height.Value/2-obj.FlangeThickness.Value, 0)
        p8 = Vector(-obj.WebThickness.Value/2, -obj.Height.Value/2, 0)
        p = Part.makePolygon([p1,p2,p3,p4,p5,p6,p7,p8,p1])
        p = Part.Face(p)
        #p.reverse()
        obj.Shape = p
        obj.Placement = pl


class ViewProviderProfile(Draft._ViewProviderDraft):

    '''General view provider for Profile classes'''

    def __init__(self,vobj):

        Draft._ViewProviderDraft.__init__(self,vobj)

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Profile.svg"

    def setEdit(self, vobj, mode):
        if mode == 1 or mode == 2:
            return None

        taskd = ProfileTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        if mode == 1 or mode == 2:
            return None

        FreeCADGui.Control.closeDialog()
        return True


class ProfileTaskPanel:

    '''The editmode TaskPanel for Profile objects'''

    def __init__(self,obj):

        self.obj = obj
        self.Profile = None
        if hasattr(obj.Proxy,"Profile"):
            self.Profile = obj.Proxy.Profile
        if isinstance(self.obj.Proxy,_ProfileC):
            self.type = "C"
        elif isinstance(self.obj.Proxy,_ProfileH):
            self.type = "H"
        elif isinstance(self.obj.Proxy,_ProfileR):
            self.type = "R"
        elif isinstance(self.obj.Proxy,_ProfileRH):
            self.type = "RH"
        elif isinstance(self.obj.Proxy,_ProfileU):
            self.type = "U"
        elif isinstance(self.obj.Proxy,_ProfileL):
            self.type = "L"
        elif isinstance(self.obj.Proxy,_ProfileT):
            self.type = "T"
        else:
            self.type = "Building Element Proxy"
        self.form = QtGui.QWidget()
        layout = QtGui.QVBoxLayout(self.form)
        self.comboCategory = QtGui.QComboBox(self.form)
        layout.addWidget(self.comboCategory)
        self.comboProfile = QtGui.QComboBox(self.form)
        layout.addWidget(self.comboProfile)
        QtCore.QObject.connect(self.comboCategory, QtCore.SIGNAL("currentTextChanged(QString)"), self.changeCategory)
        QtCore.QObject.connect(self.comboProfile, QtCore.SIGNAL("currentIndexChanged(int)"), self.changeProfile)
        # Read preset profiles and add relevant ones
        self.categories = []
        self.presets = readPresets()
        for pre in self.presets:
            if pre[1] not in self.categories:
                self.categories.append(pre[1])
        self.comboCategory.addItem(" ")
        if self.categories:
            self.comboCategory.addItems(self.categories)
        # Find current profile by label
        if not self.Profile:
            for pre in self.presets:
                if self.type == pre[1]:
                    if self.obj.Label in pre[2]:
                        self.Profile = pre
                        break
        if not self.Profile:
            # try to find by size
            if hasattr(self.obj,"Width") and hasattr(self.obj,"Height"):
                for pre in self.presets:
                    if self.type == pre[1]:
                        if abs(self.obj.Width - self.Profile[4]) < 0.1 and \
                           abs(self.obj.Height - self.Profile[5]) < 0.1:
                            self.Profile = pre
                            break
        if self.Profile:
            origprofile = list(self.Profile) # the operation below will change self.profile
            self.comboCategory.setCurrentIndex(1+self.categories.index(origprofile[1]))
            self.changeCategory(origprofile[1])
            self.comboProfile.setCurrentIndex(self.currentpresets.index(origprofile))
        self.retranslateUi(self.form)

    def changeCategory(self,text):

        self.comboProfile.clear()
        self.currentpresets = []
        for pre in self.presets:
            if pre[1] == text:
                self.currentpresets.append(pre)
                f = FreeCAD.Units.Quantity(pre[4],FreeCAD.Units.Length).getUserPreferred()
                d = params.get_param("Decimals",path="Units")
                s1 = str(round(pre[4]/f[1],d))
                s2 = str(round(pre[5]/f[1],d))
                s3 = str(f[2])
                self.comboProfile.addItem(pre[2]+" ("+s1+"x"+s2+s3+")")

    def changeProfile(self,idx):

        self.Profile = self.currentpresets[idx]

    def accept(self):

        self.obj.Label = self.Profile[2]
        if self.Profile:
            if self.Profile[3]=="C":
                _ProfileC(self.obj, self.Profile)
            elif self.Profile[3]=="H":
                _ProfileH(self.obj, self.Profile)
            elif self.Profile[3]=="R":
                _ProfileR(self.obj, self.Profile)
            elif self.Profile[3]=="RH":
                _ProfileRH(self.obj, self.Profile)
            elif self.Profile[3]=="U":
                _ProfileU(self.obj, self.Profile)
            elif self.Profile[3]=="L":
                _ProfileL(self.obj, self.Profile)
            elif self.Profile[3]=="T":
                _ProfileT(self.obj, self.Profile)
            else:
                print("Profile not supported")

            FreeCAD.ActiveDocument.recompute()
            FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):

        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):

        self.form.setWindowTitle(self.type+" "+QtGui.QApplication.translate("Arch", "Profile", None))
