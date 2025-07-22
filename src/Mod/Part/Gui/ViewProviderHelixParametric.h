/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef PARTGUI_VIEWPROVIDERHELIXPARAMETRIC_H
#define PARTGUI_VIEWPROVIDERHELIXPARAMETRIC_H

#include "ViewProviderSpline.h"
#include "ViewProviderPrimitive.h"

namespace PartGui {


class PartGuiExport ViewProviderHelixParametric : public ViewProviderPrimitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderHelixParametric);

public:
    /// constructor
    ViewProviderHelixParametric();
    /// destructor
    ~ViewProviderHelixParametric() override;
    std::vector<std::string> getDisplayModes() const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;

private:
    ViewProviderSplineExtension extension;
};

class PartGuiExport ViewProviderSpiralParametric : public ViewProviderPrimitive
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderSpiralParametric);

public:
    /// constructor
    ViewProviderSpiralParametric();
    /// destructor
    ~ViewProviderSpiralParametric() override;
    std::vector<std::string> getDisplayModes() const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;

private:
    ViewProviderSplineExtension extension;
};

} // namespace PartGui


#endif // PARTGUI_VIEWPROVIDERHELIXPARAMETRIC_H

