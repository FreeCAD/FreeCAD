/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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


#ifndef GUI_VIEWPROVIDER_ViewProviderGeoFeatureGroup_H
#define GUI_VIEWPROVIDER_ViewProviderGeoFeatureGroup_H


#include "ViewProviderDocumentObjectGroup.h"

namespace Gui {

class GuiExport ViewProviderGeoFeatureGroup : public ViewProviderDocumentObjectGroup
{
    PROPERTY_HEADER(Gui::ViewProviderGeoFeatureGroup);

public:
    /// constructor.
    ViewProviderGeoFeatureGroup();
    /// destructor.
    virtual ~ViewProviderGeoFeatureGroup();

    virtual std::vector<App::DocumentObject*> claimChildren3D(void)const;

    virtual SoGroup* getChildRoot(void) const {return pcGroupChildren;};

    virtual void attach(App::DocumentObject* pcObject);
    virtual void setDisplayMode(const char* ModeName);
    virtual std::vector<std::string> getDisplayModes(void) const;

    /// Show the object in the view: suppresses behavior of DocumentObjectGroup
    virtual void show(void) {
        ViewProviderDocumentObject::show();
    }
    /// Hide the object in the view: suppresses behavior of DocumentObjectGroup
    virtual void hide(void) {
        ViewProviderDocumentObject::hide();
    }

    virtual void updateData(const App::Property*);
protected:
    SoGroup *pcGroupChildren;
};

typedef ViewProviderPythonFeatureT<ViewProviderGeoFeatureGroup> ViewProviderGeoFeatureGroupPython;

} // namespace Gui

#endif // GUI_VIEWPROVIDER_DOCUMENTOBJECTGROUP_H

