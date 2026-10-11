// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Caio Venâncio <caio.venancio784@gmail.com>          *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <BRepAlgoAPI_Common.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <gp_Lin.hxx>

#include "FeatureThread.h"
#include "FeatureDressUp.h"
#include "ThreadUtils.h"

#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <TopExp_Explorer.hxx>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <Base/Tools.h>

#include <Mod/Part/App/TopoShapeOpCode.h>

namespace PartDesign
{

PROPERTY_SOURCE(PartDesign::Thread, PartDesign::DressUp)

Thread::Thread()
{
    addSubType = FeatureAddSub::Additive;

    threadUtils.executeReadThreadDefinitions();

    addThreadType();

    ADD_PROPERTY_TYPE(ThreadType, (0L), "Thread", App::Prop_None, "Thread type");
    ThreadType.setEnums(threadUtils.getThreadTypeEnums());

    ADD_PROPERTY_TYPE(ThreadTypeName, (0L), "Thread", App::Prop_None, "Thread type name");
    ThreadTypeName.setEnums(threadUtils.getThreadTypeNameEnums());

    ADD_PROPERTY_TYPE(ThreadDiameter, (0.0), "Thread", App::Prop_None, "Thread major diameter");

    ADD_PROPERTY_TYPE(ThreadSize, (0L), "Thread", App::Prop_None, "Thread size");
    ThreadSize.setEnums(threadUtils.getThreadDiameters(ThreadType.getValue()));

    ADD_PROPERTY_TYPE(ThreadSizePitch, (0L), "Thread", App::Prop_None, "Thread size");
    ThreadSizePitch.setEnums(
        threadUtils.getThreadPitches(ThreadType.getValue(), ThreadSize.getValue())
    );

    ADD_PROPERTY_TYPE(ThreadDirection, (0L), "Thread", App::Prop_None, "Thread direction");
    ThreadDirection.setEnums(ThreadUtils::ThreadDirectionEnums);
    ThreadDirection.setReadOnly(true);

    ADD_PROPERTY_TYPE(DepthType, (0L), "Thread", App::Prop_None, "Type");
    DepthType.setEnums(ThreadUtils::DepthTypeEnums);

    ADD_PROPERTY_TYPE(Depth, (25.0), "Thread", App::Prop_None, "Length");

    ADD_PROPERTY_TYPE(ThreadClass, (0L), "Thread", App::Prop_None, "Thread class");
    ThreadClass.setEnums(ThreadUtils::ThreadClass_None_Enums);

    ADD_PROPERTY_TYPE(
        UseCustomThreadClearance,
        (false),
        "Thread",
        App::Prop_None,
        "Use custom thread clearance"
    );

    ADD_PROPERTY_TYPE(
        CustomThreadClearance,
        (0.0),
        "Thread",
        App::Prop_None,
        "Custom thread clearance (overrides ThreadClass)"
    );

    ADD_PROPERTY_TYPE(ThreadDesignation, ("---"), "Thread", App::Prop_None, "Name");

    ADD_PROPERTY_TYPE(LateralFace, (nullptr), "Thread", (App::PropertyType)(App::Prop_None), "LateralFace");

    ADD_PROPERTY_TYPE(
        StartPlane,
        (nullptr),
        "Thread",
        App::Prop_None,
        "Reference geometry where thread will start"
    );

    ADD_PROPERTY_TYPE(
        UpToGeometry,
        (nullptr),
        "Thread",
        App::Prop_None,
        "Reference geometry where thread will end"
    );

    ADD_PROPERTY_TYPE(Threaded, (true), "Thread", App::Prop_None, "Threaded");

    ADD_PROPERTY_TYPE(ModelThread, (false), "Thread", App::Prop_None, "Model actual thread");

    ADD_PROPERTY_TYPE(CosmeticThread, (true), "Thread", App::Prop_None, "Texture the thread");

    ADD_PROPERTY_TYPE(Tapered, (false), "Thread", App::Prop_None, "Tapered");

    ADD_PROPERTY_TYPE(TaperedAngle, (90.0), "Thread", App::Prop_None, "Tapered angle");
    TaperedAngle.setReadOnly(true);

    ADD_PROPERTY_TYPE(Diameter, (0.0), "Thread", App::Prop_None, "Diameter");

    ADD_PROPERTY_TYPE(IsInternal, (false), "Thread", App::Prop_None, "Thread is internal");
}

Thread::ThreadExtent Thread::resolveThreadExtent()
{
    Part::TopoShape base = getBaseTopoShape();
    base.setTransform(Base::Matrix4D());
    return resolveThreadExtent(base);
}

Thread::ThreadExtent Thread::resolveThreadExtent(const Part::TopoShape& base)
{
    const gp_Dir direction(threadUtils.getThreadZAxis(LateralFace));
    const gp_Pnt origin = threadUtils.getThreadStartPoint(LateralFace, StartPlane);
    const gp_Pnt nearPoint = threadUtils.getThreadStartPoint(LateralFace, direction);
    const gp_Pnt farPoint = threadUtils.getThreadFarPoint(LateralFace, direction);
    const double cylinderHeight = nearPoint.Distance(farPoint);

    std::string method(DepthType.getValueAsString());
    double length = 0.0;

    if (method == "Dimension") {
        length = Depth.getValue();
    }
    else if (method == "UpToFirst") {
        length = threadUtils.getUpToFirstLength(
            LateralFace,
            StartPlane,
            base,
            ThreadType.getValue(),
            ThreadSize.getValue(),
            IsInternal.getValue(),
            UseCustomThreadClearance.getValue(),
            CustomThreadClearance.getValue(),
            ThreadClass,
            Tapered.getValue(),
            TaperedAngle.getValue()
        );
    }
    else if (method == "ThroughAll") {
        // A cone's radius is referenced at origin, so stop at its actual far
        // section even when StartPlane is offset. Retain the cylinder depth policy.
        length = threadUtils.isConicalFace(LateralFace)
            ? gp_Vec(origin, farPoint).Dot(gp_Vec(direction))
            : cylinderHeight;
    }
    else if (method == "UpToGeometry") {
        //TODO: limit UpToGeometry to not be upper than startplane
        length = threadUtils.getUpToGeometryLength(UpToGeometry, LateralFace, StartPlane);
    }
    else {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread error: Unsupported length specification")
        );
    }

