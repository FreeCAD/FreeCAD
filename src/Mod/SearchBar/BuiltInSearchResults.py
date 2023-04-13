# You can add your own result proviers and action/tooltip handlers, by importing this module and calling the registration functions as follows.
# We use wrapper functions which import the actual implementation and call it, in order to avoid loading too much code during startup.

import SearchResults

SearchResults.registerResultProvider('refreshTools',
                                     getItemGroupsCached   = lambda: __import__('ResultsRefreshTools').refreshToolsResultsProvider(),
                                     getItemGroupsUncached = lambda: [])
SearchResults.registerResultProvider('document',
                                     getItemGroupsCached   = lambda: [],
                                     getItemGroupsUncached = lambda: __import__('ResultsDocument').documentResultsProvider())
SearchResults.registerResultProvider('toolbar',
                                     getItemGroupsCached   = lambda: __import__('ResultsToolbar').toolbarResultsProvider(),
                                     getItemGroupsUncached = lambda: [])
SearchResults.registerResultProvider('param',
                                     getItemGroupsCached   = lambda: __import__('ResultsPreferences').paramResultsProvider(),
                                     getItemGroupsUncached = lambda: [])

SearchResults.registerResultHandler('refreshTools',
                                    action  = lambda nfo:            __import__('ResultsRefreshTools').refreshToolsAction(nfo),
                                    toolTip = lambda nfo, setParent: __import__('ResultsRefreshTools').refreshToolsToolTip(nfo, setParent))
SearchResults.registerResultHandler('toolbar',
                                    action  = lambda nfo:            __import__('ResultsToolbar').toolbarAction(nfo),
                                    toolTip = lambda nfo, setParent: __import__('ResultsToolbar').toolbarToolTip(nfo, setParent))
SearchResults.registerResultHandler('tool',
                                    action  = lambda nfo           : __import__('ResultsToolbar').subToolAction(nfo),
                                    toolTip = lambda nfo, setParent: __import__('ResultsToolbar').subToolToolTip(nfo, setParent))
SearchResults.registerResultHandler('subTool',
                                    action  = lambda nfo           : __import__('ResultsToolbar').subToolAction(nfo),
                                    toolTip = lambda nfo, setParent: __import__('ResultsToolbar').subToolToolTip(nfo, setParent))
SearchResults.registerResultHandler('document',
                                    action  = lambda nfo           : __import__('ResultsDocument').documentAction(nfo),
                                    toolTip = lambda nfo, setParent: __import__('ResultsDocument').documentToolTip(nfo, setParent))
SearchResults.registerResultHandler('documentObject',
                                    action  = lambda nfo           : __import__('ResultsDocument').documentObjectAction(nfo),
                                    toolTip = lambda nfo, setParent: __import__('ResultsDocument').documentObjectToolTip(nfo, setParent))
SearchResults.registerResultHandler('param',
                                    action  = lambda nfo           : __import__('ResultsPreferences').paramAction(nfo),
                                    toolTip = lambda nfo, setParent: __import__('ResultsPreferences').paramToolTip(nfo, setParent))
SearchResults.registerResultHandler('paramGroup',
                                    action  = lambda nfo           : __import__('ResultsPreferences').paramGroupAction(nfo),
                                    toolTip = lambda nfo, setParent: __import__('ResultsPreferences').paramGroupToolTip(nfo, setParent))
