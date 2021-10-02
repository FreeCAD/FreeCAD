import os
import FreeCAD as App
import FreeCADGui
from PySide import QtGui
from PySide import QtCore

"""
from SearchTools import SearchTools; from importlib import reload; reload(SearchTools)


TODO for this project:
OK find a way to use the FreeCAD 3D viewer without segfaults or disappearing widgets
OK fix sync problem when moving too fast
OK split the list of tools vs. document objects
OK save to disk the list of tools
OK always display including when switching workbenches
* slightly larger popup widget to avoid scrollbar for the extra info for document objects
* turn this into a standalone mod
OK Optimize so that it's not so slow
OK speed up startup to show the box instantly and do the slow loading on first click.
"""

################################""

class SafeViewer(QtGui.QWidget):
  """FreeCAD uses a modified version of QuarterWidget, so the import pivy.quarter one will cause segfaults.
     FreeCAD's FreeCADGui.createViewer() puts the viewer widget inside an MDI window, and detaching it without causing segfaults on exit is tricky.
     This class contains some kludges to extract the viewer as a standalone widget and destroy it safely."""
  def __init__(self, parent = None):
    super(SafeViewer, self).__init__()
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

genericToolIcon = QtGui.QIcon(QtGui.QIcon(os.path.dirname(__file__) + '/Tango-Tools-spanner-hammer.svg'))

def iconToBase64(icon, sz = QtCore.QSize(64,64), mode = QtGui.QIcon.Mode.Normal, state = QtGui.QIcon.State.On):
  buf = QtCore.QBuffer()
  buf.open(QtCore.QIODevice.WriteOnly)
  icon.pixmap(sz, mode, state).save(buf, 'PNG')
  return QtCore.QTextCodec.codecForName('UTF-8').toUnicode(buf.data().toBase64())
def iconToHTML(icon, sz = 12, mode = QtGui.QIcon.Mode.Normal, state = QtGui.QIcon.State.On):
  return '<img width="'+str(sz)+'" height="'+str(sz)+'" src="data:image/png;base64,' + iconToBase64(icon, QtCore.QSize(sz,sz), mode, state) + '" />'
def refreshToolsAction(act):
  print('Refresh list of tools')
  refreshToolbars()
def toolbarAction(act):
  print('show toolbar ' + act['toolbar'] + ' from workbenches ' + repr(act['workbenches']))
def subToolAction(act):
  toolPath = act['toolbar'] + '.' + act['tool']
  if 'subTool' in act:
    toolPath = toolPath + '.' + act['subTool']
  def runTool():
    mw = FreeCADGui.getMainWindow()
    for the_toolbar in mw.findChildren(QtGui.QToolBar, act['toolbar']):
      for tbt in the_toolbar.findChildren(QtGui.QToolButton):
        if tbt.text() == act['tool']:
          action = None
          if 'subTool' in act:
            men = tbt.menu()
            if men:
              for mac in men.actions():
                if mac.text() == act['subTool']:
                  action = mac
                  break
          else:
            action = tbt.defaultAction()
          if 'showMenu' in act and act['showMenu']:
            print('Popup submenu of tool ' + toolPath + ' available in workbenches ' + repr(act['workbenches']))
            the_toolbar.show()
            tbt.showMenu()
            return True
          elif action is not None:
            print('Run action of tool ' + toolPath + ' available in workbenches ' + repr(act['workbenches']))
            action.trigger()
            return True
    return False
  if runTool():
    return
  else:
    for workbench in act['workbenches']:
      print('Activating workbench ' + workbench + ' to access tool ' + toolPath)
      FreeCADGui.activateWorkbench(workbench)
      if runTool():
        return
  print('Tool ' + toolPath + ' not found, was it offered by an extension that is no longer present?')
def documentObjectAction(act):
  print('select object ' + act['document'] + '.' + act['object'])
  FreeCADGui.Selection.addSelection(act['document'], act['object'])
def documentAction(act):
  # Todo: this should also select the document in the tree view
  print('switch to document ' + act['document'])
  App.setActiveDocument(act['document'])
