from PySide import QtGui
from PySide import QtCore

# This is a "light" version of the SearchBox implementation, which loads the actual implementation on first click
class SearchBoxLight(QtGui.QLineEdit):
  resultSelected = QtCore.Signal(int, int)
  def __init__(self, getItemGroups, getToolTip, getItemDelegate, maxVisibleRows = 20, parent = None):
    self.isInitialized = False

    # Store arguments
    self.getItemGroups = getItemGroups
    self.getToolTip = getToolTip
    self.getItemDelegate = getItemDelegate
    self.maxVisibleRows = maxVisibleRows

    # Call parent constructor
    super(SearchBoxLight, self).__init__(parent)
    # Connect signals and slots
    self.textChanged.connect(self.filterModel)
    # Thanks to https://saurabhg.com/programming/search-box-using-qlineedit/ for indicating a few useful options
    ico = QtGui.QIcon(':/icons/help-browser.svg')
    #ico = QtGui.QIcon(':/icons/WhatsThis.svg')
    self.addAction(ico, QtGui.QLineEdit.LeadingPosition)
    self.setClearButtonEnabled(True)
    self.setPlaceholderText('Search tools, prefs & tree')
    self.setFixedWidth(200) # needed to avoid a change of width when the clear button appears/disappears
  def lazyInit(self):
    pass
  def __getattr__(self, name):
    import types
    def f(*args, **kwargs):
      import SearchBox
      SearchBox.SearchBox.lazyInit(self)
      return getattr(SearchBox.SearchBox, name)(*args, **kwargs)
    return types.MethodType(f, self)
  def focusInEvent(self, *args, **kwargs): return self.proxyFocusInEvent(*args, **kwargs)
  def focusOutEvent(self, *args, **kwargs): return self.proxyFocusOutEvent(*args, **kwargs)
  def keyPressEvent(self, *args, **kwargs): return self.proxyKeyPressEvent(*args, **kwargs)
  def onSelectionChanged(self, *args, **kwargs): return self.proxyOnSelectionChanged(*args, **kwargs)
  def filterModel(self, *args, **kwargs): return self.proxyFilterModel(*args, **kwargs)
  def listDown(self, *args, **kwargs): return self.proxyListDown(*args, **kwargs)
  def listUp(self, *args, **kwargs): return self.proxyListUp(*args, **kwargs)
  def listPageDown(self, *args, **kwargs): return self.proxyListPageDown(*args, **kwargs)
  def listPageUp(self, *args, **kwargs): return self.proxyListPageUp(*args, **kwargs)
  def listEnd(self, *args, **kwargs): return self.proxyListEnd(*args, **kwargs)
  def listStart(self, *args, **kwargs): return self.proxyListStart(*args, **kwargs)
  def listAccept(self, *args, **kwargs): return self.proxyListAccept(*args, **kwargs)
  def listAcceptToggle(self, *args, **kwargs): return self.proxyListAcceptToggle(*args, **kwargs)
  def listCancel(self, *args, **kwargs): return self.proxyListCancel(*args, **kwargs)
