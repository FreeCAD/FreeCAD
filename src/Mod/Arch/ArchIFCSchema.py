import FreeCAD, os, json

ifcVersions = ["IFC4", "IFC2X3"]
IfcVersion = ifcVersions[FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetInt("IfcVersion",0)]

with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "Arch", "Presets",
"ifc_contexts_" + IfcVersion + ".json")) as f:
    IfcContexts = json.load(f)

with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "Arch", "Presets",
"ifc_products_" + IfcVersion + ".json")) as f:
    IfcProducts = json.load(f)

with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "Arch", "Presets",
"ifc_types_" + IfcVersion + ".json")) as f:
    IfcTypes = json.load(f)
