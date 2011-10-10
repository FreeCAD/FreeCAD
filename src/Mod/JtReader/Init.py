# FreeCAD init script of the JtReader module
# (c) 2007 Juergen Riegel LGPL

# Get the Parameter Group of this module
ParGrp = App.ParamGet("System parameter:Modules").GetGroup("JtReader")

# Append the open handler
FreeCAD.EndingAdd("JtOpen (*.jt)","JtReader")



