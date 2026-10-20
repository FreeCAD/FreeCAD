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

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <BRepAlgoAPI_Common.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <gp_Lin.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

#include <Base/Tools.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureThread.h"
#include "FeatureDressUp.h"
#include "ThreadUtils.h"

namespace PartDesign
{

PROPERTY_SOURCE(PartDesign::Thread, PartDesign::DressUp)

Thread::Thread()
{
    addSubType = FeatureAddSub::Additive;

    threadUtils.executeReadThreadDefinitions();

    ADD_PROPERTY_TYPE(
        ThreadType,
        (0L),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Thread type")
    );
    ThreadType.setEnums(threadUtils.getThreadTypeEnums());

    ADD_PROPERTY_TYPE(
        ThreadTypeName,
        (0L),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Thread type name")
    );
    ThreadTypeName.setEnums(threadUtils.getThreadTypeNameEnums());

    ADD_PROPERTY_TYPE(
        ThreadDiameter,
        (0.0),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Thread major diameter")
    );

    ADD_PROPERTY_TYPE(
        ThreadSize,
        (0L),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Thread size")
    );
    ThreadSize.setEnums(threadUtils.getThreadDiameters(ThreadType.getValue()));

    ADD_PROPERTY_TYPE(
        ThreadSizePitch,
        (0L),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Thread size")
    );
    ThreadSizePitch.setEnums(
        threadUtils.getThreadPitches(ThreadType.getValue(), ThreadSize.getValue())
    );

    ADD_PROPERTY_TYPE(
        ThreadDirection,
        (0L),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Thread direction")
    );
    ThreadDirection.setEnums(ThreadUtils::ThreadDirectionEnums);
    ThreadDirection.setReadOnly(true);

    ADD_PROPERTY_TYPE(
        DepthType,
        (0L),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Type")
    );
    DepthType.setEnums(ThreadUtils::DepthTypeEnums);

    ADD_PROPERTY_TYPE(
        Depth,
        (25.0),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Length")
    );

    ADD_PROPERTY_TYPE(
        ThreadClass,
        (0L),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Thread class")
    );
    ThreadClass.setEnums(ThreadUtils::ThreadClass_None_Enums);

    ADD_PROPERTY_TYPE(
        UseCustomThreadClearance,
        (false),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Use custom thread clearance")
    );

    ADD_PROPERTY_TYPE(
        CustomThreadClearance,
        (0.0),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Custom thread clearance (overrides ThreadClass)")
    );

    ADD_PROPERTY_TYPE(
        ThreadDesignation,
        ("---"),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Name")
    );

    ADD_PROPERTY_TYPE(
        LateralFace,
        (nullptr),
        "Thread",
        (App::PropertyType)(App::Prop_None),
        QT_TRANSLATE_NOOP("App::Property", "LateralFace")
    );

    ADD_PROPERTY_TYPE(
        StartPlane,
        (nullptr),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Reference geometry where thread will start")
    );

    ADD_PROPERTY_TYPE(
        UpToGeometry,
        (nullptr),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Reference geometry where thread will end")
    );

    ADD_PROPERTY_TYPE(
        ModelThread,
        (false),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Model actual thread")
    );

    ADD_PROPERTY_TYPE(
        CosmeticThread,
        (true),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Texture the thread")
    );

    ADD_PROPERTY_TYPE(
        Diameter,
        (0.0),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Diameter")
    );

    ADD_PROPERTY_TYPE(
        IsInternal,
        (false),
        "Thread",
        App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "Thread is internal")
    );
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

    const std::string method(DepthType.getValueAsString());
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
            ThreadClass
        );
    }
    else if (method == "ThroughAll") {
        length = threadUtils.isConicalFace(LateralFace)
            ? gp_Vec(origin, farPoint).Dot(gp_Vec(direction))
            : cylinderHeight;
    }
    else if (method == "UpToGeometry") {
        // TODO: limit UpToGeometry to not be upper than startplane
        length = threadUtils.getUpToGeometryLength(UpToGeometry, LateralFace, StartPlane);
    }
    else {
        throw Base::ValueError(
            QT_TRANSLATE_NOOP("Exception", "Thread error: Unsupported length specification")
        );
    }

    if (!std::isfinite(length) || length <= Precision::Confusion()) {
        throw Base::ValueError(QT_TRANSLATE_NOOP("Exception", "Thread error: Invalid Thread depth"));
    }

    return {origin, direction, length};
}

