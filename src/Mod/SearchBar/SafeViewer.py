print("Loaded file SafeViewer.py")
from PySide import QtGui

class SafeViewer(QtGui.QWidget):
  """FreeCAD uses a modified version of QuarterWidget, so the import pivy.quarter one will cause segfaults.
     FreeCAD's FreeCADGui.createViewer() puts the viewer widget inside an MDI window, and detaching it without causing segfaults on exit is tricky.
     This class contains some kludges to extract the viewer as a standalone widget and destroy it safely."""
  def __init__(self, parent = None):
    super(SafeViewer, self).__init__()
    import FreeCADGui
    self.viewer = FreeCADGui.createViewer()
    self.graphicsView = self.viewer.graphicsView()
    self.oldGraphicsViewParent = self.graphicsView.parent()
    self.oldGraphicsViewParentParent = self.oldGraphicsViewParent.parent()
    self.oldGraphicsViewParentParentParent = self.oldGraphicsViewParentParent.parent()

    # Avoid segfault but still hide the undesired window by moving it to a new hidden MDI area.
    self.hiddenQMDIArea = QtGui.QMdiArea()
    self.hiddenQMDIArea.addSubWindow(self.oldGraphicsViewParentParentParent)

    self.private_widget = self.oldGraphicsViewParent
    self.private_widget.setParent(parent)

    self.setLayout(QtGui.QVBoxLayout())
    self.layout().addWidget(self.private_widget)
    self.layout().setContentsMargins(0,0,0,0)

    def fin(slf):
      slf.finalizer()

    import weakref
    weakref.finalize(self, fin, self)

    self.destroyed.connect(self.finalizer)

  def finalizer(self):
    # Cleanup in an order that doesn't cause a segfault:
    self.private_widget.setParent(self.oldGraphicsViewParentParent)
    self.oldGraphicsViewParentParentParent.close()
    self.oldGraphicsViewParentParentParent = None
    self.oldGraphicsViewParentParent = None
    self.oldGraphicsViewParent = None
    self.graphicsView = None
    self.viewer = None
    #self.parent = None
    self.hiddenQMDIArea = None

"""
# Example use:
from PySide import QtGui
import pivy
def mk(v):
  w = QtGui.QMainWindow()
  oldFocus = QtGui.QApplication.focusWidget()
  sv.widget.setParent(w)
  oldFocus.setFocus()
  w.show()
  col = pivy.coin.SoBaseColor()
  col.rgb = (1, 0, 0)
  trans = pivy.coin.SoTranslation()
  trans.translation.setValue([0, 0, 0])
  cub = pivy.coin.SoCube()
  myCustomNode = pivy.coin.SoSeparator()
  myCustomNode.addChild(col)
  myCustomNode.addChild(trans)
  myCustomNode.addChild(cub)
  sv.viewer.getViewer().setSceneGraph(myCustomNode)
  sv.viewer.fitAll()
  return w
sv = SafeViewer()
ww=mk(sv)
"""
