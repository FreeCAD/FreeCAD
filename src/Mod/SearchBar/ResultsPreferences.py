import os
import FreeCAD as App
import FreeCADGui
from PySide import QtGui
import Serialize

genericToolIcon = QtGui.QIcon(QtGui.QIcon(os.path.dirname(__file__) + '/Tango-Tools-spanner-hammer.svg'))

def getParam(grpPath, type_, name):
  return {
    # TODO: use letter icon based on the type, as the preferences editor does
    'icon': genericToolIcon,
    'text': name,
    'toolTip': '',
    'action': {'handler': 'param', 'path': grpPath, 'type': type_, 'name': name},
    'subitems': []
  }

def getParamGroup(grpPath):
  try:
    grp = App.ParamGet(grpPath)
  except:
    return []
  contents = grp.GetContents()
  if contents is not None:
    return [getParam(grpPath, type_, name) for (type_, name, value) in contents]
  else:
    return []

def getParamGroups(nameInConfig, nameInPath):
    userParameterPath = App.ConfigGet(nameInConfig)
    from lxml import etree
    xml = etree.parse(userParameterPath).getroot()
    xml.find('FCParamGroup[@Name="Root"]')
    root = xml.find('FCParamGroup[@Name="Root"]')
    def recur(atRoot, path, name, tree):
      params = [] if atRoot else getParamGroup(path)
      subgroups = [recur(False, path + (':' if atRoot else '/') + child.attrib['Name'], child.attrib['Name'], child) for child in tree.getchildren() if child.tag == 'FCParamGroup']
      return {
        'icon': QtGui.QIcon(':/icons/Group.svg'),
        'text': name,
        'toolTip': '',
        'action': { 'handler': 'paramGroup', 'path': path, 'name': name },
        'subitems': params + subgroups
      }
    return recur(True, nameInPath, nameInPath, root)

def getAllParams():
  try:
    from lxml import etree
    return [getParamGroups('UserParameter', 'User parameter')]
  except:
    print('Could not load the list of all parameters. Please install the LXML python library with:\npython -m pip install --upgrade lxml')
    return []

def paramGroupAction(nfo):
  FreeCADGui.runCommand('Std_DlgParameter',0)
  print('Open Parameter Editor (parameter group)')
  # TODO: find a way to select the desired group in the parameter dialog once it opens

def paramAction(nfo):
  FreeCADGui.runCommand('Std_DlgParameter',0)
  print('Open Parameter Editor (single parameter)')
  # TODO: find a way to select the desired parameter in the parameter dialog once it opens

getters = {
  'Boolean'      : 'GetBool',
  'Float'        : 'GetFloat',
  'Integer'      : 'GetInt',
  'String'       : 'GetString',
  'Unsigned Long': 'GetUnsigned',
}

def paramGroupToolTip(nfo, setParent):
  path = nfo['action']['path']
  name = nfo['action']['name']
  return '<h1>' + name + '</h1><p><code>App.ParamGet(' + repr(path) + ')</code></p'

def paramToolTip(nfo, setParent):
  path = nfo['action']['path']
  type_ = nfo['action']['type']
  name = nfo['action']['name']
  try:
    value = getattr(App.ParamGet(path), getters[type_])(name)
  except:
    value = 'An error occurred while attempting to access this value.'
  return '<p><code>App.ParamGet(' + repr(path) + ').' + getters[type_] + '(' + repr(name) + ')</code></p><p>Type: ' + type_ + '</p><p>Value: ' + repr(value) + '</p>'

def paramResultsProvider():
  return getAllParams()