    if (!std::isfinite(length) || length <= Precision::Confusion()) {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread error: Invalid Thread depth")
        );
    }

    return {origin, direction, length};
}

App::DocumentObjectExecReturn* Thread::execute()
{
    // AddSubShape caches the operation tool used by the preview and by transformed features.
    // Clear it before rebuilding so invalid or cosmetic threads cannot retain stale geometry.
    AddSubShape.setValue(Part::TopoShape());
    reducedBasePreviewShape = Part::TopoShape();

    Part::TopoShape base;
    try {
        base = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    base.setTransform(Base::Matrix4D());

    // Faces where thread should be applied
    bool isThreadEmpty = (LateralFace.getValue() == nullptr);

    // If no element is selected, then we use a copy of previous feature.
    if (isThreadEmpty) {
        this->positionByBaseFeature();
        this->Shape.setValue(base);
        return App::DocumentObject::StdReturn;
    }

    auto res = threadUtils.validateParameters(LateralFace);
    if (res != App::DocumentObject::StdReturn) {
        Base::Console().error("Failed to create thread:\n%s\n", res->Why.c_str());
        // throw Base::RuntimeError(res->Why);
        return res;
    }

    IsInternal.setValue(threadUtils.isInternalFace(LateralFace, base.getShape()));
    addSubType = IsInternal.getValue() ? FeatureAddSub::Subtractive : FeatureAddSub::Additive;
    Base::Console().message("isInternal?: %d\n", IsInternal.getValue());

    double diameter;
    try {
        diameter = threadUtils.getLateralFaceDiameter(LateralFace, StartPlane);
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    Diameter.setValue(diameter);

    int nearestSize = -1;
    int nearestPitch = -1;
    double definedDiameter = 0.0;
    if (!IsInternal.getValue()) {
        const std::vector<std::string> diameters = threadUtils.getThreadDiameters(
            ThreadType.getValue()
        );
        nearestSize = threadUtils.findNearestThreadSize(ThreadType.getValue(), diameter);
        if (nearestSize < 0 || nearestSize >= static_cast<int>(diameters.size())) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP(
                    "Exception",
                    "Thread error: Thread size index out of definition range."
                )
            );
        }
        definedDiameter = std::stod(diameters[nearestSize]);
    }
    else {
        const auto selection = threadUtils.findNearestMinorThreadSize(
            ThreadType.getValue(),
            diameter
        );
        if (!selection) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP(
                    "Exception",
                    "Thread error: No thread definition found for the selected type."
                )
            );
        }
        nearestSize = selection->sizeIndex;
        nearestPitch = selection->pitchIndex;
        definedDiameter = selection->minorDiameter;
    }

    if (nearestSize < 0) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Thread error: No thread definition found for the selected type.")
        );
    }

    const bool isCylindricalSupport = !threadUtils.isConicalFace(LateralFace);
    const bool sizeMismatch = nearestSize != ThreadSize.getValue();
    const bool pitchMismatch = IsInternal.getValue()
        && nearestPitch != ThreadSizePitch.getValue();
    const bool diameterMismatch = std::abs(diameter - definedDiameter) > Precision::Confusion();
    if (ModelThread.getValue() && isCylindricalSupport
        && (sizeMismatch || pitchMismatch || diameterMismatch)) {
        Base::Console().message("diameter: %lf\n", diameter);
        Base::Console().message("definedDiameter: %lf\n", definedDiameter);
        std::ostringstream diameterText;
        diameterText << std::fixed << std::setprecision(3) << diameter;
        std::string formattedDiameter = diameterText.str();
        formattedDiameter.erase(formattedDiameter.find_last_not_of('0') + 1);
        if (formattedDiameter.back() == '.') {
            formattedDiameter.pop_back();
        }
        const std::string threadKind = IsInternal.getValue() ? "internal" : "external";
        std::string msg = "Thread error: Selected " + threadKind
                        + " thread does not match the support diameter "
                        + formattedDiameter + " mm.";
        return new App::DocumentObjectExecReturn(msg.c_str());
    }

    double conicalAngle = threadUtils.getConicalAngle(LateralFace);
    Base::Console().message("Conical Angle: %lf\n", conicalAngle);

    try {
        const ThreadExtent extent = resolveThreadExtent(base);
        const gp_Pnt startPoint = extent.origin;
        const double length = extent.length;
        gp_Vec zDir(extent.direction);
        gp_Vec xDir = threadUtils.computePerpendicular(zDir);

        gp_Dir axisDir(zDir);
        gp_Pnt nearPoint = threadUtils.getThreadStartPoint(LateralFace, axisDir);
        gp_Pnt farPoint  = threadUtils.getThreadFarPoint(LateralFace, axisDir);
        double cylinderHeight = nearPoint.Distance(farPoint);

        double projStart = gp_Vec(nearPoint, startPoint).Dot(gp_Vec(axisDir));

        // TODO: this has to go in production
        // if (projStart < -Precision::Confusion()) {
        //     return new App::DocumentObjectExecReturn(
        //         QT_TRANSLATE_NOOP("Exception", "Thread error: Start point is above the top of the cylinder face")
        //     );
        // }

        Base::Console().message("cylinderHeight: %lf\n", cylinderHeight);
        Base::Console().message("length: %lf\n", length);

        // TODO: this has to go to production
        // if (length > cylinderHeight) {
        //     return new App::DocumentObjectExecReturn(
        //         QT_TRANSLATE_NOOP("Exception", "Thread error: Thread depth greater than cylinder height")
        //     );
        // }

        // this->Shape.setValue(base);
        // return App::DocumentObject::StdReturn;

        // if (Threaded.getValue() && ModelThread.getValue()) {
        if (ModelThread.getValue()) {
            const double selectedPitch = threadUtils.getThreadPitch(
                ThreadType.getValue(),
                ThreadSize.getValue(),
                ThreadSizePitch.getValue()
            );
            if (selectedPitch <= Precision::Confusion()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Thread error: Invalid thread pitch")
                );
            }

            const double usefulThreadLength = length - selectedPitch;
            if (usefulThreadLength <= Precision::Confusion()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP(
                        "Exception",
                        "Thread error: Thread depth must be greater than the selected pitch"
                    )
                );
            }

            std::optional<double> externalMinorDiameter;
            const bool conicalSupport = threadUtils.isConicalFace(LateralFace);
            const double faceSlope = conicalSupport ? std::tan(conicalAngle) : 0.0;
            const double reductionSlope = conicalSupport && Tapered.getValue()
                ? -std::tan(Base::toRadians(90.0 - TaperedAngle.getValue()))
                : 0.0;
            const double supportEndRadius = diameter / 2.0 + length * faceSlope;
            if (!std::isfinite(supportEndRadius)
                || supportEndRadius <= Precision::Confusion()) {
                throw Base::ValueError(QT_TRANSLATE_NOOP(
                    "Exception", "Thread error: Requested depth reaches the cone apex or has an invalid support radius"
                ));
            }
            double supportDiameter = diameter;
            if (!IsInternal.getValue()) {
                externalMinorDiameter = threadUtils.getMinorDiameter(
                    ThreadType.getValue(),
                    ThreadSize.getValue(),
                    ThreadSizePitch.getValue()
                );
                supportDiameter = *externalMinorDiameter;
            }
            const ThreadUtils::ThreadEndTrims endTrims = threadUtils.getThreadEndTrims(
                ThreadType.getValue(),
                ThreadSize.getValue(),
                ThreadSizePitch.getValue(),
                IsInternal.getValue(),
                supportDiameter,
                usefulThreadLength,
                Tapered.getValue(),
                TaperedAngle.getValue(),
                UseCustomThreadClearance.getValue(),
                CustomThreadClearance.getValue(),
                ThreadClass,
                true,
                // Internal trims intersect the actual bore. External trims will
                // intersect the reduced base, which follows the configured taper.
                IsInternal.getValue() ? faceSlope : reductionSlope
            );
            Base::Console().message(
                "[endTrim]: near=%lf far=%lf\n",
                endTrims.nearEnd,
                endTrims.farEnd
            );
            const std::string depthType(DepthType.getValueAsString());
            const bool limitInternalToolToFace = IsInternal.getValue()
                && (depthType == "Dimension" || depthType == "ThroughAll");
            const bool limitToolToFace = depthType == "Dimension" || depthType == "ThroughAll";
            double trimmingStart = 0.0;
            double trimmingEnd = length;
            if (limitToolToFace) {
                trimmingStart = std::max(trimmingStart, -projStart);
                trimmingEnd = std::min(trimmingEnd, cylinderHeight - projStart);
            }
            if (trimmingEnd - trimmingStart <= Precision::Confusion()) {
                throw Base::ValueError(QT_TRANSLATE_NOOP(
                    "Exception", "Thread error: Thread depth does not overlap the selected face"
                ));
            }
            // Extend the guide by a full turn at the entrance so every angular
            // position has a complete cutting profile at the top of the hole.
            // Keep bottomPoint unchanged: the extra length grows toward the entrance
            // and the subsequent common removes surplus outside the selected face.
            const double entranceExtension = limitInternalToolToFace
                    && StartPlane.getValue() == nullptr
                ? selectedPitch
                : 0.0;
            const double generatedThreadLength = usefulThreadLength
                + endTrims.nearEnd + endTrims.farEnd + entranceExtension;

            // A modelled external thread replaces the selected major-diameter surface with the
            // reduced base before fusing the thread profile.  Cosmetic threads must retain the
            // original base so their overlay is drawn on the actual external surface.
            if (!IsInternal.getValue()) {
                Base::Console().message("Reducing external thread base\n");

                const double majorDiameter = diameter + 2.0 * trimmingStart * faceSlope;
                const double minorDiameter = *externalMinorDiameter
                    + 2.0 * trimmingStart * reductionSlope;
                Base::Console().message("minorDiameter: %lf\n", minorDiameter);

                base = threadUtils.reduceExternalThreadBase(
                    base,
                    LateralFace,
                    majorDiameter,
                    minorDiameter,
                    trimmingEnd - trimmingStart,
                    startPoint.Translated(zDir * trimmingStart),
                    Tapered.getValue(),
                    TaperedAngle.getValue()
                );
            }

            // gp_Vec zDirFixed = zDir.Reversed();

            TopoDS_Shape thread = threadUtils.makeThread(
                    xDir, 
                    zDir, 
                    generatedThreadLength,
                    ThreadType.getValue(),
                    ThreadSize.getValue(),
                    ThreadSizePitch.getValue(),
                    ThreadDirection.getValue(),
                    ThreadClass,
                    IsInternal.getValue(),
                    Tapered.getValue(),
                    TaperedAngle.getValue(),
                    UseCustomThreadClearance.getValue(),
                    CustomThreadClearance.getValue(),
                    endTrims.farEnd,
                    usefulThreadLength
            );

            // if(IsInternal.getValue()){
                gp_Vec zDirUnit = zDir;
                zDirUnit.Normalize();
                gp_Pnt bottomPoint = startPoint.Translated(
                    zDirUnit * (usefulThreadLength + endTrims.farEnd)
                );

                double projBottom = gp_Vec(nearPoint, bottomPoint).Dot(gp_Vec(axisDir));

                // TODO: this has to go in production
                // if (projBottom > cylinderHeight - selectedPitch + endTrims.farEnd
                //     + Precision::Confusion()) {
                //     return new App::DocumentObjectExecReturn(
                //         QT_TRANSLATE_NOOP("Exception", "Thread error: Thread bottom point is below the base of the cylinder face")
                //     );
                // }
                
                // gp_Pnt axisOrigin = threadUtils.getThreadAxisOrigin(LateralFace); // It's going to be used in getthreadstart
                Base::Console().message("bottomPoint = (%f, %f, %f)\n",
                                bottomPoint.X(), bottomPoint.Y(), bottomPoint.Z());
                gp_Trsf translation;
                translation.SetTranslation(gp_Pnt(0.0, 0.0, 0.0), bottomPoint);
                TopLoc_Location locTrans(translation);
                thread.Move(locTrans);
            // }
                
            if (thread.IsNull()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Thread error: Resulting shape is empty")
                );
            }

            // Remove profile surplus using the same axial interval as the base
            // reduction, including the selected-face limits in Dimension/ThroughAll.

            Bnd_Box threadBounds;
            BRepBndLib::Add(thread, threadBounds);
            if (threadBounds.IsVoid()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Thread error: Could not determine thread bounds")
                );
            }

            double xMin;
            double yMin;
            double zMin;
            double xMax;
            double yMax;
            double zMax;
            threadBounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

            const gp_Pnt trimmingOrigin = startPoint.Translated(
                zDirUnit * trimmingEnd
            );
            const gp_Dir trimmingDirection(zDirUnit.Reversed());
            const gp_Lin trimmingAxis(trimmingOrigin, trimmingDirection);

            double trimmingRadius = 0.0;
            for (int xIndex = 0; xIndex < 2; ++xIndex) {
                for (int yIndex = 0; yIndex < 2; ++yIndex) {
                    for (int zIndex = 0; zIndex < 2; ++zIndex) {
                        const gp_Pnt corner(
                            xIndex == 0 ? xMin : xMax,
                            yIndex == 0 ? yMin : yMax,
                            zIndex == 0 ? zMin : zMax
                        );
                        const double cornerRadius = trimmingAxis.Distance(corner);
                        if (cornerRadius > trimmingRadius) {
                            trimmingRadius = cornerRadius;
                        }
                    }
                }
            }
            trimmingRadius += selectedPitch;

            BRepPrimAPI_MakeCylinder trimmingCylinder(
                gp_Ax2(trimmingOrigin, trimmingDirection),
                trimmingRadius,
                trimmingEnd - trimmingStart
            );
            BRepAlgoAPI_Common trimOperation(thread, trimmingCylinder.Shape());
            trimOperation.Build();
            if (!trimOperation.IsDone() || trimOperation.Shape().IsNull()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Thread error: Failed to trim thread ends")
                );
            }
            thread = trimOperation.Shape();

            Part::TopoShape protoThread(thread);

            // Temporary test: display only the trimmed thread tool.
            // Shape.setValue(protoThread);
            // AddSubShape.setValue(protoThread);
            // return App::DocumentObject::StdReturn;

            if (base.isNull()) {
                Shape.setValue(protoThread);
                AddSubShape.setValue(protoThread);
                return App::DocumentObject::StdReturn;
            }

            const char* maker;
            switch (getAddSubType()) {
                case Additive:
                    maker = Part::OpCodes::Fuse;
                    break;
                default:
                    maker = Part::OpCodes::Cut;
            }

            Part::TopoShape result;
            try {
                result.makeElementBoolean(maker, {base, protoThread}, nullptr, FuzzyTolerance.getValue());
                result = getSolid(result);
            }
            catch (Standard_Failure& e) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Thread error: boolean operation failed")
                );
            }
            catch (Base::Exception& e) {
                return new App::DocumentObjectExecReturn(e.what());
            }

            result = refineShapeIfActive(result);

            if (!isSingleSolidRuleSatisfied(result.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Result has multiple solids: enable 'Allow Compound' in the active body."
                ));
            }

            if (!IsInternal.getValue()) {
                reducedBasePreviewShape = base;
            }
            this->Shape.setValue(result);
            AddSubShape.setValue(protoThread);

        } else {
            this->positionByBaseFeature();
            this->Shape.setValue(base);
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    return App::DocumentObject::StdReturn;
}

