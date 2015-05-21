#! python
# -*- coding: utf-8 -*-

class CEMWorkbench ( Workbench ):
    "CEM workbench object"
    Icon = """
            /* XPM */
            static const char *test_icon[]={
            "16 16 2 1",
            "a c #000000",
            ". c None",
            "................",
            "................",
            "....#########...",
            "...##########...",
            "..####..........",
            "..####..........",
            ".####...........",
            ".####...........",
            ".####...........",
            ".####...........",
            "..####..........",
            "..####..........",
            "...##########...",
            "....#########...",
            "................",
            "................"};
            """
    MenuText = "CEM Workbench menu"
    ToolTip = "Computational electromagnetics workbench"
    
    def Initialize(self):
        import CEMGui

        self.appendToolbar("CEMTools",["CEMGui_create_PLM","CEMGui_create_lattice"])

        menu = ["CEM &Commands","CEMCommands"]
#        list = ["TemplatePyMod_Cmd1","TemplatePyMod_Cmd2","TemplatePyMod_Cmd3","TemplatePyMod_Cmd5","TemplatePyMod_Cmd6"]
#        self.appendCommandbar("CEMCommands",list)
#        self.appendMenu(menu,list)

        Log ('Loading CEM module... done\n')
    def Activated(self):
        Msg("CEM Workbench::Activated()\n")
    def Deactivated(self):
        Msg("CEM Workbench::Deactivated()\n")

Gui.addWorkbench(CEMWorkbench)