App::DocumentObjectExecReturn* Thread::execute()
try {
    AddSubShape.setValue(Part::TopoShape());
    reducedBasePreviewShape = Part::TopoShape();

    Part::TopoShape base;
    try {
        base = getBaseTopoShape();
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    base.setTransform(Base::Matrix4D());

    const bool isThreadEmpty = (LateralFace.getValue() == nullptr);

    if ((!ModelThread.getValue() && !CosmeticThread.getValue()) || isThreadEmpty) {
        this->positionByBaseFeature();
        this->Shape.setValue(base);
        return App::DocumentObject::StdReturn;
    }

    auto* const res = threadUtils.validateParameters(LateralFace);
    if (res != App::DocumentObject::StdReturn) {
        Base::Console().error("Failed to create thread:\n%s\n", res->Why.c_str());
        return res;
    }

    IsInternal.setValue(threadUtils.isInternalFace(LateralFace, base.getShape()));
    addSubType = IsInternal.getValue() ? FeatureAddSub::Subtractive : FeatureAddSub::Additive;

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
                QT_TRANSLATE_NOOP("Exception", "Thread error: Thread size index out of definition range.")
            );
        }
        definedDiameter = std::stod(diameters[nearestSize]);
    }
    else {
        const auto selection = threadUtils.findNearestMinorThreadSize(ThreadType.getValue(), diameter);
        if (!selection) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Thread error: No thread definition found for the selected type."
            ));
        }
        nearestSize = selection->sizeIndex;
        nearestPitch = selection->pitchIndex;
        definedDiameter = selection->minorDiameter;
    }

    if (nearestSize < 0) {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
            "Exception",
            "Thread error: No thread definition found for the selected type."
        ));
    }

    const bool isCylindricalSupport = !threadUtils.isConicalFace(LateralFace);
    const bool sizeMismatch = nearestSize != ThreadSize.getValue();
    const bool pitchMismatch = IsInternal.getValue() && nearestPitch != ThreadSizePitch.getValue();
    const bool diameterMismatch = std::abs(diameter - definedDiameter) > Precision::Confusion();
    if (ModelThread.getValue() && isCylindricalSupport
        && (sizeMismatch || pitchMismatch || diameterMismatch)) {
        std::ostringstream diameterText;
        diameterText << std::fixed << std::setprecision(3) << diameter;
        std::string formattedDiameter = diameterText.str();
        formattedDiameter.erase(formattedDiameter.find_last_not_of('0') + 1);
        if (formattedDiameter.back() == '.') {
            formattedDiameter.pop_back();
        }
        const std::string threadKind = IsInternal.getValue() ? "internal" : "external";
        const std::string msg = "Thread error: Selected " + threadKind
            + " thread does not match the support diameter " + formattedDiameter + " mm.";
        return new App::DocumentObjectExecReturn(msg.c_str());
    }

    const double conicalAngle = threadUtils.getConicalAngle(LateralFace);

    try {
        const ThreadExtent extent = resolveThreadExtent(base);
        const gp_Pnt startPoint = extent.origin;
        const double length = extent.length;
        const gp_Vec zDir(extent.direction);
        const gp_Vec xDir = threadUtils.computePerpendicular(zDir);

        const gp_Dir axisDir(zDir);
        const gp_Pnt nearPoint = threadUtils.getThreadStartPoint(LateralFace, axisDir);
        const gp_Pnt farPoint = threadUtils.getThreadFarPoint(LateralFace, axisDir);
        const double cylinderHeight = nearPoint.Distance(farPoint);

        const double projStart = gp_Vec(nearPoint, startPoint).Dot(gp_Vec(axisDir));

        if (projStart < -Precision::Confusion()) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Thread error: Start point is above the top of the cylinder face"
            ));
        }

        if (length > cylinderHeight) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Thread error: Thread depth greater than cylinder height")
            );
        }


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
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Thread error: Thread depth must be greater than the selected pitch"
                ));
            }

            std::optional<double> externalMinorDiameter;
            const bool conicalSupport = threadUtils.isConicalFace(LateralFace);
            const double faceSlope = conicalSupport ? std::tan(conicalAngle) : 0.0;
            const double supportEndRadius = diameter / 2.0 + length * faceSlope;
            if (!std::isfinite(supportEndRadius) || supportEndRadius <= Precision::Confusion()) {
                throw Base::ValueError(
                    QT_TRANSLATE_NOOP("Exception", "Thread error: Requested depth reaches the cone apex or has an invalid support radius")
                );
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
                -faceSlope,
                UseCustomThreadClearance.getValue(),
                CustomThreadClearance.getValue(),
                ThreadClass,
                true,
                faceSlope
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
                    "Exception",
                    "Thread error: Thread depth does not overlap the selected face"
                ));
            }
            const double entranceExtension = limitInternalToolToFace && StartPlane.getValue() == nullptr
                ? selectedPitch
                : 0.0;
            const double generatedThreadLength = usefulThreadLength + endTrims.nearEnd
                + endTrims.farEnd + entranceExtension;

            if (!IsInternal.getValue()) {

                const double majorDiameter = diameter + 2.0 * trimmingStart * faceSlope;
                const double minorDiameter = *externalMinorDiameter
                    + 2.0 * trimmingStart * faceSlope;

                base = threadUtils.reduceExternalThreadBase(
                    base,
                    LateralFace,
                    majorDiameter,
                    minorDiameter,
                    trimmingEnd - trimmingStart,
                    startPoint.Translated(zDir * trimmingStart)
                );
            }


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
                -faceSlope,
                UseCustomThreadClearance.getValue(),
                CustomThreadClearance.getValue(),
                endTrims.farEnd,
                usefulThreadLength
            );

            gp_Vec zDirUnit = zDir;
            zDirUnit.Normalize();
            const gp_Pnt bottomPoint = startPoint.Translated(
                zDirUnit * (usefulThreadLength + endTrims.farEnd)
            );

            const double projBottom = gp_Vec(nearPoint, bottomPoint).Dot(gp_Vec(axisDir));

            if (projBottom
                > cylinderHeight - selectedPitch + endTrims.farEnd + Precision::Confusion()) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Thread error: Thread bottom point is below the base of the cylinder face"
                ));
            }

            gp_Trsf translation;
            translation.SetTranslation(gp_Pnt(0.0, 0.0, 0.0), bottomPoint);
            const TopLoc_Location locTrans(translation);
            thread.Move(locTrans);

            if (thread.IsNull()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Thread error: Resulting shape is empty")
                );
            }


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

            const gp_Pnt trimmingOrigin = startPoint.Translated(zDirUnit * trimmingEnd);
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

            const Part::TopoShape protoThread(thread);


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
            catch (const Standard_Failure&) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Thread error: boolean operation failed")
                );
            }
            catch (const Base::Exception& e) {
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
        }
        else {
            this->positionByBaseFeature();
            this->Shape.setValue(base);
        }

        return App::DocumentObject::StdReturn;
    }
    catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    return App::DocumentObject::StdReturn;
}
catch (const Base::Exception& e) {
    return new App::DocumentObjectExecReturn(e.what());
}
catch (const Standard_Failure& e) {
    return new App::DocumentObjectExecReturn(e.GetMessageString());
}
catch (const std::exception& e) {
    return new App::DocumentObjectExecReturn(e.what());
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

    FeatureAddSub::updatePreviewShape();
}

