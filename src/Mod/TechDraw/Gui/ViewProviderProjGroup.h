/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef DRAWINGGUI_VIEWPROVIDERVIEWGROUP_H
#define DRAWINGGUI_VIEWPROVIDERVIEWGROUP_H

#include <Gui/ViewProviderFeature.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>

#include "ViewProviderDrawingView.h"

namespace TechDrawGui {

class TechDrawGuiExport ViewProviderProjGroup : public ViewProviderDrawingView
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderProjGroup);

public:

     ViewProviderProjGroup();  /// constructor
     ~ViewProviderProjGroup(); /// destructor

    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    virtual bool useNewSelectionModel(void) const {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;

    /// Claim all the views for the page
    std::vector<App::DocumentObject*> claimChildren(void) const;

    /// Is called by the tree if the user double click on the object
    virtual bool doubleClicked(void);
    void setupContextMenu(QMenu*, QObject*, const char*);
    virtual void updateData(const App::Property*);

    TechDraw::DrawProjGroup* getObject() const;
    virtual TechDraw::DrawProjGroup* getViewObject() const;
    void unsetEdit(int ModNum);
    virtual void onChanged(const App::Property *prop);
    virtual bool onDelete(const std::vector<std::string> &);
    virtual bool canDelete(App::DocumentObject* obj) const;

protected:
    bool setEdit(int ModNum);

};

} // namespace TechDrawGui

#endif // DRAWINGGUI_VIEWPROVIDERVIEWGROUP_H
