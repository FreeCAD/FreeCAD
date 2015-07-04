# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************


import FreeCAD,Path, xml.sax, os
from PySide import QtCore, QtGui
import DraftGui

# convenience functions


try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)


# Tooltable XML readers


class FreeCADTooltableHandler( xml.sax.ContentHandler ):
    # http://www.tutorialspoint.com/python/python_xml_processing.htm
    def __init__(self):
        self.tooltable = None
        self.tool = None
        self.number = None

    # Call when an element is found
    def startElement(self, tag, attributes):
        if tag == "Tooltable":
            self.tooltable = Path.Tooltable()
        elif tag == "Toolslot":                
            self.number = int(attributes["number"])
        elif tag == "Tool":
            self.tool = Path.Tool()
            self.tool.Name = str(attributes["name"])
            self.tool.ToolType = str(attributes["type"])
            self.tool.Material = str(attributes["mat"])
            # for some reason without the following line I get an error 
            print attributes["diameter"]
            self.tool.Diameter = float(attributes["diameter"])
            self.tool.LengthOffset = float(attributes["length"])
            self.tool.FlatRadius = float(attributes["flat"])
            self.tool.CornerRadius = float(attributes["corner"])
            self.tool.CuttingEdgeAngle = float(attributes["angle"])
            self.tool.CuttingEdgeHeight = float(attributes["height"])
            
    # Call when an elements ends
    def endElement(self, tag):
        if tag == "Toolslot":
            if self.tooltable and self.tool and self.number:
                self.tooltable.setTool(self.number,self.tool)
                self.number = None
                self.tool = None
                
                
class HeeksTooltableHandler( xml.sax.ContentHandler ):
    def __init__(self):
        self.tooltable = Path.Tooltable()
        self.tool = None
        self.number = None

    # Call when an element is found
    def startElement(self, tag, attributes):
        if tag == "Tool":     
            self.tool = Path.Tool()           
            self.number = int(attributes["tool_number"])
            self.tool.Name = str(attributes["title"])
        elif tag == "params":
            t = str(attributes["type"])
            if t == "drill":
                self.tool.ToolType = "Drill"
            elif t == "center_drill_bit":
                self.tool.ToolType = "CenterDrill"
            elif t == "end_mill":
                self.tool.ToolType = "EndMill"
            elif t == "slot_cutter":
                self.tool.ToolType = "SlotCutter"
            elif t == "ball_end_mill":
                self.tool.ToolType = "BallEndMill"
            elif t == "chamfer":
                self.tool.ToolType = "Chamfer"
            elif t == "engraving_bit":
                self.tool.ToolType = "Engraver"
            m = str(attributes["material"])
            if m == "0":
                self.tool.Material = "HighSpeedSteel"
            elif m == "1":
                self.tool.Material = "Carbide"
            # for some reason without the following line I get an error 
            print attributes["diameter"]
            self.tool.Diameter = float(attributes["diameter"])
            self.tool.LengthOffset = float(attributes["tool_length_offset"])
            self.tool.FlatRadius = float(attributes["flat_radius"])
            self.tool.CornerRadius = float(attributes["corner_radius"])
            self.tool.CuttingEdgeAngle = float(attributes["cutting_edge_angle"])
            self.tool.CuttingEdgeHeight = float(attributes["cutting_edge_height"])
            
    # Call when an elements ends
    def endElement(self, tag):
        if tag == "Tool":
            if self.tooltable and self.tool and self.number:
                self.tooltable.setTool(self.number,self.tool)
                self.number = None
                self.tool = None
            

# Tooltable Editor