void Thread::onChanged(const App::Property* prop)
try {
    if (prop == &ThreadType || prop == &IsInternal) {
        std::string type;

        if (ThreadType.isValid()) {
            type = ThreadType.getValueAsString();
            if (prop == &ThreadType) {
                ThreadSize.setEnums(threadUtils.getThreadDiameters(ThreadType.getValue()));
                if (!ThreadSize.isValid() && ThreadSize.hasEnums()) {
                    ThreadSize.setValue(0L);
                }
                if (ThreadSize.isValid()) {
                    ThreadSizePitch.setEnums(
                        threadUtils.getThreadPitches(ThreadType.getValue(), ThreadSize.getValue())
                    );
                    if (!ThreadSizePitch.isValid() && ThreadSizePitch.hasEnums()) {
                        ThreadSizePitch.setValue(0L);
                    }
                }
            }
        }
        else {
            ThreadSize.setEnums(std::vector<std::string> {});
            ThreadSizePitch.setEnums(std::vector<std::string> {});
            ThreadClass.setEnums(ThreadUtils::ThreadClass_None_Enums);
            ThreadDesignation.setValue("---");
            DressUp::onChanged(prop);
            return;
        }

        if (type == "None" || type == "BSP" || type == "NPT" || type == "ISOTyre") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_None_Enums);
        }
        else if (
            type == "ISOMetricProfile" || type == "ISOMetricFineProfile" || type == "UNC"
            || type == "UNF" || type == "UNEF" || type == "BSW" || type == "BSF"
        ) {
            ThreadClass.setEnums(
                threadUtils.getThreadClasses(ThreadType.getValue(), IsInternal.getValue())
            );
        }

        if (prop == &ThreadType) {
            ThreadDesignation.setValue(threadUtils.getThreadDesignations(
                ThreadType.getValue(),
                ThreadSize.getValue(),
                ThreadSizePitch.getValue()
            ));
        }
    }
    else if (prop == &ThreadSize) {
        if (!ThreadType.isValid() || !ThreadSize.isValid()) {
            ThreadSizePitch.setEnums(std::vector<std::string> {});
            ThreadDesignation.setValue("---");
            DressUp::onChanged(prop);
            return;
        }
        ThreadSizePitch.setEnums(
            threadUtils.getThreadPitches(ThreadType.getValue(), ThreadSize.getValue())
        );
        if (!ThreadSizePitch.isValid() && ThreadSizePitch.hasEnums()) {
            ThreadSizePitch.setValue(0L);
        }
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
            auto* const res = threadUtils.validateParameters(LateralFace);
            if (res != App::DocumentObject::StdReturn) {
                return;
            }
            diameter = threadUtils.getLateralFaceDiameter(LateralFace, StartPlane);
        }
        catch (const Standard_Failure& e) {
            Base::Console().warning(
                "Thread::onChanged: OCCT exception while calculating diameter: %s\n",
                e.GetMessageString()
            );
            DressUp::onChanged(prop);
            return;
        }
        catch (const Base::Exception& e) {
            Base::Console().warning("Thread::onChanged: Failed to calculate diameter: %s\n", e.what());
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
        catch (const Base::Exception& e) {
            Base::Console().warning("Thread::onChanged: Failed to obtain base shape: %s\n", e.what());
            DressUp::onChanged(prop);
            return;
        }

        if (base.isNull()) {
            DressUp::onChanged(prop);
            return;
        }
        const bool isInternal = threadUtils.isInternalFace(LateralFace, base.getShape());

        IsInternal.setValue(isInternal);

        int nearestSize = -1;
        int nearestPitch = -1;
        if (!IsInternal.getValue()) {
            nearestSize = threadUtils.findNearestThreadSize(ThreadType.getValue(), diameter);
        }
        else {
            const auto selection
                = threadUtils.findNearestMinorThreadSize(ThreadType.getValue(), diameter);
            if (selection) {
                nearestSize = selection->sizeIndex;
                nearestPitch = selection->pitchIndex;
            }
        }

        const auto sizes = threadUtils.getThreadDiameters(ThreadType.getValue());
        if (nearestSize >= 0 && static_cast<size_t>(nearestSize) < sizes.size()
            && nearestSize != ThreadSize.getValue()) {
            ThreadSize.setValue(nearestSize);
        }
        if (nearestPitch >= 0 && ThreadSize.isValid()) {
            const auto pitches
                = threadUtils.getThreadPitches(ThreadType.getValue(), ThreadSize.getValue());
            if (static_cast<size_t>(nearestPitch) < pitches.size()
                && nearestPitch != ThreadSizePitch.getValue()) {
                ThreadSizePitch.setValue(nearestPitch);
            }
        }
    }

    DressUp::onChanged(prop);
}
catch (const Base::Exception& e) {
    Base::Console().warning("Thread::onChanged: %s\n", e.what());
    DressUp::onChanged(prop);
}
catch (const Standard_Failure& e) {
    Base::Console().warning("Thread::onChanged: %s\n", e.GetMessageString());
    DressUp::onChanged(prop);
}
catch (const std::exception& e) {
    Base::Console().warning("Thread::onChanged: %s\n", e.what());
    DressUp::onChanged(prop);
}

