import os,FreeCAD,FreeCADGui

macrosList = []
macroPath = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro").GetString("MacroPath")

class MacroCommand():
	"A template for macro commands"
        def __init__(self,macroname):
            self.macroname = macroname

	def GetResources(self):
            return {'Pixmap'  : 'Draft_Macro',
                    'MenuText': self.macroname,
                    'ToolTip': 'Executes the '+self.macroname+' macro'}

        def Activated(self):
            target = macroPath+os.sep+self.macroname+'.FCMacro'
            if os.path.exists(target): execfile(target)

            
if macroPath:
    macros = []
    for f in os.listdir(macroPath):
        if ".FCMacro" in f:
            macros.append(f[:-8])
    for m in macros:
        cmd = 'Macro_'+m
        FreeCADGui.addCommand(cmd,MacroCommand(m))
        macrosList.append(cmd)
        
        
        