class Editor(QtGui.QDialog):
    
    def __init__(self,obj):
        
        QtGui.QDialog.__init__(self)
        self.setObjectName(_fromUtf8("TooltableEditor"))
        self.resize(468, 476)
        self.verticalLayout = QtGui.QVBoxLayout(self)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.DECIMALS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt("Decimals",2)
        self.FORMAT = DraftGui.makeFormatSpec(self.DECIMALS,'Length')
        # left groupbox
        self.groupBox = QtGui.QGroupBox(self)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Minimum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.groupBox.sizePolicy().hasHeightForWidth())
        self.groupBox.setSizePolicy(sizePolicy)
        self.groupBox.setObjectName(_fromUtf8("groupBox"))
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.groupBox)
        self.verticalLayout_2.setObjectName(_fromUtf8("verticalLayout_2"))
        self.horizontalLayout_9 = QtGui.QHBoxLayout()
        self.horizontalLayout_9.setObjectName(_fromUtf8("horizontalLayout_9"))
        
        # import button
        self.ButtonImport = QtGui.QPushButton(self.groupBox)
        icon = QtGui.QIcon.fromTheme(_fromUtf8("document-import"))
        self.ButtonImport.setIcon(icon)
        self.ButtonImport.setObjectName(_fromUtf8("ButtonImport"))
        self.horizontalLayout_9.addWidget(self.ButtonImport)
        
        # export button
        self.ButtonExport = QtGui.QPushButton(self.groupBox)
        icon = QtGui.QIcon.fromTheme(_fromUtf8("document-export"))
        self.ButtonExport.setIcon(icon)
        self.ButtonExport.setObjectName(_fromUtf8("ButtonExport"))
        self.horizontalLayout_9.addWidget(self.ButtonExport)
        
        # tools list
        self.verticalLayout_2.addLayout(self.horizontalLayout_9)
        self.ToolsList = QtGui.QTreeWidget(self.groupBox)
        self.ToolsList.setObjectName(_fromUtf8("ToolsList"))
        self.ToolsList.header().setDefaultSectionSize(40)
        self.verticalLayout_2.addWidget(self.ToolsList)
        
        # add button
        self.horizontalLayout_8 = QtGui.QHBoxLayout()
        self.horizontalLayout_8.setObjectName(_fromUtf8("horizontalLayout_8"))
        self.ButtonAdd = QtGui.QPushButton(self.groupBox)
        icon = QtGui.QIcon.fromTheme(_fromUtf8("edit-add"))
        self.ButtonAdd.setIcon(icon)
        self.ButtonAdd.setObjectName(_fromUtf8("ButtonAdd"))
        self.horizontalLayout_8.addWidget(self.ButtonAdd)
        
        # delete button
        self.ButtonDelete = QtGui.QPushButton(self.groupBox)
        icon = QtGui.QIcon.fromTheme(_fromUtf8("edit-delete"))
        self.ButtonDelete.setIcon(icon)
        self.ButtonDelete.setObjectName(_fromUtf8("ButtonDelete"))
        self.horizontalLayout_8.addWidget(self.ButtonDelete)

        # up button
        self.ButtonUp = QtGui.QPushButton(self.groupBox)
        icon = QtGui.QIcon.fromTheme(_fromUtf8("go-up"))
        self.ButtonUp.setIcon(icon)
        self.ButtonDelete.setObjectName(_fromUtf8("ButtonUp"))
        self.horizontalLayout_8.addWidget(self.ButtonUp)
        
        # down button
        self.ButtonDown = QtGui.QPushButton(self.groupBox)
        icon = QtGui.QIcon.fromTheme(_fromUtf8("go-down"))
        self.ButtonDown.setIcon(icon)
        self.ButtonDown.setObjectName(_fromUtf8("ButtonDown"))
        self.horizontalLayout_8.addWidget(self.ButtonDown)

        # right groupbox
        self.verticalLayout_2.addLayout(self.horizontalLayout_8)
        self.horizontalLayout.addWidget(self.groupBox)
        self.groupBox_2 = QtGui.QGroupBox(self)
        self.groupBox_2.setObjectName(_fromUtf8("groupBox_2"))
        self.verticalLayout_3 = QtGui.QVBoxLayout(self.groupBox_2)
        self.verticalLayout_3.setObjectName(_fromUtf8("verticalLayout_3"))
        
        # name
        self.label = QtGui.QLabel(self.groupBox_2)
        self.label.setObjectName(_fromUtf8("label"))
        self.verticalLayout_3.addWidget(self.label)
        self.NameField = QtGui.QLineEdit(self.groupBox_2)
        self.NameField.setObjectName(_fromUtf8("NameField"))
        self.verticalLayout_3.addWidget(self.NameField)
        
        # type
        self.label_2 = QtGui.QLabel(self.groupBox_2)
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.verticalLayout_3.addWidget(self.label_2)
        self.TypeField = QtGui.QComboBox(self.groupBox_2)
        self.TypeField.setObjectName(_fromUtf8("TypeField"))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.TypeField.addItem(_fromUtf8(""))
        self.verticalLayout_3.addWidget(self.TypeField)
        
        # material
        self.label_3 = QtGui.QLabel(self.groupBox_2)
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.verticalLayout_3.addWidget(self.label_3)
        self.MaterialField = QtGui.QComboBox(self.groupBox_2)
        self.MaterialField.setObjectName(_fromUtf8("MaterialField"))
        self.MaterialField.addItem(_fromUtf8(""))
        self.MaterialField.addItem(_fromUtf8(""))
        self.MaterialField.addItem(_fromUtf8(""))
        self.MaterialField.addItem(_fromUtf8(""))
        self.MaterialField.addItem(_fromUtf8(""))
        self.MaterialField.addItem(_fromUtf8(""))
        self.MaterialField.addItem(_fromUtf8(""))
        self.MaterialField.addItem(_fromUtf8(""))
        self.verticalLayout_3.addWidget(self.MaterialField)
        self.label_4 = QtGui.QLabel(self.groupBox_2)
        self.label_4.setObjectName(_fromUtf8("label_4"))
        self.verticalLayout_3.addWidget(self.label_4)
        
        # diameter
        self.horizontalLayout_2 = QtGui.QHBoxLayout()
        self.horizontalLayout_2.setObjectName(_fromUtf8("horizontalLayout_2"))
        self.label_5 = QtGui.QLabel(self.groupBox_2)
        self.label_5.setObjectName(_fromUtf8("label_5"))
        self.horizontalLayout_2.addWidget(self.label_5)
        self.DiameterField = QtGui.QDoubleSpinBox(self.groupBox_2)
        self.DiameterField.setMaximum(9999)
        self.DiameterField.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.DiameterField.setObjectName(_fromUtf8("DiameterField"))
        self.horizontalLayout_2.addWidget(self.DiameterField)
        self.verticalLayout_3.addLayout(self.horizontalLayout_2)
        
        # length offset
        self.horizontalLayout_3 = QtGui.QHBoxLayout()
        self.horizontalLayout_3.setObjectName(_fromUtf8("horizontalLayout_3"))
        self.label_6 = QtGui.QLabel(self.groupBox_2)
        self.label_6.setObjectName(_fromUtf8("label_6"))
        self.horizontalLayout_3.addWidget(self.label_6)
        self.LengthOffsetField = QtGui.QDoubleSpinBox(self.groupBox_2)
        self.LengthOffsetField.setMaximum(9999)
        self.LengthOffsetField.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.LengthOffsetField.setObjectName(_fromUtf8("LengthOffsetField"))
        self.horizontalLayout_3.addWidget(self.LengthOffsetField)
        self.verticalLayout_3.addLayout(self.horizontalLayout_3)
        
        # flat radius
        self.horizontalLayout_4 = QtGui.QHBoxLayout()
        self.horizontalLayout_4.setObjectName(_fromUtf8("horizontalLayout_4"))
        self.label_7 = QtGui.QLabel(self.groupBox_2)
        self.label_7.setObjectName(_fromUtf8("label_7"))
        self.horizontalLayout_4.addWidget(self.label_7)
        self.FlatRadiusField = QtGui.QDoubleSpinBox(self.groupBox_2)
        self.FlatRadiusField.setMaximum(9999)
        self.FlatRadiusField.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.FlatRadiusField.setObjectName(_fromUtf8("FlatRadiusField"))
        self.horizontalLayout_4.addWidget(self.FlatRadiusField)
        self.verticalLayout_3.addLayout(self.horizontalLayout_4)
        
        # corner radius
        self.horizontalLayout_5 = QtGui.QHBoxLayout()
        self.horizontalLayout_5.setObjectName(_fromUtf8("horizontalLayout_5"))
        self.label_8 = QtGui.QLabel(self.groupBox_2)
        self.label_8.setObjectName(_fromUtf8("label_8"))
        self.horizontalLayout_5.addWidget(self.label_8)
        self.CornerRadiusField = QtGui.QDoubleSpinBox(self.groupBox_2)
        self.CornerRadiusField.setMaximum(9999)
        self.CornerRadiusField.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.CornerRadiusField.setObjectName(_fromUtf8("CornerRadiusField"))
        self.horizontalLayout_5.addWidget(self.CornerRadiusField)
        self.verticalLayout_3.addLayout(self.horizontalLayout_5)
        
        # cutting edge angle
        self.horizontalLayout_6 = QtGui.QHBoxLayout()
        self.horizontalLayout_6.setObjectName(_fromUtf8("horizontalLayout_6"))
        self.label_9 = QtGui.QLabel(self.groupBox_2)
        self.label_9.setObjectName(_fromUtf8("label_9"))
        self.horizontalLayout_6.addWidget(self.label_9)
        self.CuttingEdgeAngleField = QtGui.QDoubleSpinBox(self.groupBox_2)
        self.CuttingEdgeAngleField.setMaximum(360)
        self.CuttingEdgeAngleField.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.CuttingEdgeAngleField.setObjectName(_fromUtf8("CuttingEdgeAngleField"))
        self.horizontalLayout_6.addWidget(self.CuttingEdgeAngleField)
        self.verticalLayout_3.addLayout(self.horizontalLayout_6)
        
        # cutting edge height
        self.horizontalLayout_7 = QtGui.QHBoxLayout()
        self.horizontalLayout_7.setObjectName(_fromUtf8("horizontalLayout_7"))
        self.label_10 = QtGui.QLabel(self.groupBox_2)
        self.label_10.setObjectName(_fromUtf8("label_10"))
        self.horizontalLayout_7.addWidget(self.label_10)
        self.CuttingEdgeHeightField = QtGui.QDoubleSpinBox(self.groupBox_2)
        self.CuttingEdgeHeightField.setMaximum(9999)
        self.CuttingEdgeHeightField.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.CuttingEdgeHeightField.setObjectName(_fromUtf8("CuttingEdgeHeightField"))
        self.horizontalLayout_7.addWidget(self.CuttingEdgeHeightField)
        self.verticalLayout_3.addLayout(self.horizontalLayout_7)
        self.horizontalLayout.addWidget(self.groupBox_2)
        self.verticalLayout.addLayout(self.horizontalLayout)
        
        # ok / cancel box
        self.buttonBox = QtGui.QDialogButtonBox(self)
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.verticalLayout.addWidget(self.buttonBox)

        self.retranslateUi()
        
        # connect buttons
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), self.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), self.reject)
        QtCore.QObject.connect(self.ButtonImport, QtCore.SIGNAL(_fromUtf8("clicked()")), self.read)
        QtCore.QObject.connect(self.ButtonExport, QtCore.SIGNAL(_fromUtf8("clicked()")), self.write)
        QtCore.QObject.connect(self.ButtonAdd, QtCore.SIGNAL(_fromUtf8("clicked()")), self.addnew)
        QtCore.QObject.connect(self.ButtonDelete, QtCore.SIGNAL(_fromUtf8("clicked()")), self.delete)
        QtCore.QObject.connect(self.ButtonUp, QtCore.SIGNAL(_fromUtf8("clicked()")), self.moveup)
        QtCore.QObject.connect(self.ButtonDown, QtCore.SIGNAL(_fromUtf8("clicked()")), self.movedown)
        QtCore.QObject.connect(self.ToolsList, QtCore.SIGNAL(_fromUtf8("currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)")), self.selectTool)
        QtCore.QObject.connect(self.NameField, QtCore.SIGNAL(_fromUtf8("textEdited(QString)")), self.changeName)
        QtCore.QObject.connect(self.TypeField, QtCore.SIGNAL(_fromUtf8("currentIndexChanged(int)")), self.changeType)
        QtCore.QObject.connect(self.MaterialField, QtCore.SIGNAL(_fromUtf8("currentIndexChanged(int)")), self.changeMaterial)
        QtCore.QObject.connect(self.DiameterField, QtCore.SIGNAL(_fromUtf8("valueChanged(double)")), self.changeDiameter)
        QtCore.QObject.connect(self.LengthOffsetField, QtCore.SIGNAL(_fromUtf8("valueChanged(double)")), self.changeLengthOffset)
        QtCore.QObject.connect(self.FlatRadiusField, QtCore.SIGNAL(_fromUtf8("valueChanged(double)")), self.changeFlatRadius)
        QtCore.QObject.connect(self.CornerRadiusField, QtCore.SIGNAL(_fromUtf8("valueChanged(double)")), self.changeCornerRadius)
        QtCore.QObject.connect(self.CuttingEdgeAngleField, QtCore.SIGNAL(_fromUtf8("valueChanged(double)")), self.changeCuttingEdgeAngle)
        QtCore.QObject.connect(self.CuttingEdgeHeightField, QtCore.SIGNAL(_fromUtf8("valueChanged(double)")), self.changeCuttingEdgeHeight)
        QtCore.QMetaObject.connectSlotsByName(self)
        self.tooltable = obj.Tooltable.copy()
        self.tool = None
        self.number = None
        self.reset()

    def retranslateUi(self):
        self.setWindowTitle(_translate("TooltableEditor", "Tooltable editor", None))
        self.groupBox.setTitle(_translate("TooltableEditor", "Tools list", None))
        self.ButtonImport.setText(_translate("TooltableEditor", "Import...", None))
        self.ButtonExport.setText(_translate("TooltableEditor", "Export...", None))
        self.ToolsList.headerItem().setText(0, _translate("TooltableEditor", "Slot", None))
        self.ToolsList.headerItem().setText(1, _translate("TooltableEditor", "Tool", None))
        self.ButtonAdd.setText(_translate("TooltableEditor", "Add new", None))
        self.ButtonDelete.setText(_translate("TooltableEditor", "Delete", None))
        self.ButtonUp.setText(_translate("TooltableEditor", "Move up", None))
        self.ButtonDown.setText(_translate("TooltableEditor", "Move down", None))
        self.groupBox_2.setTitle(_translate("TooltableEditor", "Tool properties", None))
        self.label.setText(_translate("TooltableEditor", "Name", None))
        self.label_2.setText(_translate("TooltableEditor", "Type", None))
        self.TypeField.setItemText(0, _translate("TooltableEditor", "Undefined", None))
        self.TypeField.setItemText(1, _translate("TooltableEditor", "Drill", None))
        self.TypeField.setItemText(2, _translate("TooltableEditor", "Center Drill", None))
        self.TypeField.setItemText(3, _translate("TooltableEditor", "Counter Sink", None))
        self.TypeField.setItemText(4, _translate("TooltableEditor", "Counter Bore", None))
        self.TypeField.setItemText(5, _translate("TooltableEditor", "Reamer", None))
        self.TypeField.setItemText(6, _translate("TooltableEditor", "Tap", None))
        self.TypeField.setItemText(7, _translate("TooltableEditor", "End Mill", None))
        self.TypeField.setItemText(8, _translate("TooltableEditor", "Slot Cutter", None))
        self.TypeField.setItemText(9, _translate("TooltableEditor", "Ball End Mill", None))
        self.TypeField.setItemText(10, _translate("TooltableEditor", "Chamfer Mill", None))
        self.TypeField.setItemText(11, _translate("TooltableEditor", "Corner Round", None))
        self.TypeField.setItemText(12, _translate("TooltableEditor", "Engraver", None))
        self.label_3.setText(_translate("TooltableEditor", "Material", None))
        self.MaterialField.setItemText(0, _translate("TooltableEditor", "Undefined", None))
        self.MaterialField.setItemText(1, _translate("TooltableEditor", "High Speed Steel", None))
        self.MaterialField.setItemText(2, _translate("TooltableEditor", "High Carbon Tool Steel", None))
        self.MaterialField.setItemText(3, _translate("TooltableEditor", "Cast Alloy", None))
        self.MaterialField.setItemText(4, _translate("TooltableEditor", "Carbide", None))
        self.MaterialField.setItemText(5, _translate("TooltableEditor", "Ceramics", None))
        self.MaterialField.setItemText(6, _translate("TooltableEditor", "Diamond", None))
        self.MaterialField.setItemText(7, _translate("TooltableEditor", "Sialon", None))
        self.label_4.setText(_translate("TooltableEditor", "Properties", None))
        self.label_5.setText(_translate("TooltableEditor", "Diameter", None))
