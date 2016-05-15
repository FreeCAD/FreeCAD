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

from FrozenClass import FrozenClass

from DepGraphTools import getAllDependencies, getAllDependent, isContainer

class TempoVis(FrozenClass):
    '''TempoVis - helper object to save visibilities of objects before doing 
    some GUI editing, hiding or showing relevant stuff during edit, and 
    then restoring all visibilities after editing.
    
    Constructors:
    TempoVis(document): creates a new TempoVis. Supplying document is mandatory. Objects not belonging to the document can't be modified via TempoVis.'''
    
    def __define_attributes(self):
        self.data = {} # dict. key = ("Object","Property"), value = original value of the property
        self.document = None
        self.restore_on_delete = False
        
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
            if type(doc_obj_or_list) is not list:
                doc_obj_or_list = [doc_obj_or_list]
            for doc_obj in doc_obj_or_list:
                if doc_obj.Document is not self.document:  #ignore objects from other documents
                    raise ValueError("Document object to be modified does not belong to document TempoVis was made for.")
                oldval = getattr(doc_obj.ViewObject, prop_name)
                setattr(doc_obj.ViewObject, prop_name, new_value)
                #assert(getattr(doc_obj.ViewObject, prop_name)==new_value)
                if not self.data.has_key((doc_obj.Name,prop_name)):
                    self.data[(doc_obj.Name,prop_name)] = oldval
                    self.restore_on_delete = True
    
    def show(self, doc_obj_or_list):
        '''show(doc_obj_or_list): shows objects (sets their Visibility to True). doc_obj_or_list can be a document object, or a list of document objects'''
        self.modifyVPProperty(doc_obj_or_list, "Visibility", True)
        
    def hide(self, doc_obj_or_list):
        '''hide(doc_obj_or_list): hides objects (sets their Visibility to False). doc_obj_or_list can be a document object, or a list of document objects'''
        self.modifyVPProperty(doc_obj_or_list, "Visibility", False)
    
    def hide_all_dependent(self, doc_obj):
        '''hide_all_dependent(doc_obj): hides all objects that depend on doc_obj. Groups, Parts and Bodies are not hidden by this.'''
        self.hide( [o for o in getAllDependent(doc_obj) if not isContainer(o)])
                
    def show_all_dependent(self, doc_obj):
        '''show_all_dependent(doc_obj): shows all objects that depend on doc_obj. This method is probably useless.'''
        self.show( getAllDependent(doc_obj) )

    def hide_all_dependencies(self, doc_obj):
        '''hide_all_dependencies(doc_obj): hides all objects that doc_obj depends on (directly and indirectly).'''
        self.hide( getAllDependencies(doc_obj) )
                
    def show_all_dependencies(self, doc_obj):
        '''show_all_dependencies(doc_obj): shows all objects that doc_obj depends on (directly and indirectly). This method is probably useless.'''
        self.show( getAllDependencies(doc_obj) )
            
    def restore(self):
        '''restore(): restore all ViewProvider properties modified via TempoVis to their original values. Called automatically when instance is destroyed, unless it was called explicitly.'''
        for obj_name, prop_name in self.data:
            setattr(self.document.getObject(obj_name).ViewObject, prop_name, self.data[(obj_name, prop_name)])
        self.restore_on_delete = False
    
    def forget(self):
        '''forget(): resets TempoVis'''
        self.data = {}
        self.restore_on_delete = False        
        
    def __del__(self):
        if self.restore_on_delete:
            self.restore()
    
    