actionHandlers = {
  'refreshTools': refreshToolsAction,
  'toolbar': toolbarAction,
  'tool': subToolAction,
  'subTool': subToolAction,
  'documentObject': documentObjectAction,
  'document': documentAction
}

# For some reason, the viewer always works except when used for two consecutive items in the search results: it then disappears after a short zoom-in+zoom-out animation.
# I'm giving up on getting this viewer to work in a clean way, and will try swapping two instances so that the same one is never used twice in a row.
# Also, in order to avoid segfaults when the module is reloaded (which causes the previous viewer to be garbage collected at some point), we're using a global property that will survive module reloads.
if not hasattr(App, '_SearchTools3DViewer'):
  App._SearchTools3DViewer = None

import pivy
class DocumentObjectToolTipWidget(QtGui.QWidget):
  def __init__(self, nfo):
    super(DocumentObjectToolTipWidget, self).__init__()
    html = '<p>' + nfo['toolTip']['label'] + '</p><p><code>App.getDocument(' + repr(str(nfo['toolTip']['docName'])) + ').getObject(' + repr(str(nfo['toolTip']['name'])) + ')</code></p>'
    description = QtGui.QTextEdit()
    description.setReadOnly(True)
    description.setAlignment(QtCore.Qt.AlignTop)
    description.setText(html)

    if App._SearchTools3DViewer is None:
      oldFocus = QtGui.QApplication.focusWidget()
      App._SearchTools3DViewer = SafeViewer()
      oldFocus.setFocus()
      # Tried setting the preview to a fixed size to prevent it from disappearing when changing its contents, this sets it to a fixed size but doesn't actually pick the size, .resize does that but isn't enough to fix the bug.
      #safeViewerInstance.setSizePolicy(QtGui.QSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Fixed))
    self.preview = App._SearchTools3DViewer

    obj = App.getDocument(str(nfo['toolTip']['docName'])).getObject(str(nfo['toolTip']['name']))
    ## dummy preview:
    #col = pivy.coin.SoBaseColor()
    #col.rgb = (1, 0, 0)
    #trans = pivy.coin.SoTranslation()
    #trans.translation.setValue([0, 0, 0])
    #cub = pivy.coin.SoCube()
    #myCustomNode = pivy.coin.SoSeparator()
    #myCustomNode.addChild(col)
    #myCustomNode.addChild(trans)
    #myCustomNode.addChild(cub)
    #self.preview.viewer.getViewer().setSceneGraph(myCustomNode)

    # This is really a bad way to do this… to prevent the setExtraInfo function from
    # finalizing the object, we remove the parent ourselves.
    oldParent = self.preview.parent()
    lay = QtGui.QVBoxLayout()
    self.setLayout(lay)
    lay.addWidget(description)
    lay.addWidget(self.preview)
    if oldParent is not None:
      oldParent.hide() # hide before detaching, or we have widgets floating as their own window that appear for a split second in some cases.
      oldParent.setParent(None)
  
    # Tried hiding/detaching the preview to prevent it from disappearing when changing its contents
    self.preview.viewer.stopAnimating()
    self.preview.viewer.getViewer().setSceneGraph(obj.ViewObject.RootNode)
    self.preview.viewer.setCameraOrientation(App.Rotation(1,1,0, 0.2))
    self.preview.viewer.fitAll()

  def finalizer(self):
    #self.preview.finalizer()
    # Detach the widget so that it may be reused without getting deleted
    self.preview.setParent(None)


def easyToolTipWidget(html):
  foo = QtGui.QTextEdit()
  foo.setReadOnly(True)
  foo.setAlignment(QtCore.Qt.AlignTop)
  foo.setText(html)
  return foo
def refreshToolsToolTip(nfo):
  return easyToolTipWidget(iconToHTML(genericToolIcon) + '<p>Load all workbenches to refresh this list of tools. This may take a minute, depending on the number of installed workbenches.</p>')
def toolbarToolTip(nfo):
  return easyToolTipWidget('<p>Display toolbar ' + nfo['toolTip'] + '</p><p>This toolbar appears in the following workbenches: <ul>' + ''.join(['<li>' + iconToHTML(QtGui.QIcon(FreeCADGui.listWorkbenches()[wb].Icon)) + wb + '</li>' for wb in nfo['action']['workbenches']]) + '</ul></p>')
