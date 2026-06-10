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

#include <Base/Console.h>
#include <Base/Converter.h>
#include <Base/Tools.h>

#include "DrawAuxiliaryView.h"
#include "DrawUtil.h"

using namespace TechDraw;

// NOLINTBEGIN
PROPERTY_SOURCE(TechDraw::DrawAuxiliaryView, TechDraw::DrawViewPart)

const char* DrawAuxiliaryView::ProjectionModeEnums[] = {"Across", "Along", nullptr};
// NOLINTEND

DrawAuxiliaryView::DrawAuxiliaryView()
{
    static const char* agroup = "Auxiliary";

    ADD_PROPERTY_TYPE(BaseView,
                      (nullptr),
                      agroup,
                      App::Prop_None,
                      "The base view used to calculate this auxiliary view");
    BaseView.setScope(App::LinkScope::Global);

    ADD_PROPERTY_TYPE(AuxiliaryDirection,
                      (1.0, 0.0, 0.0),
                      agroup,
                      App::Prop_None,
                      "Reference line direction in the base view coordinate system");

    ADD_PROPERTY_TYPE(ReferenceStart,
                      (0.0, 0.0, 0.0),
                      agroup,
                      App::Prop_None,
                      "Start point of the reference marker in base view coordinates");
    ADD_PROPERTY_TYPE(ReferenceEnd,
                      (1.0, 0.0, 0.0),
                      agroup,
                      App::Prop_None,
                      "End point of the reference marker in base view coordinates");

    ProjectionMode.setEnums(ProjectionModeEnums);
    ADD_PROPERTY_TYPE(ProjectionMode,
                      ((long)0),
                      agroup,
                      App::Prop_None,
                      "Use a view direction across or along the reference line");

    ADD_PROPERTY_TYPE(ReverseDirection,
                      (false),
                      agroup,
                      App::Prop_None,
                      "Reverse the auxiliary view direction");
    ADD_PROPERTY_TYPE(KeepAligned,
                      (true),
                      agroup,
                      App::Prop_None,
                      "Keep the auxiliary view aligned with its base reference marker");

    ADD_PROPERTY_TYPE(AlignmentOffset,
                      (0.0, 0.0, 0.0),
                      agroup,
                      App::Prop_Hidden,
                      "Page offset from the base view used when KeepAligned is true");
    ADD_PROPERTY_TYPE(AlignmentOffsetInitialized,
                      (false),
                      agroup,
                      App::Prop_Hidden,
                      "Whether AlignmentOffset has been captured for the current base view");

    ADD_PROPERTY_TYPE(ReferenceLabel,
                      ("A"),
                      agroup,
                      App::Prop_None,
                      "Reference label shown on the base and auxiliary views");
}

App::DocumentObjectExecReturn* DrawAuxiliaryView::execute()
{
    if (!getBaseDVP()) {
        clearBaseSources();
        return new App::DocumentObjectExecReturn("BaseView object not found");
    }

    updateProjectionFromBase();
    updateAlignmentFromBase();
    return DrawViewPart::execute();
}

short DrawAuxiliaryView::mustExecute() const
{
    if (isRestoring()) {
        return DrawViewPart::mustExecute();
    }

    if (BaseView.isTouched()
        || AuxiliaryDirection.isTouched()
        || ProjectionMode.isTouched()
        || ReverseDirection.isTouched()
        || KeepAligned.isTouched()
        || ReferenceStart.isTouched()
        || ReferenceEnd.isTouched()
        || AlignmentOffset.isTouched()
        || AlignmentOffsetInitialized.isTouched()) {
        return 1;
    }

    return DrawViewPart::mustExecute();
}

void DrawAuxiliaryView::onChanged(const App::Property* prop)
{
    if (isRestoring()) {
        DrawViewPart::onChanged(prop);
        return;
    }

    if (prop == &BaseView) {
        AlignmentOffsetInitialized.setValue(false);
        if (!getBaseDVP()) {
            clearBaseSources();
        }
    }

    if (prop == &ReferenceStart || prop == &ReferenceEnd) {
        syncDirectionFromReference();
    }

    if (prop == &BaseView
        || prop == &AuxiliaryDirection
        || prop == &ProjectionMode
        || prop == &ReverseDirection) {
        updateProjectionFromBase();
    }

    if (prop == &KeepAligned && KeepAligned.getValue()) {
        captureAlignmentOffset();
        updateAlignmentFromBase();
    }

    if ((prop == &X || prop == &Y) && KeepAligned.getValue()) {
        captureAlignmentOffset();
    }

    if (prop == &BaseView
        || prop == &ReferenceStart
        || prop == &ReferenceEnd
        || prop == &ReferenceLabel
        || prop == &AuxiliaryDirection
        || prop == &ProjectionMode
        || prop == &ReverseDirection) {
        auto base = getBaseDVP();
        if (base) {
            base->requestPaint();
        }
    }

    DrawViewPart::onChanged(prop);
}

