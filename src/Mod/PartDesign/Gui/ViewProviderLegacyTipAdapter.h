/***************************************************************************
 *   Copyright (c) 2025 Walter Steffè <walter.steffe@hierarchical-electromagnetics.com> *
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


#ifndef PARTDESIGN_VIEWPROVIDERLEGACYTIPADAPTER_H
#define PARTDESIGN_VIEWPROVIDERLEGACYTIPADAPTER_H

#include <Mod/PartDesign/Gui/ViewProvider.h>
#include <QIcon>
#include <vector>

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderLegacyTipAdapter : public PartDesignGui::ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderLegacyTipAdapter);
public:
    ViewProviderLegacyTipAdapter();
    ~ViewProviderLegacyTipAdapter() override = default;

    QIcon getIcon() const override;
    std::vector<App::DocumentObject*> claimChildren() const override;
    void attach(App::DocumentObject* obj) override;
    void onChanged(const App::Property* prop) override;

};

} // namespace PartDesignGui



#endif // PARTDESIGN_VIEWPROVIDERLEGACYTIPADAPTER_H
