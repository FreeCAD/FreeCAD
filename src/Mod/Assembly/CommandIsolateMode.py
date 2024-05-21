import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP, QEvent
from ContextCreatorLibrary import ContextCreationSystem

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets, QtUiTools

import UtilsAssembly
import Preferences

transparency_level = 75

# translate = App.Qt.translate

__title__ = "Isolate Mode"
__author__ = "drwho495"
__url__ = "https://github.com/drwho495/FreeCAD-Context-Fork"

class IsolateManager(QtWidgets.QMainWindow):
    def __init__(self, part, assembly, allParts):
        super().__init__()

        if part == None:
            self.error("You need to select a part!")
            return False
        if assembly == None:
            self.error("You need to select an assembly as active!")
            return False

        self.partToKeep = part
        self.assembly = assembly
        self.allParts = allParts
        # TODO: This only works on my machine!
        self.isolateUI = Gui.PySideUic.loadUi("/home/hypocritical/Documents/GitHub/FreeCAD-daily/FreeCAD/src/Mod/Assembly/Gui/Resources/panels/TaskAssemblyIsolateMode.ui")
        #self.isolateUI.setWindowFlags(QtCore.Qt.WindowStaysOnTopHint)
        self.isolateUI.transparencySlider.valueChanged.connect(self.sliderChanged)
        self.isolateUI.transparencyBox.textChanged.connect(self.onLabelChanged)
        self.isolateUI.doneButton.clicked.connect(self.isolateDone)
        self.isolateUI.installEventFilter(self)
        self.isolateUI.setFixedSize(392, 116)
        self.isolateUI.show()

        # do not isolate the part to keep!
        for part in self.allParts:
           if self.partToKeep.Name == part.Name:
               self.allParts.remove(part)
        
        self.setTransparency(50)

    def eventFilter(self, target, event):
        if event.type() == QEvent.Close:
            print("window closing...")
            self.setTransparency(0)
            return True
        else:
            # event.ignore()
            return False

    def sliderChanged(self):
        self.isolateUI.transparencyBox.setText(str(self.isolateUI.transparencySlider.sliderPosition()))

    def setTransparency(self, transp):
        for part in self.allParts:
            if transp >= 99:
                part.Visibility = False
            else:
                part.Visibility = True

            try:
                if not hasattr(part.ViewObject, "Transparency"):
                    for item in part.OutList:
                        if hasattr(item.ViewObject, "Transparency"):
                            item.ViewObject.Transparency = transp
                            break
                else:
                    part.ViewObject.Transparency = transp
            except:
                print(part.Label) # for debugging

    def isInt(self, number):
        try:
            int(number)
            return True
        except ValueError:
            return False
    
    def isolateDone(self):
        self.isolateUI.close()
        pass

    def onLabelChanged(self):
        newValue = self.isolateUI.transparencyBox.text()
        if newValue == "":
            newValue = "0"
            self.isolateUI.transparencyBox.setText(newValue)

        if self.isInt(newValue) == False:
            newValue = "50"
            self.isolateUI.transparencyBox.setText(newValue)

        if int(newValue) > 100:
            self.isolateUI.transparencyBox.setText("100")
        if int(newValue) < 0:
            self.isolateUI.transparencyBox.setText("0")

        # TODO: set slider value here
        self.isolateUI.transparencySlider.setSliderPosition(int(newValue))
        self.setTransparency(int(newValue))

    def error(self, message):
        class errorBox(QtGui.QMessageBox):
            def __init__(self):
                super(errorBox, self).information(None, "Error", message)
        
        errorBox()

class IsolateGuiManager(QtCore.QObject):
    def __init__(self, assembly, view):
        super().__init__()

        self.assembly = assembly
        self.view = view
        self.doc = App.ActiveDocument

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblySelectIsolateMode.ui")
        self.form.installEventFilter(self)
        self.form.partList.installEventFilter(self)

        pref = Preferences.preferences()
        # Actions
        self.form.partList.itemClicked.connect(self.onItemClicked)
        self.form.filterPartList.textChanged.connect(self.onFilterChange)

        self.allParts = []
        self.translation = 0
        self.selectedPart = None
        self.partMoving = False
        self.totalTranslation = App.Vector()
        self.groundedObj = None

        self.insertionStack = []  # used to handle cancellation of insertions.

        self.buildPartList()

        App.setActiveTransaction("Isolate Parts")

    def onItemClicked(self, item):
        for selected in self.form.partList.selectedIndexes():
            selectedPart = self.allParts[selected.row()]
            self.selectedPart = selectedPart
            print(selectedPart)
        if not selectedPart:
            return
    
    def onFilterChange(self):
        pass

    def buildPartList(self):
        for part in self.assembly.OutList:
            if hasattr(part, 'Placement') and hasattr(part, 'Shape'): # Make sure the part can be isolated
                self.allParts.append(part)

        self.form.partList.clear()
        for part in self.allParts:
            newItem = QtGui.QListWidgetItem()
            newItem.setText(part.Label)
            newItem.setIcon(part.ViewObject.Icon)
            self.form.partList.addItem(newItem)
    
    def accept(self, resetEdit=True):
        print("Creating isolation...")
        Gui.Control.closeDialog()
        self.isolateManager = IsolateManager(self.selectedPart, UtilsAssembly.activeAssembly(), self.allParts)
        
class CommandCreateAssemblyIsolation:
    def __init__(self):
        print("Assembly Isolation Mode Loaded")
        pass

    def GetResources(self):
        return {
            "Pixmap": "Invisible",
            "MenuText": QT_TRANSLATE_NOOP("Invisible", "Isolate a part"),
            "Accel": "I",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Invisible",
                "Isolate a part in an assembly",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        if Preferences.preferences().GetBool("EnforceOneAssemblyRule", True):
            activeAssembly = UtilsAssembly.activeAssembly()

            return activeAssembly != None and Gui.Control.activeDialog() == False

        return App.ActiveDocument is not None

    def Activated(self):
        App.setActiveTransaction("Isolate a part")

        # print("Open Assembly Context UI")
        activeAssembly = UtilsAssembly.activeAssembly()
        if activeAssembly:
            print("Open Assembly Isolate UI")
            self.panel = IsolateGuiManager(activeAssembly, Gui.activeDocument().activeView())
            if Gui.Control.activeDialog() != False:
                App.Console.PrintWarning("The task menu is already being used!")
            else:
                Gui.Control.showDialog(self.panel)
        else:
            App.Console.PrintWarning("You need to have an assembly as your active body!")
    
    


if App.GuiUp:
    Gui.addCommand("Assembly_IsolatePart", CommandCreateAssemblyIsolation())
