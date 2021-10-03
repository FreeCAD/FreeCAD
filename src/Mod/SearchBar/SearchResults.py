actionHandlers = { }
toolTipHandlers = { }
resultProvidersCached = { }
resultProvidersUncached = { }

# name : string
# getItemGroupsCached: () -> [itemGroup]
# getItemGroupsUncached: () -> [itemGroup]
def registerResultProvider(name, getItemGroupsCached, getItemGroupsUncached):
  resultProvidersCached[name] = getItemGroupsCached
  resultProvidersUncached[name] = getItemGroupsUncached

# name : str
# action : act -> None
# toolTip : groupId, setParent -> (str or QWidget)
def registerResultHandler(name, action, toolTip):
  actionHandlers[name] = action
  toolTipHandlers[name] = toolTip
