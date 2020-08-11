from typing import Union
from typing_extensions import Protocol

from DraftGui import DraftToolBar
from draftguitools.gui_snapper import Snapper as SnapperClass

def activateWorkbench() -> None: ...
def addWorkbench() -> None: ...
def removeWorkbench() -> None: ...
def getWorkbench() -> None: ...
def listWorkbenches() -> None: ...
def activeWorkbench() -> None: ...
def addResourcePath() -> None: ...
def addLanguagePath() -> None: ...
def addIconPath() -> None: ...
def addIcon() -> None: ...
def getIcon() -> None: ...
def isIconCached() -> None: ...
def getMainWindow() -> None: ...
def updateGui() -> None: ...
def updateLocale() -> None: ...
def getLocale() -> None: ...
def setLocale() -> None: ...
def supportedLocales() -> None: ...
def createDialog() -> None: ...
def addCommand(name: str, commandObject: Union[PythonCommand, PythonGroupCommand], source: str = ...) -> None: ...
def runCommand() -> None: ...
def SendMsgToActiveView() -> None: ...
def sendMsgToFocusView() -> None: ...
def hide() -> None: ...
def show() -> None: ...
def hideObject() -> None: ...
def showObject() -> None: ...
def open() -> None: ...
def insert() -> None: ...
def export() -> None: ...
def activeDocument() -> None: ...
def activeView() -> None: ...
def activateView() -> None: ...
def editDocument() -> None: ...
def getDocument() -> None: ...
def doCommand() -> None: ...
def doCommandGui() -> None: ...
def addModule() -> None: ...
def showDownloads() -> None: ...
def showPreferences() -> None: ...
def createViewer() -> None: ...
def getMarkerIndex() -> None: ...
def addDocumentObserver() -> None: ...
def removeDocumentObserver() -> None: ...
def reload() -> None: ...
def loadFile() -> None: ...
def coinRemoveAllChildren() -> None: ...

class PythonGroupCommand(Protocol):
    def GetCommands(self) -> None: ...

class PythonCommand(Protocol):
    def GetResources(self) -> None: ...


# The `DraftToolBar` class is defined in the `DraftGui` module
# and globally initialized in the `Gui` namespace
draftToolBar: Union[DraftToolBar, None]
Snapper: Union[SnapperClass, None]

