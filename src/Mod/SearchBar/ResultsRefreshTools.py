import os
from PySide import QtGui
import Serialize

def refreshToolsAction(nfo):
  import RefreshTools
  RefreshTools.refreshToolsAction()

def refreshToolsToolTip(nfo, setParent):
  return Serialize.iconToHTML(genericToolIcon) + '<p>Load all workbenches to refresh this list of tools. This may take a minute, depending on the number of installed workbenches.</p>'

genericToolIcon = QtGui.QIcon(QtGui.QIcon(os.path.dirname(__file__) + '/Tango-Tools-spanner-hammer.svg'))
def refreshToolsResultsProvider():
  return [
    {
      'icon': genericToolIcon,
      'text': 'Refresh list of tools',
      'toolTip': '',
      'action': {'handler': 'refreshTools'},
      'subitems': []
    }
  ]