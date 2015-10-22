# TemplatePyMod gui init module  
# (c) 2007 Juergen Riegel LGPL
#


class TemplatePyModWorkbench ( Workbench ):
	"Test workbench object"
	Icon = """
			/* XPM */
			static const char *test_icon[]={
			"16 16 2 1",
			"a c #000000",
			". c None",
			"................",
			"................",
			"..############..",
			"..############..",
			"..############..",
			"......####......",
			"......####......",
			"......####......",
			"......####......",
			"......####......",
			"......####......",
			"......####......",
			"......####......",
			"......####......",
			"................",
			"................"};
			"""
	MenuText = "Python sandbox"
	ToolTip = "Python template workbench"
	
	def Initialize(self):
		import Commands

		self.appendToolbar("TemplateTools",["TemplatePyMod_Cmd1","TemplatePyMod_Cmd2","TemplatePyMod_Cmd3","TemplatePyMod_Cmd4","TemplatePyMod_Cmd5"])

		menu = ["ModulePy &Commands","PyModuleCommands"]
		list = ["TemplatePyMod_Cmd1","TemplatePyMod_Cmd2","TemplatePyMod_Cmd3","TemplatePyMod_Cmd5","TemplatePyMod_Cmd6"]
		self.appendCommandbar("PyModuleCommands",list)
		self.appendMenu(menu,list)

		Log ('Loading TemplatePyMod module... done\n')
	def Activated(self):
		Msg("TemplatePyModWorkbench::Activated()\n")
	def Deactivated(self):
		Msg("TemplatePyModWorkbench::Deactivated()\n")

Gui.addWorkbench(TemplatePyModWorkbench)
