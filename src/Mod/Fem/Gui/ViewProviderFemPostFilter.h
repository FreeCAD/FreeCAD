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

#ifndef FEM_VIEWPROVIDERFEMPOSTFILTER_H
#define FEM_VIEWPROVIDERFEMPOSTFILTER_H

#include "ViewProviderFemPostObject.h"


namespace FemGui
{

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
    void show() override;
    void onSelectionChanged(const Gui::SelectionChanges&) override;
    ~ViewProviderFemPostDataAtPoint() override;

protected:
    void setupTaskDialog(TaskDlgPost* dlg) override;
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

}  // namespace FemGui


#endif  // FEM_VIEWPROVIDERFEMPOSTFILTER_H
