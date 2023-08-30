# ***************************************************************************
# *   Copyright (c) 2023 edi <edi271@a1.net>                                *
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
"""Provides the TechDraw HoleShaftFit Task Dialog."""

__title__ = "TechDrawTools.TaskHoleShaftFit"
__author__ = "edi"
__url__ = "https://www.freecad.org"
__version__ = "00.01"
__date__ = "2023/02/07"

import FreeCAD as App
import FreeCADGui as Gui

from functools import partial

import os

translate = App.Qt.translate

class TaskHoleShaftFit:
    def __init__(self,sel):

        loose = translate("TechDraw_HoleShaftFit", "loose fit")
        snug = translate("TechDraw_HoleShaftFit", "snug fit")
        press = translate("TechDraw_HoleShaftFit", "press fit")
        self.isHole = True
        self.sel = sel
        self.holeValues = [["h9","D10",loose],["h9","E9",loose],["h9","F8",loose],["h6","G7",loose],
                          ["c11","H11",loose],["f7","H8",loose],["h6","H7",loose],["h7","H8",loose],
                          ["k6","H7",snug],["n6","H7",snug],["r6","H7",press],["s6","H7",press],
                          ["h6","K7",snug],["h6","N7",snug],["h6","R7",press],["h6","S7",press]]
        self.shaftValues = [["H11","c11",loose],["H8","f7",loose],["H7","h6",loose],["H8","h7",loose],
                           ["D10","h9",loose],["E9","h9",loose],["F8","h9",loose],["G7","h6",loose],
                           ["K7","h6",snug],["N7","h6",snug],["R7","h6",press],["S7","h6",press],
                           ["H7","k6",snug],["H7","n6",snug],["H7","r6",press],["H7","s6",press]]

        self._uiPath = App.getHomePath()
        self._uiPath = os.path.join(self._uiPath, "Mod/TechDraw/TechDrawTools/Gui/TaskHoleShaftFit.ui")
        self.form = Gui.PySideUic.loadUi(self._uiPath)

        self.form.setWindowTitle(translate("TechDraw_HoleShaftFit", "Hole / Shaft Fit ISO 286"))

        self.form.rbHoleBase.clicked.connect(partial(self.on_HoleShaftChanged,True))
        self.form.rbShaftBase.clicked.connect(partial(self.on_HoleShaftChanged,False))
        self.form.cbField.currentIndexChanged.connect(self.on_FieldChanged)

    def setHoleFields(self):
        '''set hole fields in the combo box'''
        for i in range(self.form.cbField.count()):
            self.form.cbField.removeItem(0)
        for value in self.holeValues:
            self.form.cbField.addItem(value[1])
        self.form.lbBaseField.setText('             '+self.holeValues[0][0]+" /")
        self.form.lbFitType.setText(self.holeValues[0][2])

    def setShaftFields(self):
        '''set shaft fields in the combo box'''
        for i in range(self.form.cbField.count()):
            self.form.cbField.removeItem(0)
        for value in self.shaftValues:
            self.form.cbField.addItem(value[1])
        self.form.lbBaseField.setText('             '+self.shaftValues[0][0]+" /")
        self.form.lbFitType.setText(self.shaftValues[0][2])

    def on_HoleShaftChanged(self,isHole):
        '''slot: change the used base fit hole/shaft'''
        if isHole:
            self.isHole = isHole
            self.setShaftFields()
        else:
            self.isHole = isHole
            self.setHoleFields()

    def on_FieldChanged(self):
        '''slot: change of the desired field'''
        currentIndex = self.form.cbField.currentIndex()
        if self.isHole:
            self.form.lbBaseField.setText('             '+self.shaftValues[currentIndex][0]+" /")
            self.form.lbFitType.setText(self.shaftValues[currentIndex][2])
        else:
            self.form.lbBaseField.setText('             '+self.holeValues[currentIndex][0]+" /")
            self.form.lbFitType.setText(self.holeValues[currentIndex][2])

    def accept(self):
        '''slot: OK pressed'''
        currentIndex = self.form.cbField.currentIndex()
        if self.isHole:
            selectedField = self.shaftValues[currentIndex][1]
        else:
            selectedField = self.holeValues[currentIndex][1]
        fieldChar = selectedField[0]
        quality = int(selectedField[1:])
        dim = self.sel[0].Object
        value = dim.getRawValue()
        iso = ISO286()
        iso.calculate(value,fieldChar,quality)
        rangeValues = iso.getValues()
        mainFormat = dim.FormatSpec
        dim.FormatSpec = mainFormat+selectedField
        dim.EqualTolerance = False
        dim.FormatSpecOverTolerance = '(%+.3f)'
        dim.OverTolerance = rangeValues[0]
        dim.UnderTolerance = rangeValues[1]
        Gui.Control.closeDialog()

    def reject(self):
        return True

