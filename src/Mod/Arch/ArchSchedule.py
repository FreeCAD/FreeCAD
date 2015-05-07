# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - Yorik van Havre <yorik@uncreated.net>            *
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

import FreeCAD, time
if FreeCAD.GuiUp:
    import FreeCADGui, Arch_rc, os
    from PySide import QtCore, QtGui
    from DraftTools import translate

__title__ = "Arch Schedule Managment"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


class _CommandArchSchedule:
    "the Arch Schedule command definition"
    def GetResources(self):
        return {'Pixmap': 'Arch_Schedule',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Schedule","Create schedule..."),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Schedule","Creates a materials or areas schedule.")}

    def Activated(self):
        taskd = _ArchScheduleTaskPanel()
        FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False


class _ArchScheduleTaskPanel:
    '''The editmode TaskPanel for MechanicalMaterial objects'''
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/ArchSchedule.ui")
        QtCore.QObject.connect(self.form.ComboType, QtCore.SIGNAL("currentIndexChanged(int)"), self.changeType)

    def changeType(self,idx):
        "changes the type of schedule"
        if idx == 0:
            # quantities
            self.form.GroupQuantities.maximumHeight = 1677215
        elif idx == 1:
            # spaces
            self.form.GroupQuantities.maximumHeight = 17
        elif idx == 2:
            # windows
            self.form.GroupQuantities.maximumHeight = 17

    def accept(self):
        sp = FreeCAD.ActiveDocument.addObject('Spreadsheet::Sheet','Schedule')
        if self.form.ComboType.currentIndex() == 0:
            self.fillMaterialsSchedule(sp)
        elif self.form.ComboType.currentIndex() == 1:
            self.fillSpacesSchedule(sp)
        FreeCADGui.Control.closeDialog()
        
    def getRoles(self,roles):
        "gets all objects of the given roles in the document"
        objs = []
        for obj in FreeCAD.ActiveDocument.Objects:
            if obj.ViewObject.isVisible():
                if hasattr(obj,"Role"):
                    for r in roles:
                        if obj.Role == r:
                            objs.append(obj)
                            break
        return objs
        
    def getMaterials(self,objs):
        "classify the given objects by material"
        res = {}
        for obj in objs:
            if hasattr(obj,"BaseMaterial"):
                if obj.BaseMaterial:
                    res.setdefault(obj.BaseMaterial.Name,[]).append(obj)
        return res
        
    def getEntry(self,mat,entry):
        "returns the given entry from a given material name"
        res = ""
        m = FreeCAD.ActiveDocument.getObject(mat)
        if m:
            if m.Material:
                if entry in m.Material.keys():
                    res = m.Material[entry].encode('utf8')
        return res
                
    def fillMaterialsSchedule(self,sp):
        "creates a materials schedule in the given spreadsheet"
        
        # table title
        
        col = self.form.FieldStartCell.text()[0]
        row = int(self.form.FieldStartCell.text()[1:])
        global volunit,curunit
        volunit = self.form.FieldVolumeUnit.text().encode("utf8")
        curunit = self.form.FieldCurrencyUnit.text().encode("utf8")
        
        sec = 1
        sp.set(col+str(row), translate("Arch","Quantities schedule"))
        row += 1
        sp.set(col+str(row), translate("Arch","Project") + ": " + FreeCAD.ActiveDocument.Label)
        row += 1
        sp.set(col+str(row), translate("Arch","Date") + ": " + time.asctime())
        row += 2
        
        # column headers
        
        end = 1
        sp.set(chr(ord(col)+end)+str(row), translate("Arch","Material"))
        end += 1
        if self.form.CheckDescription.isChecked():
            sp.set(chr(ord(col)+end)+str(row),translate("Arch","Description"))
            end += 1
        if self.form.CheckColor.isChecked():
            sp.set(chr(ord(col)+end)+str(row),translate("Arch","Color"))
            end += 1
        if self.form.CheckFinish.isChecked():
            sp.set(chr(ord(col)+end)+str(row),translate("Arch","Finish"))
            end += 1
        if self.form.CheckUrl.isChecked():
            sp.set(chr(ord(col)+end)+str(row),translate("Arch","URL"))
            end += 1
        sp.set(chr(ord(col)+end)+str(row),translate("Arch","Item"))
        end += 1
        sp.set(chr(ord(col)+end)+str(row),translate("Arch","Volume")+" ("+volunit+")")
        end += 1
        if self.form.CheckPrice.isChecked():
            sp.set(chr(ord(col)+end)+str(row),translate("Arch","Unit price")+" ("+curunit+"/"+volunit+")")
            end += 1
            sp.set(chr(ord(col)+end)+str(row),translate("Arch","Total price")+" ("+curunit+")")
        row += 2
        
        # volume-based row data
        
        if self.form.CheckStructures.isChecked():
            row,col,sec = self.printVolume(sp,"Column",translate("Arch","Columns"),row,col,sec)
            row,col,sec = self.printVolume(sp,"Beam",translate("Arch","Beams"),row,col,sec)
            row,col,sec = self.printVolume(sp,"Slab",translate("Arch","Slabs"),row,col,sec)
            row,col,sec = self.printVolume(sp,"Foundation",translate("Arch","Foundations"),row,col,sec)
            row,col,sec = self.printVolume(sp,"Pile",None,row,col,sec)
        if self.form.CheckStairs.isChecked():
            row,col,sec = self.printVolume(sp,"Stair",translate("Arch","Stairs"),row,col,sec)
            row,col,sec = self.printVolume(sp,"Stair Flight",None,row,col,sec)
            row,col,sec = self.printVolume(sp,"Ramp",None,row,col,sec)
            row,col,sec = self.printVolume(sp,"Ramp Flight",None,row,col,sec)
        if self.form.CheckWalls.isChecked():
            row,col,sec = self.printVolume(sp,"Wall",translate("Arch","Walls"),row,col,sec)
            row,col,sec = self.printVolume(sp,"Wall Layer",None,row,col,sec)
            row,col,sec = self.printVolume(sp,"Curtain Wall",None,row,col,sec)
        if self.form.CheckOthers.isChecked():
            sp.set(col+str(row), str(sec) + ". " + translate("Arch","Miscellaneous"))
            sec += 1
            row += 2
            row,col,sec = self.printVolume(sp,"Chimney",None,row,col,sec)
            row,col,sec = self.printVolume(sp,"Covering",None,row,col,sec)
            row,col,sec = self.printVolume(sp,"Shading Device",None,row,col,sec)
            row,col,sec = self.printVolume(sp,"Member",None,row,col,sec)
            row,col,sec = self.printVolume(sp,"Railing",None,row,col,sec)
            row,col,sec = self.printVolume(sp,"Tendon",None,row,col,sec)
            row,col,sec = self.printVolume(sp,"Undefined",None,row,col,sec)
            
        # TODO area: Covering Door Plate Rebar Roof Window      
        # TODO count: Furniture Hydro Equipment Electric Equipment      
            
    def printVolume(self,sp,role,title,row,col,sec):
        "print object type quantities to the spreadsheet"
            
        objs = self.getRoles([role])
        totprice = 0
        if objs:
            if title:
                sp.set(col+str(row), str(sec) + ". " + title)
                sec += 1
                row += 2
            mats = self.getMaterials(objs)
            for mat,mobjs in mats.items():
                end = 1
                sp.set(chr(ord(col)+end)+str(row),self.getEntry(mat,"Name"))
                end += 1
                if self.form.CheckDescription.isChecked():
                    sp.set(chr(ord(col)+end)+str(row),self.getEntry(mat,"Description"))
                    end += 1
                if self.form.CheckColor.isChecked():
                    if self.getEntry(mat,"DiffuseColor"):
                        sp.setBackground(chr(ord(col)+end)+str(row),tuple([float(f) for f in self.getEntry(mat,"DiffuseColor").strip("()").split(",")]))
                    end += 1
                if self.form.CheckFinish.isChecked():
                    sp.set(chr(ord(col)+end)+str(row),self.getEntry(mat,"Finish"))
                    end += 1
                if self.form.CheckUrl.isChecked():
                    sp.set(chr(ord(col)+end)+str(row),self.getEntry(mat,"ProductURL"))
                    end += 1
                total = 0
                for mobj in mobjs:
                    if not self.form.CheckOnlyShowTotals.isChecked():
                        v = FreeCAD.Units.Quantity(mobj.Shape.Volume,FreeCAD.Units.Volume).getValueAs(volunit.replace("³","^3")).Value
                        sp.set(chr(ord(col)+end)+str(row),mobj.Label)
                        sp.set(chr(ord(col)+end+1)+str(row),str(v))
                        total += v
                        if self.form.CheckPrice.isChecked():
                            sp.set(chr(ord(col)+end+2)+str(row),self.getEntry(mat,"SpecificPrice"))
                            sp.set(chr(ord(col)+end+3)+str(row),"="+chr(ord(col)+end+1)+str(row)+"*"+chr(ord(col)+end+2)+str(row))
                        row += 1
                sp.set(chr(ord(col)+end)+str(row),translate("Arch","Total"))
                sp.set(chr(ord(col)+end+1)+str(row),str(total))
                if self.form.CheckPrice.isChecked():
                    sp.set(chr(ord(col)+end+2)+str(row),self.getEntry(mat,"SpecificPrice"))
                    sp.set(chr(ord(col)+end+3)+str(row),"="+chr(ord(col)+end+1)+str(row)+"*"+chr(ord(col)+end+2)+str(row))
                    try:
                        totprice += total * float(self.getEntry(mat,"SpecificPrice"))
                    except:
                        print "Arch.Schedule: Unable to add price"
                row += 2
            if self.form.CheckPrice.isChecked():
                if totprice:
                    sp.set(chr(ord(col)+end)+str(row),translate("Arch","Total")+" "+title)
                    sp.set(chr(ord(col)+end+3)+str(row),str(totprice))
                    row += 2
        return row,col,sec
        

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Schedule',_CommandArchSchedule())
