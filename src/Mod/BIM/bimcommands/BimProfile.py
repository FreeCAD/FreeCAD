# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""BIM Panel-related Arch_"""


import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Profile:

    """The FreeCAD Arch_Profile command definition"""

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Profile',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Profile","Profile"),
                'Accel': "P, F",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Profile","Creates a profile")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        import ArchProfile
        self.Profile = None
        self.Categories = []
        self.Presets = ArchProfile.readPresets()
        for pre in self.Presets:
            if pre[1] not in self.Categories:
                self.Categories.append(pre[1])
        FreeCADGui.Snapper.getPoint(callback=self.getPoint,extradlg=[self.taskbox()],title=translate("Arch","Create profile"))

    def taskbox(self):

        "sets up a taskbox widget"

        from PySide import QtGui
        from draftutils import params
        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch","Profile settings"))
        grid = QtGui.QGridLayout(w)

        # categories box
        labelc = QtGui.QLabel(translate("Arch","Category"))
        self.vCategory = QtGui.QComboBox()
        self.vCategory.addItems([" "] + self.Categories)
        grid.addWidget(labelc,1,0,1,1)
        grid.addWidget(self.vCategory,1,1,1,1)

        # presets box
        labelp = QtGui.QLabel(translate("Arch","Preset"))
        self.vPresets = QtGui.QComboBox()
        self.pSelect = [None]
        fpresets = [" "]
        self.vPresets.addItems(fpresets)
        grid.addWidget(labelp,2,0,1,1)
        grid.addWidget(self.vPresets,2,1,1,1)

        # connect slots
        self.vCategory.currentIndexChanged.connect(self.setCategory)
        self.vPresets.currentIndexChanged.connect(self.setPreset)

        # restore preset
        stored = params.get_param_arch("StructurePreset")
        if stored:
            if ";" in stored:
                stored = stored.split(";")
                if len(stored) >= 3:
                    if stored[1] in self.Categories:
                        self.vCategory.setCurrentIndex(1+self.Categories.index(stored[1]))
                        self.setCategory(1+self.Categories.index(stored[1]))
                        ps = [p[2] for p in self.pSelect]
                        if stored[2] in ps:
                            self.vPresets.setCurrentIndex(ps.index(stored[2]))
        return w

    def getPoint(self,point=None,obj=None):

        "this function is called by the snapper when it has a 3D point"

        if not point:
            return
        if not self.Profile:
            return
        pt = "FreeCAD.Vector("+str(point.x)+","+str(point.y)+","+str(point.z)+")"
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Profile"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand('p = Arch.makeProfile('+str(self.Profile)+')')
        FreeCADGui.addModule('WorkingPlane')
        FreeCADGui.doCommand('p.Placement = WorkingPlane.get_working_plane().get_placement()')
        FreeCADGui.doCommand('p.Placement.Base = ' + pt)
        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("Draft.autogroup(p)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    def setCategory(self,i):

        from draftutils import params
        self.vPresets.clear()
        if i == 0:
            self.pSelect = [None]
            self.vPresets.addItems([" "])
            params.set_param_arch("StructurePreset","")
        else:
            self.pSelect = [p for p in self.Presets if p[1] == self.Categories[i-1]]
            fpresets = []
            for p in self.pSelect:
                f = FreeCAD.Units.Quantity(p[4],FreeCAD.Units.Length).getUserPreferred()
                d = params.get_param("Decimals",path="Units")
                s1 = str(round(p[4]/f[1],d))
                s2 = str(round(p[5]/f[1],d))
                s3 = str(f[2])
                fpresets.append(p[2]+" ("+s1+"x"+s2+s3+")")
            self.vPresets.addItems(fpresets)
            self.setPreset(0)

    def setPreset(self,i):

        from draftutils import params
        self.Profile = None
        elt = self.pSelect[i]
        if elt:
            p=elt[0]-1 # Presets indexes are 1-based
            self.Profile = self.Presets[p]
            params.set_param_arch("StructurePreset",";".join([str(i) for i in self.Profile]))


FreeCADGui.addCommand('Arch_Profile',Arch_Profile())
