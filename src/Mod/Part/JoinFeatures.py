# ***************************************************************************
# *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

import FreeCAD, Part


__title__ = "JoinFeatures module (legacy)"
__author__ = "DeepSOIC"
__url__ = "https://www.freecad.org"
__doc__ = "Legacy JoinFeatures module provided for ability to load projects made with \
FreeCAD v0.16. Do not use. Use BOPTools.JoinFeatures instead."


def getParamRefine():
    return FreeCAD.ParamGet(
        "User parameter:BaseApp/Preferences/Mod/Part/Boolean"
    ).GetBool("RefineModel")


def shapeOfMaxVol(compound):
    if compound.ShapeType == "Compound":
        maxVol = 0
        cntEq = 0
        shMax = None
        for sh in compound.childShapes():
            v = sh.Volume
            if v > maxVol + 1e-8:
                maxVol = v
                shMax = sh
                cntEq = 1
            elif abs(v - maxVol) <= 1e-8:
                cntEq = cntEq + 1
        if cntEq > 1:
            raise ValueError("Equal volumes, can't figure out what to cut off!")
        return shMax
    else:
        return compound



class _PartJoinFeature:
    "The PartJoinFeature object"

    def __init__(self, obj):
        self.Type = "PartJoinFeature"
        obj.addProperty(
            "App::PropertyEnumeration",
            "Mode",
            "Join",
            "The mode of operation. bypass = make compound (fast)", locked=True,
        )
        obj.Mode = ["bypass", "Connect", "Embed", "Cutout"]
        obj.addProperty("App::PropertyLink", "Base", "Join", "First object", locked=True)
        obj.addProperty("App::PropertyLink", "Tool", "Join", "Second object", locked=True)
        obj.addProperty(
            "App::PropertyBool",
            "Refine",
            "Join",
            "True = refine resulting shape. False = output as is.", locked=True,
        )

        obj.Proxy = self

    def execute(self, obj):
        rst = None
        if obj.Mode == "bypass":
            rst = Part.makeCompound([obj.Base.Shape, obj.Tool.Shape])
        else:
            cut1 = obj.Base.Shape.cut(obj.Tool.Shape)
            cut1 = shapeOfMaxVol(cut1)
            if obj.Mode == "Connect":
                cut2 = obj.Tool.Shape.cut(obj.Base.Shape)
                cut2 = shapeOfMaxVol(cut2)
                rst = cut1.multiFuse([cut2, obj.Tool.Shape.common(obj.Base.Shape)])
            elif obj.Mode == "Embed":
                rst = cut1.fuse(obj.Tool.Shape)
            elif obj.Mode == "Cutout":
                rst = cut1
            if obj.Refine:
                rst = rst.removeSplitter()
        obj.Shape = rst
        return


class _ViewProviderPartJoinFeature:
    "A View Provider for the PartJoinFeature object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        if self.Object is None:
            return ":/icons/booleans/Part_JoinConnect.svg"
        else:
            return {
                    "bypass": ":/icons/booleans/Part_JoinBypass.svg",
                    "Connect": ":/icons/booleans/Part_JoinConnect.svg",
                    "Embed": ":/icons/booleans/Part_JoinEmbed.svg",
                    "Cutout": ":/icons/booleans/Part_JoinCutout.svg",
                }[self.Object.Mode]

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def setEdit(self, vobj, mode):
        return False

    def unsetEdit(self, vobj, mode):
        return

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def claimChildren(self):
        return [self.Object.Base, self.Object.Tool]

    def onDelete(self, feature, subelements):
        try:
            self.Object.Base.ViewObject.show()
            self.Object.Tool.ViewObject.show()
        except Exception as err:
            FreeCAD.Console.PrintError("Error in onDelete: " + str(err))
        return True
