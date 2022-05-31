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
"""Provides the Image_Scaling GuiCommand."""

__title__ = "ImageTools._CommandImageScaling"
__author__ = "JAndersM"
__url__ = "http://www.freecadweb.org/index-fr.html"
__version__ = "00.02"
__date__ = "03/05/2019"

import math
import FreeCAD
from PySide import QtCore

if FreeCAD.GuiUp:
    from PySide import QtGui
    import pivy.coin as pvy

    import FreeCADGui

# Translation-related code
# See forum thread "A new Part tool is being born... JoinFeatures!"
# http://forum.freecadweb.org/viewtopic.php?f=22&t=11112&start=30#p90239
    try:
        _fromUtf8 = QtCore.QString.fromUtf8
    except (Exception):
        def _fromUtf8(s):
            return s

    translate = FreeCAD.Qt.translate

# command class
class _CommandImageScaling:
    "Command to Scale an Image to an Image Plane"
    def GetResources(self):
        return {'Pixmap': "Image_Scaling",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Image_Scaling", "Scale image plane"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Image_Scaling", "Scales an image plane by defining a distance between two points")}

    def Activated(self):
        import draftguitools.gui_trackers as trackers
        cmdCreateImageScaling(name="ImageScaling", trackers=trackers)
        
    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Image_Scaling', _CommandImageScaling())


# helper
def cmdCreateImageScaling(name, trackers):

    def distance(p1,p2):
        dx=p2[0]-p1[0]
        dy=p2[1]-p1[1]
        dz=p2[2]-p1[2]
        return math.sqrt(dx*dx+dy*dy+dz*dz)
        
    def centerOnScreen (widg):
        '''centerOnScreen()
        Centers the window on the screen.'''
        resolution = QtGui.QDesktopWidget().screenGeometry() # TODO: fix multi monitor support
        xp=(resolution.width() / 2) - widg.frameGeometry().width()/2
        yp=(resolution.height() / 2) - widg.frameGeometry().height()/2
        widg.move(xp, yp)
    
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

            self.verticalLayout = QtGui.QVBoxLayout(Dialog)
            self.verticalLayout.setObjectName("verticalLayout")
            self.horizontalLayout = QtGui.QHBoxLayout()
            self.horizontalLayout.setObjectName("horizontalLayout")

            self.label = QtGui.QLabel(Dialog)
            self.label.setObjectName(_fromUtf8("label"))
            self.horizontalLayout.addWidget(self.label)

            self.lineEdit = QtGui.QLineEdit(Dialog)
            self.lineEdit.setObjectName(_fromUtf8("lineEdit"))
            self.horizontalLayout.addWidget(self.lineEdit)

            self.label1 = QtGui.QLabel(Dialog)
            self.label1.setObjectName(_fromUtf8("label1"))

            self.buttonBox = QtGui.QDialogButtonBox(Dialog)
            self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
            self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
            self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
            self.buttonBox.button(QtGui.QDialogButtonBox.Ok).setEnabled(False)

            self.verticalLayout.addLayout(self.horizontalLayout)
            self.verticalLayout.addWidget(self.label1)
            self.verticalLayout.addWidget(self.buttonBox)

            self.retranslateUi(Dialog)
            QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("accepted()")), self.accept)
            QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), self.reject)
            QtCore.QMetaObject.connectSlotsByName(Dialog)
            self.tracker = trackers.lineTracker(scolor=(1,0,0))
            self.tracker.raiseTracker()
            self.tracker.on()
            self.dialog.show()
    
        def retranslateUi(self, Dialog):
            Dialog.setWindowTitle(translate("Dialog", "Scale image plane", None))
            self.label.setText(translate("Dialog", "Distance [mm]", None))
            self.label1.setText(translate("Dialog", "Select first point", None))
            
        def accept(self):
            sel = FreeCADGui.Selection.getSelection()
            try:
                try:
                    q = FreeCAD.Units.parseQuantity(self.lineEdit.text())
                    d = q.Value
                    if q.Unit == FreeCAD.Units.Unit(): # plain number
                        ok = True
                    elif q.Unit == FreeCAD.Units.Length:
                        ok = True
                except Exception:
                    ok = False
                if not ok:
                    raise ValueError
                s=d/self.distance
                sel[0].XSize.Value=sel[0].XSize.Value*s
                sel[0].YSize.Value=sel[0].YSize.Value*s
                FreeCAD.Console.PrintMessage("Image: Scale="+str(s)+"\n")
                self.tracker.off()
                self.tracker.finalize()
                self.dialog.hide()
                FreeCADGui.SendMsgToActiveView("ViewFit")
            except (ValueError, ZeroDivisionError):
                self.label1.setText("<font color='red'>" + translate("Dialog", "Enter distance", None) + "</font>")
                return
            except (IndexError, AttributeError):
                self.label1.setText("<font color='red'>" + translate("Dialog", "Select ImagePlane", None) + "</font>")
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
                self.label1.setText(translate("Dialog", "Select second point", None))
                if len(self.stack)==1:
                    self.tracker.p1(point)
                elif len(self.stack) == 2:
                    self.distance=distance(self.stack[0], self.stack[1])
                    self.tracker.p2(point)
                    self.view.removeEventCallbackPivy(pvy.SoMouseButtonEvent.getClassTypeId(),self.callback)
                    self.view.removeEventCallbackPivy(pvy.SoLocation2Event.getClassTypeId(),self.callmouse)
                    self.buttonBox.button(QtGui.QDialogButtonBox.Ok).setEnabled(True)
                    self.label1.setText(translate("Dialog", "Select Image Plane and type distance", None))
    
    #Init        
    if FreeCADGui.ActiveDocument is not None:
        d = QtGui.QWidget()
        ui = Ui_Dialog()
        ui.setupUi(d)
        centerOnScreen (d)
    else:
        FreeCAD.Console.PrintWarning("no document to work with\n")

    #FreeCAD.ActiveDocument.commitTransaction()
