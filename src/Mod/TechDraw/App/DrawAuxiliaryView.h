/***************************************************************************
 *   Copyright (c) 2026 FreeCAD contributors                               *
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

#include "DrawViewPart.h"

namespace TechDraw
{

// NOLINTBEGIN
class TechDrawExport DrawAuxiliaryView: public TechDraw::DrawViewPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawAuxiliaryView);

public:
    DrawAuxiliaryView();
    ~DrawAuxiliaryView() override = default;

    App::PropertyLink BaseView;
    App::PropertyVector AuxiliaryDirection;
    App::PropertyVector ReferenceStart;
    App::PropertyVector ReferenceEnd;
    App::PropertyEnumeration ProjectionMode;
    App::PropertyBool ReverseDirection;
    App::PropertyBool KeepAligned;
    App::PropertyVector AlignmentOffset;
    App::PropertyBool AlignmentOffsetInitialized;
    App::PropertyString ReferenceLabel;
// NOLINTEND

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    void onChanged(const App::Property* prop) override;
    void unsetupObject() override;

    const char* getViewProviderName() const override
    {
        return "TechDrawGui::ViewProviderViewPart";
    }

    TechDraw::DrawViewPart* getBaseDVP() const;
    Base::Vector3d getAuxiliaryLocalDirection() const;
    Base::Vector3d getAuxiliaryDirection() const;
    Base::Vector3d getAuxiliaryXDirection() const;
    void captureAlignmentOffset();
    void clearBaseSources();
    void mirrorSourcesFromBase(TechDraw::DrawViewPart* base);
    void syncDirectionFromReference();
    void updateAlignmentFromBase();
    void updateProjectionFromBase();

    static const char* ProjectionModeEnums[];

private:
    Base::Vector3d getSafeReferenceDirection() const;
};

using DrawAuxiliaryViewPython = App::FeaturePythonT<DrawAuxiliaryView>;

}// namespace TechDraw
