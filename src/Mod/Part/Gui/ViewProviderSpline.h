// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Mod/Part/Gui/ViewProviderExt.h>
#include <Gui/ViewProviderExtensionPython.h>

#include <Mod/Part/PartGlobal.h>

namespace PartGui
{

class PartGuiExport ViewProviderSplineExtension: public Gui::ViewProviderExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderSplineExtension);

public:
    /// Constructor
    ViewProviderSplineExtension();
    ~ViewProviderSplineExtension() override = default;

    App::PropertyBool ControlPoints;

    void extensionUpdateData(const App::Property*) override;
    void extensionSetupContextMenu(QMenu*, QObject*, const char*) override;

protected:
    void extensionOnChanged(const App::Property* p) override;
    void toggleControlPoints(bool);
    void showControlPoints(bool, const App::Property* prop);
    void showControlPointsOfEdge(const TopoDS_Edge&);
    void showControlPointsOfFace(const TopoDS_Face&);

    SoSwitch* pcControlPoints {nullptr};
};

class PartGuiExport ViewProviderSpline: public ViewProviderPartExt
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderSpline);

public:
    /// constructor
    ViewProviderSpline();
    /// destructor
    ~ViewProviderSpline() override;

    QIcon getIcon() const override;

private:
    ViewProviderSplineExtension extension;
};

using ViewProviderSplineExtensionPython
    = Gui::ViewProviderExtensionPythonT<PartGui::ViewProviderSplineExtension>;

}  // namespace PartGui
