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

from Show.FrozenClass import FrozenClass

from Show.DepGraphTools import getAllDependencies, getAllDependent, isContainer

class TempoVis(FrozenClass):
    '''TempoVis - helper object to save visibilities of objects before doing
    some GUI editing, hiding or showing relevant stuff during edit, and
    then restoring all visibilities after editing.

    Constructors:
    TempoVis(document): creates a new TempoVis. Supplying document is mandatory. Objects not belonging to the document can't be modified via TempoVis.'''

    def __define_attributes(self):
        self.data = {} # dict. key = ("Object","Property"), value = original value of the property
        self.data_pickstyle = {} # dict. key = "Object", value = original value of pickstyle

        self.cam_string = ""          # inventor ASCII string representing the camera
        self.viewer = None            # viewer the camera is saved from

        self.document = None
        self.restore_on_delete = False # if true, restore() gets called upon object deletion. It becomes False after explicit call to Restore, and set to true by many methods.
        
        self.links_are_lost = False # set to true after restore from JSON. Indicates to attempt to use ActiveDocument/ActiveViewer instead.
        
        self._freeze()

    def __init__(self, document):
        self.__define_attributes()

        self.document = document

    def modifyVPProperty(self, doc_obj_or_list, prop_name, new_value):
        '''modifyVPProperty(self, doc_obj_or_list, prop_name, new_value): modifies
        prop_name property of ViewProvider of doc_obj_or_list, and remembers
        original value of the property. Original values will be restored upon
        TempoVis deletion, or call to restore().'''

        if App.GuiUp:
            if not hasattr(doc_obj_or_list, '__iter__'):
                doc_obj_or_list = [doc_obj_or_list]
            for doc_obj in doc_obj_or_list:
                if not hasattr(doc_obj.ViewObject, prop_name):
                    App.Console.PrintWarning("TempoVis: object {obj} has no attribute {attr}. Skipped.\n"
                                             .format(obj= doc_obj.Name, attr= prop_name))
                    continue # silently ignore if object doesn't have the property...

                if doc_obj.Document is not self.document:  #ignore objects from other documents
                    raise ValueError("Document object to be modified does not belong to document TempoVis was made for.")
                oldval = getattr(doc_obj.ViewObject, prop_name)
                setattr(doc_obj.ViewObject, prop_name, new_value)
                self.restore_on_delete = True
                if (doc_obj.Name,prop_name) not in self.data:
                    self.data[(doc_obj.Name,prop_name)] = oldval

    def show(self, doc_obj_or_list):
        '''show(doc_obj_or_list): shows objects (sets their Visibility to True). doc_obj_or_list can be a document object, or a list of document objects'''
        self.modifyVPProperty(doc_obj_or_list, "Visibility", True)

    def hide(self, doc_obj_or_list):
        '''hide(doc_obj_or_list): hides objects (sets their Visibility to False). doc_obj_or_list can be a document object, or a list of document objects'''
        self.modifyVPProperty(doc_obj_or_list, "Visibility", False)

    def get_all_dependent(self, doc_obj):
        '''get_all_dependent(doc_obj): gets all objects that depend on doc_obj. Groups, Parts and Bodies are not hidden by this.'''
        return [o for o in getAllDependent(doc_obj) if not isContainer(o)]

    def hide_all_dependent(self, doc_obj):
        '''hide_all_dependent(doc_obj): hides all objects that depend on doc_obj. Groups, Parts and Bodies are not hidden by this.'''
        self.hide( self.get_all_dependent(doc_obj) )

    def show_all_dependent(self, doc_obj):
        '''show_all_dependent(doc_obj): shows all objects that depend on doc_obj. This method is probably useless.'''
        self.show( getAllDependent(doc_obj) )

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
            
        for obj_name, prop_name in self.data:
            try:
                setattr(self.document.getObject(obj_name).ViewObject, prop_name, self.data[(obj_name, prop_name)])
            except Exception as err:
                App.Console.PrintWarning("TempoVis: failed to restore {obj}.{prop}. {err}\n"
                                         .format(err= err.message,
                                                 obj= obj_name,
                                                 prop= prop_name))
        
        self.restoreUnpickable()
        
        try:
            self.restoreCamera()
        except Exception as err:
            App.Console.PrintWarning("TempoVis: failed to restore camera. {err}\n"
                                     .format(err= err.message))
        self.restore_on_delete = False

    def forget(self):
        '''forget(): resets TempoVis'''
        self.data = {}
        self.data_pickstyle = {}

        self.cam_string = ""
        self.viewer = None

        self.restore_on_delete = False

    def __del__(self):
        if self.restore_on_delete:
            self.restore()

    def __getstate__(self):
        return (self.data.items(), 
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
    
