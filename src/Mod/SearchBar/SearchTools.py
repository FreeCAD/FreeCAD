if True:
  from PySide import QtGui

  def toolbarAction(act):
    print('show toolbar ' + act['toolbar'] + ' from workbenches ' + repr(act['workbenches']))
  def subToolAction(act):
    toolPath = act['toolbar'] + '.' + act['tool']
    if 'subTool' in act:
      toolPath = toolPath + '.' + act['subTool']
    def runTool():
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
        Gui.activateWorkbench(workbench)
        if runTool():
          return
    print('Tool ' + toolPath + ' not found, was it offered by an extension that is no longer present?')
  def documentObjectAction(act):
    print('select object ' + act['document'] + '.' + act['object'])
    Gui.Selection.addSelection(act['document'], act['object'])
  def documentAction(act):
    # Todo: this should also select the document in the tree view
    print('switch to document ' + act['document'])
    App.setActiveDocument(act['document'])
  actionHandlers = {
    'toolbarAction': toolbarAction,
    'toolAction': subToolAction,
    'subToolAction': subToolAction,
    'documentObjectAction': documentObjectAction,
    'documentAction': documentAction
  }

  # Inspired by https://stackoverflow.com/a/5443220/324969
  from PySide import QtGui
  #
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
  class SearchBox(QtGui.QLineEdit):
    resultSelected = QtCore.Signal(int, str)
    def __init__(self, itemGroups, itemDelegate = IndentedItemDelegate(), maxVisibleRows = 20, parent = None):
      # Call parent cosntructor
      super(SearchBox, self).__init__(parent)
      # Save arguments
      #self.model = model
      self.itemGroups = itemGroups
      self.maxVisibleRows = maxVisibleRows # TODO: use this to compute the correct height
      # Create proxy model
      self.proxyModel = QtCore.QIdentityProxyModel()
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
      self.extraInfo = QtGui.QTextEdit()
      self.extraInfo.setReadOnly(True)
      self.extraInfo.setWindowFlags(QtGui.Qt.ToolTip)
      self.extraInfo.setWindowFlag(QtGui.Qt.FramelessWindowHint)
      self.extraInfo.setAlignment(QtCore.Qt.AlignTop)
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
      self.filterModel(self.text())
    def focusInEvent(self, qFocusEvent):
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
      self.listView.show()
      self.showExtraInfo()
    def hideList(self):
      self.listView.hide()
      self.hideExtraInfo()
    def hideExtraInfo(self):
      self.extraInfo.hide()
    def selectResult(self, mode, index):
      action = str(index.model().itemData(index.siblingAtColumn(2))[0])
      self.hideList()
      # TODO: allow other options, e.g. some items could act as combinators / cumulative filters
      self.setText('')
      self.filterModel(self.text())
      # TODO: emit index relative to the base model
      self.resultSelected.emit(index, action)
    def filterModel(self, userInput):
      def matches(s):
        return userInput.lower() in s.lower()
      def filterGroup(group):
        if matches(group['text']):
          # If a group matches, include the entire subtree (might need to disable this if it causes too much noise)
          return group
        else:
          subitems = filterGroups(group['subitems'])
          if len(subitems) > 0 or matches(group['text']):
            return { 'text': group['text'], 'icon': group['icon'], 'action': group['action'], 'toolTipHTML':group['toolTipHTML'], 'subitems': subitems }
          else:
            return None
      def filterGroups(groups):
        groups = (filterGroup(group) for group in groups)
        return [group for group in groups if group is not None]
      def addGroups(filteredGroups, depth=0):
        for group in filteredGroups:
          mdl.appendRow([QtGui.QStandardItem(group['icon'] or QtGui.QIcon(), group['text']),
                         QtGui.QStandardItem(str(depth)),
                         QtGui.QStandardItem(group['action']),
                         QtGui.QStandardItem(group['toolTipHTML'])])
          addGroups(group['subitems'], depth+1)
      mdl = QtGui.QStandardItemModel()
      mdl.appendColumn([])
      addGroups(filterGroups(self.itemGroups))
      self.proxyModel.setSourceModel(mdl)
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
      h = 100
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
      toolTipHTML = str(index.model().itemData(index.siblingAtColumn(3))[0])
      self.extraInfo.setText(toolTipHTML)
      self.setFloatingWidgetsGeometry()
    def clearExtraInfo(self, index):
      self.extraInfo.setText('')
    def showExtraInfo(self):
      self.extraInfo.show()

  mw = Gui.getMainWindow()
  mdi = mw.findChild(QtGui.QMdiArea)

  wdg = QtGui.QWidget()
  lay = QtGui.QGridLayout(wdg)
  mwx = QtGui.QMainWindow()
  mbr = mw.findChildren(QtGui.QToolBar, 'File')[0]

  def getAllToolbars():
    all_tbs = dict()
    for wbname, workbench in Gui.listWorkbenches().items():
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
          buf = QtCore.QBuffer()
          buf.open(QtCore.QIODevice.WriteOnly)
          icon.pixmap(sz, mode, state).save(buf, 'PNG')
          iconPixmaps[strW][strH][strMode][strState] = QtCore.QTextCodec.codecForName('UTF-8').toUnicode(buf.data().toBase64())
    return iconPixmaps
  # workbenches is a list(str), toolbar is a str, text is a str, icon is a QtGui.QIcon
  def serializeTool(tool):
    return {
      'workbenches': tool['workbenches'],
      'toolbar': tool['toolbar'],
      'text': tool['text'],
      'icon': serializeIcon(tool['icon']),
      'toolTipHTML': tool['toolTipHTML']
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
      'icon': deserializeIcon(tool['icon']),
      'toolTipHTML': tool['toolTipHTML']
    }
  
  # TODO: save serialized tools in App.getUserAppDataDir() ################################################################################################################
  #       + never cache the document objects

  def GatherTools():
    itemGroups = []
    all_tbs = getAllToolbars()
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
                    action = { 'handler': 'subToolAction', 'workbenches': toolbarIsInWorkbenches, 'toolbar': toolbarName, 'tool': text, 'subTool': mac.text() }
                    subgroup.append({'icon':mac.icon(), 'text':mac.text(), 'toolTipHTML': mac.toolTip(), 'action':json.dumps(action), 'subitems':[]})
              # The default action of a menu changes dynamically, instead of triggering the last action, just show the menu.
              action = { 'handler': 'toolAction', 'workbenches': toolbarIsInWorkbenches, 'toolbar': toolbarName, 'tool': text, 'showMenu': bool(men) }
              group.append({'icon':icon, 'text':text, 'toolTipHTML': tbt.toolTip(), 'action': json.dumps(action), 'subitems': subgroup})
        # TODO: move the 'workbenches' field to the itemgroup
        action = { 'handler': 'toolbarAction', 'workbenches': toolbarIsInWorkbenches, 'toolbar': toolbarName }
        itemGroups.append({
          'icon': QtGui.QIcon(':/icons/Group.svg'),
          'text': toolbarName,
          'toolTipHTML': '<p>Display toolbar ' + toolbarName + '</p><p>This toolbar appears in the following workbenches: <ul>' + ''.join(['<li>' + wb + '</li>' for wb in toolbarIsInWorkbenches]) + '</ul></p>',
          'action': json.dumps(action),
          'subitems': group
        })
    #
    def document(doc):
      group = []
      for o in doc.Objects:
        #all_actions.append(lambda: )
        action = { 'handler': 'documentObjectAction', 'document': o.Document.Name, 'object': o.Name }
        item = {
          'icon': o.ViewObject.Icon if o.ViewObject and o.ViewObject.Icon else None,
          'text': o.Label + ' (' + o.Name + ')',
          # TODO: preview of the object
          'toolTipHTML': '<p>' + o.Label + '</p><p><code>App.getDocument(' + repr(o.Document.Name) + ').getObject(' + repr(o.Name) + ')</code></p><p>' + '</p><p><img src="data:image/png;base64,.............."></p>',
          'action': json.dumps(action),
          'subitems': []
        }
        group.append(item)

      action = { 'handler': 'documentAction', 'document': doc.Name }
      itemGroups.append({
        'icon': QtGui.QIcon(':/icons/Document.svg'),
        'text': doc.Label + ' (' + doc.Name + ')',
        # TODO: preview of the document
        'toolTipHTML': '<p>' + doc.Label + '</p><p><code>App.getDocument(' + repr(doc.Name) + ')</code></p><p><img src="data:image/png;base64,.............."></p>',
        'action':json.dumps(action),
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
      'toolTipHTML': itemGroup['toolTipHTML'],
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
      'toolTipHTML': itemGroup['toolTipHTML'],
      'action': itemGroup['action'],
      'subitems': deserializeItemGroups(itemGroup['subitems'])
    }
  def deserializeItemGroups(serializedItemGroups):
    return [deserializeItemGroup(itemGroup) for itemGroup in serializedItemGroups]
  def deserialize(serializeItemGroups):
    return deserializeItemGroups(json.loads(serializedItemGroups))

  serializedItemGroups = serialize(GatherTools())
  itemGroups = deserialize(serializedItemGroups)

  #
  sea = SearchBox(itemGroups)
  def onResultSelected(index, metadata):
    action = json.loads(metadata)
    actionHandlers[action['handler']](action)
  sea.resultSelected.connect(onResultSelected)
  wax = QtGui.QWidgetAction(None)
  wax.setDefaultWidget(sea)
  #mbr.addWidget(sea)
  mbr.addAction(wax)
