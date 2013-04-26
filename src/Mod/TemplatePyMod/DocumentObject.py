# FreeCAD module provding base classes for document objects and view provider  
# (c) 2011 Werner Mayer LGPL

class DocumentObject(object):
    """The Document object is the base class for all FreeCAD objects.
    Example of use:
    
    import FreeCAD
    
    doc=FreeCAD.newDocument()
    
    myobj = doc.addObject("Mesh::FeaturePython","MyName")
    
    myobj.addProperty("App::PropertyLinkList","Layers","Base", "Layers")"""
    def __init__(self):
        self.__object__=None
    def execute(self):
        "this method is executed on object creation and whenever the document is recomputed"
        raise Exception("Not yet implemented")
    #def onChanged(self,prop):
    #    return None
    #def __getattr__(self, attr):
    #    if hasattr(self.__object__,attr):
    #        return getattr(self.__object__,attr)
    #    else:
    #        return object.__getattribute__(self,attr)
    def addProperty(self,type,name='',group='',doc='',attr=0,readonly=False,hidden=False):
        "adds a new property to this object"
        self.__object__.addProperty(type,name,group,doc,attr,readonly,hidden)
    def supportedProperties(self):
        "lists the property types supported by this object"
        return self.__object__.supportedProperties()
    def isDerivedFrom(self, obj):
        """returns True if this object is derived from the given C++ class, for
        example Part::Feature"""
        return self.__object__.isDerivedFrom(obj)
    def getAllDerivedFrom(self):
        "returns all parent C++ classes of this object"
        return self.__object__.getAllDerivedFrom()
    def getProperty(self,attr):
        "returns the value of a given property"
        return self.__object__.getPropertyByName(attr)
    def getTypeOfProperty(self,attr):
        "returns the type of a given property"
        return self.__object__.getTypeOfProperty(attr)
    def getGroupOfProperty(self,attr):
        "returns the group of a given property"
        return self.__object__.getGroupOfProperty(attr)
    def getDocumentationOfProperty(self,attr):
        "returns the documentation string of a given property"
        return self.__object__.getDocumentationOfProperty(attr)
    def touch(self):
        "marks this object to be recomputed"
        return self.__object__.touch()
    def purgeTouched(self):
        "removes the to-be-recomputed flag of this object"
        return self.__object__.purgeTouched()
    def __setstate__(self,value):
        """allows to save custom attributes of this object as strings, so
        they can be saved when saving the FreeCAD document"""
        return None
    def __getstate__(self):
        """reads values previously saved with __setstate__()"""
        return None
    @property
    def PropertiesList(self):
        "lists the current properties of this object"
        return self.__object__.PropertiesList
    @property
    def Type(self):
        "shows the C++ class of this object"
        return self.__object__.Type
    @property
    def Module(self):
        "gives the module this object is defined in"
        return self.__object__.Module
    @property
    def Content(self):
        """shows the contents of the properties of this object as an xml string.
        This is the content that is saved when the file is saved by FreeCAD"""
        return self.__object__.Content
    @property
    def MemSize(self):
        "shows the amount of memory this object uses"
        return self.__object__.MemSize
    @property
    def Name(self):
        "the name ofthis object, unique in the FreeCAD document"
        return self.__object__.Name
    @property
    def Document(self):
        "the document this object is part of"
        return self.__object__.Document
    @property
    def State(self):
        "shows if this object is valid (presents no errors)"
        return self.__object__.State
    @property
    def ViewObject(self):
        return self.__object__.ViewObject
    @ViewObject.setter
    def ViewObject(self,value):
        """returns or sets the ViewObject associated with this object. Returns
        None if FreeCAD is running in console mode"""
        self.__object__.ViewObject=value
    @property
    def InList(self):
        "lists the parents of this object"
        return self.__object__.InList
    @property
    def OutList(self):
        "lists the children of this object"
        return self.__object__.OutList

class ViewProvider(object):
    """The ViewProvider is the counterpart of the DocumentObject in
    the GUI space. It is only present when FreeCAD runs in GUI mode.
    It contains all that is needed to represent the DocumentObject in
    the 3D view and the FreeCAD interface"""
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
        "adds a coin node as a display mode to this object"
        self.__vobject__.addDisplayMode(node,mode)
    #def getDefaultDisplayMode(self):
    #    return ""
    #def getDisplayModes(self):
    #    return []
    #def setDisplayMode(self,mode):
    #    return mode
    def addProperty(self,type,name='',group='',doc='',attr=0,readonly=False,hidden=False):
        "adds a new property to this object"
        self.__vobject__.addProperty(type,name,group,doc,attr,readonly,hidden)
    def update(self):
        "this method is executed whenever any of the properties of this ViewProvider changes"
        self.__vobject__.update()
    def show(self):
        "switches this object to visible"
        self.__vobject__.show()
    def hide(self):
        "switches this object to invisible"
        self.__vobject__.hide()
    def isVisible(self):
        "shows wether this object is visible or invisible"
        return self.__vobject__.isVisible()
    def toString(self):
        "returns a string representation of the coin node of this object"
        return self.__vobject__.toString()
    def startEditing(self,mode=0):
        "sets this object in edit mode"
        return self.__vobject__.startEditing(mode)
    def finishEditing(self):
        "leaves edit mode for this object"
        self.__vobject__.finishEditing()
    def isEditing(self):
        "shows wether this object is in edit mode"
        self.__vobject__.isEditing()
    def setTransformation(self,trsf):
        "defines a transformation for this object"
        return self.__vobject__.setTransformation(trsf)
    def supportedProperties(self):
        "lists the property types this ViewProvider supports"
        return self.__vobject__.supportedProperties()
    def isDerivedFrom(self, obj):
        """returns True if this object is derived from the given C++ class, for
        example Part::Feature"""
        return self.__vobject__.isDerivedFrom(obj)
    def getAllDerivedFrom(self):
        "returns all parent C++ classes of this object"
        return self.__vobject__.getAllDerivedFrom()
    def getProperty(self,attr):
        "returns the value of a given property"
        return self.__vobject__.getPropertyByName(attr)
    def getTypeOfProperty(self,attr):
        "returns the type of a given property"
        return self.__vobject__.getTypeOfProperty(attr)
    def getGroupOfProperty(self,attr):
        "returns the group of a given property"
        return self.__vobject__.getGroupOfProperty(attr)
    def getDocumentationOfProperty(self,attr):
        "returns the documentation string of a given property"
        return self.__vobject__.getDocumentationOfProperty(attr)
    @property
    def Annotation(self):
        "returns the Annotation coin node of this object"
        return self.__vobject__.Annotation
    @property
    def RootNode(self):
        "returns the Root coin node of this object"
        return self.__vobject__.RootNode
    @property
    def DisplayModes(self):
        "lists the display modes of this object"
        return self.__vobject__.listDisplayModes()
    @property
    def PropertiesList(self):
        "lists the current properties of this object"
        return self.__vobject__.PropertiesList
    @property
    def Type(self):
        "shows the C++ class of this object"
        return self.__vobject__.Type
    @property
    def Module(self):
        "gives the module this object is defined in"
        return self.__vobject__.Module
    @property
    def Content(self):
        """shows the contents of the properties of this object as an xml string.
        This is the content that is saved when the file is saved by FreeCAD"""
        return self.__vobject__.Content
    @property
    def MemSize(self):
        "shows the amount of memory this object uses"
        return self.__vobject__.MemSize
    @property
    def Object(self):
        "returns the DocumentObject this ViewProvider is associated to"
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