def subToolToolTip(nfo):
  return easyToolTipWidget(iconToHTML(nfo['icon'], 32) + '<p>' + nfo['toolTip'] + '</p>')
def documentObjectToolTip(nfo):
  return DocumentObjectToolTipWidget(nfo)
def documentToolTip(nfo):
  return easyToolTipWidget('<p>' + nfo['toolTip']['label'] + '</p><p><code>App.getDocument(' + repr(str(nfo['toolTip']['name'])) + ')</code></p><p><img src="data:image/png;base64,.............."></p>')
toolTipHandlers = {
  'refreshTools': refreshToolsToolTip,
  'toolbar': toolbarToolTip,
  'tool': subToolToolTip,
  'subTool': subToolToolTip,
  'documentObject': documentObjectToolTip,
  'document': documentToolTip
}

# Inspired by https://stackoverflow.com/a/5443220/324969
# Inspired by https://forum.qt.io/topic/69807/qtreeview-indent-entire-row
class IndentedItemDelegate(QtGui.QStyledItemDelegate):
  def __init__(self):
    super(IndentedItemDelegate, self).__init__()
  def paint(self, painter, option, index):
    depth = int(option.widget.model().itemData(index.siblingAtColumn(1))[0])
    indent = 16 * depth
    option.rect.adjust(indent, 0, 0, 0)
    super(IndentedItemDelegate, self).paint(painter, option, index)
