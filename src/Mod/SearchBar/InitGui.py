# Avoid garbage collection by storing the action in a global variable
wax = None

def addToolSearchBox():
  import FreeCADGui
  from PySide import QtGui
  import SearchBoxLight
  global wax, sea
  mw = FreeCADGui.getMainWindow()
  if mw:
    sea = SearchBoxLight.SearchBoxLight(getItemGroups   = lambda: __import__('GetItemGroups').getItemGroups(),
                                        getToolTip      = lambda groupId, setParent: __import__('GetItemGroups').getToolTip(groupId, setParent),
                                        getItemDelegate = lambda: __import__('IndentedItemDelegate').IndentedItemDelegate())
    sea.resultSelected.connect(lambda index, groupId: __import__('GetItemGroups').onResultSelected(index, groupId))
    wax = QtGui.QWidgetAction(None)
    wax.setWhatsThis('Use this search bar to find tools, document objects, preferences and more')
    sea.setWhatsThis('Use this search bar to find tools, document objects, preferences and more')
    wax.setDefaultWidget(sea)
    ##mbr.addWidget(sea)
    #mbr.addAction(wax)
    tbr = QtGui.QToolBar("SearchBar") #QtGui.QDockWidget()
    # Include FreeCAD in the name so that one can find windows labeled with FreeCAD easily in window managers which allow search through the list of open windows.
    tbr.setObjectName("SearchBar")
    tbr.addAction(wax)
    mw.addToolBar(tbr)
    tbr.show()

addToolSearchBox()
import FreeCADGui
FreeCADGui.getMainWindow().workbenchActivated.connect(addToolSearchBox)
