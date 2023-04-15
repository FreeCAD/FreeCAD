from PySide import QtGui
import FreeCADGui
import Serialize

def toolbarAction(nfo):
  act = nfo['action']
  print('show toolbar ' + act['toolbar'] + ' from workbenches ' + repr(act['workbenches']))

def subToolAction(nfo):
  act = nfo['action']
  toolPath = act['toolbar'] + '.' + act['tool']
  if 'subTool' in act:
    toolPath = toolPath + '.' + act['subTool']
  def runTool():
    mw = FreeCADGui.getMainWindow()
    for the_toolbar in mw.findChildren(QtGui.QToolBar, act['toolbar']):
      for tbt in the_toolbar.findChildren(QtGui.QToolButton):
        if tbt.text() == act['tool']:
          action = None
          if 'subTool' in act:
            men = tbt.menu()
            if men:
              for mac in men.actions():
                if mac.text() == act['subTool']:
                  action = mac
                  break
          else:
            action = tbt.defaultAction()
          if 'showMenu' in act and act['showMenu']:
            print('Popup submenu of tool ' + toolPath + ' available in workbenches ' + repr(act['workbenches']))
            the_toolbar.show()
            tbt.showMenu()
            return True
          elif action is not None:
            print('Run action of tool ' + toolPath + ' available in workbenches ' + repr(act['workbenches']))
            action.trigger()
            return True
    return False
  if runTool():
    return
  else:
    for workbench in act['workbenches']:
      print('Activating workbench ' + workbench + ' to access tool ' + toolPath)
      FreeCADGui.activateWorkbench(workbench)
      if runTool():
        return
  print('Tool ' + toolPath + ' not found, was it offered by an extension that is no longer present?')

def toolbarToolTip(nfo, setParent):
  workbenches = FreeCADGui.listWorkbenches()
  in_workbenches = ['<li>' + (Serialize.iconToHTML(QtGui.QIcon(workbenches[wb].Icon)) if wb in workbenches else '? ') + wb + '</li>' for wb in nfo['action']['workbenches']]
  return '<p>Show the ' + nfo['text'] + ' toolbar</p><p>This toolbar appears in the following workbenches: <ul>' + ''.join(in_workbenches) + '</ul></p>'

def subToolToolTip(nfo, setParent):
  return Serialize.iconToHTML(nfo['icon'], 32) + '<p>' + nfo['toolTip'] + '</p>'

def getAllToolbars():
  all_tbs = dict()
  for wbname, workbench in FreeCADGui.listWorkbenches().items():
    try:
      tbs = workbench.listToolbars()
    except:
      continue
    # careful, tbs contains all the toolbars of the workbench, including shared toolbars
    for tb in tbs:
      if tb not in all_tbs:
        all_tbs[tb] = set()
      all_tbs[tb].add(wbname)
  return all_tbs

def toolbarResultsProvider():
  itemGroups = []
  all_tbs = getAllToolbars()
  mw = FreeCADGui.getMainWindow()
  for toolbarName, toolbarIsInWorkbenches in all_tbs.items():
    toolbarIsInWorkbenches = sorted(list(toolbarIsInWorkbenches))
    for the_toolbar in mw.findChildren(QtGui.QToolBar, toolbarName):
      group = []
      for tbt in the_toolbar.findChildren(QtGui.QToolButton):
        text = tbt.text()
        act = tbt.defaultAction()
        if text != '':
            # TODO: there also is the tooltip
            icon = tbt.icon()
            men = tbt.menu()
            subgroup = []
            if men:
              subgroup = []
              for mac in men.actions():
                if mac.text():
                  action = { 'handler': 'subTool', 'workbenches': toolbarIsInWorkbenches, 'toolbar': toolbarName, 'tool': text, 'subTool': mac.text() }
                  subgroup.append({'icon':mac.icon(), 'text':mac.text(), 'toolTip': mac.toolTip(), 'action':action, 'subitems':[]})
            # The default action of a menu changes dynamically, instead of triggering the last action, just show the menu.
            action = { 'handler': 'tool', 'workbenches': toolbarIsInWorkbenches, 'toolbar': toolbarName, 'tool': text, 'showMenu': bool(men) }
            group.append({'icon':icon, 'text':text, 'toolTip': tbt.toolTip(), 'action': action, 'subitems': subgroup})
      # TODO: move the 'workbenches' field to the itemgroup
      action = { 'handler': 'toolbar', 'workbenches': toolbarIsInWorkbenches, 'toolbar': toolbarName }
      itemGroups.append({
        'icon': QtGui.QIcon(':/icons/Group.svg'),
        'text': toolbarName,
        'toolTip': '',
        'action': action,
        'subitems': group
      })
  return itemGroups
