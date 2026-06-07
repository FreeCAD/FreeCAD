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

#include <algorithm>

#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>

#include <Base/Console.h>
#include <Base/Converter.h>
#include <Base/Tools.h>

#include "DrawAuxiliaryView.h"
#include "DrawProjGroup.h"
#include "DrawAuxiliaryViewPy.h"
#include "DrawProjGroupItem.h"
#include "DrawUtil.h"
#include "Preferences.h"


using namespace TechDraw;


PROPERTY_SOURCE(TechDraw::DrawAuxiliaryView, TechDraw::DrawViewPart)

const char* DrawAuxiliaryView::AuxiliaryOrientationEnums[] = {"Along", "Across", nullptr};

DrawAuxiliaryView::DrawAuxiliaryView()
{
    static const char* group = "Auxiliary view";

    ADD_PROPERTY_TYPE(BaseView, (nullptr), group, App::Prop_None,
                      "2D base view this auxiliary view relates to");
    BaseView.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(AuxiliaryDirection, (1.0, 0.0, 0.0), group, App::Prop_None,
                      "Projection direction in the BaseView local coordinate system");
    AuxiliaryOrientation.setEnums(AuxiliaryOrientationEnums);
    ADD_PROPERTY_TYPE(AuxiliaryOrientation, ((long)0), group, App::Prop_None,
                      "Use the reference direction as-is or perpendicular to it");
    ADD_PROPERTY_TYPE(ReferenceLabel, ("A"), group, App::Prop_None,
                      "Reference label shown on both the base and auxiliary views");
    ADD_PROPERTY_TYPE(ReferenceStart, (0.0, 0.0, 0.0), group, App::Prop_None,
                      "Start point for the reference marker in BaseView coordinates");
    ADD_PROPERTY_TYPE(ReferenceEnd, (1.0, 0.0, 0.0), group, App::Prop_None,
                      "End point for the reference marker in BaseView coordinates");
    ADD_PROPERTY_TYPE(ReverseDirection, (false), group, App::Prop_None,
                      "Reverse the auxiliary projection direction");
    ADD_PROPERTY_TYPE(KeepAligned, (true), group, App::Prop_None,
                      "Keep the auxiliary view aligned to the base view");

    ScaleType.setValue("Custom");
    Direction.setStatus(App::Property::ReadOnly, true);
    XDirection.setStatus(App::Property::ReadOnly, true);
}

short DrawAuxiliaryView::mustExecute() const
{
    if (isRestoring()) {
        return TechDraw::DrawView::mustExecute();
    }

    if (BaseView.isTouched() || AuxiliaryDirection.isTouched()
        || AuxiliaryOrientation.isTouched()
        || ReverseDirection.isTouched()
        || KeepAligned.isTouched()) {
        return 1;
    }

    return TechDraw::DrawViewPart::mustExecute();
}

void DrawAuxiliaryView::onChanged(const App::Property* prop)
{
    if (isRestoring()) {
        DrawViewPart::onChanged(prop);
        return;
    }

    if (prop == &ReferenceLabel) {
        std::string lblText = "Auxiliary " + std::string(ReferenceLabel.getValue());
        Label.setValue(lblText);
    }

    if (prop == &BaseView || prop == &AuxiliaryDirection || prop == &AuxiliaryOrientation
        || prop == &ReferenceLabel
        || prop == &ReferenceStart || prop == &ReferenceEnd || prop == &ReverseDirection
        || prop == &KeepAligned) {
        requestBasePaint();
    }

    DrawViewPart::onChanged(prop);
}

App::DocumentObjectExecReturn* DrawAuxiliaryView::execute()
{
    if (!keepUpdated()) {
        return DrawView::execute();
    }

    if (!isBaseValid()) {
        return new App::DocumentObjectExecReturn(
            "BaseView must be a TechDraw::DrawViewPart or DrawProjGroupItem");
    }

    if (waitingForHlr()) {
        return DrawView::execute();
    }

    auto* base = getBaseDVP();
    TopoDS_Shape shape = base->getSourceShape();
    if (shape.IsNull()) {
        Base::Console().message("DAV::execute - %s - BaseView source shape is Null.\n",
                                getNameInDocument());
        return DrawView::execute();
    }

    updateProjectionFromBase();
    partExec(shape);
    requestBasePaint();

    return DrawView::execute();
}

void DrawAuxiliaryView::postHlrTasks()
{
    DrawViewPart::postHlrTasks();
    autoPosition();
}

void DrawAuxiliaryView::unsetupObject()
{
    requestBasePaint();
    DrawViewPart::unsetupObject();
}

