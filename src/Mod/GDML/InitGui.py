# GDML gui init module

class GDMLWorkbench ( Workbench ):
	"GDML workbench object"
	MenuText = "GDML"
	ToolTip = "GDML workbench"
	def Initialize(self):
		# load the module
		import GDMLGui
	def GetClassName(self):
		return "GDMLGui::Workbench"

Gui.addWorkbench(GDMLWorkbench())
