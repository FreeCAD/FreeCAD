
  # for wbname, workbench in Gui.listWorkbenches().items():
  #   try:
  #     tbs = workbench.listToolbars()
  #     # careful, tbs contains all the toolbars of the workbench, including shared toolbars
  #     for tb in mw.findChildren(QtGui.QToolBar, 'XternalApplications'):
  #         for bt in tb.findChildren(QtGui.QToolButton):
  #           text = bt.text()
  #           if text != '':
  #               # TODO: there also is the tooltip
  #               icon = bt.icon()

  #               # To preview the icon, assign it as the icon of a dummy window.
  #               mdi.setWindowIcon(icon) # probably sets the default icon to use for windows without an icon?
  #               mwx.setWindowIcon(icon) # untested
  #   except:
  #     pass



#from PySide import QtGui
#qwd = QtGui.QWidget()
#but1 = QtGui.QPushButton("hi")
#but2 = QtGui.QPushButton("hello")
#lay = QtGui.QGridLayout(qwd)
#lay.addWidget(but1)
#lay.addWidget(but2)
#mwx = QtGui.QMainWindow()
#mwx.setCentralWidget(qwd)
#mw = Gui.getMainWindow()
#mdi = mw.findChild(QtGui.QMdiArea)
#mdi.addSubWindow(mwx)
#mwx.show()
#but3 = QtGui.QPushButton("XXX")
#lay.addWidget(but3)
#











  #lay.addWidget(sea)
  #
  #lsv = QtGui.QListView()
  #sim = QtGui.QStandardItemModel()
  #sim.appendColumn([])
  #flt = QtCore.QSortFilterProxyModel()
  #flt.setSourceModel(sim)
  #flt.setFilterCaseSensitivity(QtCore.Qt.CaseSensitivity.CaseInsensitive)
  ## make the QListView non-editable
  #lsv.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
  #lsv.setModel(flt)
  ##lay.addWidget(lsv)
  #
  #lsv.setWindowFlag(QtGui.Qt.FramelessWindowHint)
  #lsv.setWindowFlag(QtGui.Qt.ToolTip)
  #def searchPopup(str):
  #  def getScreenPosition(widget):
  #    geo = widget.geometry()
  #    parent = widget.parent()
  #    parentGeo = getScreenPosition(parent) if parent is not None else QtCore.QPoint(0,0)
  #    return QtCore.QPoint(geo.x() + parentGeo.x(), geo.y() + parentGeo.y())
  #  pos = getScreenPosition(sea)
  #  siz = sea.size()
  #  scr = QtGui.QGuiApplication.screenAt(pos).geometry()
  #  x = pos.x()
  #  y = pos.y() + siz.height()
  #  hint_w = lsv.sizeHint().width()
  #  w = max(siz.width(), hint_w)
  #  x = min(scr.x() + scr.width() - hint_w, x)
  #  h = 100
  #  lsv.setGeometry(x, y, w, h)
  #  lsv.show()
  #  flt.setFilterWildcard(str)
  #sea.textChanged.connect(searchPopup)
  #
  #cbx = QtGui.QComboBox()
  #cbx.setModel(sim)
  #cbx.setEditable(True)
  
  #mwx.setCentralWidget(wdg)
  #mdi.addSubWindow(mwx)
  
  #xap = mw.findChildren(QtGui.QToolBar, 'XternalApplications')[0]
  #mbr = mw.findChildren(QtGui.QMenuBar)[0]


  #le = QtGui.QLineEdit()
  #mbr.addWidget(cbx)
  #qom = QtGui.QCompleter()
  #qom.setModel(sim)
  #qom.setPopup(lsv)
  #qom.setParent(le)
  #def onChanged(x):
  #  print(x)
  #  lsv.show()
  #  #qom.complete()
  #le.textChanged.connect(onChanged)

  
  #mw.findChildren(QtGui.QToolBar, 'XternalApplications')
  #mw.findChildren(QtGui.QToolBar, 'XternalApplications')[0]



  #
  #cbx.setCurrentText('')
  #mwx.show()


{
  'File': ['Std_New', 'Std_Open', 'Std_Save', 'Std_Print', 'Separator', 'Std_Cut', 'Std_Copy', 'Std_Paste', 'Separator', 'Std_Undo', 'Std_Redo', 'Separator', 'Std_Refresh', 'Separator', 'Std_WhatsThis'],
  'Workbench': ['Std_Workbench'],
  'Macro': ['Std_DlgMacroRecord', 'Std_MacroStopRecord', 'Std_DlgMacroExecute', 'Std_DlgMacroExecuteDirect'],
  'View': ['Std_ViewFitAll', 'Std_ViewFitSelection', 'Std_DrawStyle', 'Std_SelBoundingBox', 'Separator', 'Std_SelBack', 'Std_SelForward', 'Std_LinkSelectActions', 'Separator', 'Std_TreeViewActions', 'Std_ViewIsometric', 'Separator', 'Std_ViewFront', 'Std_ViewTop', 'Std_ViewRight', 'Separator', 'Std_ViewRear', 'Std_ViewBottom', 'Std_ViewLeft', 'Separator', 'Std_MeasureDistance'],
  'Structure': ['Std_Part', 'Std_Group', 'Std_LinkMake', 'Std_LinkActions'],
  'XternalApplications': ['XternalAppsOpenInkscapeCommand', 'XternalAppsReloadInkscapeCommand', 'XternalAppsToolInkscapeFractalizeCommand']
}

