# Get the Parameter Group of this module
ParGrp = App.ParamGet("System parameter:Modules").GetGroup("Arch")

# Set the needed information
ParGrp.SetString("HelpIndex",        "http://free-cad.sf.net")
ParGrp.SetString("WorkBenchName",    "Arch")

