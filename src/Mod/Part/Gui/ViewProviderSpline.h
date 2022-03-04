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

#ifndef PARTGUI_VIEWPROVIDERPARTSPLINE_H
#define PARTGUI_VIEWPROVIDERPARTSPLINE_H

#include <Mod/Part/Gui/ViewProviderExt.h>
#include <Gui/ViewProviderExtensionPython.h>

namespace PartGui
{

class PartGuiExport ViewProviderSplineExtension : public Gui::ViewProviderExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderSplineExtension);

public:
    /// Constructor
    ViewProviderSplineExtension();
    virtual ~ViewProviderSplineExtension() = default;

    App::PropertyBool ControlPoints;

    virtual void extensionUpdateData(const App::Property*) override;
    virtual void extensionSetupContextMenu(QMenu*, QObject*, const char*) override;

protected:
    virtual void extensionOnChanged(const App::Property* p) override;
    void toggleControlPoints(bool);
    void showControlPoints(bool, const App::Property* prop);
    void showControlPointsOfEdge(const TopoDS_Edge&);
    void showControlPointsOfFace(const TopoDS_Face&);

    SoSwitch     *pcControlPoints;
};

class PartGuiExport ViewProviderSpline : public ViewProviderPartExt
{
    PROPERTY_HEADER(PartGui::ViewProviderSpline);

public:
    /// constructor
    ViewProviderSpline();
    /// destructor
    virtual ~ViewProviderSpline();

    QIcon getIcon() const;

private:
    ViewProviderSplineExtension extension;
};

typedef Gui::ViewProviderExtensionPythonT<PartGui::ViewProviderSplineExtension> ViewProviderSplineExtensionPython;

} //namespace PartGui


#endif // PARTGUI_VIEWPROVIDERPARTSPLINE_H