#
globalGroups = []
class SearchBox(QtGui.QLineEdit):
  resultSelected = QtCore.Signal(int, dict)
  def __init__(self, getItemGroups, itemDelegate = IndentedItemDelegate(), maxVisibleRows = 20, parent = None):
    # Call parent cosntructor
    super(SearchBox, self).__init__(parent)
    # Save arguments
    #self.model = model
    self.getItemGroups = getItemGroups
    self.itemGroups = None # Will be initialized by calling getItemGroups() the first time the search box gains focus, through focusInEvent and refreshItemGroups
    self.maxVisibleRows = maxVisibleRows # TODO: use this to compute the correct height
    # Create proxy model
    self.proxyModel = QtCore.QIdentityProxyModel()
    # Filtered model to which items are manually added. Store it as a property of the object instead of a local variable, to prevent grbage collection.
    self.mdl = QtGui.QStandardItemModel()
    #self.proxyModel.setModel(self.model)
    # Create list view
    self.listView = QtGui.QListView(self)
    self.listView.setWindowFlags(QtGui.Qt.ToolTip)
    self.listView.setWindowFlag(QtGui.Qt.FramelessWindowHint)
    self.listView.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)
    self.listView.setModel(self.proxyModel)
    self.listView.setItemDelegate(itemDelegate) # https://stackoverflow.com/a/65930408/324969
    # make the QListView non-editable
    self.listView.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
    # Create pane for showing extra info about the currently-selected tool
    #self.extraInfo = QtGui.QLabel()
    self.extraInfo = QtGui.QWidget()
    self.extraInfo.setWindowFlags(QtGui.Qt.ToolTip)
    self.extraInfo.setWindowFlag(QtGui.Qt.FramelessWindowHint)
    self.extraInfo.setLayout(QtGui.QVBoxLayout())
    self.extraInfo.layout().setContentsMargins(0,0,0,0)
    self.setExtraInfoIsActive = False
    self.pendingExtraInfo = None
    # Connect signals and slots
    self.textChanged.connect(self.filterModel)
    self.listView.clicked.connect(lambda x: self.selectResult('select', x))
    self.listView.selectionModel().selectionChanged.connect(self.onSelectionChanged)
    # Thanks to https://saurabhg.com/programming/search-box-using-qlineedit/ for indicating a few useful options
    ico = QtGui.QIcon(':/icons/help-browser.svg')
    #ico = QtGui.QIcon(':/icons/WhatsThis.svg')
    self.addAction(ico, QtGui.QLineEdit.LeadingPosition)
    self.setClearButtonEnabled(True)
    self.setPlaceholderText('Search tools, prefs & tree')
    self.setFixedWidth(200) # needed to avoid a change of width when the clear button appears/disappears
    # Initialize the model with the full list (assuming the text() is empty)
    #self.filterModel(self.text()) # This is done by refreshItemGroups on focusInEvent, because the initial loading from cache can take time
  def refreshItemGroups(self):
    self.itemGroups = self.getItemGroups()
    self.filterModel(self.text())
  def focusInEvent(self, qFocusEvent):
    self.refreshItemGroups()
    self.showList()
    super(SearchBox, self).focusInEvent(qFocusEvent)
  def focusOutEvent(self, qFocusEvent):
    self.hideList()
    super(SearchBox, self).focusOutEvent(qFocusEvent)
  def keyPressEvent(self, qKeyEvent):
    key = qKeyEvent.key()
    listMovementKeys = {
      QtCore.Qt.Key_Down:     lambda current, nbRows: (current + 1) % nbRows,
      QtCore.Qt.Key_Up:       lambda current, nbRows: (current - 1) % nbRows,
      QtCore.Qt.Key_PageDown: lambda current, nbRows: min(current + max(1, self.maxVisibleRows / 2), nbRows - 1),
      QtCore.Qt.Key_PageUp:   lambda current, nbRows: max(current - max(1, self.maxVisibleRows / 2), 0),
      QtCore.Qt.Key_Home:     lambda current, nbRows: 0,
      QtCore.Qt.Key_End:      lambda current, nbRows: nbRows - 1,
    }
    acceptKeys = {
      QtCore.Qt.Key_Enter:  'select',
      QtCore.Qt.Key_Return: 'select',
      # space on a toolbar/category should toggle the entire category in the search results
      QtCore.Qt.Key_Space:  'toggle',
    }
    cancelKeys = {
      QtCore.Qt.Key_Escape: True,
    }
    
    currentIndex = self.listView.currentIndex()
    if key in listMovementKeys:
      self.showList()
      if self.listView.isEnabled():
          currentRow = currentIndex.row()
          nbRows = self.listView.model().rowCount()
          if nbRows > 0:
            newRow = listMovementKeys[key](currentRow, nbRows)
            index = self.listView.model().index(newRow, 0)
            self.listView.setCurrentIndex(index)
    elif key in acceptKeys:
      self.showList()
      if currentIndex.isValid():
          self.selectResult(acceptKeys[key], currentIndex)
    elif key in cancelKeys:
      self.hideList()
      self.clearFocus()
    else:
      self.showList()
      super(SearchBox, self).keyPressEvent(qKeyEvent)
  def showList(self):
    self.setFloatingWidgetsGeometry()
    if not self.listView.isVisible():
      self.listView.show()
    self.showExtraInfo()
  def hideList(self):
    self.listView.hide()
    self.hideExtraInfo()
  def hideExtraInfo(self):
    self.extraInfo.hide()
  def selectResult(self, mode, index):
    groupIdx = int(index.model().itemData(index.siblingAtColumn(2))[0])
    nfo = globalGroups[groupIdx]
    self.hideList()
    # TODO: allow other options, e.g. some items could act as combinators / cumulative filters
    self.setText('')
    self.filterModel(self.text())
    # TODO: emit index relative to the base model
    self.resultSelected.emit(index, nfo)
  def filterModel(self, userInput):
    # TODO: this will cause a race condition if it is accessed while being modified
    def matches(s):
      return userInput.lower() in s.lower()
    def filterGroup(group):
      if matches(group['text']):
        # If a group matches, include the entire subtree (might need to disable this if it causes too much noise)
        return group
      else:
        subitems = filterGroups(group['subitems'])
        if len(subitems) > 0 or matches(group['text']):
          return { 'id': group['id'], 'text': group['text'], 'icon': group['icon'], 'action': group['action'], 'toolTip':group['toolTip'], 'subitems': subitems }
        else:
          return None
    def filterGroups(groups):
      groups = (filterGroup(group) for group in groups)
      return [group for group in groups if group is not None]
    self.mdl = QtGui.QStandardItemModel()
    self.mdl.appendColumn([])
    def addGroups(filteredGroups, depth=0):
      for group in filteredGroups:
        # TODO: this is not very clean, we should memorize the index from the original itemgroups
        #globalGroups[groupIdx] = json.dumps(serializeItemGroup(group))
        self.mdl.appendRow([QtGui.QStandardItem(group['icon'] or genericToolIcon, group['text']),
                            QtGui.QStandardItem(str(depth)),
                            QtGui.QStandardItem(str(group['id']))])
        addGroups(group['subitems'], depth+1)
    addGroups(filterGroups(self.itemGroups))
    self.proxyModel.setSourceModel(self.mdl)
    # TODO: try to find the already-highlighted item
    nbRows = self.listView.model().rowCount()
    if nbRows > 0:
      index = self.listView.model().index(0, 0)
      self.listView.setCurrentIndex(index)
      self.setExtraInfo(index)
    else:
      self.clearExtraInfo()
    #self.showList()
  def setFloatingWidgetsGeometry(self):
    def getScreenPosition(widget):
      geo = widget.geometry()
      parent = widget.parent()
      parentPos = getScreenPosition(parent) if parent is not None else QtCore.QPoint(0,0)
      return QtCore.QPoint(geo.x() + parentPos.x(), geo.y() + parentPos.y())
    pos = getScreenPosition(self)
    siz = self.size()
    screen = QtGui.QGuiApplication.screenAt(pos)
    x = pos.x()
    y = pos.y() + siz.height()
    hint_w = self.listView.sizeHint().width()
    # TODO: this can still bump into the bottom of the screen, in that case we should flip
    w = max(siz.width(), hint_w)
    h = 200 # TODO: set height / size here according to desired number of items
    extraw = w # choose a preferred width that doesn't change all the time,
                # self.extraInfo.sizeHint().width() would change for every item.
    extrax = x - extraw
    if screen is not None:
      scr = screen.geometry()
      x = min(scr.x() + scr.width() - hint_w, x)
      extraleftw = x - scr.x()
      extrarightw = scr.x() + scr.width() - x
      # flip the extraInfo if it doesn't fit on the screen
      if extraleftw < extraw and extrarightw > extraleftw:
        extrax = x + w
        extraw = min(extrarightw, extraw)
      else:
        extrax = x - extraw
        extraw = min(extraleftw, extraw)
    self.listView.setGeometry(x, y, w, h)
    self.extraInfo.setGeometry(extrax, y, extraw, h)
  def onSelectionChanged(self, selected, deselected):
    # The list has .setSelectionMode(QtGui.QAbstractItemView.SingleSelection),
    # so there is always at most one index in selected.indexes() and at most one
    # index in deselected.indexes()
    selected = selected.indexes()
    deselected = deselected.indexes()
    if len(selected) > 0:
      index = selected[0]
      self.setExtraInfo(index)
      # Poor attempt to circumvent a glitch where the extra info pane stays visible after pressing Return
      if not self.listView.isHidden():
        self.showExtraInfo()
    elif len(deselected) > 0:
      self.hideExtraInfo()
  def setExtraInfo(self, index):
    global globalGroups
    # TODO: use an atomic swap or mutex if possible
    if self.setExtraInfoIsActive:
      self.pendingExtraInfo = index
      #print("boom")
    else:
      self.setExtraInfoIsActive = True
      #print("lock")
      # setExtraInfo can be called multiple times while this function is running,
      # so just before existing we check for the latest pending call and execute it,
      # if during that second execution some other calls are made the latest of those will
      # be queued by the code a few lines above this one, and the loop will continue processing
      # until an iteration during which no further call was made.
      while True:
        nfo = str(index.model().itemData(index.siblingAtColumn(2))[0])
        # TODO: move this outside of this class, probably use a single metadata
        nfo = globalGroups[int(nfo)]# TODO: used to be deserializeItemGroup(json.loads(nfo))
        #TODO: used to be: nfo['action'] = json.loads(nfo['action'])
        #while len(self.extraInfo.children()) > 0:
        #  self.extraInfo.children()[0].setParent(None)
        w = self.extraInfo.layout().takeAt(0)
        toolTipWidget = toolTipHandlers[nfo['action']['handler']](nfo)
        while w:
          if hasattr(w.widget(), 'finalizer'):
            # The 3D viewer segfaults very easily if it is used after being destroyed, and some Python/C++ interop seems to overzealously destroys some widgets, including this one, too soon?
            # Ensuring that we properly detacth the 3D viewer widget before discarding its parent seems to avoid these crashes.
            #print('FINALIZER')
            w.widget().finalizer()
          if w.widget() is not None:
            w.widget().hide() # hide before detaching, or we have widgets floating as their own window that appear for a split second in some cases.
            w.widget().setParent(None)
          w = self.extraInfo.layout().takeAt(0)
        self.extraInfo.layout().addWidget(toolTipWidget)
        global toto
        toto = self.extraInfo
        #toolTipHTML = toolTipHandlers[nfo['action']['handler']](nfo)
        #self.extraInfo.setText(toolTipHTML)
        self.setFloatingWidgetsGeometry()
        if self.pendingExtraInfo is not None:
          index = self.pendingExtraInfo
          self.pendingExtraInfo = None
        else:
          break
      #print("unlock")
      self.setExtraInfoIsActive = False
  def clearExtraInfo(self):
    # TODO: just clear the contents but keep the widget visible.
    self.extraInfo.hide()
  def showExtraInfo(self):
    self.extraInfo.show()

