/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#ifndef PARTGUI_ViewProviderBoolean_H
#define PARTGUI_ViewProviderBoolean_H

#include "ViewProvider.h"
#include <Gui/ViewProviderGeoFeatureGroupExtension.h>


namespace PartDesignGui {

class PartDesignGuiExport ViewProviderBoolean : public ViewProvider,
                                                public Gui::ViewProviderGeoFeatureGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesignGui::ViewProviderBoolean);

public:
    /// constructor
    ViewProviderBoolean();
    /// destructor
    ~ViewProviderBoolean() override;

    App::PropertyEnumeration Display;

    /// grouping handling
    void setupContextMenu(QMenu*, QObject*, const char*) override;

    bool onDelete(const std::vector<std::string> &) override;
    void attach(App::DocumentObject*) override;
    void onChanged(const App::Property* prop) override;

protected:
    bool setEdit(int ModNum) override;

    static const char* DisplayEnum[];

};

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderBoolean_H
