from PySide import QtGui
import FreeCAD

class SafeViewer(QtGui.QWidget):
  """FreeCAD uses a modified version of QuarterWidget, so the import pivy.quarter one will cause segfaults.
     FreeCAD's FreeCADGui.createViewer() puts the viewer widget inside an MDI window, and detaching it without causing segfaults on exit is tricky.
     This class contains some kludges to extract the viewer as a standalone widget and destroy it safely."""
  enabled = FreeCAD.ParamGet('User parameter:BaseApp/Preferences/Mod/SearchBar').GetBool('PreviewEnabled', False)
  instances = []
  def __init__(self, parent = None):
    super(SafeViewer, self).__init__()
    SafeViewer.instances.append(self)
    self.init_parent = parent
    self.instance_enabled = False # Has this specific instance been enabled?
    if SafeViewer.enabled:
      self.displaying_warning = False
      self.enable()
    else:
      import FreeCADGui
      from PySide import QtCore
      self.displaying_warning = True
      self.lbl_warning = QtGui.QTextEdit()
      self.lbl_warning.setReadOnly(True)
      self.lbl_warning.setAlignment(QtCore.Qt.AlignTop)
      self.lbl_warning.setText("Warning: the 3D preview has some stability issues. It can cause FreeCAD to crash (usually when quitting the application) and could in theory cause data loss, inside and outside of FreeCAD.")
      self.btn_enable_for_this_session = QtGui.QPushButton('Enable 3D preview for this session')
      self.btn_enable_for_this_session.clicked.connect(self.enable_for_this_session)
      self.btn_enable_for_future_sessions = QtGui.QPushButton('Enable 3D preview for future sessions')
      self.btn_enable_for_future_sessions.clicked.connect(self.enable_for_future_sessions)
      self.setLayout(QtGui.QVBoxLayout())
      self.layout().addWidget(self.lbl_warning)
      self.layout().addWidget(self.btn_enable_for_this_session)
      self.layout().addWidget(self.btn_enable_for_future_sessions)
  
  def enable_for_this_session(self):
    if not SafeViewer.enabled:
      for instance in SafeViewer.instances:
        instance.enable()

  def enable_for_future_sessions(self):
    if not SafeViewer.enabled:
      # Store in prefs
      FreeCAD.ParamGet('User parameter:BaseApp/Preferences/Mod/SearchBar').SetBool('PreviewEnabled', True)
      # Then enable as usual
      self.enable_for_this_session()

  def enable(self):
    if not self.instance_enabled:
      import FreeCADGui
      # TODO: use a mutex wrapping the entire method, if possible
      SafeViewer.enabled = True
      self.instance_enabled = True # Has this specific instance been enabled?

      if (self.displaying_warning):
        self.layout().removeWidget(self.lbl_warning)
        self.layout().removeWidget(self.btn_enable_for_this_session)
        self.layout().removeWidget(self.btn_enable_for_future_sessions)

      self.viewer = FreeCADGui.createViewer()
      self.graphicsView = self.viewer.graphicsView()
      self.oldGraphicsViewParent = self.graphicsView.parent()
      self.oldGraphicsViewParentParent = self.oldGraphicsViewParent.parent()
      self.oldGraphicsViewParentParentParent = self.oldGraphicsViewParentParent.parent()

      # Avoid segfault but still hide the undesired window by moving it to a new hidden MDI area.
      self.hiddenQMDIArea = QtGui.QMdiArea()
      self.hiddenQMDIArea.addSubWindow(self.oldGraphicsViewParentParentParent)

      self.private_widget = self.oldGraphicsViewParent
      self.private_widget.setParent(self.init_parent)

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
    if SafeViewer.enabled:
      self.private_widget.setParent(self.oldGraphicsViewParentParent)
      self.oldGraphicsViewParentParentParent.close()
      self.oldGraphicsViewParentParentParent = None
      self.oldGraphicsViewParentParent = None
      self.oldGraphicsViewParent = None
      self.graphicsView = None
      self.viewer = None
      #self.parent = None
      self.init_parent = None
      self.hiddenQMDIArea = None

  def showSceneGraph(self, g):
    import FreeCAD as App
    if SafeViewer.enabled:
      self.viewer.getViewer().setSceneGraph(g)
      self.viewer.setCameraOrientation(App.Rotation(1,1,0, 0.2))
      self.viewer.fitAll()

"""
# Example use:
from PySide import QtGui
import pivy
from SafeViewer import SafeViewer
sv = SafeViewer()
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
ww=mk(sv)
"""
