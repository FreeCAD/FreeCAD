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

namespace PartDesign
{

PROPERTY_SOURCE(PartDesign::Thread, PartDesign::DressUp)

Thread::Thread()
{
    addThreadType();

    ADD_PROPERTY_TYPE(ThreadType, (0L), "Thread", App::Prop_None, "Thread type");
    ThreadType.setEnums(ThreadUtils::ThreadTypeEnums);

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
}

App::DocumentObjectExecReturn* Thread::execute()
{
    Part::TopoShape TopShape;
    try {
        TopShape = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    TopShape.setTransform(Base::Matrix4D());

    // Faces where thread should be applied
    bool isThreadEmpty = (LateralFace.getValue() == nullptr);

    // If no element is selected, then we use a copy of previous feature.
    if (isThreadEmpty) {
        this->positionByBaseFeature();
        this->Shape.setValue(TopShape);
        return App::DocumentObject::StdReturn;
    }

    auto res = threadUtils.validateParameters(LateralFace);
    if (res != App::DocumentObject::StdReturn) {
        Base::Console().error("Failed to create thread:\n%s\n", res->Why.c_str());

        throw Base::RuntimeError(res->Why);

        return res;
    }

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

        TopoDS_Shape thread
            = threadUtils.makeThread(emptyXDir, emptyZDir, testLength, ThreadType, ThreadSize);
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    return App::DocumentObject::StdReturn;
}

void Thread::onChanged(const App::Property* prop)
{
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

    DressUp::onChanged(prop);
}

void Thread::addThreadType()
{
    /*TODO*/
}

}  // namespace PartDesign
