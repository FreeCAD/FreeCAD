class ArchWorkbench(Workbench):
	"Arch workbench object"
	Icon = """
                /* XPM */
                static char * arch_xpm[] = {
                "16 16 9 1",
                " 	c None",
                ".	c #543016",
                "+	c #6D2F08",
                "@	c #954109",
                "#	c #874C24",
                "$	c #AE6331",
                "%	c #C86423",
                "&	c #FD7C26",
                "*	c #F5924F",
                "                ",
                "                ",
                "       #        ",
                "      ***$#     ",
                "    .*******.   ",
                "   *##$****#+   ",
                " #**%&&##$#@@   ",
                ".$**%&&&&+@@+   ",
                "@&@#$$%&&@@+..  ",
                "@&&&%#.#$#+..#$.",
                " %&&&&+%#.$**$@+",
                "   @%&+&&&$##@@+",
                "     @.&&&&&@@@ ",
                "        @%&&@@  ",
                "           @+   ",
                "                "};
			"""
	MenuText = "Arch"
	ToolTip = "Architecture workbench"
	
	def Initialize(self):
                import draftTools,draftGui,Arch_rc,Arch
                archtools = ["Arch_Wall","Arch_Structure","Arch_Cell",
                             "Arch_Floor","Arch_Building","Arch_Site",
                             "Arch_Window","Arch_Axis",
                             "Arch_SectionPlane","Arch_Add","Arch_Remove"]
                drafttools = ["Draft_Line","Draft_Wire","Draft_Rectangle",
                              "Draft_Polygon","Draft_Arc",
                              "Draft_Circle","Draft_Dimension",
                              "Draft_Move","Draft_Rotate",
                              "Draft_Offset","Draft_Upgrade",
                              "Draft_Downgrade","Draft_Trimex"]
                meshtools = ["Arch_SplitMesh","Arch_MeshToShape",
                             "Arch_SelectNonSolidMeshes","Arch_RemoveShape"]
                self.appendToolbar(str(draftTools.translate("arch","Arch tools")),archtools)
                self.appendToolbar(str(draftTools.translate("arch","Draft tools")),drafttools)
                self.appendMenu([str(draftTools.translate("arch","Architecture")),str(draftTools.translate("arch","Tools"))],meshtools)
                self.appendMenu(str(draftTools.translate("arch","Architecture")),archtools)
                self.appendMenu(str(draftTools.translate("arch","Draft")),drafttools)
                FreeCADGui.addIconPath(":/icons")
                FreeCADGui.addLanguagePath(":/translations")
                FreeCADGui.addPreferencePage(":/ui/archprefs-base.ui","Arch")
                FreeCAD.addImportType("Industry Foundation Classes (*.ifc)","importIFC")
                FreeCAD.addExportType("Wavefront OBJ - Arch module (*.obj)","importOBJ")
                try:
                        import collada
                except:
                        Log("pycollada not found, no collada support\n")
                else:
                        FreeCAD.addImportType("Collada (*.dae)","importDAE")
                        FreeCAD.addExportType("Collada (*.dae)","importDAE")
		Log ('Loading Arch module... done\n')
	def Activated(self):
                FreeCADGui.draftToolBar.Activated()
		Msg("Arch workbench activated\n")
	def Deactivated(self):
                FreeCADGui.draftToolBar.Deactivated()
		Msg("Arch workbench deactivated\n")

FreeCADGui.addWorkbench(ArchWorkbench)

