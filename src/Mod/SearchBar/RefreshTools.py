import os
import FreeCAD as App

def loadAllWorkbenches():
  from PySide import QtGui
  import FreeCADGui
  activeWorkbench = FreeCADGui.activeWorkbench().name()
  lbl = QtGui.QLabel('Loading workbench … (…/…)')
  lbl.show()
  lst = FreeCADGui.listWorkbenches()
  for i, wb in enumerate(lst):
    msg = 'Loading workbench ' + wb + ' (' + str(i) + '/' + str(len(lst)) + ')'
    print(msg)
    lbl.setText(msg)
    geo = lbl.geometry()
    geo.setSize(lbl.sizeHint())
    lbl.setGeometry(geo)
    lbl.repaint()
    FreeCADGui.updateGui() # Probably slower with this, because it redraws the entire GUI with all tool buttons changed etc. but allows the label to actually be updated, and it looks nice and gives a quick overview of all the workbenches…
    try:
      FreeCADGui.activateWorkbench(wb)
    except:
      pass
  lbl.hide()
  FreeCADGui.activateWorkbench(activeWorkbench)

def cachePath():
  return os.path.join(App.getUserAppDataDir(), 'Cache_SearchBarMod')

def gatherTools():
  itemGroups = []
  import SearchResults
  for providerName, provider in SearchResults.resultProvidersCached.items():
    itemGroups = itemGroups + provider()
  return itemGroups

def writeCacheTools():
  import Serialize
  serializedItemGroups = Serialize.serialize(gatherTools())
  # Todo: use wb and a specific encoding.
  with open(cachePath(), 'w') as cache:
    cache.write(serializedItemGroups)
  # I prefer to systematically deserialize, instead of taking the original version,
  # this avoids possible inconsistencies between the original and the cache and
  # makes sure cache-related bugs are noticed quickly.
  import Serialize
  itemGroups = Serialize.deserialize(serializedItemGroups)
  print('SearchBox: Cache has been written.')
  return itemGroups

def readCacheTools():
  # Todo: use rb and a specific encoding.
  with open(cachePath(), 'r') as cache:
    serializedItemGroups = cache.read()
  import Serialize
  itemGroups = Serialize.deserialize(serializedItemGroups)
  print('SearchBox: Tools were loaded from the cache.')
  return itemGroups


def refreshToolbars(doLoadAllWorkbenches = True):
  if doLoadAllWorkbenches:
    loadAllWorkbenches()
    return writeCacheTools()
  else:
    try:
      return readCacheTools()
    except:
      return writeCacheTools()

def refreshToolsAction():
  from PySide import QtGui
  print('Refresh list of tools')
  fw = QtGui.QApplication.focusWidget()
  if fw is not None:
    fw.clearFocus()
  reply = QtGui.QMessageBox.question(None, "Load all workbenches?", "Load all workbenches? This can cause FreeCAD to become unstable, and this \"reload tools\" feature contained a bug that crashed freecad systematically, so please make sure you save your work before. It's a good idea to restart FreeCAD after this operation.", QtGui.QMessageBox.Yes, QtGui.QMessageBox.No)
  if reply == QtGui.QMessageBox.Yes:
    refreshToolbars()
  else:
    print('cancelled')
