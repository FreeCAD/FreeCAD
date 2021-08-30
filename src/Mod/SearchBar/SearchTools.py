if True:
  from PySide import QtGui

  def toolbarAction(act):
    print('show toolbar ' + act['toolbar'] + ' from workbenches ' + repr(act['workbenches']))
  def toolAction(act):
    print('start action for tool ' + act['toolbar'] + '.' + act['tool'] + ' from workbenches ' + repr(act['workbenches']))
  def subToolAction(act):
    print('start action for tool ' + act['toolbar'] + '.' + act['tool'] + '.' + act['subTool'] + ' from workbenches ' + repr(act['workbenches']))
  def documentObjectAction():
    print('select object ' + o.Document.Name + '.' + o.Name)
  def documentAction():
    print('switch to document ' + o.Document.Name)
  actionHandlers = {
    'toolbarAction': toolbarAction,
    'toolAction': toolAction,
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
      self.listView.setModel(self.proxyModel)
      self.listView.setItemDelegate(itemDelegate) # https://stackoverflow.com/a/65930408/324969
      # make the QListView non-editable
      self.listView.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
      # Connect signals and slots
      self.textChanged.connect(self.filterModel)
      self.listView.clicked.connect(self.selectResult)
      self.listView.selectionModel().selectionChanged.connect(self.showExtraInfo)
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
      self.listView.hide()
      super(SearchBox, self).focusOutEvent(qFocusEvent)
    def keyPressEvent(self, qKeyEvent):
      key = qKeyEvent.key()
      listMovementKeys = {
        QtCore.Qt.Key_Down:     lambda current, nbRows: (current + 1) % nbRows,
        QtCore.Qt.Key_Up:       lambda current, nbRows: (current - 1) % nbRows,
        QtCore.Qt.Key_PageDown: lambda current, nbRows: max(current + min(1, self.maxVisibleRows / 2), nbRows),
        QtCore.Qt.Key_PageUp:   lambda current, nbRows: min(current - min(1, self.maxVisibleRows / 2), 0),
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
            self.selectResult(acceptKeys[key], currentIndex, currentIndex.data())
      elif key in cancelKeys:
        self.listView.hide()
        self.clearFocus()
      else:
        self.showList()
        super(SearchBox, self).keyPressEvent(qKeyEvent)
    def showList(self):
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
      if screen is not None:
        scr = screen.geometry()
        x = min(scr.x() + scr.width() - hint_w, x)
      self.listView.setGeometry(x, y, w, h)
      self.listView.show()
    def selectResult(self):
      self.listView.hide()
      # TODO: allow other options, e.g. some items could act as combinators / cumulative filters
      self.setText('')
      self.filterModel(self.text())
      self.resultSelected.emit("TODO: index", "TODO: str")
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
            return { 'text': group['text'], 'icon': group['icon'], 'action': group['action'], 'subitems': subitems }
          else:
            return None
      def filterGroups(groups):
        groups = (filterGroup(group) for group in groups)
        return [group for group in groups if group is not None]
      def addGroups(filteredGroups, depth=0):
        for group in filteredGroups:
          mdl.appendRow([QtGui.QStandardItem(group['icon'] or QtGui.QIcon(), group['text']),
                         QtGui.QStandardItem(str(depth)),
                         QtGui.QStandardItem(group['action'])])
          addGroups(group['subitems'], depth+1)
      mdl = QtGui.QStandardItemModel()
      mdl.appendColumn([])
      addGroups(filterGroups(self.itemGroups))

      print('setSourceModel for userInput ' + repr(userInput) + ' with ' + str(mdl.rowCount()) + ' rows.')
      self.proxyModel.setSourceModel(mdl)
      #print('TODO: do the actual filtering')
      #flt = QtCore.QSortFilterProxyModel()
      #flt.setSourceModel(self.model)
      #flt.setFilterCaseSensitivity(QtCore.Qt.CaseSensitivity.CaseInsensitive)
      #flt.setFilterWildcard(query)
      #self.listView.setModel(flt)
      # TODO: try to find the already-highlighted item
      nbRows = self.listView.model().rowCount()
      if nbRows > 0:
        index = self.listView.model().index(0, 0)
        self.listView.setCurrentIndex(index)
      #self.showList()
    def showExtraInfo(selected, deselected):
      print('show extra info...', repr(selected))
      pass
  #mdl = QtCore.QStringListModel(['aaa', 'aab', 'aac', 'bxy', 'bac'])
  #sbx = SearchBox(mdl, 10, None)
  #sbx.show()

  # Inspired by https://stackoverflow.com/a/7767999/324969
  #class SearchQCompleter(QtGui.QCompleter):
  #  def __init__(self, model, itemDelegate):
  #    super(SearchQCompleter, self).__init__()
  #    super(SearchQCompleter, self).setModel(QtCore.QIdentityProxyModel())
  #    #https://stackoverflow.com/a/65930408/324969
  #    super(SearchQCompleter, self).popup().setItemDelegate(itemDelegate)
  #    self.setModel(model)
  #  
  #  def setModel(self, itemGroups):
  #    self.itemGroups = itemGroups
  #    self.filterModel('')
  #  
  #  def filterModel(self, userInput):
  #    def matches(s):
  #      return userInput.lower() in s.lower()
  #    def filterGroup(group):
  #      if matches(group['text']):
  #        # If a group matches, include the entire subtree (might need to disable this if it causes too much noise)
  #        return group
  #      else:
  #        subitems = filterGroups(group['subitems'])
  #        if len(subitems) > 0 or matches(group['text']):
  #          return { 'text': group['text'], 'icon': group['icon'], 'action': group['action'], 'subitems': subitems }
  #        else:
  #          return None
  #    def filterGroups(groups):
  #      groups = (filterGroup(group) for group in groups)
  #      return [group for group in groups if group is not None]
  #    def addGroups(filteredGroups, depth=0):
  #      for group in filteredGroups:
  #        mdl.appendRow([QtGui.QStandardItem(group['icon'] or QtGui.QIcon(), group['text']),
  #                       QtGui.QStandardItem(str(depth)),
  #                       QtGui.QStandardItem(group['action'])])
  #        addGroups(group['subitems'], depth+1)
  #    mdl = QtGui.QStandardItemModel()
  #    mdl.appendColumn([])
  #    addGroups(filterGroups(itemGroups))
  #
  #    print('setSourceModel for userInput ' + repr(userInput) + ' with ' + str(mdl.rowCount()) + ' rows.')
  #    self.model().setSourceModel(mdl)
  #    # https://stackoverflow.com/a/65930408/324969
  #  
  #  # the splitPath(self, path) method is called every time the input string changes, before
  #  # drawing the completion list, we latch onto this method to also update the model to contain
  #  # the appropriate results, as given by the custom filterAcceptsRow method above.
  #  def splitPath(self, path):
  #    self.filterModel(path)
  #    # Pretend that the user endered the empty string, so that all items from the filteredProxyModel match.
  #    return ''
  #
  #class SearchBox(QtGui.QLineEdit):
  #  resultSelected = QtCore.Signal(int, str)
  #  def __init__(self, itemGroups):
  #    super(SearchBox, self).__init__()
  #    qom = SearchQCompleter(itemGroups, IndentedItemDelegate())
  #    qom.setMaxVisibleItems(20)
  #    #qom.setCompletionMode(QtGui.QCompleter.CompletionMode.PopupCompletion)
  #    # Thanks to https://saurabhg.com/programming/search-box-using-qlineedit/ for indicating a few useful options
  #    ico = QtGui.QIcon(':/icons/help-browser.svg')
  #    #ico = QtGui.QIcon(':/icons/WhatsThis.svg')
  #    self.addAction(ico, QtGui.QLineEdit.LeadingPosition)
  #    self.setClearButtonEnabled(True)
  #    self.setPlaceholderText('Search tools, prefs & tree')
  #    self.setFixedWidth(200)
  #    # Signals & slots for enter / click
  #    def completerActivated(index):
  #      print('fooooooooooooo')
  #      print(qom.model().rowCount(), index.row())
  #      # TODO: run the action!
  #      result = str(qom.model().data(index.siblingAtColumn(1)))
  #      print('res='+result)
  #      self.clear()
  #      self.completer().setCompletionPrefix('')
  #      self.resultSelected.emit(str(index), result)
  #    def returnPressed():
  #      #self.clear()
  #      #self.completer().setCompletionPrefix('')
  #      pass
  #      #text = sea.text()
  #      #self.clear()
  #      #self.resultSelected.emit('text returnPressed' + text)
  #    self.returnPressed.connect(returnPressed)
  #    #QtCore.QObject.connect(self.completer(), QtCore.SIGNAL('activated(QModelIndex)'), completerActivated) #, QtCore.Qt.ConnectionType.QueuedConnection)
  #    qom.activated.connect(completerActivated, QtCore.Qt.ConnectionType.DirectConnection) #, QtCore.Qt.ConnectionType.QueuedConnection)
  #    #self.completer().activated.connect(returnPressedOrCompleterActivated)
  #    def textChanged():
  #      print('textChanged')
  #      # Workaround: Clear completion prefix and still show the completion box when doing backspace after typing a single character
  #      if self.text() == '':
  #        self.completer().setCompletionPrefix(self.text())
  #        self.completer().complete()
  #    self.textChanged.connect(textChanged)
  #    QtCore.QObject.connect(qom.popup(), QtCore.SIGNAL('clicked(QModelIndex)'), lambda x: print(x))
  #    self.setCompleter(qom)
  #  def focusInEvent(self, e):
  #    super(SearchBox, self).focusInEvent(e)
  #    self.completer().setCompletionPrefix(self.text())
  #    self.completer().complete()
  #    self.completer().setCurrentRow(1) # Does not work
  #    #d=QtGui.QKeyEvent(QtCore.QEvent.KeyPress, QtCore.Qt.Key_Down, QtCore.Qt.NoModifier)
  #    #QtCore.QCoreApplication.postEvent(self, d)
  #  def mousePressEvent(self, e):
  #    super(SearchBox, self).mousePressEvent(e)
  #    self.completer().setCompletionPrefix(self.text())
  #    self.completer().complete()
  #    self.completer().setCurrentRow(1) # Does not work
  #    #d=QtGui.QKeyEvent(QtCore.QEvent.KeyPress, QtCore.Qt.Key_Down, QtCore.Qt.NoModifier)
  #    #QtCore.QCoreApplication.postEvent(self, d)
  #
  mw = Gui.getMainWindow()
  mdi = mw.findChild(QtGui.QMdiArea)

  wdg = QtGui.QWidget()
  lay = QtGui.QGridLayout(wdg)
  mwx = QtGui.QMainWindow()
  mbr = mw.findChildren(QtGui.QToolBar, 'File')[0]

  all_tbs = set()
  for wbname, workbench in Gui.listWorkbenches().items():
    try:
      tbs = workbench.listToolbars()
    except:
      continue
    # careful, tbs contains all the toolbars of the workbench, including shared toolbars
    for tb in tbs:
      all_tbs.add(tb)
  
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
      'icon': serializeIcon(tool['icon'])
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
      'icon': deserializeIcon(tool['icon'])
    }
  
  #serialized = serializeTools([{'workbenches': ['wb1', 'wb2'], 'toolbar': 'tbr', 'text': 'aa', 'icon': ic}])
  #serialized = serializeTools([])

  #TODO:save in App.getUserAppDataDir() ################################################################################################################

  def GatherTools():
    itemGroups = []
    for toolbarName in all_tbs:
      for the_toolbar in mw.findChildren(QtGui.QToolBar, toolbarName):
          group = []

          for tbt in the_toolbar.findChildren(QtGui.QToolButton):
            text = tbt.text()
            act = tbt.defaultAction()
            if text != '':
                # TODO: there also is the tooltip
                icon = tbt.icon()
                #sim.appendRow([QtGui.QStandardItem(icon, 't:' + text), QtGui.QStandardItem('tool')])
                men = tbt.menu()
                subgroup = []
                if men:
                  subgroup = []
                  for mac in men.actions():
                    if mac.text():
                      #all_actions.append(mac.trigger)
                      action = { 'handler': 'subToolAction', 'workbenches': [], 'toolbar': toolbarName, 'tool': text, 'subtool': mac.text() }
                      #print('whaaaat', str(len(all_actions)))
                      subgroup.append({'icon':mac.icon(), 'text':mac.text(), 'action':json.dumps(action), 'subitems':[]})
                #all_actions.append(tbt.actions().trigger)
                #global lalala
                #lalala=tbt
                #print('whuuuut', str(len(all_actions)))
                action = { 'handler': 'toolAction', 'workbenches': [], 'toolbar': toolbarName, 'tool': text }
                group.append({'icon':icon, 'text':text, 'action': json.dumps(action), 'subitems': subgroup})

                #viu = mw.findChildren(QtGui.QToolBar, 'View')[0]
                #tbt = viu.findChildren(QtGui.QToolButton)
                #men = tbt[3].menu()
                #acs = men.actions()          # QtGui.QAction list
                #act = tbt[2].defaultAction() # QtGui.QAction
                #act.trigger()
          action = { 'handler': 'toolbarAction', 'workbenches': [], 'toolbar': toolbarName }
          itemGroups.append({
            'icon': QtGui.QIcon(':/icons/Group.svg'),
            'text': toolbarName,
            'action': json.dumps(action),
            'subitems': group
          })
    #
    def document(doc):
      group = []
      for o in doc.Objects:
        #all_actions.append(lambda: Gui.Selection.addSelection(o.Document.Name, o.Name))
        action = { 'handler': 'documentObjectAction', 'document': o.Document.Name, 'object': o.Name }
        item = {
          'icon': o.ViewObject.Icon if o.ViewObject and o.ViewObject.Icon else None,
          'text': o.Label + ' (' + o.Name + ')',
          'action': json.dumps(action),
          'subitems': []
        }
        group.append(item)

      # Todo: this should also select the document in the tree view
      action = { 'handler': 'documentAction', 'document': o.Document.Name }
      itemGroups.append({
        'icon': QtGui.QIcon(':/icons/Document.svg'),
        'text': doc.Label + ' (' + doc.Name + ')',
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
      'action': itemGroup['action'],
      'subitems': serializeItemGroups(itemGroup['subitems'])
    }
  def serializeItemGroups(itemGroups):
    return [serializeItemGroup(itemGroup) for itemGroup in itemGroups]
  def serialize(itemGroups):
    return json.dumps(serializeItemGroups(itemGroups))
  def deserializeItemGroup(itemGroup):
    #print('dIG' + text + " : " + repr(itemGroup['icon'])[:100] + "......................." + repr(itemGroup['icon'])[-100:])
    return {
      'icon': deserializeIcon(itemGroup['icon']),
      'text': itemGroup['text'],
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
  sea.resultSelected.connect(lambda x, y: print('aaa' + repr(y) + 'end'))
  wax = QtGui.QWidgetAction(None)
  wax.setDefaultWidget(sea)
  #mbr.addWidget(sea)
  mbr.addAction(wax)