void DrawAuxiliaryView::unsetupObject()
{
    auto base = getBaseDVP();
    if (base) {
        base->requestPaint();
    }
    DrawViewPart::unsetupObject();
}

TechDraw::DrawViewPart* DrawAuxiliaryView::getBaseDVP() const
{
    return dynamic_cast<TechDraw::DrawViewPart*>(BaseView.getValue());
}

Base::Vector3d DrawAuxiliaryView::getSafeReferenceDirection() const
{
    Base::Vector3d reference = AuxiliaryDirection.getValue();
    reference.z = 0.0;
    if (DrawUtil::fpCompare(reference.Length(), 0.0)) {
        return Base::Vector3d(1.0, 0.0, 0.0);
    }

    reference.Normalize();
    return reference;
}

Base::Vector3d DrawAuxiliaryView::getAuxiliaryLocalDirection() const
{
    const Base::Vector3d reference = getSafeReferenceDirection();
    Base::Vector3d localDirection = reference;
    if (ProjectionMode.isValue("Across")) {
        localDirection = Base::Vector3d(-reference.y, reference.x, 0.0);
    }

    if (ReverseDirection.getValue()) {
        localDirection = localDirection * -1.0;
    }

    if (DrawUtil::fpCompare(localDirection.Length(), 0.0)) {
        return Base::Vector3d(0.0, 1.0, 0.0);
    }

    localDirection.Normalize();
    return localDirection;
}

Base::Vector3d DrawAuxiliaryView::getAuxiliaryDirection() const
{
    TechDraw::DrawViewPart* base = getBaseDVP();
    if (!base) {
        return Direction.getValue();
    }

    return base->localVectorToDirection(getAuxiliaryLocalDirection());
}

Base::Vector3d DrawAuxiliaryView::getAuxiliaryXDirection() const
{
    TechDraw::DrawViewPart* base = getBaseDVP();
    if (!base) {
        return getXDirection();
    }

    gp_Ax2 auxiliaryCS = base->localVectorToCS(getAuxiliaryLocalDirection());
    return Base::convertTo<Base::Vector3d>(auxiliaryCS.XDirection());
}

void DrawAuxiliaryView::captureAlignmentOffset()
{
    TechDraw::DrawViewPart* base = getBaseDVP();
    if (!base) {
        AlignmentOffsetInitialized.setValue(false);
        return;
    }

    Base::Vector3d offset = getPosition() - base->getPosition();
    offset.z = 0.0;
    AlignmentOffset.setValue(offset);
    AlignmentOffsetInitialized.setValue(true);
}

void DrawAuxiliaryView::clearBaseSources()
{
    std::vector<App::DocumentObject*> emptySources;
    if (!Source.getValues().empty()) {
        Source.setValues(emptySources);
    }
    if (!XSource.getValues().empty()) {
        XSource.setValues(emptySources);
    }
}

void DrawAuxiliaryView::mirrorSourcesFromBase(TechDraw::DrawViewPart* base)
{
    if (!base) {
        clearBaseSources();
        return;
    }

    auto baseSources = base->Source.getValues();
    if (Source.getValues() != baseSources) {
        Source.setValues(baseSources);
    }

    auto baseXSources = base->XSource.getValues();
    if (XSource.getValues() != baseXSources) {
        XSource.setValues(baseXSources);
    }
}

void DrawAuxiliaryView::syncDirectionFromReference()
{
    Base::Vector3d reference = ReferenceEnd.getValue() - ReferenceStart.getValue();
    reference.z = 0.0;
    AuxiliaryDirection.setValue(reference);
}

void DrawAuxiliaryView::updateAlignmentFromBase()
{
    if (!KeepAligned.getValue()) {
        return;
    }

    TechDraw::DrawViewPart* base = getBaseDVP();
    if (!base) {
        return;
    }

    if (!AlignmentOffsetInitialized.getValue()) {
        captureAlignmentOffset();
        if (!AlignmentOffsetInitialized.getValue()) {
            return;
        }
    }

    Base::Vector3d offset = AlignmentOffset.getValue();
    offset.z = 0.0;
    const Base::Vector3d position = base->getPosition() + offset;
    setPosition(position.x, position.y, true);
}

void DrawAuxiliaryView::updateProjectionFromBase()
{
    TechDraw::DrawViewPart* base = getBaseDVP();
    if (!base) {
        clearBaseSources();
        return;
    }

    mirrorSourcesFromBase(base);

    Direction.setValue(getAuxiliaryDirection());
    XDirection.setValue(getAuxiliaryXDirection());
}

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawAuxiliaryViewPython, TechDraw::DrawAuxiliaryView)
template<>
const char* TechDraw::DrawAuxiliaryViewPython::getViewProviderName() const
{
    return "TechDrawGui::ViewProviderViewPart";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawAuxiliaryView>;
}// namespace App
