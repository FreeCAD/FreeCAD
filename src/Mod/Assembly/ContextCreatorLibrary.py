import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets

transparency_level = 75
experimental_mode = True

import UtilsAssembly
import Preferences
import Part

class ContextCreationSystem:
    COPYABLE_OBJECT_TYPES = ["Body", "Part", "LinkedObject", "DocumentObject", "Feature"]

    def __init__(self, editPart, assembly):
        self.mainDocumentFileName = App.ActiveDocument.FileName
        pass

    def getObjFromRefString(self, objName, location):
        document = App.openDocument(location)
        return document.getObject(objName)

    def getLinkedDoc(self, obj):
        document = None
        
        if hasattr(obj, "LinkedObject"):
            document = obj.LinkedObject.Document
        else:
            for child in obj.OutList:
                if hasattr(child, "LinkedObject"):
                    document = child.LinkedObject.Document
                    continue
                    
        return document

    def getLinkedObj(self, obj):
        object = None
        
        if hasattr(obj, "LinkedObject"):
            obj.LinkTransform = False
            object = obj.LinkedObject
        else:
            for child in obj.OutList:
                if hasattr(child, "LinkedObject"):
                    child.LinkTransform = False
                    object = child.LinkedObject
                    continue
                    
        return object

    def getCopyableObjectsInAssembly(self, assembly):
        objects = []
        
        for obj in assembly.OutList:
            if type(obj).__name__ in self.COPYABLE_OBJECT_TYPES and "Placement" in obj.PropertiesList:
                objects.append(obj)
        return objects
    
    def restartDocument(self, doc):
        docFileName = doc.FileName
        doc.save()
        App.closeDocument(doc.Name)
        print(docFileName)
        print("doc closed")
        App.open(docFileName)


    def addOffsets(self, objects,linked_obj, linked_placement):
        linked_obj.Placement = linked_placement

    def createGroup(self, objects, target_document, linked_obj, edit_selection):
        if App.ActiveDocument.getObjectsByLabel("AssemblyContext"):
            App.ActiveDocument.removeObject(App.ActiveDocument.getObjectsByLabel("AssemblyContext")[0].Name)

        group = App.ActiveDocument.addObject("App::DocumentObjectGroup", "Group")
        group.addProperty("App::PropertyString", "EditedPart")
        group.EditedPart = linked_obj.Name
        group.addProperty("App::PropertyString", "EditedPartLink")
        group.addProperty("App::PropertyString", "EditedPartLinkLocation")
        group.EditedPartLink = edit_selection.Name
        group.EditedPartLinkLocation = self.mainDocumentFileName
        
        for obj in objects:
            group.addObject(obj)
        group.Label = "AssemblyContext"
        groupName = self.move_group_to_doc(target_document, group, True)

        return groupName

    def _removeChildren(self, obj):
        for child in obj.OutList:
            App.ActiveDocument.removeObject(child.Name)

    '''
    def addProperties(objects, copiedObjects):
        objectNames = []
        for obj in objects:
            objectsNames.append(obj.Label)
        for copiedObj in copiedObjects:
            if copiedObj.Label in objectNames:
                propertyString = App.ActiveDocument.Name + "." + copiedObj.Name
                copiedObj.addProperty("App::PropertyString")
    '''

    def createCopy(self, objects, edit_obj):
        copiedObjects = []
        for obj in objects:
            if(obj is edit_obj):
                continue
            propertyString = obj.Name
            #copiedObj = App.ActiveDocument.copyObject(obj, True)
            copiedObj = App.ActiveDocument.addObject("Part::Feature", obj.Label)
            try:
                copiedObj.Shape = obj.Shape
            except:
                print("error while copying Shape attributes")
            copiedObj.ViewObject.Transparency = transparency_level
            copiedObj.addProperty("App::PropertyString", "RefObj")
            copiedObj.addProperty("App::PropertyString", "RefObjLocation")
            copiedObj.RefObj = propertyString
            copiedObj.RefObjLocation = self.mainDocumentFileName
            copiedObjects.append(copiedObj)
            
            
        return copiedObjects
    
    
    def redoShapes(self, createdGroupName):
        '''
        #self.restartDocument(App.ActiveDocument)
        currentDoc = App.ActiveDocument
        print(createdGroupName)
        print(currentDoc.FileName)
        group = currentDoc.getObject(createdGroupName)
        for obj in group.OutList:
                if "RefObj" in obj.PropertiesList:
                    refObj = self.getObjFromRefString(obj.RefObj, obj.RefObjLocation)
                    assemblyDocument = refObj.Document
                        
                    try:
                        obj.Placement = refObj.Placement
                        #obj.Shape = Part.Shape()
                        self.updateShape(obj, refObj)
                        currentDoc.recompute()
                    except:
                        print("could not update object: " + refObj.Label)
        self.restartDocument(currentDoc)
        '''
        pass


    def createContext(self, linkedObj, target_document, normalObjects, linked_placement, edit_selection):
        Gui.setActiveDocument(target_document.Name)
        objects = self.createCopy(normalObjects, edit_selection)
        self.addOffsets(objects, linkedObj, linked_placement)
        createdGroupName = self.createGroup(objects, target_document, linkedObj, edit_selection)
        #self.restartDocument(App.ActiveDocument)
        if experimental_mode:
            self.redoShapes(createdGroupName)

    def _copy_selection_test(self):
        objects = App.ActiveDocument.copyObject(Gui.Selection.getSelection(), True)
        self.createGroup(objects)
        
    def updateShape(self, obj, refObj):
        print("Updating Shape....")
        solid = Part.Solid(refObj.Shape)
        obj.Shape = solid
        #obj.recompute(True)
        App.ActiveDocument=App.ActiveDocument
        documentFileName = App.ActiveDocument.FileName
        
    def move_group_to_doc(self, dest_doc, group, deleteOriginal):
        # Get the source and destination documents
        source_doc = App.ActiveDocument

        dest_doc = App.openDocument(dest_doc.FileName) #reload dest_doc to avoid errors
        source_doc = App.openDocument(source_doc.FileName)
        App.ActiveDocument = source_doc

        # Copy the object to the destination document
        new_obj = dest_doc.copyObject(group, True)

        # Delete the original object from the source document
        # source_doc.removeObject(obj.Name)
        if deleteOriginal:
            for obj in group.OutList:
                if obj.OutList:
                    self.removeChildren(obj)
                source_doc.removeObject(obj.Name)
            source_doc.removeObject(group.Name)

        return new_obj.Name

    def getPartParent(self, obj):
        final_obj = None
        
        if type(obj).__name__ == "Part":
            return obj
        
        for parent in obj.InList:
            if(type(parent).__name__ == "Part"):
                final_obj = parent
            else:
                print(type(parent).__name__)
        return final_obj
    
    def getSubShapeBinders(self, document):
        return [obj for obj in document.Objects if obj.isDerivedFrom("PartDesign::SubShapeBinder")]

    def UpdateContext(self):
        selection = Gui.Selection.getSelection()[0]
        if "EditedPart" in selection.PropertiesList:
            self.editedPart = App.ActiveDocument.getObject(selection.EditedPart)
            self.objectToUpdate = self.getCopyableObjectsInAssembly(selection)
                
            EditedPartLink = self.getObjFromRefString(selection.EditedPartLink, selection.EditedPartLinkLocation)
            self.editedPart.Placement = EditedPartLink.Placement
                
            for obj in self.objectToUpdate:
                if "RefObj" in obj.PropertiesList:
                    refObj = self.getObjFromRefString(obj.RefObj, obj.RefObjLocation)
                    assemblyDocument = refObj.Document
                        
                    try:
                        obj.Placement = refObj.Placement
                        #obj.Shape = Part.Shape()
                        self.updateShape(obj, refObj)
                        #App.ActiveDocument.recompute()
                    except:
                        print("could not update object: " + refObj.Label)
            current_doc = App.open(self.mainDocumentFileName)
            current_doc_filename = current_doc.FileName
            
            # App.ActiveDocument.recompute()
            self.restartDocument(App.ActiveDocument)

            if experimental_mode:
                subShapeBinders = self.getSubShapeBinders(App.ActiveDocument)

                for binder in subShapeBinders:
                    binder.touch()
                    binder.recompute()
                App.ActiveDocument.recompute()
        else:
            App.Console.PrintWarning("The selection is not an assembly context!")