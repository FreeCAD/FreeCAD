# FreeCAD module providing base classes for document objects and view provider  
# (c) 2011 Werner Mayer LGPL

import FreeCAD

class DocumentObject(object):
    """The Document object is the base class for all FreeCAD objects."""

    def __init__(self):
        self.__object__=None
        self.initialised=False
    #------------------------------Methods for the user to override :

    def execute(self):
        "this method is executed on object creation and whenever the document is recomputed"
        raise NotImplementedError("Not yet implemented")

    def init(self):
        #will be called just after object creation, you can use this for example to create properties
        pass
    def propertyChanged(self,prop):
        #will be called each time a property is changed
        pass
    #--------------------------------


    def __getattr__(self, attr):
        if attr !="__object__" and hasattr(self.__object__,attr):
            return getattr(self.__object__,attr)
        else:
            return object.__getattribute__(self,attr)
    def __setattr__(self, attr, value):
        if attr !="__object__" and hasattr(self.__object__,attr):
            setattr(self.__object__,attr,value)
        else:
            object.__setattr__(self,attr,value)
    def onChanged(self,prop):
        if prop=="Proxy":
            #recreate the functions in the __object__
            d = self.__class__.__dict__
            for key in d:
                item = d[key]
                #check if the function is valid
                if hasattr(item, '__call__') and key!="onChanged" and key!="execute" and key!="init" and key[0]!="_":
                    #check if the function doesn't already exist in the object:
                    if not(hasattr(self.__object__,key)):
                        #add a link to the Proxy function in the __object__ :
                        self.addProperty("App::PropertyPythonObject", key, "", "",2)
                        setattr(self.__object__,key,getattr(self,key))
                    else:
                        FreeCAD.Console.PrintWarning('!!! The function : "'+key+'" already exist in the object, cannot override. !!!\n')
            #call the init function
            if hasattr(self,'initialised'):
                if self.initialised==False:
                    self.init()
                    self.initialised = True
        self.propertyChanged(prop)
    def addProperty(self,typ,name='',group='',doc='',attr=0,readonly=False,hidden=False):
        "adds a new property to this object"
        return self.__object__.addProperty(typ,name,group,doc,attr,readonly,hidden)
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
    def getEnumerationsOfProperty(self,attr):
        "returns the documentation string of a given property"
        return self.__object__.getEnumerationsOfProperty(attr)
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
    #    return self.__vobject__.Object.OutList
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
        "shows whether this object is visible or invisible"
        return self.__vobject__.isVisible()
    def toString(self):
        "returns a string representation of the coin node of this object"
        return self.__vobject__.toString()
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
    def __setstate__(self,value):
        """allows to save custom attributes of this object as strings, so
        they can be saved when saving the FreeCAD document"""
        return None
    def __getstate__(self):
        """reads values previously saved with __setstate__()"""
        return None
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


#Example :
import Part

class Box(DocumentObject):
    #type :
    type = "Part::FeaturePython"

    #-----------------------------INIT----------------------------------------
    def init(self):
        self.addProperty("App::PropertyLength","Length","Box","Length of the box").Length=1.0
        self.addProperty("App::PropertyLength","Width","Box","Width of the box").Width=1.0
        self.addProperty("App::PropertyLength","Height","Box", "Height of the box").Height=1.0

    #-----------------------------BEHAVIOR------------------------------------
    def propertyChanged(self,prop):
        FreeCAD.Console.PrintMessage("Box property changed : "+ prop+ "\n")
        if prop == "Length" or prop == "Width" or prop == "Height":
            self._recomputeShape()

    def execute(self):
        FreeCAD.Console.PrintMessage("Recompute Python Box feature\n")
        self._recomputeShape()

    #---------------------------PUBLIC FUNCTIONS-------------------------------
    #These functions will be present in the object
    def customFunctionSetLength(self,attr):
        self.Length = attr
        self._privateFunctionExample(attr)

    #---------------------------PRIVATE FUNCTIONS------------------------------
    #These function won't be present in the object (begin with '_')
    def _privateFunctionExample(self,attr):
        FreeCAD.Console.PrintMessage("The length : "+str(attr)+"\n")

    def _recomputeShape(self):
        if hasattr(self,"Length") and hasattr(self,"Width") and hasattr(self,"Height"):
            self.Shape = Part.makeBox(self.Length,self.Width,self.Height)


def makeBox():
    FreeCAD.newDocument()
    box = FreeCAD.ActiveDocument.addObject(Box.type,"MyBox",Box(),None)
    box.customFunctionSetLength(4)
