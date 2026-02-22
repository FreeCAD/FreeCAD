// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#pragma once

#include <Gui/ViewProviderFeaturePython.h>
#include <Mod/Part/Gui/ViewProviderPlaneParametric.h>
#include <Mod/CAM/PathGlobal.h>

namespace PathGui
{

class PathGuiExport ViewProviderArea: public PartGui::ViewProviderPlaneParametric
{
    PROPERTY_HEADER_WITH_OVERRIDE(PathGui::ViewProviderArea);

public:
    ViewProviderArea();
    ~ViewProviderArea() override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren() const override;
    void updateData(const App::Property*) override;
    bool onDelete(const std::vector<std::string>&) override;

    /// drag and drop
    bool canDragObjects() const override;
    bool canDragObject(App::DocumentObject*) const override;
    void dragObject(App::DocumentObject*) override;
    bool canDropObjects() const override;
    bool canDropObject(App::DocumentObject*) const override;
    void dropObject(App::DocumentObject*) override;
};

using ViewProviderAreaPython = Gui::ViewProviderFeaturePythonT<ViewProviderArea>;


class PathGuiExport ViewProviderAreaView: public PartGui::ViewProviderPlaneParametric
{
    PROPERTY_HEADER_WITH_OVERRIDE(PathGui::ViewProviderAreaView);

public:
    ViewProviderAreaView();
    ~ViewProviderAreaView() override;
    std::vector<App::DocumentObject*> claimChildren() const override;
    void updateData(const App::Property*) override;
    bool onDelete(const std::vector<std::string>&) override;

    /// drag and drop
    bool canDragObjects() const override;
    bool canDragObject(App::DocumentObject*) const override;
    void dragObject(App::DocumentObject*) override;
    bool canDropObjects() const override;
    bool canDropObject(App::DocumentObject*) const override;
    void dropObject(App::DocumentObject*) override;
};

using ViewProviderAreaViewPython = Gui::ViewProviderFeaturePythonT<ViewProviderAreaView>;

}  // namespace PathGui