def getAllToolbars():
  all_tbs = dict()
  for wbname, workbench in FreeCADGui.listWorkbenches().items():
    try:
      tbs = workbench.listToolbars()
    except:
      continue
    # careful, tbs contains all the toolbars of the workbench, including shared toolbars
    for tb in tbs:
      if tb not in all_tbs:
        all_tbs[tb] = set()
      all_tbs[tb].add(wbname)
  return all_tbs

import json
def serializeIcon(icon):
  iconPixmaps = {}
  for sz in icon.availableSizes():
    strW = str(sz.width())
    strH = str(sz.height())
    iconPixmaps[strW] = {}
    iconPixmaps[strW][strH] = {}
    for strMode, mode in {'normal':QtGui.QIcon.Mode.Normal, 'disabled':QtGui.QIcon.Mode.Disabled, 'active':QtGui.QIcon.Mode.Active, 'selected':QtGui.QIcon.Mode.Selected}.items():
      iconPixmaps[strW][strH][strMode] = {}
      for strState, state in {'off':QtGui.QIcon.State.Off, 'on':QtGui.QIcon.State.On}.items():
        iconPixmaps[strW][strH][strMode][strState] = iconToBase64(icon, sz, mode, state)
  return iconPixmaps
# workbenches is a list(str), toolbar is a str, text is a str, icon is a QtGui.QIcon
def serializeTool(tool):
  return {
    'workbenches': tool['workbenches'],
    'toolbar': tool['toolbar'],
    'text': tool['text'],
    'toolTip': tool['toolTip'],
    'icon': serializeIcon(tool['icon']),
  }

