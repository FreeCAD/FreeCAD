# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Victor Titov (DeepSOIC) <vv.titov@gmail.com>     *
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

__title__ = "ImageTools._CommandImageScaling"
__author__  = "JAndersM"
__url__     = "http://www.freecadweb.org/index-fr.html"
__version__ = "00.02"
__date__    = "03/05/2019" 
 
 
import FreeCAD
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui
    from PySide import QtCore
    import FreeCADGui, FreeCAD, Part
    import math
    import pivy.coin as pvy
    from PySide import QtCore, QtGui
    import DraftTrackers, Draft

# translation-related code
#(see forum thread "A new Part tool is being born... JoinFeatures!"
#http://forum.freecadweb.org/viewtopic.php?f=22&t=11112&start=30#p90239 )
    try:
        _fromUtf8 = QtCore.QString.fromUtf8
    except (Exception):
        def _fromUtf8(s):
            return s
    try:
        _encoding = QtGui.QApplication.UnicodeUTF8
        def _translate(context, text, disambig):
            return QtGui.QApplication.translate(context, text, disambig, _encoding)
    except (AttributeError):
        def _translate(context, text, disambig):
            return QtGui.QApplication.translate(context, text, disambig)


# command class
class _CommandImageScaling:
    "Command to Scale an Image to an Image Plane"
    def GetResources(self):
        return {'Pixmap': "Image_Scaling",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Image_Scaling", "Scale image plane"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Image_Scaling", "Scales an image plane by defining a distance between two points")}

    def Activated(self):
        cmdCreateImageScaling(name="ImageScaling")
        
    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Image_Scaling', _CommandImageScaling())


# helper
def cmdCreateImageScaling(name):

    def distance(p1,p2):
        dx=p2[0]-p1[0]
        dy=p2[1]-p1[1]
        dz=p2[2]-p1[2]
        return math.sqrt(dx*dx+dy*dy+dz*dz)
        
    sizeX = 300; sizeY = 102
    def centerOnScreen (widg):
        '''centerOnScreen()
        Centers the window on the screen.'''
        resolution = QtGui.QDesktopWidget().screenGeometry()
        xp=(resolution.width() / 2) - sizeX/2
        yp=(resolution.height() / 2) - sizeY/2
        widg.setGeometry(xp, yp, sizeX, sizeY)
    
    class Ui_Dialog(object):
        def setupUi(self, Dialog):
            self.view = FreeCADGui.ActiveDocument.ActiveView
            self.stack = []
            self.callback = self.view.addEventCallbackPivy(pvy.SoMouseButtonEvent.getClassTypeId(),self.getpoint)
            self.callmouse=self.view.addEventCallbackPivy(pvy.SoLocation2Event.getClassTypeId(),self.getmousepoint)
            self.distance=0
            self.dialog=Dialog
            Dialog.setObjectName(_fromUtf8("Dialog"))
            Dialog.setWindowFlags(QtCore.Qt.WindowStaysOnTopHint)
            Dialog.resize(sizeX, sizeY)
            self.buttonBox = QtGui.QDialogButtonBox(Dialog)
            self.buttonBox.setGeometry(QtCore.QRect(50, 70, 191, 32))
            self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
            self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
            self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
            self.buttonBox.button(QtGui.QDialogButtonBox.Ok).setEnabled(False)
            self.label = QtGui.QLabel(Dialog)
            self.label.setGeometry(QtCore.QRect(30, 10, 86, 17))
            self.label.setObjectName(_fromUtf8("label"))
            self.lineEdit = QtGui.QLineEdit(Dialog)
            self.lineEdit.setGeometry(QtCore.QRect(140, 10, 153, 29))
            self.lineEdit.setObjectName(_fromUtf8("lineEdit"))
            self.label1 = QtGui.QLabel(Dialog)
            self.label1.setGeometry(QtCore.QRect(20, 45, 260, 17))
            self.label1.setObjectName(_fromUtf8("label1"))
            self.retranslateUi(Dialog)
            QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), self.accept)
            QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), self.reject)
            QtCore.QMetaObject.connectSlotsByName(Dialog)
            self.tracker = DraftTrackers.lineTracker(scolor=(1,0,0))
            self.tracker.raiseTracker()
            self.tracker.on()
            self.dialog.show()
    
        def retranslateUi(self, Dialog):
            Dialog.setWindowTitle(_translate("Dialog", "Scale image plane", None))
            self.label.setText(_translate("Dialog", "Distance [mm]", None))
            self.label1.setText(_translate("Dialog", "Select first point", None))
            
        def accept(self):
            sel = FreeCADGui.Selection.getSelection()
            try:
                locale=QtCore.QLocale.system()
                #d, ok = locale.toFloat(str(eval(self.lineEdit.text())))
                try:
                    d = float(str(eval(self.lineEdit.text().replace(',','.'))))
                    ok = True
                except:
                    ok = False
                if not ok:
                    raise ValueError
                s=d/self.distance
                sel[0].XSize.Value=sel[0].XSize.Value*s
                sel[0].YSize.Value=sel[0].YSize.Value*s
                FreeCAD.Console.PrintMessage("Scale="+str(s))
                self.tracker.off()
                self.tracker.finalize()
                self.dialog.hide()
                FreeCADGui.SendMsgToActiveView("ViewFit")
            except (ValueError, ZeroDivisionError):
                self.label1.setText(_translate("Dialog", "<font color='red'>Enter distance</font>", None))
                return
            except (IndexError, AttributeError):
                self.label1.setText(_translate("Dialog", "<font color='red'>Select ImagePlane</font>", None))
                return
            
        def reject(self):
            if len(self.stack) != 2:
                self.view.removeEventCallbackPivy(pvy.SoMouseButtonEvent.getClassTypeId(),self.callback)
                self.view.removeEventCallbackPivy(pvy.SoLocation2Event.getClassTypeId(),self.callmouse)
            self.stack=[]
            self.tracker.off()
            self.tracker.finalize()
            self.dialog.hide()
        
        def getmousepoint(self, event_cb):
            event = event_cb.getEvent()
            if len(self.stack)==1:
                pos = event.getPosition()
                point = self.view.getPoint(pos[0],pos[1])
                self.tracker.p2(point)
                
        def getpoint(self,event_cb):
            event = event_cb.getEvent()           
            if event.getState() == pvy.SoMouseButtonEvent.DOWN:
                pos = event.getPosition()
                point = self.view.getPoint(pos[0],pos[1])
                self.stack.append(point)
                self.label1.setText(_translate("Dialog", "Select second point", None))
                if len(self.stack)==1:
                    self.tracker.p1(point)
                elif len(self.stack) == 2:
                    self.distance=distance(self.stack[0], self.stack[1])
                    self.tracker.p2(point)
                    self.view.removeEventCallbackPivy(pvy.SoMouseButtonEvent.getClassTypeId(),self.callback)
                    self.view.removeEventCallbackPivy(pvy.SoLocation2Event.getClassTypeId(),self.callmouse)
                    self.buttonBox.button(QtGui.QDialogButtonBox.Ok).setEnabled(True)
                    self.label1.setText(_translate("Dialog", "Select Image Plane and type distance", None))
    
    #Init        
    if FreeCADGui.ActiveDocument is not None:
        d = QtGui.QWidget()
        ui = Ui_Dialog()
        ui.setupUi(d)
        centerOnScreen (d)
    else:
        FreeCAD.Console.PrintWarning("no document to work with\n")

    #FreeCAD.ActiveDocument.commitTransaction()
