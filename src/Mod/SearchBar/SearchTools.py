if True:
  from PySide import QtGui
  mw = Gui.getMainWindow()
  mdi = mw.findChild(QtGui.QMdiArea)
  
  mw.findChildren(QtGui.QToolBar, 'XternalApplications')
  mw.findChildren(QtGui.QToolBar, 'XternalApplications')[0]

  wdg = QtGui.QWidget()
  lay = QtGui.QGridLayout(wdg)
  mwx = QtGui.QMainWindow()

  sea = QtGui.QLineEdit()
  lay.addWidget(sea)

  lsv = QtGui.QListView()
  sim = QtGui.QStandardItemModel()
  flt = QtCore.QSortFilterProxyModel()
  flt.setSourceModel(sim)
  flt.setFilterCaseSensitivity(QtCore.Qt.CaseSensitivity.CaseInsensitive)
  sea.textChanged.connect(flt.setFilterWildcard)
  # make the QListView non-editable
  lsv.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
  lsv.setModel(flt)
  lay.addWidget(lsv)

  mwx.setCentralWidget(wdg)
  mdi.addSubWindow(mwx)

  all_tbs = set()
  for wbname, workbench in Gui.listWorkbenches().items():
    try:
      tbs = workbench.listToolbars()
    except:
      continue
    # careful, tbs contains all the toolbars of the workbench, including shared toolbars
    for tb in tbs:
      all_tbs.add(tb)
    
  for toolbar_name in all_tbs:
    for the_toolbar in mw.findChildren(QtGui.QToolBar, toolbar_name):
        #header = QtGui.QPushButton(toolbar_name)
        #lay.addWidget(header)
        
        #sim.insertRow(sim.rowCount())
        #sim.setData(sim.index(sim.rowCount() - 1, 0), toolbar_name)
        sim.appendRow(QtGui.QStandardItem(toolbar_name))

        for bt in the_toolbar.findChildren(QtGui.QToolButton):
          text = bt.text()
          if text != '':
              print(text)
              # TODO: there also is the tooltip
              icon = bt.icon()
              
              # To preview the icon, assign it as the icon of a dummy button.
              #but3 = QtGui.QPushButton(text)
              #but3.setIcon(icon)
              #lay.addWidget(but3)

              #slm.insertRow(slm.rowCount())
              #slm.setData(slm.index(slm.rowCount() - 1, 0), icon)
              #slm.setData(slm.index(slm.rowCount() - 1, 1), text)
              sim.appendRow(QtGui.QStandardItem(icon, text))

              #mwx = QtGui.QMainWindow()
              #mwx.show()
              #mdi.addSubWindow(mwx)
              #mdi.setWindowIcon(icon) # probably sets the default icon to use for windows without an icon?
              mwx.setWindowIcon(icon) # untested
  mwx.show()

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



from PySide import QtGui
qwd = QtGui.QWidget()
but1 = QtGui.QPushButton("hi")
but2 = QtGui.QPushButton("hello")
lay = QtGui.QGridLayout(qwd)
lay.addWidget(but1)
lay.addWidget(but2)
mwx = QtGui.QMainWindow()
mwx.setCentralWidget(qwd)
mw = Gui.getMainWindow()
mdi = mw.findChild(QtGui.QMdiArea)
mdi.addSubWindow(mwx)
mwx.show()
but3 = QtGui.QPushButton("XXX")
lay.addWidget(but3)