const Part::TopoShape& Thread::getReducedBasePreviewShape() const
{
    return reducedBasePreviewShape;
}

void Thread::updatePreviewShape()
{
    if (!ModelThread.getValue() || AddSubShape.getShape().isNull()) {
        PreviewShape.setValue(Part::TopoShape());
        return;
    }

    // Thread is a dress-up feature, but its preview is an explicit additive/subtractive tool.
    // Use FeatureAddSub's tool preview instead of DressUp's generated-face preview.
    FeatureAddSub::updatePreviewShape();
}

void Thread::onChanged(const App::Property* prop)
{
    // Base::Console().message("onChangedObject: %s\n", prop->getName());
    if (prop == &ThreadType) {
        std::string type;

        if (ThreadType.isValid()) {
            type = ThreadType.getValueAsString();
            ThreadSize.setEnums(threadUtils.getThreadDiameters(ThreadType.getValue()));
        }

        if (type == "None") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_None_Enums);
        }
        else if (type == "ISOMetricProfile") {
            ThreadClass.setEnums(threadUtils.getThreadClasses(ThreadType.getValue(), IsInternal.getValue()));
        }
        else if (type == "ISOMetricFineProfile") {
            ThreadClass.setEnums(threadUtils.getThreadClasses(ThreadType.getValue(), IsInternal.getValue()));
        }
        else if (type == "UNC") {
            ThreadClass.setEnums(threadUtils.getThreadClasses(ThreadType.getValue(), IsInternal.getValue()));
        }
        else if (type == "UNF") {
            ThreadClass.setEnums(threadUtils.getThreadClasses(ThreadType.getValue(), IsInternal.getValue()));
        }
        else if (type == "UNEF") {
            ThreadClass.setEnums(threadUtils.getThreadClasses(ThreadType.getValue(), IsInternal.getValue()));
        }
        else if (type == "BSP") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_None_Enums);
        }
        else if (type == "NPT") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_None_Enums);
        }
        else if (type == "BSW") {
            ThreadClass.setEnums(threadUtils.getThreadClasses(ThreadType.getValue(), IsInternal.getValue()));
        }
        else if (type == "BSF") {
            ThreadClass.setEnums(threadUtils.getThreadClasses(ThreadType.getValue(), IsInternal.getValue()));
        }
        else if (type == "ISOTyre") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_None_Enums);
        }

        ThreadDesignation.setValue(threadUtils.getThreadDesignations(
            ThreadType.getValue(),
            ThreadSize.getValue(),
            ThreadSizePitch.getValue()
        ));
    }
    else if (prop == &ThreadSize) {
        ThreadSizePitch.setEnums(
            threadUtils.getThreadPitches(ThreadType.getValue(), ThreadSize.getValue())
        );
        ThreadDesignation.setValue(threadUtils.getThreadDesignations(
            ThreadType.getValue(),
            ThreadSize.getValue(),
            ThreadSizePitch.getValue()
        ));
    }
    else if (prop == &ThreadSizePitch) {
        ThreadDesignation.setValue(threadUtils.getThreadDesignations(
            ThreadType.getValue(),
            ThreadSize.getValue(),
            ThreadSizePitch.getValue()
        ));
    }
    else if (prop == &LateralFace || prop == &StartPlane) {
        if (this->testStatus(App::ObjectStatus::Restore)
            || this->testStatus(App::ObjectStatus::Remove) || this->isRestoring()) {
            DressUp::onChanged(prop);
            return;
        }

        App::DocumentObject* faceObj = LateralFace.getValue();

        if (!faceObj) {
            DressUp::onChanged(prop);
            return;
        }

        if (!faceObj->getNameInDocument()) {
            DressUp::onChanged(prop);
            return;
        }

        if (faceObj->isError() || !faceObj->isValid()) {
            DressUp::onChanged(prop);
            return;
        }

        double diameter = 0.0;
        try {
            auto res = threadUtils.validateParameters(LateralFace);
            if (res != App::DocumentObject::StdReturn) {
                return;
            }
            if (prop == &LateralFace && threadUtils.isConicalFace(LateralFace)) {
                // The helix runs opposite to zDir, so its taper is the negative of
                // the face semi-angle: tan(90 - TaperedAngle) = -tan(semiAngle).
                const double angle = 90.0 + Base::toDegrees(threadUtils.getConicalAngle(LateralFace));
                TaperedAngle.setValue(angle);
                Tapered.setValue(true);
            }
            diameter = threadUtils.getLateralFaceDiameter(LateralFace, StartPlane);
        }
        catch (const Standard_Failure& e) {
            Base::Console().warning(
                "Thread::onChanged: OCCT exception ao calcular diâmetro: %s\n",
                e.GetMessageString()
            );
            DressUp::onChanged(prop);
            return;
        }
        catch (const Base::Exception& e) {
            Base::Console().warning("Thread::onChanged: erro ao calcular diâmetro: %s\n", e.what());
            DressUp::onChanged(prop);
            return;
        }

        if (diameter <= 0.0) {
            DressUp::onChanged(prop);
            return;
        }

        Diameter.setValue(diameter);

        Part::TopoShape base;
        try {
            base = getBaseTopoShape();
        }
        catch (Base::Exception& e) {
            Base::Console().warning("Thread::onChanged: erro ao obter base: %s\n", e.what());
            DressUp::onChanged(prop);
            return;
        }

        if (base.isNull()) {
            DressUp::onChanged(prop);
            return;
        }
        bool isInternal = threadUtils.isInternalFace(LateralFace, base.getShape());

        IsInternal.setValue(isInternal);

        int nearestSize = -1;
        int nearestPitch = -1;
        if (!IsInternal.getValue()) {
            nearestSize = threadUtils.findNearestThreadSize(ThreadType.getValue(), diameter);
        }
        else {
            Base::Console().message("Calculando o menor thread size...\n");
            const auto selection = threadUtils.findNearestMinorThreadSize(
                ThreadType.getValue(),
                diameter
            );
            if (selection) {
                nearestSize = selection->sizeIndex;
                nearestPitch = selection->pitchIndex;
            }
            Base::Console().message("Olha o thread size aqui: %d\n", nearestSize);
        }

        if (nearestSize >= 0 && nearestSize != ThreadSize.getValue()) {
            ThreadSize.setValue(nearestSize);
        }
        if (nearestPitch >= 0 && nearestPitch != ThreadSizePitch.getValue()) {
            ThreadSizePitch.setValue(nearestPitch);
        }
    } else if (prop == &Tapered) {
        TaperedAngle.setReadOnly(!Tapered.getValue());
        // Property restoration can assign Tapered after TaperedAngle. Keep the
        // saved angle instead of replacing it with a default during that process.
        if (!this->testStatus(App::ObjectStatus::Restore)
            && !this->testStatus(App::ObjectStatus::Remove) && !this->isRestoring()) {
            if (Tapered.getValue()) {
                bool isConical = threadUtils.isThreadConical(ThreadType.getValue());
                if (isConical && TaperedAngle.getValue() == 90) {
                    TaperedAngle.setValue(threadUtils.getThreadProfileAngle());
                }
            }
            else {
                TaperedAngle.setValue(90);
            }
        }
    }

    DressUp::onChanged(prop);
}