def deserializeIcon(iconPixmaps):
  ico = QtGui.QIcon()
  for strW, wPixmaps in iconPixmaps.items():
    for strH, hPixmaps in wPixmaps.items():
      for strMode, modePixmaps in hPixmaps.items():
        mode = {'normal':QtGui.QIcon.Mode.Normal, 'disabled':QtGui.QIcon.Mode.Disabled, 'active':QtGui.QIcon.Mode.Active, 'selected':QtGui.QIcon.Mode.Selected}[strMode]
        for strState, statePixmap in modePixmaps.items():
          state = {'off':QtGui.QIcon.State.Off, 'on':QtGui.QIcon.State.On}[strState]
          pxm = QtGui.QPixmap()
          pxm.loadFromData(QtCore.QByteArray.fromBase64(QtCore.QTextCodec.codecForName('UTF-8').fromUnicode(statePixmap)))
          ico.addPixmap(pxm, mode, state)
  return ico

def deserializeTool(tool):
  return {
    'workbenches': tool['workbenches'],
    'toolbar': tool['toolbar'],
    'text': tool['text'],
    'toolTip': tool['toolTip'],
    'icon': deserializeIcon(tool['icon']),
  }

def gatherTools():
  itemGroups = []
  itemGroups.append({
    'icon': genericToolIcon,
    'text': 'Refresh list of tools',
    'toolTip': '',
    'action': {'handler': 'refreshTools'},
    'subitems': []
  })
  all_tbs = getAllToolbars()
  mw = FreeCADGui.getMainWindow()
  for toolbarName, toolbarIsInWorkbenches in all_tbs.items():
    toolbarIsInWorkbenches = sorted(list(toolbarIsInWorkbenches))
    for the_toolbar in mw.findChildren(QtGui.QToolBar, toolbarName):
      group = []
      for tbt in the_toolbar.findChildren(QtGui.QToolButton):
        text = tbt.text()
        act = tbt.defaultAction()
        if text != '':
            # TODO: there also is the tooltip
            icon = tbt.icon()
            men = tbt.menu()
            subgroup = []
            if men:
              subgroup = []
              for mac in men.actions():
                if mac.text():
                  action = { 'handler': 'subTool', 'workbenches': toolbarIsInWorkbenches, 'toolbar': toolbarName, 'tool': text, 'subTool': mac.text() }
                  subgroup.append({'icon':mac.icon(), 'text':mac.text(), 'toolTip': mac.toolTip(), 'action':action, 'subitems':[]})
            # The default action of a menu changes dynamically, instead of triggering the last action, just show the menu.
            action = { 'handler': 'tool', 'workbenches': toolbarIsInWorkbenches, 'toolbar': toolbarName, 'tool': text, 'showMenu': bool(men) }
            group.append({'icon':icon, 'text':text, 'toolTip': tbt.toolTip(), 'action': action, 'subitems': subgroup})
      # TODO: move the 'workbenches' field to the itemgroup
      action = { 'handler': 'toolbar', 'workbenches': toolbarIsInWorkbenches, 'toolbar': toolbarName }
      itemGroups.append({
        'icon': QtGui.QIcon(':/icons/Group.svg'),
        'text': toolbarName,
        'toolTip': '',
        'action': action,
        'subitems': group
      })
  return itemGroups