[
  <PySide2.QtWidgets.QLayout(0x560f36f68d60) at 0x7fe626763cc0>,
  <PySide2.QtWidgets.QToolButton(0x560f36d45bf0, name="qt_toolbar_ext_button") at 0x7fe67c316c00>, 
  <PySide2.QtWidgets.QAction(0x560f35673bb0 text="Workbench" toolTip="Workbench" checked=true menuRole=TextHeuristicRole visible=true) at 0x7fe6b9f6a780>,
  <PySide2.QtWidgets.QWidgetAction(0x560f356ab770 text="" menuRole=TextHeuristicRole visible=true) at 0x7fe62673f9c0>, 
  <PySide2.QtWidgets.QComboBox(0x560f35676800) at 0x7fe66d978140>, <PySide2.QtCore.QPropertyAnimation(0x560f37d2ce70) at 0x7fe6b9dfb680>
]



      #def processGroup(group, depth = 0, depthAdded = 0, path = [], includeAll = False):
      #  # group: group of {icon, text, subitems} elements, each element is added if it matches() the userInput
      #  # depth in the hierarchy of subitems (initial group has depth 0)
      #  # depthAdded indicates the position in the path[] of ancestors up to which elements have already been added
      #  # path is a list of ancestors, root at index 0 and parent at index len(path)-1
      #  # includeAll indicates if the whole subtree should be added (e.g. because an ancestor matched())
      #  if group['icon'] is not None:
      #    header = QtGui.QStandardItem(group['icon'], group['text'])
      #  else:
      #    header = QtGui.QStandardItem(group['text'])
      #  headerRow = [header, QtGui.QStandardItem(str(depth)), QtGui.QStandardItem('internal data:' + str(header))]
      #  if depth == len(path):
      #    path.append(headerRow)
      #  else:
      #    path[depth] = headerRow
      #  #
      #  #print(includeAll, header.data(0))
      #  if includeAll or matches(header.data(0)):
      #    includeAll = True
      #    added = True
      #    for ancestorRow in path[depthAdded:depth+1]:
      #      print(ancestorRow[0].data(0), includeAll, depthAdded, depth)
      #      mdl.appendRow(ancestorRow)
      #    depthAdded = depth
      #  else:
      #    added = False
      #  for item in group['subitems']:
      #    # If the group given to processGroup or any of its descendents have been added, then
      #    # the ancestors up to the current depth have been added.
      #    if added:
      #      depthAdded = depth
      #    print('recur:',item,includeAll)
      #    added = added or processGroup(item, depth+1, depthAdded, path, includeAll)
      #  return added
      


      #mdl = QtGui.QStandardItemModel()
      #for group in itemGroups:
      #  mdl.appendRow(QtGui.QStandardItem(*group['header']))
      #  for item in group['items']:
      #    mdl.appendRow(QtGui.QStandardItem(*item))
      #super(SearchQCompleter, self).setModel(mdl)
      #super(SearchQCompleter, self).popup().setItemDelegate(self.itemDelegate)


            #for group in self.itemGroups:
      #  processGroup(group)
      
      #class FilterProxyModel(QtCore.QSortFilterProxyModel):
      #  def filterAcceptsRow(self, sourceRow, sourceParent):
      #    #mdl = self.sourceModel
      #    #label = mdl.data(mdl.index(sourceRow, 0, sourceParent))
      #    #metadata = mdl.data(mdl.index(sourceRow, 1, sourceParent))
      #    #return metadata == 'toolbar' or userInput.lower() in label.lower()
      #    return True
      #filteredProxyModel = FilterProxyModel()
      #filteredProxyModel.setSourceModel(mdl)


















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

                #sim.appendRow([QtGui.QStandardItem(icon, 't:' + text), QtGui.QStandardItem('tool')])

                      #all_actions.append(mac.trigger)

                      #print('whaaaat', str(len(all_actions)))

                #all_actions.append(tbt.actions().trigger)
                #global lalala
                #lalala=tbt
                #print('whuuuut', str(len(all_actions)))

                #viu = mw.findChildren(QtGui.QToolBar, 'View')[0]
                #tbt = viu.findChildren(QtGui.QToolButton)
                #men = tbt[3].menu()
                #acs = men.actions()          # QtGui.QAction list
                #act = tbt[2].defaultAction() # QtGui.QAction
                #act.trigger()
