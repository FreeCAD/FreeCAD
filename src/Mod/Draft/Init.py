
# Get the Parameter Group of this module
ParGrp = App.ParamGet("System parameter:Modules").GetGroup("Mod/Draft")

# Set the needed information
ParGrp.SetString("HelpIndex",        "http://apps.sourceforge.net/mediawiki/free-cad/index.php?title=Draft_Module")
ParGrp.SetString("WorkBenchName",    "Draft")
ParGrp.SetString("WorkBenchModule",  "Draft.py")