Base::Vector3d Thread::guessNormalDirection() const
{
    const Base::Vector3d zDir(0.0, 0.0, 1.0);
    return zDir;
}

std::vector<gp_Pnt> Thread::getThreadLocations() const
{
    try {
        gp_Vec zDir = threadUtils.getThreadZAxis(LateralFace);
        zDir.Normalize();
        return {threadUtils.getThreadStartPoint(LateralFace, gp_Dir(zDir))};
    }
    catch (const Standard_Failure&) {
        return {};
    }
    catch (const Base::Exception&) {
        return {};
    }
}

double Thread::getThreadPitch() const
{
    return threadUtils
        .getThreadPitch(ThreadType.getValue(), ThreadSize.getValue(), ThreadSizePitch.getValue());
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
    catch (const Standard_Failure&) {
        return std::nullopt;
    }
    catch (const Base::Exception&) {
        return std::nullopt;
    }
}

std::optional<gp_Pnt> Thread::getThreadOrigin() const
{
    try {
        gp_Vec zDir = threadUtils.getThreadZAxis(LateralFace);
        zDir.Normalize();
        return threadUtils.getThreadStartPoint(LateralFace, gp_Dir(zDir));
    }
    catch (const Standard_Failure&) {
        return std::nullopt;
    }
    catch (const Base::Exception&) {
        return std::nullopt;
    }
}

}  // namespace PartDesign
