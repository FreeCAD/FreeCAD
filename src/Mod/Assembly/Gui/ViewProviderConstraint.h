/***************************************************************************
 *   Copyright (c) 2013 Stefan Tröger <stefantroeger@gmx.net>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef ASSEMBLYGUI_VIEWPROVIDERCONSTRAINT_H
#define ASSEMBLYGUI_VIEWPROVIDERCONSTRAINT_H

#include <Mod/Part/Gui/ViewProvider.h>
#include <Base/Placement.h>
#include <Gui/Selection.h>
#include <Inventor/nodes/SoAnnotation.h>
#include "Mod/Assembly/App/Item.h"

namespace AssemblyGui {

//class for internal use to allow access to protected functions of view provider part.
class AssemblyGuiExport ViewProviderConstraintInternal : public PartGui::ViewProviderPart {

    PROPERTY_HEADER(AssemblyGui::ViewProviderConstraintInternal);

public:
    ViewProviderConstraintInternal();
    void updateVis(const TopoDS_Shape& shape);
    void setDisplayMM(const char* mode) {
        setDisplayMaskMode(mode);
    };
    void onChGO(const App::Property* prop) {
        ViewProviderGeometryObject::onChanged(prop);
    };
    void onChPa(const App::Property* prop) {
        ViewProviderPart::onChanged(prop);
    };
    //update the transformation node with a Placement
    void updatePlacement(const Base::Placement& p);
    //switch the display mode node on or off
    void switch_node(bool onoff);
};


//this class adds highlight functionality to the constraint: when a constraint is selected
//in the tree all used geometries are shown
class AssemblyGuiExport ViewProviderConstraint: public PartGui::ViewProviderPart,
    public Gui::SelectionObserver  {

    PROPERTY_HEADER(AssemblyGui::ViewProviderConstraint);

public:
    ViewProviderConstraint();

    //attach needs to be overridden to attach the second viewprovider and to include
    //annotation nodes
    virtual void attach(App::DocumentObject* pcObj);
    
    //update is for visual only
    virtual void update(const App::Property*);
    //needs to be overridden as this viewprovider dos not represent a Part::Feature
    virtual void updateData(const App::Property*);
    //needs to be overridden as this viewprovider dos not represent a Part::Feature
    virtual void onChanged(const App::Property* prop);
    
    //get the shape which is used by the constraint for highlighting
    virtual TopoDS_Shape getConstraintShape(int link);
    
    //needs to be overridden as we use the modeselection node for on and off and not for
    //hide and show in the normal way
    virtual bool isShow(void) const;
    
    //avoid unneeded context menu entries
    virtual void setupContextMenu(QMenu* menu, QObject* receiver, const char* member);

    //only flat lines supported
    virtual void setDisplayMode(const char* ModeName);
    virtual std::vector<std::string> getDisplayModes(void) const;
    
    //bring up constraint task when in edit mode
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    
    //update visualisation and placements of the scenegraph
    void draw();
    
private:
    //we need two separate visual representations, as both constraint parts have different
    //placements.
    ViewProviderConstraintInternal internal_vp;
    
    void upstream_placement(Base::Placement& p, Assembly::Item* item);
    
    //watch if something got selected in the tree
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    
    bool m_selected;
};

};

#endif // VIEWPROVIDERCONSTRAINTFIX_H
