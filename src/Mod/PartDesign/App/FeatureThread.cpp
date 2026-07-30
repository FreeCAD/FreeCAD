// SPDX-License-Identifier: LGPL-2.1-or-later

#include <iostream>

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
    ThreadType.setEnums(threadUtils.getThreadTypeEnums());

    ADD_PROPERTY_TYPE(ThreadDiameter, (0.0), "Thread", App::Prop_None, "Thread major diameter");

    ADD_PROPERTY_TYPE(ThreadSize, (0L), "Thread", App::Prop_None, "Thread size");
    ThreadSize.setEnums(threadUtils.getThreadDiameters(ThreadType.getValue()));

    ADD_PROPERTY_TYPE(ThreadSizePitch, (0L), "Thread", App::Prop_None, "Thread size");
    ThreadSizePitch.setEnums(
        threadUtils.getThreadPitches(ThreadType.getValue(), ThreadSize.getValue())
    );

    ADD_PROPERTY_TYPE(ThreadDirection, (0L), "Thread", App::Prop_None, "Thread direction");
    ThreadDirection.setEnums(threadUtils.getThreadDirectionEnums());
    ThreadDirection.setReadOnly(true);

    ADD_PROPERTY_TYPE(DepthType, (0L), "Thread", App::Prop_None, "Type");
    DepthType.setEnums(threadUtils.getDepthTypeEnums());

    ADD_PROPERTY_TYPE(Depth, (25.0), "Thread", App::Prop_None, "Length");

    ADD_PROPERTY_TYPE(ThreadClass, (0L), "Thread", App::Prop_None, "Thread class");
    ThreadClass.setEnums(threadUtils.getThreadClass_None_Enums());

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
    std::cout << "EXECUTANDO THREAD" << std::endl;

    Part::TopoShape TopShape;
    try {
        TopShape = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        std::cout << "SAIU 2" << std::endl;
        return new App::DocumentObjectExecReturn(e.what());
    }
    TopShape.setTransform(Base::Matrix4D());

    // Faces where thread should be applied
    bool isThreadEmpty = (LateralFace.getValue() == nullptr);

    // If no element is selected, then we use a copy of previous feature.
    if (isThreadEmpty) {
        this->positionByBaseFeature();
        this->Shape.setValue(TopShape);
        std::cout << "SAIU 3" << std::endl;
        return App::DocumentObject::StdReturn;
    }

    if (!LateralFace.getSubValues().empty()) {
        std::cout << "Subvalue:" << LateralFace.getSubValues()[0].c_str() << std::endl;
    }

    auto res = threadUtils.validateParameters(LateralFace);
    if (res != App::DocumentObject::StdReturn) {
        Base::Console().error(
            "Failed to create thread:\n%s\n",
            res->Why.c_str());

        throw Base::RuntimeError(res->Why.c_str());

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
            std::cout << "SAIU 5" << std::endl;
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Thread error: Unsupported length specification")
            );
        }

        if (length <= 0.0) {
            std::cout << "SAIU 6" << std::endl;
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Thread error: Invalid Thread depth")
            );
        }

        TopoDS_Shape thread
            = threadUtils.makeThread(emptyXDir, emptyZDir, testLength, ThreadType, ThreadSize);
    }
    catch (Base::Exception& e) {
        std::cout << "SAIU 7" << std::endl;
        return new App::DocumentObjectExecReturn(e.what());
    }

    std::cout << "SAIU 8" << std::endl;
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
            ThreadClass.setEnums(threadUtils.getThreadClass_None_Enums());
        }
        else if (type == "ISOMetricProfile") {
            ThreadClass.setEnums(threadUtils.getThreadClass_ISOmetric_Enums());
        }
        else if (type == "ISOMetricFineProfile") {
            ThreadClass.setEnums(threadUtils.getThreadClass_ISOmetricfine_Enums());
        }
        else if (type == "UNC") {
            ThreadClass.setEnums(threadUtils.getThreadClass_UNC_Enums());
        }
        else if (type == "UNF") {
            ThreadClass.setEnums(threadUtils.getThreadClass_UNF_Enums());
        }
        else if (type == "UNEF") {
            ThreadClass.setEnums(threadUtils.getThreadClass_UNEF_Enums());
        }
        else if (type == "BSP") {
            ThreadClass.setEnums(threadUtils.getThreadClass_None_Enums());
        }
        else if (type == "NPT") {
            ThreadClass.setEnums(threadUtils.getThreadClass_None_Enums());
        }
        else if (type == "BSW") {
            ThreadClass.setEnums(threadUtils.getThreadClass_BSW_Enums());
        }
        else if (type == "BSF") {
            ThreadClass.setEnums(threadUtils.getThreadClass_BSF_Enums());
        }
        else if (type == "ISOTyre") {
            ThreadClass.setEnums(threadUtils.getThreadClass_None_Enums());
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
{}

}  // namespace PartDesign