#   {"activateWorkbench",(PyCFunction) Application::sActivateWorkbenchHandler, METH_VARARGS,
#    "activateWorkbench(string) -> None\n\n"
#    "Activate the workbench by name"},
#   {"addWorkbench",     (PyCFunction) Application::sAddWorkbenchHandler, METH_VARARGS,
#    "addWorkbench(string, object) -> None\n\n"
#    "Add a workbench under a defined name."},
#   {"removeWorkbench",  (PyCFunction) Application::sRemoveWorkbenchHandler, METH_VARARGS,
#    "removeWorkbench(string) -> None\n\n"
#    "Remove the workbench with name"},
#   {"getWorkbench",     (PyCFunction) Application::sGetWorkbenchHandler, METH_VARARGS,
#    "getWorkbench(string) -> object\n\n"
#    "Get the workbench by its name"},
#   {"listWorkbenches",   (PyCFunction) Application::sListWorkbenchHandlers, METH_VARARGS,
#    "listWorkbenches() -> list\n\n"
#    "Show a list of all workbenches"},
#   {"activeWorkbench", (PyCFunction) Application::sActiveWorkbenchHandler, METH_VARARGS,
#    "activeWorkbench() -> object\n\n"
#    "Return the active workbench object"},
#   {"addResourcePath",             (PyCFunction) Application::sAddResPath, METH_VARARGS,
#    "addResourcePath(string) -> None\n\n"
#    "Add a new path to the system where to find resource files\n"
#    "like icons or localization files"},
#   {"addLanguagePath",             (PyCFunction) Application::sAddLangPath, METH_VARARGS,
#    "addLanguagePath(string) -> None\n\n"
#    "Add a new path to the system where to find language files"},
#   {"addIconPath",             (PyCFunction) Application::sAddIconPath, METH_VARARGS,
#    "addIconPath(string) -> None\n\n"
#    "Add a new path to the system where to find icon files"},
#   {"addIcon",                 (PyCFunction) Application::sAddIcon, METH_VARARGS,
#    "addIcon(string, string or list) -> None\n\n"
#    "Add an icon as file name or in XPM format to the system"},
#   {"getIcon",                 (PyCFunction) Application::sGetIcon, METH_VARARGS,
#    "getIcon(string) -> QIcon\n\n"
#    "Get an icon in the system"},
#   {"isIconCached",           (PyCFunction) Application::sIsIconCached, METH_VARARGS,
#    "isIconCached(String) -> Bool\n\n"
#    "Check if an icon with the given name is cached"},
#   {"getMainWindow",           (PyCFunction) Application::sGetMainWindow, METH_VARARGS,
#    "getMainWindow() -> QMainWindow\n\n"
#    "Return the main window instance"},
#   {"updateGui",               (PyCFunction) Application::sUpdateGui, METH_VARARGS,
#    "updateGui() -> None\n\n"
#    "Update the main window and all its windows"},
#   {"updateLocale",            (PyCFunction) Application::sUpdateLocale, METH_VARARGS,
#    "updateLocale() -> None\n\n"
#    "Update the localization"},
#   {"getLocale",            (PyCFunction) Application::sGetLocale, METH_VARARGS,
#    "getLocale() -> string\n\n"
#    "Returns the locale currently used by FreeCAD"},
#   {"setLocale",            (PyCFunction) Application::sSetLocale, METH_VARARGS,
#    "getLocale(string) -> None\n\n"
#    "Sets the locale used by FreeCAD. You can set it by\n"
#    "top-level domain (e.g. \"de\") or the language name (e.g. \"German\")"},
#   {"supportedLocales", (PyCFunction) Application::sSupportedLocales, METH_VARARGS,
#    "supportedLocales() -> dict\n\n"
#    "Returns a dict of all supported languages/top-level domains"},
#   {"createDialog",            (PyCFunction) Application::sCreateDialog, METH_VARARGS,
#    "createDialog(string) -- Open a UI file"},
#   {"addPreferencePage",       (PyCFunction) Application::sAddPreferencePage, METH_VARARGS,
#    "addPreferencePage(string,string) -- Add a UI form to the\n"
#    "preferences dialog. The first argument specifies the file name"
#    "and the second specifies the group name"},
#   {"addCommand",              (PyCFunction) Application::sAddCommand, METH_VARARGS,
#    "addCommand(string, object) -> None\n\n"
#    "Add a command object"},
#   {"runCommand",              (PyCFunction) Application::sRunCommand, METH_VARARGS,
#    "runCommand(string) -> None\n\n"
#    "Run command with name"},
#   {"SendMsgToActiveView",     (PyCFunction) Application::sSendActiveView, METH_VARARGS,
#    "deprecated -- use class View"},
#   {"sendMsgToFocusView",     (PyCFunction) Application::sSendFocusView, METH_VARARGS,
#    "send message to the focused view"},
#   {"hide",                    (PyCFunction) Application::sHide, METH_VARARGS,
#    "deprecated"},
#   {"show",                    (PyCFunction) Application::sShow, METH_VARARGS,
#    "deprecated"},
#   {"hideObject",              (PyCFunction) Application::sHideObject, METH_VARARGS,
#    "hideObject(object) -> None\n\n"
#    "Hide the view provider to the given object"},
#   {"showObject",              (PyCFunction) Application::sShowObject, METH_VARARGS,
#    "showObject(object) -> None\n\n"
#    "Show the view provider to the given object"},
#   {"open",                    (PyCFunction) Application::sOpen, METH_VARARGS,
#    "Open a macro, Inventor or VRML file"},
#   {"insert",                  (PyCFunction) Application::sInsert, METH_VARARGS,
#    "Open a macro, Inventor or VRML file"},
#   {"export",                  (PyCFunction) Application::sExport, METH_VARARGS,
#    "save scene to Inventor or VRML file"},
#   {"activeDocument",          (PyCFunction) Application::sActiveDocument, METH_VARARGS,
#    "activeDocument() -> object or None\n\n"
#    "Return the active document or None if no one exists"},
#   {"setActiveDocument",       (PyCFunction) Application::sSetActiveDocument, METH_VARARGS,
#    "setActiveDocument(string or App.Document) -> None\n\n"
#    "Activate the specified document"},
#   {"activeView", (PyCFunction)Application::sActiveView, METH_VARARGS,
#    "activeView(typename=None) -> object or None\n\n"
#    "Return the active view of the active document or None if no one exists" },
#   {"activateView", (PyCFunction)Application::sActivateView, METH_VARARGS,
#    "activateView(type)\n\n"
#    "Activate a view of the given type of the active document"},
#   {"editDocument", (PyCFunction)Application::sEditDocument, METH_VARARGS,
#    "editDocument() -> object or None\n\n"
#    "Return the current editing document or None if no one exists" },
#   {"getDocument",             (PyCFunction) Application::sGetDocument, METH_VARARGS,
#    "getDocument(string) -> object\n\n"
#    "Get a document by its name"},
#   {"doCommand",               (PyCFunction) Application::sDoCommand, METH_VARARGS,
#    "doCommand(string) -> None\n\n"
#    "Prints the given string in the python console and runs it"},
#   {"doCommandGui",               (PyCFunction) Application::sDoCommandGui, METH_VARARGS,
#    "doCommandGui(string) -> None\n\n"
#    "Prints the given string in the python console and runs it but doesn't record it in macros"},
#   {"addModule",               (PyCFunction) Application::sAddModule, METH_VARARGS,
#    "addModule(string) -> None\n\n"
#    "Prints the given module import only once in the macro recording"},
#   {"showDownloads",               (PyCFunction) Application::sShowDownloads, METH_VARARGS,
#    "showDownloads() -> None\n\n"
#    "Shows the downloads manager window"},
#   {"showPreferences",               (PyCFunction) Application::sShowPreferences, METH_VARARGS,
#    "showPreferences([string,int]) -> None\n\n"
#    "Shows the preferences window. If string and int are provided, the given page index in the given group is shown."},
#   {"createViewer",               (PyCFunction) Application::sCreateViewer, METH_VARARGS,
#    "createViewer([int]) -> View3DInventor/SplitView3DInventor\n\n"
#    "shows and returns a viewer. If the integer argument is given and > 1: -> splitViewer"},

