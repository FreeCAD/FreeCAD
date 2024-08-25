from PySide import QtCore
from PySide import QtGui
import json

def iconToBase64(icon, sz = QtCore.QSize(64,64), mode = QtGui.QIcon.Mode.Normal, state = QtGui.QIcon.State.On):
  buf = QtCore.QBuffer()
  buf.open(QtCore.QIODevice.WriteOnly)
  icon.pixmap(sz, mode, state).save(buf, 'PNG')
  return QtCore.QTextCodec.codecForName('UTF-8').toUnicode(buf.data().toBase64())

def iconToHTML(icon, sz = 12, mode = QtGui.QIcon.Mode.Normal, state = QtGui.QIcon.State.On):
  return '<img width="'+str(sz)+'" height="'+str(sz)+'" src="data:image/png;base64,' + iconToBase64(icon, QtCore.QSize(sz,sz), mode, state) + '" />'

def serializeIcon(icon):
  iconPixmaps = {}
  for sz in icon.availableSizes():
    strW = str(sz.width())
    strH = str(sz.height())
    iconPixmaps[strW] = {}
    iconPixmaps[strW][strH] = {}
    for strMode, mode in {'normal':QtGui.QIcon.Mode.Normal, 'disabled':QtGui.QIcon.Mode.Disabled, 'active':QtGui.QIcon.Mode.Active, 'selected':QtGui.QIcon.Mode.Selected}.items():
      iconPixmaps[strW][strH][strMode] = {}
      for strState, state in {'off':QtGui.QIcon.State.Off, 'on':QtGui.QIcon.State.On}.items():
        iconPixmaps[strW][strH][strMode][strState] = iconToBase64(icon, sz, mode, state)
  return iconPixmaps

# workbenches is a list(str), toolbar is a str, text is a str, icon is a QtGui.QIcon
def serializeTool(tool):
  return {
    'workbenches': tool['workbenches'],
    'toolbar': tool['toolbar'],
    'text': tool['text'],
    'toolTip': tool['toolTip'],
    'icon': serializeIcon(tool['icon']),
  }

def deserializeIcon(iconPixmaps):
  ico = QtGui.QIcon()
  for strW, wPixmaps in iconPixmaps.items():
    for strH, hPixmaps in wPixmaps.items():
      for strMode, modePixmaps in hPixmaps.items():
        mode = {'normal':QtGui.QIcon.Mode.Normal, 'disabled':QtGui.QIcon.Mode.Disabled, 'active':QtGui.QIcon.Mode.Active, 'selected':QtGui.QIcon.Mode.Selected}[strMode]
        for strState, statePixmap in modePixmaps.items():
          state = {'off':QtGui.QIcon.State.Off, 'on':QtGui.QIcon.State.On}[strState]
          pxm = QtGui.QPixmap()
          pxm.loadFromData(QtCore.QByteArray.fromBase64(QtCore.QTextCodec.codecForName('UTF-8').fromUnicode(statePixmap)))
          ico.addPixmap(pxm, mode, state)
  return ico

def deserializeTool(tool):
  return {
    'workbenches': tool['workbenches'],
    'toolbar': tool['toolbar'],
    'text': tool['text'],
    'toolTip': tool['toolTip'],
    'icon': deserializeIcon(tool['icon']),
  }

def serializeItemGroup(itemGroup):
  return {
    'icon': serializeIcon(itemGroup['icon']),
    'text': itemGroup['text'],
    'toolTip': itemGroup['toolTip'],
    'action': itemGroup['action'],
    'subitems': serializeItemGroups(itemGroup['subitems'])
  }

def serializeItemGroups(itemGroups):
  return [serializeItemGroup(itemGroup) for itemGroup in itemGroups]

def serialize(itemGroups):
  return json.dumps(serializeItemGroups(itemGroups))

def deserializeItemGroup(itemGroup):
  return {
    'icon': deserializeIcon(itemGroup['icon']),
    'text': itemGroup['text'],
    'toolTip': itemGroup['toolTip'],
    'action': itemGroup['action'],
    'subitems': deserializeItemGroups(itemGroup['subitems'])
  }

def deserializeItemGroups(serializedItemGroups):
  return [deserializeItemGroup(itemGroup) for itemGroup in serializedItemGroups]

def deserialize(serializedItemGroups):
  return deserializeItemGroups(json.loads(serializedItemGroups))
