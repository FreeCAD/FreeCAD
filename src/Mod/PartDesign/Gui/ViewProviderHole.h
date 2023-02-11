/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTGUI_ViewProviderHole_H
#define PARTGUI_ViewProviderHole_H

#include "ViewProvider.h"


namespace PartDesignGui {

class PartDesignGuiExport ViewProviderHole : public ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderHole);

public:
    /// constructor
    ViewProviderHole();
    /// destructor
    ~ViewProviderHole() override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren()const override;
    void setupContextMenu(QMenu *menu, QObject *receiver, const char *member) override;
    bool onDelete(const std::vector<std::string> &s) override;
protected:
    bool setEdit(int ModNum) override;
};



} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderHole_H