#   {"getMarkerIndex", (PyCFunction) Application::sGetMarkerIndex, METH_VARARGS,
#    "Get marker index according to marker size setting"},
   
#     {"addDocumentObserver",  (PyCFunction) Application::sAddDocObserver, METH_VARARGS,
#      "addDocumentObserver() -> None\n\n"
#      "Add an observer to get notified about changes on documents."},
#     {"removeDocumentObserver",  (PyCFunction) Application::sRemoveDocObserver, METH_VARARGS,
#      "removeDocumentObserver() -> None\n\n"
#      "Remove an added document observer."},

#   {"reload",                    (PyCFunction) Application::sReload, METH_VARARGS,
#    "reload(name) -> doc\n\n"
#    "Reload a partial opened document"},

#   {"loadFile",       (PyCFunction) Application::sLoadFile, METH_VARARGS,
#    "loadFile(string=filename,[string=module]) -> None\n\n"
#    "Loads an arbitrary file by delegating to the given Python module:\n"
#    "* If no module is given it will be determined by the file extension.\n"
#    "* If more than one module can load a file the first one one will be taken.\n"
#    "* If no module exists to load the file an exception will be raised."},

#   {"coinRemoveAllChildren",     (PyCFunction) Application::sCoinRemoveAllChildren, METH_VARARGS,
#    "Remove all children from a group node"},



