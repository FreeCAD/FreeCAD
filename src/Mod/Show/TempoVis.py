#/***************************************************************************
# *   Copyright (c) Victor Titov (DeepSOIC)                                 *
# *                                           (vv.titov@gmail.com) 2016     *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/

import FreeCAD as App
if App.GuiUp:
    import FreeCADGui as Gui

from .FrozenClass import FrozenClass

from .DepGraphTools import getAllDependencies, getAllDependent, isContainer

from . import Containers
Container = Containers.Container

class TempoVis(FrozenClass):
    '''TempoVis - helper object to save visibilities of objects before doing
    some GUI editing, hiding or showing relevant stuff during edit, and
    then restoring all visibilities after editing.

    Constructors:
    TempoVis(document): creates a new TempoVis. Supplying document is mandatory. Objects not belonging to the document can't be modified via TempoVis.'''

    def __define_attributes(self):
        self.data = {} # dict. key = ("Object","Property"), value = original value of the property
        self.data_pickstyle = {} # dict. key = "Object", value = original value of pickstyle
        self.data_clipplane = {} # dict. key = "Object", value = original state of plane-clipping
        self.clippingDoc = None

        self.sketch_clipplane_on = False #True if some clipping planes are active
        
        self.cam_string = ""          # inventor ASCII string representing the camera
        self.viewer = None            # viewer the camera is saved from

        self.document = None
        self.restore_on_delete = False # if true, restore() gets called upon object deletion. It becomes False after explicit call to Restore, and set to true by many methods.
        
        self.links_are_lost = False # set to true after restore from JSON. Indicates to attempt to use ActiveDocument/ActiveViewer instead.
        
        self._freeze()

    def __init__(self, document):
        self.__define_attributes()

        self.document = document
    
    @staticmethod
    def is3DObject(obj):
        """is3DObject(obj): tests if the object has some 3d geometry. 
        TempoVis is made only for objects in 3d view, so all objects that don't pass this check are ignored by TempoVis."""
        
        # See "Gui Problem Sketcher and TechDraw" https://forum.freecadweb.org/viewtopic.php?f=3&t=22797
 
        # observation: all viewproviders have transform node, then a switch node. If that switch node contains something, the object has something in 3d view.
        try:
            from pivy import coin
            return obj.ViewObject.SwitchNode.getNumChildren()>0
        except Exception as err:
            App.Console.PrintWarning(u"Show.TempoVis.isIn3DObject error: {err}\n".format(err= str(err)))
            return True #assume.
    
    def modifyVPProperty(self, doc_obj_or_list, prop_names, new_value):
        '''modifyVPProperty(self, doc_obj_or_list, prop_name, new_value): modifies
        prop_name property of ViewProvider of doc_obj_or_list, and remembers
        original value of the property. Original values will be restored upon
        TempoVis deletion, or call to restore().'''

        if App.GuiUp:
            if not hasattr(doc_obj_or_list, '__iter__'):
                doc_obj_or_list = [doc_obj_or_list]
            if not isinstance(prop_names,(list,tuple)):
                prop_names = [prop_names]
            for doc_obj in doc_obj_or_list:
                if not self.is3DObject(doc_obj):
                    continue
                for prop_name in prop_names:
                    if not hasattr(doc_obj.ViewObject, prop_name):
                        App.Console.PrintWarning("TempoVis: object {obj} has no attribute {attr}. Skipped.\n"
                                                .format(obj= doc_obj.Name, attr= prop_name))
                        continue # silently ignore if object doesn't have the property...

                    # Because the introduction of external objects, we shall now
                    # accept objects from all opening documents.
                    #
                    #  if doc_obj.Document is not self.document:  #ignore objects from other documents
                    #      raise ValueError("Document object to be modified does not belong to document TempoVis was made for.")
                    oldval = getattr(doc_obj.ViewObject, prop_name)
                    if new_value is not None:
                        setattr(doc_obj.ViewObject, prop_name, new_value)
                    self.restore_on_delete = True
                    key = (doc_obj.Name,prop_name,doc_obj.Document.Name)
                    self.data.setdefault(key,oldval)

    def saveBodyVisibleFeature(self,doc_obj_or_list):
        if not hasattr(doc_obj_or_list, '__iter__'):
            doc_obj_or_list = [doc_obj_or_list]
        objs = []
        bodies = set()
        for obj in doc_obj_or_list:
            body = getattr(obj,'Body',None)
            if not body or body in bodies:
                continue
            bodies.add(body)
            feature = getattr(body,'VisibleFeature',None)
            if feature:
                objs.append(feature)
        self.modifyVPProperty(objs, 'Visibility',None)

    def show(self, doc_obj_or_list):
        '''show(doc_obj_or_list): shows objects (sets their Visibility to True). doc_obj_or_list can be a document object, or a list of document objects'''
        self.saveBodyVisibleFeature(doc_obj_or_list)
        self.modifyVPProperty(doc_obj_or_list, ('Visibility','LinkVisibility'),True)

    def hide(self, doc_obj_or_list):
        '''hide(doc_obj_or_list): hides objects (sets their Visibility to False). doc_obj_or_list can be a document object, or a list of document objects'''
        self.saveBodyVisibleFeature(doc_obj_or_list)
        self.modifyVPProperty(doc_obj_or_list, ('Visibility',"LinkVisibility"), False)

    def get_all_dependent(self, doc_obj):
        '''get_all_dependent(doc_obj): gets all objects that depend on doc_obj. Containers of the object are excluded from the list.'''
        cnt_chain = Containers.ContainerChain(doc_obj)
        return [o for o in getAllDependent(doc_obj) if not o in cnt_chain]

    def hide_all_dependent(self, doc_obj):
        '''hide_all_dependent(doc_obj): hides all objects that depend on doc_obj. Groups, Parts and Bodies are not hidden by this.'''
        self.hide( self.get_all_dependent(doc_obj) )

    def show_all_dependent(self, doc_obj):
        '''show_all_dependent(doc_obj): shows all objects that depend on doc_obj. This method is probably useless.'''
        self.show( getAllDependent(doc_obj) )

    def restore_all_dependent(self, doc_obj):
        '''show_all_dependent(doc_obj): restores original visibilities of all dependent objects.'''
        self.restoreVPProperty( getAllDependent(doc_obj), ('Visibility','LinkVisibility') )

    def hide_all_dependencies(self, doc_obj):
        '''hide_all_dependencies(doc_obj): hides all objects that doc_obj depends on (directly and indirectly).'''
        self.hide( getAllDependencies(doc_obj) )

    def show_all_dependencies(self, doc_obj):
        '''show_all_dependencies(doc_obj): shows all objects that doc_obj depends on (directly and indirectly). This method is probably useless.'''
        self.show( getAllDependencies(doc_obj) )

    def saveCamera(self):
        vw = Gui.ActiveDocument.ActiveView
        self.cam_string        = vw.getCamera()
        self.viewer            = vw
        
        self.restore_on_delete = True

    def restoreCamera(self):
        if not self.cam_string:
            return
        vw = self.viewer
        if self.links_are_lost: # can happen after save-restore
            import FreeCADGui as Gui
            vw = Gui.ActiveDocument.ActiveView

        vw.setCamera(self.cam_string)

    def restore(self):
        '''restore(): restore all ViewProvider properties modified via TempoVis to their
        original values, and saved camera, if any. Called automatically when instance is
        destroyed, unless it was called explicitly. Should not raise exceptions.'''
        
        if self.links_are_lost:
            self.document = App.ActiveDocument
            self.viewer = Gui.ActiveDocument.ActiveView
            self.links_are_lost = False
            
        for (obj_name, prop_name, doc_name), value in self.data.items():
            try:
                setattr(App.getDocument(doc_name).getObject(obj_name).ViewObject,prop_name,value)
            except Exception as err:
                App.Console.PrintWarning("TempoVis: failed to restore {obj}.{prop}. {err}\n"
                                         .format(err= err.message,
                                                 obj= obj_name,
                                                 prop= prop_name))
        
        self.restoreUnpickable()
        self.restoreClipPlanes()
        
        try:
            self.restoreCamera()
        except Exception as err:
            App.Console.PrintWarning("TempoVis: failed to restore camera. {err}\n"
                                     .format(err= err.message))
        self.restore_on_delete = False
    
    def restoreVPProperty(self, object_or_list, prop_names):
        """restoreVPProperty(object_or_list, prop_name): restores original values of certain property for certain objects."""
        if App.GuiUp:
            if not hasattr(object_or_list, '__iter__'):
                object_or_list = [object_or_list]
            if not isinstance(prop_names,(tuple,list)):
                prop_names = [prop_names]
            for doc_obj in object_or_list:
                if not self.is3DObject(doc_obj):
                    continue
                for prop_name in prop_names:
                    key = (doc_obj.Name, prop_name, doc_obj.Document.Name)
                    if key in self.data:
                        try:
                            setattr(doc_obj.ViewObject, prop_name, self.data[key])
                        except Exception as err:
                            App.Console.PrintWarning("TempoVis: failed to restore {obj}.{prop}. {err}\n"
                                                    .format(err= err.message,
                                                            obj= doc_obj.Name,
                                                            prop= prop_name))
    

    def forget(self):
        '''forget(): resets TempoVis'''
        self.data = {}
        self.data_pickstyle = {}
        self.data_clipplane = {}

        self.cam_string = ""
        self.viewer = None

        self.restore_on_delete = False

    def __del__(self):
        if self.restore_on_delete:
            self.restore()

    def __getstate__(self):
        return (list(self.data.items()),
                self.cam_string,
                self.restore_on_delete)

    def __setstate__(self, state):
        self.__define_attributes()
        
        items, self.cam_string, self.restore_on_delete = state
        
        # need to convert keys to tuples (dict doesn't accept list as key; tuples are converted to lists by json)
        items = [(tuple(item[0]), item[1]) for item in items]
        self.data = dict(items)
        self.links_are_lost = True
        
    def _getPickStyleNode(self, viewprovider, make_if_missing = True):
        from pivy import coin
        sa = coin.SoSearchAction()
        sa.setType(coin.SoPickStyle.getClassTypeId())
        sa.traverse(viewprovider.RootNode)
        if sa.isFound() and sa.getPath().getLength() == 1:
            return sa.getPath().getTail()
        else:
            if not make_if_missing:
                return None
            pick_style = coin.SoPickStyle()
            pick_style.style.setValue(coin.SoPickStyle.SHAPE)
            viewprovider.RootNode.insertChild(pick_style, 0)
            return pick_style
            
    def _getPickStyle(self, viewprovider):
        ps = self._getPickStyleNode(viewprovider, make_if_missing= False)
        if ps is not None:
            return ps.style.getValue()
        else:
            return 0 #coin.SoPickStyle.SHAPE
    
    def _setPickStyle(self, viewprovider, pickstyle):
        ps = self._getPickStyleNode(viewprovider, make_if_missing= pickstyle != 0) #coin.SoPickStyle.SHAPE
        if ps is not None:
            return ps.style.setValue(pickstyle)

    def setUnpickable(self, doc_obj_or_list, actual_pick_style = 2): #2 is coin.SoPickStyle.UNPICKABLE
        '''setUnpickable(doc_obj_or_list, actual_pick_style = 2): sets object unpickable (transparent to clicks).
        doc_obj_or_list: object or list of objects to alter (App)
        actual_pick_style: optional parameter, specifying the actual pick style: 
        0 = regular, 1 = bounding box, 2 (default) = unpickable.
        
        Implementation detail: uses SoPickStyle node. If viewprovider already has a node 
        of this type as direct child, one is used. Otherwise, new one is created and 
        inserted as the very first node, and remains there even after restore()/deleting 
        tempovis. '''
        
        if App.GuiUp:
            if not hasattr(doc_obj_or_list, '__iter__'):
                doc_obj_or_list = [doc_obj_or_list]
            for doc_obj in doc_obj_or_list:
                if not self.is3DObject(doc_obj):
                    continue
                if doc_obj.Document is not self.document:  #ignore objects from other documents
                    raise ValueError("Document object to be modified does not belong to document TempoVis was made for.")
                oldval = self._getPickStyle(doc_obj.ViewObject)
                if actual_pick_style != oldval:
                    self._setPickStyle(doc_obj.ViewObject, actual_pick_style)
                    self.restore_on_delete = True
                if doc_obj.Name not in self.data_pickstyle:
                    self.data_pickstyle[doc_obj.Name] = oldval
                    
    def restoreUnpickable(self):
        for obj_name in self.data_pickstyle:
            try:
                self._setPickStyle(self.document.getObject(obj_name).ViewObject, self.data_pickstyle[obj_name])
            except Exception as err:
                App.Console.PrintWarning("TempoVis: failed to restore pickability of {obj}. {err}\n"
                                         .format(err= err.message,
                                                 obj= obj_name))
    
    def _getClipplaneNode(self, viewprovider, make_if_missing = True):
        from pivy import coin
        sa = coin.SoSearchAction()
        sa.setType(coin.SoClipPlane.getClassTypeId())
        sa.setName('TVClipPlane')
        sa.traverse(viewprovider.RootNode)
        if sa.isFound() and sa.getPath().getLength() == 1:
            return sa.getPath().getTail()
        elif not sa.isFound():
            if not make_if_missing:
                return None
            clipplane = coin.SoClipPlane()
            viewprovider.RootNode.insertChild(clipplane, 0)
            clipplane.setName('TVClipPlane')
            clipplane.on.setValue(False) #make sure the plane is not activated by default
            return clipplane
            
    def _enableClipPlane(self, obj, enable, placement = None, offset = 0.0):
        """Enables or disables clipping for an object. Placement specifies the plane (plane 
        is placement's XY plane), and should be in global CS. 
        Offset shifts the plane; positive offset reveals more material, negative offset 
        hides more material."""
        if not hasattr(obj, 'getGlobalPlacement'):
            print("    {obj} has no attribute 'getGlobalPlacement'".format(obj= obj.Name))
            return #can't clip these yet... skip them for now. Example object: Draft Label
            
        node = self._getClipplaneNode(obj.ViewObject, make_if_missing= enable)
        if node is None:
            if enable:
                App.Console.PrintError("TempoVis: failed to set clip plane to {obj}.\n".format(obj= obj.Name))
            return
        if placement is not None:
            from pivy import coin
            plm_local = obj.getGlobalPlacement().multiply(obj.Placement.inverse()) # placement of CS the object is in
            plm_plane = plm_local.inverse().multiply(placement)
            normal = plm_plane.Rotation.multVec(App.Vector(0,0,-1))
            basepoint = plm_plane.Base + normal * (-offset)
            normal_coin = coin.SbVec3f(*tuple(normal))
            basepoint_coin = coin.SbVec3f(*tuple(basepoint))
            node.plane.setValue(coin.SbPlane(normal_coin,basepoint_coin))
        node.on.setValue(enable)
        
    def clipPlane(self, doc_obj_or_list, enable, placement, offset = 0.02): 
        '''clipPlane(doc_obj_or_list, enable, placement, offset): slices off the object with a clipping plane.
        doc_obj_or_list: object or list of objects to alter (App)
        enable: True if you want clipping, False if you want to remove clipping: 
        placement: XY plane of local coordinates of the placement is the clipping plane. The placement must be in document's global coordinate system.
        offset: shifts the plane. Positive offset reveals more of the object.
        
        Implementation detail: uses SoClipPlane node. If viewprovider already has a node 
        of this type as direct child, one is used. Otherwise, new one is created and 
        inserted as the very first node. The node is left, but disabled when tempovis is restoring.'''
        
        if App.GuiUp:
            if not hasattr(doc_obj_or_list, '__iter__'):
                doc_obj_or_list = [doc_obj_or_list]
            for doc_obj in doc_obj_or_list:
                if not self.is3DObject(doc_obj):
                    continue
                #  if doc_obj.Document is not self.document:  #ignore objects from other documents
                #      raise ValueError("Document object to be modified does not belong to document TempoVis was made for.")
                print("Clipping {obj}".format(obj= doc_obj.Name))
                self._enableClipPlane(doc_obj, enable, placement, offset)
                self.restore_on_delete = True
                if doc_obj.Name not in self.data_pickstyle:
                    self.data_clipplane[doc_obj.Name] = False

    def restoreClipPlanes(self):
        self.sketchClipPlane(None,False)
        for obj_name in self.data_clipplane:
            try:
                self._enableClipPlane(self.document.getObject(obj_name), self.data_clipplane[obj_name])
            except Exception as err:
                App.Console.PrintWarning("TempoVis: failed to remove clipplane for {obj}. {err}\n"
                                         .format(err= err.message,
                                                 obj= obj_name))

    @staticmethod
    def allVisibleObjects(aroundObject):
        """allVisibleObjects(aroundObject): returns list of objects that have to be toggled invisible for only aroundObject to remain. 
        If a whole container can be made invisible, it is returned, instead of its child objects."""
        
        chain = Containers.VisGroupChain(aroundObject)
        result = []
        for i in range(len(chain)):
            cnt = chain[i]
            cnt_next = chain[i+1] if i+1 < len(chain) else aroundObject
            container = Container(cnt)
            for obj in container.getVisGroupChildren():
                if not TempoVis.is3DObject(obj):
                    continue
                if obj is not cnt_next:
                    if container.isChildVisible(obj):
                        result.append(obj)
        return result
        
    def sketchClipPlane(self, sketch, enable = None):
        """sketchClipPlane(sketch, enable = None): Clips all objects by plane of sketch. 
        If enable argument is omitted, calling the routine repeatedly will toggle clipping plane."""
        
        editDoc = self.clippingDoc
        if not editDoc:
            editDoc = Gui.editDocument()
        if editDoc:
            self.clippingDoc = editDoc
            toggle = 1 if enable else -1 if enable is None else 0
            editDoc.ActiveView.toggleClippingPlane(
                    toggle, pla=App.Placement(editDoc.EditingTransform))
            return
        elif not sketch:
            return

        if enable is None:
            enable = not self.sketch_clipplane_on
            self.sketch_clipplane_on = enable
        self.clipPlane(self.allVisibleObjects(sketch), enable, sketch.getGlobalPlacement(), 0.02)