class ISO286:
    '''This class represents a subset of the ISO 286 standard'''

    def getNominalRange(self,measureValue):
        '''return index of selected nominal range field, 0 < measureValue < 500 mm'''
        measureRanges = [0,3,6,10,14,18,24,30,40,50,65,80,100,120,140,160,180,200,225,250,280,315,355,400,450,500]
        index = 1
        while measureValue > measureRanges[index]:
            index = index+1
        return index-1

    def getITValue(self,valueQuality,valueNominalRange):
        '''return IT-value  (value of quality in micrometers)'''
        '''tables IT6 to IT11 from 0 to 500 mm'''
        IT6 = [6,8,9,11,11,13,13,16,16,19,19,22,22,25,25,25,29,29,29,32,32,36,36,40,40]
        IT7 = [10,12,15,18,18,21,21,25,25,30,30,35,35,40,40,40,46,46,46,52,52,57,57,63,63]
        IT8 = [14,18,22,27,27,33,33,39,39,46,46,54,54,63,63,63,72,72,72,81,81,89,89,97,97]
        IT9 = [25,30,36,43,43,52,52,62,62,74,74,87,87,100,100,100,115,115,115,130,130,140,140,155,155]
        IT10 = [40,48,58,70,70,84,84,100,100,120,120,140,140,160,160,160,185,185,185,210,210,230,230,250,250]
        IT11 = [60,75,90,110,110,130,130,160,160,190,190,220,220,250,250,250,290,290,290,320,320,360,360,400,400]
        qualityTable = [IT6,IT7,IT8,IT9,IT10,IT11]
        return qualityTable[valueQuality-6][valueNominalRange]

    def getFieldValue(self,fieldCharacter,valueNominalRange):
        '''return es or ES value of the field in micrometers'''
        cField = [-60,-70,-80,-95,-95,-110,-110,-120,-130,-140,-150,-170,-180,-200,-210,-230,-240,-260,-280,-300,-330,-360,-400,-440,-480]
        fField = [-6,-10,-13,-16,-16,-20,-20,-25,-25,-30,-30,-36,-36,-43,-43,-43,-50,-50,-50,-56,-56,-62,-62,-68,-68]
        gField = [-2,-4,-5,-6,-6,-7,-7,-9,-9,-10,-10,-12,-12,-14,-14,-14,-15,-15,-15,-17,-17,-18,-18,-20,-20]
        hField = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
        kField = [6,9,10,12,12,15,15,18,18,21,21,25,25,28,28,28,33,33,33,36,36,40,40,45,45]
        nField = [10,16,19,23,23,28,28,33,33,39,39,45,45,52,52,60,60,66,66,73,73,80,80]
        rField = [16,23,28,34,34,41,41,50,50,60,62,73,76,88,90,93,106,109,113,126,130,144,150,166,172]
        sField = [20,27,32,39,39,48,48,59,59,72,78,93,101,117,125,133,151,159,169,190,202,226,244,272,292]
        DField = [60,78,98,120,120,149,149,180,180,220,220,260,260,305,305,305,355,355,355,400,400,440,440,480,480]
        EField = [39,50,61,75,75,92,92,112,112,134,134,159,159,185,185,185,215,215,215,240,240,265,265,290,290]
        FField = [20,28,35,43,43,53,53,64,64,76,76,90,90,106,106,106,122,122,122,137,137,151,151,165,165]
        GField = [12,16,20,24,24,28,28,34,34,40,40,47,47,54,54,54,61,61,61,69,69,75,75,83,83]
        HField = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
        KField = [0,3,5,6,6,6,6,7,7,9,9,10,10,12,12,12,13,13,13,16,16,17,17,18,18]
        NField = [-4,-4,-4,-5,-5,-7,-7,-8,-8,-9,-9,-10,-10,-12,-12,-12,-14,-14,-14,-14,-14,-16,-16,-17,-17]
        RField = [-10,-11,-13,-16,-16,-20,-20,-25,-25,-30,-32,-38,-41,-48,-50,-53,-60,-63,-67,-74,-78,-87,-93,-103,-109]
        SField = [-14,-15,-17,-21,-21,-27,-27,-34,-34,-42,-48,-58,-66,-77,-85,-93,-105,-113,-123,-138,-150,-169,-187,-209,-229]
        fieldDict = {'c':cField,'f':fField,'g':gField,'h':hField,'k':kField,'n':nField,'r':rField,'s':sField,
                     'D':DField,'E':EField,'F':FField,'G':GField,'H':HField,'K':KField,'N':NField,'R':RField,'S':SField}
        return fieldDict[fieldCharacter][valueNominalRange]

    def calculate(self,value,fieldChar,quality):
        '''calculate upper and lower field values'''
        self.nominalRange = self. getNominalRange(value)
        self.upperValue = self.getFieldValue(fieldChar,self.nominalRange)
        self.lowerValue = self.upperValue-self.getITValue(quality,self.nominalRange)
        if fieldChar == 'H':
            self.upperValue = -self.lowerValue
            self.lowerValue = 0
        # hack to print zero tolerance value as (+0.000)
        if self.upperValue == 0:
            self.upperValue = 0.1
        if self.lowerValue == 0:
            self.lowerValue = 0.1

    def getValues(self):
        '''return range values in mm'''
        return (self.upperValue/1000,self.lowerValue/1000)