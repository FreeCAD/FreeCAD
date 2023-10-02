/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


namespace PartGui {

class PartGuiExport ViewProviderBoolean : public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderBoolean);

public:
    /// constructor
    ViewProviderBoolean();
    /// destructor
    ~ViewProviderBoolean() override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren() const override;
    QIcon getIcon() const override;
    void updateData(const App::Property*) override;
    bool onDelete(const std::vector<std::string> &) override;
};

/// ViewProvider for the MultiFuse feature
class PartGuiExport ViewProviderMultiFuse : public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderMultiFuse);

public:
    /// constructor
    ViewProviderMultiFuse();
    /// destructor
    ~ViewProviderMultiFuse() override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren() const override;
    QIcon getIcon() const override;
    void updateData(const App::Property*) override;
    bool onDelete(const std::vector<std::string> &) override;

    /// drag and drop
    bool canDragObjects() const override;
    bool canDragObject(App::DocumentObject*) const override;
    void dragObject(App::DocumentObject*) override;
    bool canDropObjects() const override;
    bool canDropObject(App::DocumentObject*) const override;
    void dropObject(App::DocumentObject*) override;
};

/// ViewProvider for the MultiFuse feature
class PartGuiExport ViewProviderMultiCommon : public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderMultiCommon);

public:
    /// constructor
    ViewProviderMultiCommon();
    /// destructor
    ~ViewProviderMultiCommon() override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren() const override;
    QIcon getIcon() const override;
    void updateData(const App::Property*) override;
    bool onDelete(const std::vector<std::string> &) override;

    /// drag and drop
    bool canDragObjects() const override;
    bool canDragObject(App::DocumentObject*) const override;
    void dragObject(App::DocumentObject*) override;
    bool canDropObjects() const override;
    bool canDropObject(App::DocumentObject*) const override;
    void dropObject(App::DocumentObject*) override;
};


} // namespace PartGui


#endif // PARTGUI_ViewProviderBoolean_H
