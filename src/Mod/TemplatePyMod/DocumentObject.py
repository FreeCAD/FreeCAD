# FreeCAD module provding base classes for document objects and view provider  
# (c) 2013 Werner Mayer LGPL


### Examples
import FreeCAD, FreeCADGui
App=FreeCAD
Gui=FreeCADGui

class MyDocumentObjectFloat(FreeCAD.DocumentObject):
  def __init__(self,*args):
    super(MyDocumentObjectFloat, self).__init__(*args)
    self.addProperty("App::PropertyFloat","MyFloat")
    self.Test=3
  def onChanged(self, prop):
    print prop
  def execute(self):
    print "execute"
  def __getstate__(self):
    return {"Test":self.Test}
  def __setstate__(self, dict):
    for i in dict.keys():
      setattr(self, i, dict[i])

def makeDocumentObjectFloat():
    App.newDocument()
    my=App.ActiveDocument.addObject("App::FeaturePython","Test",MyDocumentObjectFloat)
    print my.MyFloat
    my.execute()
    my.MyFloat=3.0
    my.Test


class MyDocumentObject(FreeCAD.DocumentObject):
  def __init__(self,*args):
    super(MyDocumentObject,self).__init__(*args)
    self.addProperty("App::PropertyFloat","MyFloat")
  def onChanged(self, prop):
    print "MyDocumentObject.onChanged(%s)" % (prop)
  def execute(self):
    print "MyDocumentObject.execute"

class MyExtDocumentObject(MyDocumentObject):
  def __init__(self,*args):
    super(MyExtDocumentObject,self).__init__(*args)
    self.addProperty("App::PropertyInteger","MyInt")

class MyViewProvider(FreeCADGui.ViewProviderDocumentObject):
  def __init__(self,*args):
    super(MyViewProvider,self).__init__(*args)
    self.addProperty("App::PropertyInteger","MyInt")
  def getIcon(self):
    return ":/icons/utilities-terminal.svg"
  def attach(self):
    print "Attach"
  def claimChildren(self):
    return []
  def setEdit(self, arg):
    return True
  def unsetEdit(self, arg):
    return True
  def getDisplayModes(self):
    return ["Shaded", "Wireframe"]
  def getDefaultDisplayMode(self):
    return "Shaded"
  def onChanged(self,prop):
    print "MyViewProvider.onChanged(%s)" % (prop)
  def updateData(self,prop):
    print "MyViewProvider.updateData(%s)" % (prop)

class MyObjectGroup(App.DocumentObject):
  def execute(self):
    print "MyObjectGroup.execute"

def makeDocumentObject():
    App.newDocument()
    my=App.ActiveDocument.addObject("App::FeaturePython","Test",MyExtDocumentObject,MyViewProvider)
    print my.MyFloat
    my.execute()
    my.MyFloat=3.0

    grp=App.ActiveDocument.addObject("App::DocumentObjectGroupPython","Group",MyObjectGroup)
    grp.addObject(my)

import Part

class MyBox(Part.Feature):
    def __init__(self,*args):
        super(MyBox,self).__init__(*args)
        self.addProperty("App::PropertyFloat","Length")
        self.addProperty("App::PropertyFloat","Width")
        self.addProperty("App::PropertyFloat","Height")
    def execute(self):
        self.Shape = Part.makeBox(self.Length,self.Width,self.Height)

def makeBox():
    App.newDocument()
    box = App.ActiveDocument.addObject("Part::FeaturePython","Box",MyBox)
    box.ViewObject.Proxy=1
    box.Length=7
    box.Width=3
    box.Height=2
    App.ActiveDocument.recompute()

