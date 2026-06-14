/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include <Gui/ViewProviderFeaturePython.h>
#include "ViewProviderFemPostObject.h"

namespace FemGui
{

// ***************************************************************************
// Special classes to enable python filter view providers
// ***************************************************************************

// Special class for the python view providers, which need some special behaviour
class FemGuiExport ViewProviderFemPostFilterPythonBase: public ViewProviderFemPostObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostFilterPythonBase);

public:
    /// constructor.
    ViewProviderFemPostFilterPythonBase();
    ~ViewProviderFemPostFilterPythonBase() override;

    // we do not use default display modes but let the python implementation choose
    // Python view provider needs to return a sublist of PostObject supporter DisplayModes
    std::vector<std::string> getDisplayModes() const override;
};


// Viewprovider for the python filters
using ViewProviderPostFilterPython
    = Gui::ViewProviderFeaturePythonT<ViewProviderFemPostFilterPythonBase>;


// ***************************************************************************
// in the following, the different filters sorted alphabetically
// ***************************************************************************


// ***************************************************************************
// data along line filter
class FemGuiExport ViewProviderFemPostDataAlongLine: public ViewProviderFemPostObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostDataAlongLine);

public:
    /// constructor.
    ViewProviderFemPostDataAlongLine();
    ~ViewProviderFemPostDataAlongLine() override;

protected:
    void setupTaskDialog(TaskDlgPost* dlg) override;
};


// ***************************************************************************
// data at point filter
class FemGuiExport ViewProviderFemPostDataAtPoint: public ViewProviderFemPostObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostDataAtPoint);

public:
    /// constructor.
    ViewProviderFemPostDataAtPoint();

    App::PropertyFloatConstraint PointSize;

    void show() override;
    void onChanged(const App::Property* prop) override;
    void onSelectionChanged(const Gui::SelectionChanges&) override;
    ~ViewProviderFemPostDataAtPoint() override;

protected:
    void setupTaskDialog(TaskDlgPost* dlg) override;

private:
    static App::PropertyFloatConstraint::Constraints sizeRange;
};


// ***************************************************************************
// clip filter
class FemGuiExport ViewProviderFemPostClip: public ViewProviderFemPostObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostClip);

public:
    /// constructor.
    ViewProviderFemPostClip();
    ~ViewProviderFemPostClip() override;

protected:
    void setupTaskDialog(TaskDlgPost* dlg) override;
};


// ***************************************************************************
// contours filter
class FemGuiExport ViewProviderFemPostContours: public ViewProviderFemPostObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostContours);

public:
    /// constructor.
    ViewProviderFemPostContours();
    ~ViewProviderFemPostContours() override;

protected:
    void setupTaskDialog(TaskDlgPost* dlg) override;
};


// ***************************************************************************
// cut filter
class FemGuiExport ViewProviderFemPostCut: public ViewProviderFemPostObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostCut);

public:
    /// constructor.
    ViewProviderFemPostCut();
    ~ViewProviderFemPostCut() override;

protected:
    void setupTaskDialog(TaskDlgPost* dlg) override;
};


// ***************************************************************************
// scalar clip filter
class FemGuiExport ViewProviderFemPostScalarClip: public ViewProviderFemPostObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostScalarClip);

public:
    /// constructor.
    ViewProviderFemPostScalarClip();
    ~ViewProviderFemPostScalarClip() override;

protected:
    void setupTaskDialog(TaskDlgPost* dlg) override;
};


// ***************************************************************************
// warp vector filter
class FemGuiExport ViewProviderFemPostWarpVector: public ViewProviderFemPostObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostWarpVector);

public:
    /// constructor.
    ViewProviderFemPostWarpVector();
    ~ViewProviderFemPostWarpVector() override;

protected:
    void setupTaskDialog(TaskDlgPost* dlg) override;
};


// ***************************************************************************
// calculator filter
class FemGuiExport ViewProviderFemPostCalculator: public ViewProviderFemPostObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostCalculator);

public:
    /// constructor.
    ViewProviderFemPostCalculator();
    ~ViewProviderFemPostCalculator() override;

    void updateData(const App::Property* prop) override;

protected:
    void setupTaskDialog(TaskDlgPost* dlg) override;
};

}  // namespace FemGui
