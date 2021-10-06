print("Loaded file InitGui.py")
# Avoid garbage collection by storing the action in a global variable
wax = None

def addToolSearchBox():
  import FreeCADGui
  from PySide import QtGui
  import SearchBoxLight
  global wax, sea
  mw = FreeCADGui.getMainWindow()
  mbr = mw.findChildren(QtGui.QToolBar, 'File')
  # The toolbar will be unavailable if this file is loaded during startup, because no workbench is active and no toolbars are visible.
  if len(mbr) > 0:
    # Get the first toolbar named 'File', and add 
    mbr = mbr[0]
    # Create search box widget
    sea = SearchBoxLight.SearchBoxLight(getItemGroups   = lambda: __import__('GetItemGroups').getItemGroups(),
                                        getToolTip      = lambda groupId, setParent: __import__('GetItemGroups').getToolTip(groupId, setParent),
                                        getItemDelegate = lambda: __import__('IndentedItemDelegate').IndentedItemDelegate())
    sea.resultSelected.connect(lambda index, groupId: __import__('GetItemGroups').onResultSelected(index, groupId))
    wax = QtGui.QWidgetAction(None)
    wax.setWhatsThis('Use this search bar to find tools, document objects, preferences and more')
    wax.setDefaultWidget(sea)
    #mbr.addWidget(sea)
    mbr.addAction(wax)

addToolSearchBox()
import FreeCADGui
FreeCADGui.getMainWindow().workbenchActivated.connect(addToolSearchBox)
