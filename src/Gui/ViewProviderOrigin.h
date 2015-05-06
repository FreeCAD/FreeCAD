/***************************************************************************
 *   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2015     *
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


#ifndef GUI_VIEWPROVIDER_ViewProviderOrigin_H
#define GUI_VIEWPROVIDER_ViewProviderOrigin_H


#include "ViewProviderGeoFeatureGroup.h"

#include <App/PropertyStandard.h>
#include <App/Origin.h>



namespace Gui {

class Document;  
  
class GuiExport ViewProviderOrigin : public ViewProviderGeoFeatureGroup
{
    PROPERTY_HEADER(Gui::ViewProviderOrigin);

public:
    /// constructor.
    ViewProviderOrigin();
    /// destructor.
    virtual ~ViewProviderOrigin();
    
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    
    virtual QIcon getIcon(void) const;
    
    //temporary mode to override visibility of grouped objects
    void setTemporaryVisibilityMode(bool onoff, Gui::Document* doc = NULL);
    bool isTemporaryVisibilityMode();
    void setTemporaryVisibility(App::DocumentObject* obj, bool onoff);
    
private:
    bool tempVisMode = false;
    Gui::Document* tempVisDoc;
    std::map<Gui::ViewProvider*, bool> tempVisMap; 
};

typedef ViewProviderPythonFeatureT<ViewProviderOrigin> ViewProviderOriginPython;

} // namespace Gui

#endif // GUI_VIEWPROVIDER_DOCUMENTOBJECTGROUP_H

