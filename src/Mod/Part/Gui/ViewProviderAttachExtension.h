// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <Gui/ViewProviderExtensionPython.h>
#include <Mod/Part/PartGlobal.h>


namespace PartGui
{

class PartGuiExport ViewProviderAttachExtension: public Gui::ViewProviderExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderAttachExtension);

public:
    /// Constructor
    ViewProviderAttachExtension();
    ~ViewProviderAttachExtension() override = default;

    QIcon extensionMergeColorfullOverlayIcons(const QIcon& orig) const override;

    void extensionUpdateData(const App::Property*) override;
    void extensionSetupContextMenu(QMenu*, QObject*, const char*) override;

    void showAttachmentEditor(std::function<void()> onAccept = {}, std::function<void()> onReject = {});
};

using ViewProviderAttachExtensionPython
    = Gui::ViewProviderExtensionPythonT<PartGui::ViewProviderAttachExtension>;

}  // namespace PartGui
