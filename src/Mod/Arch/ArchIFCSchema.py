import FreeCAD, os, json

ifcVersions = ["IFC4", "IFC2X3"]
IfcVersion = ifcVersions[FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetInt("IfcVersion",0)]

# BEGIN FIXME
# the file ifc_contexts_IFC2x3.json is missing at all in sources
# All Arch module is broken if pref is set to IFC2x3
# we will use the IFC4 one instead, to get back the Arch module
# Still lots of stuff does not work, but at least the Arch loads
'''
with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "Arch", "Presets",
"ifc_contexts_" + IfcVersion + ".json")) as f:
    IfcContexts = json.load(f)
'''
with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "Arch", "Presets",
"ifc_contexts_" + "IFC4" + ".json")) as f:
    IfcContexts = json.load(f)
# END FIXME

with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "Arch", "Presets",
"ifc_products_" + IfcVersion + ".json")) as f:
    IfcProducts = json.load(f)

with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "Arch", "Presets",
"ifc_types_" + IfcVersion + ".json")) as f:
    IfcTypes = json.load(f)
