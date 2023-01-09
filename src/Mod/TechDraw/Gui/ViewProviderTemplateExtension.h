/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAWGUI_VIEWPROVIDERTEMPLATEEXTENSION_H
#define TECHDRAWGUI_VIEWPROVIDERTEMPLATEEXTENSION_H

#include <Gui/ViewProviderExtension.h>
#include <Gui/ViewProviderExtensionPy.h>
#include <Gui/ViewProviderExtensionPython.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDrawGui
{
class ViewProviderTemplate;

class TechDrawGuiExport ViewProviderTemplateExtension: public Gui::ViewProviderExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderTemplateExtension);

public:
    /// Constructor
    ViewProviderTemplateExtension();
    ~ViewProviderTemplateExtension() override;

    bool extensionCanDragObjects() const override;
    bool extensionCanDragObject(App::DocumentObject* docObj) const override;
    bool extensionCanDropObjects() const override;
    bool extensionCanDropObject(App::DocumentObject* docObj) const override;

    const ViewProviderTemplate* getViewProviderTemplate() const;
    const char* whoAmI() const;

private:
};

using ViewProviderTemplateExtensionPython =
    Gui::ViewProviderExtensionPythonT<TechDrawGui::ViewProviderTemplateExtension>;

}// namespace TechDrawGui

#endif// TECHDRAWGUI_VIEWPROVIDERTEMPLATEEXTENSION_H
