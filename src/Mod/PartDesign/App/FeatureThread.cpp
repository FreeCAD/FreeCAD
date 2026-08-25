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

#include "FeatureThread.h"
#include "FeatureDressUp.h"
#include "ThreadUtils.h"

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
}

App::DocumentObjectExecReturn* Thread::execute()
{
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

        throw Base::RuntimeError(res->Why);

        return res;
    }

    IsInternal.setValue(threadUtils.isInternalFace(LateralFace, base.getShape()));

    addSubType = IsInternal.getValue() ? FeatureAddSub::Subtractive : FeatureAddSub::Additive;
    Base::Console().message("isInternal?: %d\n", IsInternal.getValue());

    gp_Pnt startPoint = threadUtils.getThreadStartPoint(LateralFace, StartPlane);
    Base::Console().message("startPoint = (%f, %f, %f)\n",
                         startPoint.X(), startPoint.Y(), startPoint.Z());
    // double diameter = threadUtils.getLateralFaceDiameter(LateralFace);
    // Base::Console().message("diameter: %lf\n", diameter);

    try {
        gp_Vec emptyXDir;
        gp_Vec emptyZDir;
        double testLength = 10.0;

        gp_Vec zDir = threadUtils.getThreadZAxis(LateralFace);
        gp_Vec xDir = threadUtils.computePerpendicular(zDir);
        std::string method(DepthType.getValueAsString());
        double length = 0.0;

        if (method == "Dimension") {
            length = Depth.getValue();
        }
        else if (method == "UpToFirst") {
            /* TODO */
        }
        else if (method == "ThroughAll") {
            length = threadUtils.getThroughAllLength();
        }
        else if (method == "UpToGeometry") {
            /* TODO */
            length = 10;
        }
        else {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Thread error: Unsupported length specification")
            );
        }

        if (length <= 0.0) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Thread error: Invalid Thread depth")
            );
        }

        if(!IsInternal.getValue()){
            Base::Console().message("Lowering cylinder\n");
            
            double majorDiameter =
                threadUtils.getLateralFaceDiameter(LateralFace);

            double minorDiameter =
                threadUtils.getMinorDiameter(
                    ThreadType.getValue(),
                    ThreadSize.getValue()//,
                    // ThreadClass
                );

            Base::Console().message("minorDiameter: %lf\n", minorDiameter);

            Part::TopoShape reducedBase =
                threadUtils.reduceExternalThreadBase(
                    base,
                    LateralFace,
                    majorDiameter,
                    minorDiameter,
                    length
                );

            base = reducedBase;
        }

        // gp_Vec zDirFixed = zDir.Reversed(); 

        std::cout << "before makeThread\n";
        TopoDS_Shape thread = threadUtils.makeThread(
                xDir, 
                zDir, 
                length, 
                ThreadType.getValue(),
                ThreadSize.getValue(),
                ThreadDirection.getValue(),
                ThreadClass,
                IsInternal.getValue()
        );
        std::cout << "after makeThread\n";

        // if(IsInternal.getValue()){
            gp_Vec zDirUnit = zDir;
            zDirUnit.Normalize();
            gp_Pnt bottomPoint = startPoint.Translated(zDirUnit * length);
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

        Part::TopoShape protoThread(thread);

        if (base.isNull()) {
            Shape.setValue(protoThread);
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

        this->Shape.setValue(result);

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
            ThreadClass.setEnums(ThreadUtils::ThreadClass_ISOmetric_Enums);
        }
        else if (type == "ISOMetricFineProfile") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_ISOmetricfine_Enums);
        }
        else if (type == "UNC") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_UNC_Enums);
        }
        else if (type == "UNF") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_UNF_Enums);
        }
        else if (type == "UNEF") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_UNEF_Enums);
        }
        else if (type == "BSP") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_None_Enums);
        }
        else if (type == "NPT") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_None_Enums);
        }
        else if (type == "BSW") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_BSW_Enums);
        }
        else if (type == "BSF") {
            ThreadClass.setEnums(ThreadUtils::ThreadClass_BSF_Enums);
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
    else if (prop == &LateralFace) {
        if (this->testStatus(App::ObjectStatus::Restore)
            || this->testStatus(App::ObjectStatus::Remove)
            || this->isRestoring()) {
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
            diameter = threadUtils.getLateralFaceDiameter(LateralFace);
        }
        catch (const Standard_Failure& e) {
            Base::Console().warning("Thread::onChanged: OCCT exception ao calcular diâmetro: %s\n", e.GetMessageString());
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

        Part::TopoShape base;
        try {
            base = getBaseTopoShape();
        }
        catch (Base::Exception& e) {
            Base::Console().warning(
                "Thread::onChanged: erro ao obter base: %s\n",
                e.what()
            );
            DressUp::onChanged(prop);
            return;
        }

        if (base.isNull()) {
            DressUp::onChanged(prop);
            return;
        }
        bool isInternal = threadUtils.isInternalFace(
            LateralFace,
            base.getShape()
        );

        IsInternal.setValue(isInternal);

        int nearestSize = -1;
        if(!IsInternal.getValue()){
            nearestSize = threadUtils.findNearestThreadSize(ThreadType.getValue(), diameter);
        } else {
            Base::Console().message("Calculando o menor thread size...\n");
            nearestSize = threadUtils.findNearestMinorThreadSize(ThreadType.getValue(), diameter);
            Base::Console().message("Olha o thread size aqui: %d\n", nearestSize);
        }

        if (nearestSize >= 0 && nearestSize != ThreadSize.getValue()) {
            ThreadSize.setValue(nearestSize);
        }
    }

    DressUp::onChanged(prop);
}

void Thread::addThreadType()
{
    /*TODO*/
}

}  // namespace PartDesign
