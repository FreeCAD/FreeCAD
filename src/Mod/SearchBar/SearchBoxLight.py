print("Loaded file SearchBoxLight.py")
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

    # Call parent cosntructor
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
  def focusInEvent(self, *args, **kwargs):
    self.proxyFocusInEvent(*args, **kwargs)
  def focusOutEvent(self, *args, **kwargs):
    self.proxyFocusOutEvent(*args, **kwargs)
  def keyPressEvent(self, *args, **kwargs):
    self.proxyKeyPressEvent(*args, **kwargs)
  def onSelectionChanged(self, *args, **kwargs):
    self.proxyOnSelectionChanged(*args, **kwargs)
  def filterModel(self, *args, **kwargs):
    self.proxyFilterModel(*args, **kwargs)

#    .focusInEvent(self, qFocusEvent)
#    
#  def focusInEvent(self, qFocusEvent):
#  def focusOutEvent(self, qFocusEvent):
#    import SearchBox
#    SearchBox.SearchBox.lazyInit(self)
#    SearchBox.SearchBox.focusOutEvent(self, qFocusEvent)
#  def keyPressEvent(self, qKeyEvent):
#    import SearchBox
#    SearchBox.SearchBox.lazyInit(self)
#    SearchBox.SearchBox.keyPressEvent(self, qKeyEvent)
#  def showList(self):
#    import SearchBox
#    SearchBox.SearchBox.lazyInit(self)
#    SearchBox.SearchBox.showList(self)
#  def hideList(self):
#    import SearchBox
#    SearchBox.SearchBox.lazyInit(self)
#    SearchBox.SearchBox.hideList(self)
#  def hideExtraInfo(self):
#    import SearchBox
#    SearchBox.SearchBox.lazyInit(self)
#    SearchBox.SearchBox.hideExtraInfo(self)
#  def showExtraInfo(self):
#    import SearchBox
#    SearchBox.SearchBox.lazyInit(self)
#    SearchBox.SearchBox.showExtraInfo(self)
#  def filterModel(self, userInput):
#    import SearchBox
#    SearchBox.SearchBox.lazyInit(self)
#    SearchBox.SearchBox.filterModel(self, userInput)