def gatherDocumentObjects():
  itemGroups = []
  def document(doc):
    group = []
    for o in doc.Objects:
      #all_actions.append(lambda: )
      action = { 'handler': 'documentObject', 'document': o.Document.Name, 'object': o.Name }
      item = {
        'icon': o.ViewObject.Icon if o.ViewObject and o.ViewObject.Icon else None,
        'text': o.Label + ' (' + o.Name + ')',
        # TODO: preview of the object
        'toolTip': { 'label': o.Label, 'name': o.Name, 'docName': o.Document.Name},
        'action': action,
        'subitems': []
      }
      group.append(item)

    action = { 'handler': 'document', 'document': doc.Name }
    itemGroups.append({
      'icon': QtGui.QIcon(':/icons/Document.svg'),
      'text': doc.Label + ' (' + doc.Name + ')',
      # TODO: preview of the document
      'toolTip': { 'label': doc.Label, 'name': doc.Name},
      'action':action,
      'subitems': group })
  if App.ActiveDocument:
    document(App.ActiveDocument)
  for docname, doc in App.listDocuments().items():
    if not App.activeDocument or docname != App.ActiveDocument.Name:
      document(doc)
  return itemGroups

def serializeItemGroup(itemGroup):
  return {
    'icon': serializeIcon(itemGroup['icon']),
    'text': itemGroup['text'],
    'toolTip': itemGroup['toolTip'],
    'action': itemGroup['action'],
    'subitems': serializeItemGroups(itemGroup['subitems'])
  }
def serializeItemGroups(itemGroups):
  return [serializeItemGroup(itemGroup) for itemGroup in itemGroups]
def serialize(itemGroups):
  return json.dumps(serializeItemGroups(itemGroups))
def deserializeItemGroup(itemGroup):
  return {
    'icon': deserializeIcon(itemGroup['icon']),
    'text': itemGroup['text'],
    'toolTip': itemGroup['toolTip'],
    'action': itemGroup['action'],
    'subitems': deserializeItemGroups(itemGroup['subitems'])
  }