DrawViewPart* DrawAuxiliaryView::getBaseDVP() const
{
    App::DocumentObject* base = BaseView.getValue();
    if (!base || !base->isDerivedFrom<TechDraw::DrawViewPart>()) {
        return nullptr;
    }
    return static_cast<TechDraw::DrawViewPart*>(base);
}

bool DrawAuxiliaryView::isBaseValid() const
{
    return canUseAsBaseView(getBaseDVP());
}

bool DrawAuxiliaryView::canUseAsBaseView(const DrawViewPart* base)
{
    return base && (base->getTypeId() == TechDraw::DrawViewPart::getClassTypeId()
                    || base->isDerivedFrom<TechDraw::DrawProjGroupItem>());
}

Base::Vector3d DrawAuxiliaryView::getEffectiveAuxiliaryDirection() const
{
    Base::Vector3d localDirection = AuxiliaryDirection.getValue();
    localDirection.z = 0.0;

    if (AuxiliaryOrientation.isValue("Across")) {
        localDirection = Base::Vector3d(-localDirection.y, localDirection.x, 0.0);
    }

    if (DrawUtil::fpCompare(localDirection.Length(), 0.0)) {
        localDirection = Base::Vector3d(1.0, 0.0, 0.0);
    }

    localDirection.Normalize();
    if (ReverseDirection.getValue()) {
        localDirection *= -1.0;
    }

    return localDirection;
}

void DrawAuxiliaryView::updateProjectionFromBase()
{
    auto* base = getBaseDVP();
    if (!base) {
        return;
    }

    gp_Ax2 auxCS = base->localVectorToCS(getEffectiveAuxiliaryDirection());
    Base::Vector3d newDirection(auxCS.Direction().X(),
                                auxCS.Direction().Y(),
                                auxCS.Direction().Z());
    Base::Vector3d newXDirection(auxCS.XDirection().X(),
                                 auxCS.XDirection().Y(),
                                 auxCS.XDirection().Z());

    Direction.setValue(newDirection);
    Direction.purgeTouched();
    XDirection.setValue(newXDirection);
    XDirection.purgeTouched();
}

void DrawAuxiliaryView::autoPosition()
{
    if (!KeepAligned.getValue() || LockPosition.getValue()) {
        return;
    }

    auto* base = getBaseDVP();
    if (!base || !base->hasGeometry() || !hasGeometry()) {
        return;
    }

    Base::Vector3d localAlignment = getEffectiveAuxiliaryDirection();
    Base::Vector3d pageAlignment = localAlignment;
    pageAlignment.RotateZ(Base::toRadians(base->Rotation.getValue()));
    pageAlignment.z = 0.0;
    if (DrawUtil::fpCompare(pageAlignment.Length(), 0.0)) {
        return;
    }
    pageAlignment.Normalize();

    Base::Vector3d basePosition(base->X.getValue(), base->Y.getValue(), 0.0);
    if (DrawView::isProjGroupItem(base)) {
        auto* item = static_cast<DrawProjGroupItem*>(base);
        auto* group = item->getPGroup();
        if (group) {
            basePosition.x += group->X.getValue();
            basePosition.y += group->Y.getValue();
        }
    }

    const double baseSize = base->getSizeAlongVector(localAlignment) * base->getScale();
    const double auxiliarySize = getSizeAlongVector(Base::Vector3d(1.0, 0.0, 0.0)) * getScale();
    const double spacing = std::max(Preferences::groupSpaceX(), Preferences::groupSpaceY());
    const double distance = (baseSize + auxiliarySize) / 2.0 + spacing;

    Base::Vector3d newPosition = basePosition + pageAlignment * distance;
    const double tolerance = 0.001;
    bool changed = false;

    if (!DrawUtil::fpCompare(X.getValue(), newPosition.x, tolerance)) {
        X.setValue(newPosition.x);
        changed = true;
    }
    if (!DrawUtil::fpCompare(Y.getValue(), newPosition.y, tolerance)) {
        Y.setValue(newPosition.y);
        changed = true;
    }

    if (changed) {
        requestPaint();
        purgeTouched();
    }
}

void DrawAuxiliaryView::requestBasePaint() const
{
    auto* base = getBaseDVP();
    if (base) {
        base->requestPaint();
    }
}

PyObject* DrawAuxiliaryView::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new DrawAuxiliaryViewPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawAuxiliaryViewPython, TechDraw::DrawAuxiliaryView)
template<> const char* TechDraw::DrawAuxiliaryViewPython::getViewProviderName() const
{
    return "TechDrawGui::ViewProviderViewPart";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawAuxiliaryView>;
}  // namespace App
