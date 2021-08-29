
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
