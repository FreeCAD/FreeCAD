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


#ifndef PARTGUI_ViewProviderHelix_H
#define PARTGUI_ViewProviderHelix_H

#include "ViewProviderAddSub.h"


namespace PartDesignGui {

class PartDesignGuiExport ViewProviderHelix : public ViewProviderAddSub
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderHelix);

public:
    /// constructor
    ViewProviderHelix();
    /// destructor
    ~ViewProviderHelix() override;

    void setupContextMenu(QMenu*, QObject*, const char*) override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren() const override;

    bool onDelete(const std::vector<std::string> &) override;

protected:
    QIcon getIcon() const override;

    /// Returns a newly created TaskDlgHelixParameters
    TaskDlgFeatureParameters *getEditDialog() override;
    bool  setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
};


} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderHelix_H
