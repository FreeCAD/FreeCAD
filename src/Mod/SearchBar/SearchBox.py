if True:
    from PySide import QtGui
    class SearchBox(QtGui.QLineEdit):
        def __init__(self, model, maxVisibleRows, parent):
            # Call parent cosntructor
            super(SearchBox, self).__init__(parent)
            # Save arguments
            self.model = model
            self.maxVisibleRows = maxVisibleRows
            # Create list view
            self.listView = QtGui.QListView(self)
            self.listView.setWindowFlags(QtGui.Qt.ToolTip)
            self.listView.setWindowFlag(QtGui.Qt.FramelessWindowHint)
            self.listView.setModel(self.model)
            # make the QListView non-editable
            self.listView.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
            # Connect signals and slots
            self.textChanged.connect(self.filterList)
            self.listView.clicked.connect(self.selectResult)
            self.listView.selectionModel().selectionChanged.connect(self.showExtraInfo)
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
                print(geo)
                parent = widget.parent()
                parentPos = getScreenPosition(parent) if parent is not None else QtCore.QPoint(0,0)
                return QtCore.QPoint(geo.x() + parentPos.x(), geo.y() + parentPos.y())
            pos = getScreenPosition(self)
            siz = self.size()
            screen = QtGui.QGuiApplication.screenAt(pos)
            print(pos, siz, screen)
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
            self.filterList(self.text())
        def filterList(self, query):
            print('TODO: do the actual filtering')
            flt = QtCore.QSortFilterProxyModel()
            flt.setSourceModel(self.model)
            flt.setFilterCaseSensitivity(QtCore.Qt.CaseSensitivity.CaseInsensitive)
            flt.setFilterWildcard(query)
            self.listView.setModel(flt)
            # TODO: try to find the already-highlighted item
            nbRows = self.listView.model().rowCount()
            if nbRows > 0:
                index = self.listView.model().index(0, 0)
                self.listView.setCurrentIndex(index)
            #self.showList()
        def showExtraInfo(selected, deselected):
            print('show extra info...')
            pass
    mdl = QtCore.QStringListModel(['aaa', 'aab', 'aac', 'bxy', 'bac'])
    sbx = SearchBox(mdl, 10, None)
    sbx.show()