void Thread::addThreadType()
{
    /*TODO*/
}

Base::Vector3d Thread::guessNormalDirection() const
{
    Base::Vector3d zDir(0.0, 0.0, 1.0);
    return zDir;
}

std::vector<gp_Pnt> Thread::getThreadLocations() const
{
    try {
        gp_Vec zDir = threadUtils.getThreadZAxis(LateralFace);
        zDir.Normalize();
        return {threadUtils.getThreadStartPoint(LateralFace, gp_Dir(zDir))};
    }     
    catch (const Standard_Failure&) { return {}; }
    catch (const Base::Exception&) { return {}; }
}

double Thread::getThreadPitch() const
{
    return threadUtils.getThreadPitch(
        ThreadType.getValue(),
        ThreadSize.getValue(),
        ThreadSizePitch.getValue()
    );
}

std::optional<gp_Dir> Thread::getThreadNormal() const
{
    try {
        gp_Vec zDir = threadUtils.getThreadZAxis(LateralFace);
        if (zDir.Magnitude() < Precision::Confusion()) {
            return std::nullopt;
        }
        zDir.Normalize();
        return gp_Dir(zDir);
    }
    catch (const Standard_Failure&) { return std::nullopt; }
    catch (const Base::Exception&) { return std::nullopt; }
}

std::optional<gp_Pnt> Thread::getThreadOrigin() const
{
    try {
        gp_Vec zDir = threadUtils.getThreadZAxis(LateralFace);
        zDir.Normalize();
        return threadUtils.getThreadStartPoint(LateralFace, gp_Dir(zDir));
    }
    catch (const Standard_Failure&) { return std::nullopt; }
    catch (const Base::Exception&) { return std::nullopt; }
}

}  // namespace PartDesign
