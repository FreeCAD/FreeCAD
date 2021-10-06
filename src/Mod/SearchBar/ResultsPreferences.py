print("Loaded file ResultsPreferences.py")
import os
import FreeCAD as App
from PySide import QtGui
import Serialize

genericToolIcon = QtGui.QIcon(QtGui.QIcon(os.path.dirname(__file__) + '/Tango-Tools-spanner-hammer.svg'))

def getParamGroups(nameInConfig, nameInPath):
    userParameterPath = App.ConfigGet(nameInConfig)
    from lxml import etree
    xml = etree.parse(userParameterPath).getroot()
    xml.find('FCParamGroup[@Name="Root"]')
    root = xml.find('FCParamGroup[@Name="Root"]')
    def recur(atRoot, path, tree):
        if not atRoot:
            yield path
        for child in tree.getchildren():
            if child.tag == 'FCParamGroup':
                for descendant in recur(False, path + (':' if atRoot else '/') + child.attrib['Name'], child):
                    yield descendant
    return recur(True, nameInPath, root)

def getParams(nameInConfig, nameInPath):
    for grpPath in getParamGroups(nameInConfig, nameInPath):
        grp = App.ParamGet(grpPath)
        contents = grp.GetContents()
        if contents is not None:
            for (type_, name, value) in contents:
                yield (grpPath, type_, name)

def getAllParams():
  def getParam(p):
    return {
      'icon': genericToolIcon,
      'text': p[0] + '/' + p[2],
      'toolTip': '',
      'action': {'handler': 'param', 'path': p[0], 'type': p[1], 'name': p[2]},
      'subitems': []
    }
  return [getParam(p) for p in getParams('UserParameter', 'User parameter')]

getAllParams()

def paramAction(nfo):
  import RefreshTools
  RefreshTools.refreshToolsAction()

def paramToolTip(nfo, setParent):
  path = nfo['action']['path']
  type_ = nfo['action']['type']
  name = nfo['action']['name']
  getters = {
    'Boolean'      : 'GetBool',
    'Float'        : 'GetFloat',
    'Integer'      : 'GetInt',
    'String'       : 'GetString',
    'Unsigned Long': 'GetUnsigned',
  }
  try:
    value = getattr(App.ParamGet(path), getters[type_])(name)
  except:
    value = 'An error occurred while attempting to access this value.'
  return '<p><code>App.ParamGet(' + repr(path) + ').' + getters[type_] + '(' + repr(name) + ')</code></p><p>Type: ' + type_ + '</p><p>Value: ' + repr(value) + '</p>'

def paramResultsProvider():
  return getAllParams()