def deserializeItemGroups(serializedItemGroups):
  return [deserializeItemGroup(itemGroup) for itemGroup in serializedItemGroups]
def deserialize(serializeItemGroups):
  return deserializeItemGroups(json.loads(serializedItemGroups))

itemGroups = None
serializedItemGroups = None

def loadAllWorkbenches():
  activeWorkbench = FreeCADGui.activeWorkbench().name()
  lbl = QtGui.QLabel('Loading workbench … (…/…)')
  lbl.show()
  lst = FreeCADGui.listWorkbenches()
  for i, wb in enumerate(lst):
    msg = 'Loading workbench ' + wb + ' (' + str(i) + '/' + str(len(lst)) + ')'
    print(msg)
    lbl.setText(msg)
    geo = lbl.geometry()
    geo.setSize(lbl.sizeHint())
    lbl.setGeometry(geo)
    lbl.repaint()
    FreeCADGui.updateGui() # Probably slower with this, because it redraws the entire GUI with all tool buttons changed etc. but allows the label to actually be updated, and it looks nice and gives a quick overview of all the workbenches…
    try:
      FreeCADGui.activateWorkbench(wb)
    except:
      pass
  lbl.hide()
  FreeCADGui.activateWorkbench(activeWorkbench)

def cachePath():
  return os.path.join(App.getUserAppDataDir(), 'Cache_SearchToolsMod')
def writeCacheTools():
  global itemGroups, serializedItemGroups
  serializedItemGroups = serialize(gatherTools())
  # Todo: use wb and a specific encoding.
  with open(cachePath(), 'w') as cache:
    cache.write(serializedItemGroups)
  # I prefer to systematically deserialize, instead of taking the original version,
  # this avoids possible inconsistencies between the original and the cache and
  # makes sure cache-related bugs are noticed quickly.
  itemGroups = deserialize(serializedItemGroups)
  print('SearchBox: Cache has been written.')

def readCacheTools():
  global itemGroups, serializedItemGroups
  # Todo: use rb and a specific encoding.
  with open(cachePath(), 'r') as cache:
    serializedItemGroups = cache.read()
  itemGroups = deserialize(serializedItemGroups)
  print('SearchBox: Tools were loaded from the cache.')


def refreshToolbars(doLoadAllWorkbenches = True):
  if doLoadAllWorkbenches:
    loadAllWorkbenches()
    writeCacheTools()
  else:
    try:
      readCacheTools()
    except:
      writeCacheTools()

# Avoid garbage collection by storing the action in a global variable
wax = None

def getItemGroups():
  global itemGroups, serializedItemGroups, globalGroups
  # Load the list of tools, preferably from the cache, if it has not already been loaded:
  if itemGroups is None:
    if serializedItemGroups is None:
      refreshToolbars(doLoadAllWorkbenches = False)
    else:
      itemGroups = deserialize(serializedItemGroups)

  # Aggregate the tools (cached) and document objects (not cached), and assign an index to each
  igs = itemGroups + gatherDocumentObjects()
  globalGroups = []
  def addId(group):
    globalGroups.append(group)
    group['id'] = len(globalGroups) - 1
    for subitem in group['subitems']:
      addId(subitem)
  for ig in igs:
    addId(ig)
  
  return igs

def addToolSearchBox():
  global wax, sea
  sea = SearchBox(getItemGroups)
  mw = FreeCADGui.getMainWindow()
  mbr = mw.findChildren(QtGui.QToolBar, 'File')[0]
  # Create search box widget
  def onResultSelected(index, nfo):
    action = nfo['action']
    actionHandlers[action['handler']](action)
  sea.resultSelected.connect(onResultSelected)
  wax = QtGui.QWidgetAction(None)
  wax.setDefaultWidget(sea)
  #mbr.addWidget(sea)
  #print("addAction" + repr(mbr) + ' add(' + repr(wax))
  mbr.addAction(wax)

addToolSearchBox()
FreeCADGui.getMainWindow().workbenchActivated.connect(addToolSearchBox)
