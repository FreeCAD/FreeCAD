import FreeCAD,FreeCADGui,sys
rf=FreeCAD.ParamGet("User parameter:BaseApp/Preferences/RecentFiles")
FreeCAD.loadFile(rf.GetString("MRU0"))
