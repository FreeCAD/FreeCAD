# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""BIM Panel-related Arch_"""

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

        FreeCAD.activeDraftCommand = self  # register as a Draft command for auto grid on/off
        self.doc = FreeCAD.ActiveDocument
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
        self.vCategory.addItems(self.Categories)
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

        # restore preset
        categoryIdx = -1
        presetIdx = -1
        stored = params.get_param_arch("ProfilePreset")
        if stored and ";" in stored and len(stored.split(";")) >= 3:
            stored = stored.split(";")
            if stored[1] in self.Categories:
                categoryIdx = self.Categories.index(stored[1])
                self.setCategory(categoryIdx)
                self.vCategory.setCurrentIndex(categoryIdx)
                ps = [p[2] for p in self.pSelect]
                if stored[2] in ps:
                    presetIdx = ps.index(stored[2])
                    self.setPreset(presetIdx)
                    self.vPresets.setCurrentIndex(presetIdx)
        if categoryIdx == -1:
            self.setCategory(0)
            self.vCategory.setCurrentIndex(0)
        if presetIdx == -1:
            self.setPreset(0)
            self.vPresets.setCurrentIndex(0)

        # connect slots
        self.vCategory.currentIndexChanged.connect(self.setCategory)
        self.vPresets.currentIndexChanged.connect(self.setPreset)

        return w

    def getPoint(self,point=None,obj=None):

        "this function is called by the snapper when it has a 3D point"

        FreeCAD.activeDraftCommand = None
        FreeCADGui.Snapper.off()
        if not point:
            return
        if not self.Profile:
            return
        pt = "FreeCAD.Vector("+str(point.x)+","+str(point.y)+","+str(point.z)+")"
        self.doc.openTransaction(translate("Arch","Create Profile"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand('p = Arch.makeProfile('+str(self.Profile)+')')
        FreeCADGui.addModule('WorkingPlane')
        FreeCADGui.doCommand('p.Placement = WorkingPlane.get_working_plane().get_placement()')
        FreeCADGui.doCommand('p.Placement.Base = ' + pt)
        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("Draft.autogroup(p)")
        self.doc.commitTransaction()
        self.doc.recompute()

    def setCategory(self,i):

        from draftutils import params
        self.vPresets.clear()
        self.pSelect = [p for p in self.Presets if p[1] == self.Categories[i]]
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
        self.Profile = self.pSelect[i]
        params.set_param_arch("ProfilePreset",";".join([str(i) for i in self.Profile]))


FreeCADGui.addCommand('Arch_Profile',Arch_Profile())
