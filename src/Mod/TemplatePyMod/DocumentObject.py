# FreeCAD module provding base classes for document objects and view provider  
# (c) 2011 Werner Mayer LGPL

class DocumentObject(object):
    def __init__(self):
        self.__object__=None
    def execute(self):
        raise Exception("Not yet implemented")
    #def onChanged(self,prop):
    #    return None
    #def __getattr__(self, attr):
    #    if hasattr(self.__object__,attr):
    #        return getattr(self.__object__,attr)
    #    else:
    #        return object.__getattribute__(self,attr)
    def addProperty(self,type,name='',group='',doc='',attr=0,readonly=False,hidden=False):
        self.__object__.addProperty(type,name,group,doc,attr,readonly,hidden)
    def supportedProperties(self):
        return self.__object__.supportedProperties()
    def isDerivedFrom(self, obj):
        return self.__object__.isDerivedFrom(obj)
    def getAllDerivedFrom(self):
        return self.__object__.getAllDerivedFrom()
    def getProperty(self,attr):
        return self.__object__.getPropertyByName(attr)
    def getTypeOfProperty(self,attr):
        return self.__object__.getTypeOfProperty(attr)
    def getGroupOfProperty(self,attr):
        return self.__object__.getGroupOfProperty(attr)
    def getDocumentationOfProperty(self,attr):
        return self.__object__.getDocumentationOfProperty(attr)
    def touch(self):
        return self.__object__.touch()
    def purgeTouched(self):
        return self.__object__.purgeTouched()
    def __setstate__(self,value):
        return None
    def __getstate__(self):
        return None
    @property
    def PropertiesList(self):
        return self.__object__.PropertiesList
    @property
    def Type(self):
        return self.__object__.Type
    @property
    def Module(self):
        return self.__object__.Module
    @property
    def Content(self):
        return self.__object__.Content
    @property
    def MemSize(self):
        return self.__object__.MemSize
    @property
    def Name(self):
        return self.__object__.Name
    @property
    def Document(self):
        return self.__object__.Document
    @property
    def State(self):
        return self.__object__.State
    @property
    def ViewObject(self):
        return self.__object__.ViewObject
    @ViewObject.setter
    def ViewObject(self,value):
        self.__object__.ViewObject=value
    @property
    def InList(self):
        return self.__object__.InList
    @property
    def OutList(self):
        return self.__object__.OutList

class ViewProvider(object):
    def __init__(self):
        self.__vobject__=None
    #def getIcon(self):
    #    return ""
    #def claimChildren(self):
    #    return []
    #def setEdit(self,mode):
    #    return False
    #def unsetEdit(self,mode):
    #    return False
    #def attach(self):
    #    return None
    #def updateData(self, prop):
    #    return None
    #def onChanged(self, prop):
    #    return None
    def addDisplayMode(self,node,mode):
        self.__vobject__.addDisplayMode(node,mode)
    #def getDefaultDisplayMode(self):
    #    return ""
    #def getDisplayModes(self):
    #    return []
    #def setDisplayMode(self,mode):
    #    return mode
    def addProperty(self,type,name='',group='',doc='',attr=0,readonly=False,hidden=False):
        self.__vobject__.addProperty(type,name,group,doc,attr,readonly,hidden)
    def update(self):
        self.__vobject__.update()
    def show(self):
        self.__vobject__.show()
    def hide(self):
        self.__vobject__.hide()
    def isVisible(self):
        return self.__vobject__.isVisible()
    def toString(self):
        return self.__vobject__.toString()
    def startEditing(self,mode=0):
        return self.__vobject__.startEditing(mode)
    def finishEditing(self):
        self.__vobject__.finishEditing()
    def isEditing(self):
        self.__vobject__.isEditing()
    def setTransformation(self,trsf):
        return self.__vobject__.setTransformation(trsf)
    def supportedProperties(self):
        return self.__vobject__.supportedProperties()
    def isDerivedFrom(self, obj):
        return self.__vobject__.isDerivedFrom(obj)
    def getAllDerivedFrom(self):
        return self.__vobject__.getAllDerivedFrom()
    def getProperty(self,attr):
        return self.__vobject__.getPropertyByName(attr)
    def getTypeOfProperty(self,attr):
        return self.__vobject__.getTypeOfProperty(attr)
    def getGroupOfProperty(self,attr):
        return self.__vobject__.getGroupOfProperty(attr)
    def getDocumentationOfProperty(self,attr):
        return self.__vobject__.getDocumentationOfProperty(attr)
    @property
    def Annotation(self):
        return self.__vobject__.Annotation
    @property
    def RootNode(self):
        return self.__vobject__.RootNode
    @property
    def DisplayModes(self):
        return self.__vobject__.listDisplayModes()
    @property
    def PropertiesList(self):
        return self.__vobject__.PropertiesList
    @property
    def Type(self):
        return self.__vobject__.Type
    @property
    def Module(self):
        return self.__vobject__.Module
    @property
    def Content(self):
        return self.__vobject__.Content
    @property
    def MemSize(self):
        return self.__vobject__.MemSize
    @property
    def Object(self):
        return self.__vobject__.Object


### Examples


class MyFeature(DocumentObject):
    def execute(self):
        print "Execute my feature"
    def onChanged(self,prop):
        print "Property %s has changed" % (prop)

def testMethod():
    import FreeCAD
    doc=FreeCAD.newDocument()
    obj=MyFeature()
    doc.addObject("Mesh::FeaturePython","MyName",obj,None)
    obj.addProperty("App::PropertyLinkList","Layers","Base", "Layers")