#        self.DiameterField.setSuffix(_translate("TooltableEditor", "mm", None))
        self.label_6.setText(_translate("TooltableEditor", "Length offset", None))
        self.LengthOffsetField.setSuffix(_translate("TooltableEditor", "mm", None))
        self.label_7.setText(_translate("TooltableEditor", "Flat radius", None))
        self.FlatRadiusField.setSuffix(_translate("TooltableEditor", "mm", None))
        self.label_8.setText(_translate("TooltableEditor", "Corner radius", None))
        self.CornerRadiusField.setSuffix(_translate("TooltableEditor", "mm", None))
        self.label_9.setText(_translate("TooltableEditor", "Cutting edge angle", None))
        self.CuttingEdgeAngleField.setSuffix(_translate("TooltableEditor", "°", None))
        self.label_10.setText(_translate("TooltableEditor", "Cutting edge height", None))
        self.CuttingEdgeHeightField.setSuffix(_translate("TooltableEditor", "mm", None))
    
    def reset(self):
        "resets the editor with the contents of the current internal tooltable"
        self.tool = None
        self.number = None
        self.ToolsList.clear()
        for number,tool in self.tooltable.Tools.iteritems():
            item = QtGui.QTreeWidgetItem(self.ToolsList)
            item.setText(0,str(number))
            item.setText(1,tool.Name)
        self.NameField.setText("")
        self.TypeField.setCurrentIndex(-1)
        self.MaterialField.setCurrentIndex(-1)
        self.DiameterField.setValue(0)
        self.LengthOffsetField.setValue(0)
        self.FlatRadiusField.setValue(0)
        self.CornerRadiusField.setValue(0)
        self.CuttingEdgeAngleField.setValue(0)
        self.CuttingEdgeHeightField.setValue(0)
            
    def selectTool(self,current,previous):
        "fills the data of the currently selected tool"
        if current:
            number = int(current.text(0))
            tool = self.tooltable.getTool(number)
            if tool:
                self.number = number
                self.tool = tool
                self.NameField.setText(tool.Name)
                self.TypeField.setCurrentIndex(self.getType(tool.ToolType))
                self.MaterialField.setCurrentIndex(self.getMaterial(tool.Material))
                self.DiameterField.setValue(tool.Diameter)
                self.LengthOffsetField.setValue(tool.LengthOffset)
                self.FlatRadiusField.setValue(tool.FlatRadius)
                self.CornerRadiusField.setValue(tool.CornerRadius)
                self.CuttingEdgeAngleField.setValue(tool.CuttingEdgeAngle)
                self.CuttingEdgeHeightField.setValue(tool.CuttingEdgeHeight)
            
    def getType(self,tooltype):
        "gets a combobox index number for a given type or viceversa"
        toolslist = ["Drill","CenterDrill","CounterSink","CounterBore",
                     "Reamer","Tap","EndMill","SlotCutter","BallEndMill",
                     "ChamferMill","CornerRound","Engraver"]
        if isinstance(tooltype,str):
            if tooltype in toolslist:
                return toolslist.index(tooltype)+1
            else:
                return 0
        else:
            if tooltype == 0:
                return "Undefined"
            else:
                return toolslist[tooltype-1]
                
    def getMaterial(self,material):
        "gets a combobox index number for a given material or viceversa"
        matslist = ["HighSpeedSteel","HighCarbonToolSteel","CastAlloy",
                    "Carbide","Ceramics","Diamond","Sialon"]
        if isinstance(material,str):
            if material in matslist:
                return matslist.index(material)+1
            else:
                return 0
        else:
            if material == 0:
                return "Undefined"
            else:
                return matslist[material-1]
                
    def changeName(self,text):
        "called when the corresponding field has changed (needed for nasty pyside bug)"
        if self.tool:
            self.tool.Name = str(text)
            self.changeTool()
            if self.number:
                l = self.ToolsList.findItems(str(self.number),QtCore.Qt.MatchExactly,0)
                if len(l) == 1:
                    l[0].setText(1,text)
            
    def changeType(self,num):
        "called when the corresponding field has changed (needed for nasty pyside bug)"
        if self.tool:
            self.tool.ToolType = self.getType(num)
            self.changeTool()

    def changeMaterial(self,num):
        "called when the corresponding field has changed (needed for nasty pyside bug)"
        if self.tool:
            self.tool.Material = self.getMaterial(num)
            self.changeTool()
            
    def changeDiameter(self,value):
        "called when the corresponding field has changed (needed for nasty pyside bug)"
        if self.tool:
            self.tool.Diameter = value
            self.changeTool()
            
    def changeLengthOffset(self,value):
        "called when the corresponding field has changed (needed for nasty pyside bug)"
        if self.tool:
            self.tool.LengthOffset = value
            self.changeTool()
            
    def changeFlatRadius(self,value):
        "called when the corresponding field has changed (needed for nasty pyside bug)"
        if self.tool:
            self.tool.FlatRadius = value
            self.changeTool()
            
    def changeCornerRadius(self,value):
        "called when the corresponding field has changed (needed for nasty pyside bug)"
        if self.tool:
            self.tool.CornerRadius = value
            self.changeTool()
            
    def changeCuttingEdgeAngle(self,value):
        "called when the corresponding field has changed (needed for nasty pyside bug)"
        if self.tool:
            self.tool.CuttingEdgeAngle = value
            self.changeTool()
            
    def changeCuttingEdgeHeight(self,value):
        "called when the corresponding field has changed (needed for nasty pyside bug)"
        if self.tool:
            self.tool.CuttingEdgeHeight = value
            self.changeTool()
                
    def changeTool(self):
        "changes a given tool"
        if self.number and self.tool:
            self.tooltable.setTool(self.number,self.tool)
            
    def delete(self):
        "deletes the current tool"
        if self.number:
            self.tooltable.deleteTool(self.number)
            self.reset()

    def addnew(self):
        "adds a new tool at the end of the table"
        tool = Path.Tool()
        print self.NameField
        if self.NameField.text():
            tool.Name = str(self.NameField.text())
        tool.ToolType = self.getType(self.TypeField.currentIndex())
        tool.Material = self.getMaterial(self.MaterialField.currentIndex())
        tool.Diameter = self.DiameterField.value()
        tool.LengthOffset = self.LengthOffsetField.value()
        tool.FlatRadius = self.FlatRadiusField.value()
        tool.CornerRadius = self.CornerRadiusField.value()
        tool.CuttingEdgeAngle = self.CuttingEdgeAngleField.value()
        tool.CuttingEdgeHeight = self.CuttingEdgeHeightField.value()
        self.tooltable.addTools(tool)
        self.reset()
        
    def read(self):
        "imports a tooltable from a file"
        filename = QtGui.QFileDialog.getOpenFileName(self, _translate("TooltableEditor","Open tooltable",None),None, _translate("TooltableEditor","Tooltable XML (*.xml);;HeeksCAD tooltable (*.tooltable)",None))
        if filename:
            parser = xml.sax.make_parser()
            parser.setFeature(xml.sax.handler.feature_namespaces, 0)
            if os.path.splitext(filename[0])[1].lower() == ".tooltable":
                Handler = HeeksTooltableHandler()
            else:
                Handler = FreeCADTooltableHandler()
            parser.setContentHandler( Handler )
            parser.parse(str(filename[0]))
            if Handler.tooltable:
                self.tooltable = Handler.tooltable
                self.reset()
        
    def write(self):
        "exports the tooltable to a file"
        if self.tooltable:
            filename = QtGui.QFileDialog.getSaveFileName(self, _translate("TooltableEditor","Save tooltable",None),None, _translate("TooltableEditor","Tooltable XML (*.xml)",None))
            if filename:
                fil = open(str(filename[0]),"wb")
                fil.write('<?xml version="1.0" encoding="UTF-8"?>\n')
                fil.write(self.tooltable.Content)
                fil.close()
                print "Written ",filename[0]
                
    def moveup(self):
        "moves a tool to a lower number, if possible"
        if self.number:
            if self.number < 2:
                return
            target = self.number - 1
            t1 = self.tooltable.getTool(self.number).copy()
            self.tooltable.deleteTool(self.number)
            if target in self.tooltable.Tools.keys():
                t2 = self.tooltable.getTool(target).copy()
                self.tooltable.deleteTool(target)
                self.tooltable.setTool(self.number,t2)
            self.tooltable.setTool(target,t1)
            self.reset()
                            
    def movedown(self):
        "moves a tool to a higher number, if possible"
        if self.number:
            target = self.number + 1
            t1 = self.tooltable.getTool(self.number).copy()
            self.tooltable.deleteTool(self.number)
            if target in self.tooltable.Tools.keys():
                t2 = self.tooltable.getTool(target).copy()
                self.tooltable.deleteTool(target)
                self.tooltable.setTool(self.number,t2)
            self.tooltable.setTool(target,t1)
            self.reset()

def edit(objectname):
    """edit(objectname): this is the main function of this module.
    opens an editor dialog to edit the Tooltable of the given object"""
    obj = FreeCAD.ActiveDocument.getObject(objectname)
    if not obj:
        raise Exception(_translate("TooltableEditor","Object not found",None))
    if not hasattr(obj,"Tooltable"):
        raise Exception(_translate("TooltableEditor","Object doesn't have a tooltable property",None))
    dialog = Editor(obj)
    r = dialog.exec_()
    if r:
        tooltable = dialog.tooltable
        FreeCAD.ActiveDocument.openTransaction("Edit Tooltable")
        obj.Tooltable = tooltable
        FreeCAD.ActiveDocument.commitTransaction()

    obj.ViewObject.finishEditing()

