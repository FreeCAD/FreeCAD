/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#pragma once

#include "ViewProviderExtensionPython.h"


namespace Gui
{

class GuiExport ViewProviderGroupExtension: public ViewProviderExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderGroupExtension);

public:
    /// Constructor
    ViewProviderGroupExtension();
    ~ViewProviderGroupExtension() override;

    std::vector<App::DocumentObject*> extensionClaimChildren() const override;
    bool extensionCanDragObjects() const override;
    bool extensionCanDragObject(App::DocumentObject*) const override;
    void extensionDragObject(App::DocumentObject*) override;
    bool extensionCanDropObjects() const override;
    bool extensionCanDropObject(App::DocumentObject*) const override;
    void extensionDropObject(App::DocumentObject*) override;

    void extensionHide() override;
    void extensionShow() override;

    bool extensionOnDelete(const std::vector<std::string>&) override;

private:
    bool guard {false};
    std::vector<ViewProvider*> nodes;
};

using ViewProviderGroupExtensionPython = ViewProviderExtensionPythonT<Gui::ViewProviderGroupExtension>;

}  // namespace Gui
