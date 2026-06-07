// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2026 meaqua9420                                        *
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

#include <App/FeaturePython.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawViewPart.h"


namespace TechDraw
{

class TechDrawExport DrawAuxiliaryView : public DrawViewPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawAuxiliaryView);

public:
    static const char* AuxiliaryOrientationEnums[];

    DrawAuxiliaryView();
    ~DrawAuxiliaryView() override = default;

    App::PropertyLink BaseView;
    App::PropertyVector AuxiliaryDirection;
    App::PropertyEnumeration AuxiliaryOrientation;
    App::PropertyString ReferenceLabel;
    App::PropertyVector ReferenceStart;
    App::PropertyVector ReferenceEnd;
    App::PropertyBool ReverseDirection;
    App::PropertyBool KeepAligned;

    short mustExecute() const override;
    App::DocumentObjectExecReturn* execute() override;
    void postHlrTasks() override;
    void onChanged(const App::Property* prop) override;
    void unsetupObject() override;
    PyObject* getPyObject() override;
    const char* getViewProviderName() const override
    {
        return "TechDrawGui::ViewProviderViewPart";
    }

    static bool canUseAsBaseView(const DrawViewPart* base);

    DrawViewPart* getBaseDVP() const;
    bool isBaseValid() const;
    Base::Vector3d getEffectiveAuxiliaryDirection() const;
    void updateProjectionFromBase();
    void autoPosition();

private:
    void requestBasePaint() const;
};

using DrawAuxiliaryViewPython = App::FeaturePythonT<DrawAuxiliaryView>;

}  // namespace TechDraw
