/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Gui/ViewProviderExtensionPython.h>

namespace TechDrawGui
{

class TechDrawGuiExport ViewProviderCosmeticExtension : public Gui::ViewProviderExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderCosmeticExtension);

public:
    /// Constructor
    ViewProviderCosmeticExtension();
    ~ViewProviderCosmeticExtension() override = default;

    QIcon extensionMergeGreyableOverlayIcons(const QIcon & orig) const override;
};

using ViewProviderCosmeticExtensionPython = Gui::ViewProviderExtensionPythonT<TechDrawGui::ViewProviderCosmeticExtension>;

} //namespace TechDrawGui