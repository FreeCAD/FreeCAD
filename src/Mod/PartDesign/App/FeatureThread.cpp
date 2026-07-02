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

    ADD_PROPERTY_TYPE(ThreadFit, (0L), "Thread", App::Prop_None, "Clearance Thread fit");
    // ThreadFit.setEnums(ClearanceMetricEnums);

    ADD_PROPERTY_TYPE(ThreadDiameter, (0.0), "Thread", App::Prop_None, "Thread major diameter");

    ADD_PROPERTY_TYPE(ThreadSize, (0L), "Thread", App::Prop_None, "Thread size");
    ThreadSize.setEnums(threadUtils.getThreadDesignations(ThreadType.getValue()));

    ADD_PROPERTY_TYPE(ThreadDirection, (0L), "Thread", App::Prop_None, "Thread direction");
    // ThreadDirection.setEnums(ThreadDirectionEnums);
    // ThreadDirection.setReadOnly(true);
}

void Thread::updateDiameterParam()
{
    // int threadType = ThreadType.getValue();
    // int threadSize = ThreadSize.getValue();
    // if (threadType > 0 && threadSize > 0) {
        // ThreadDiameter.setValue(threadDescription[threadType][threadSize].diameter);
    // }
    // if (auto opt = determineDiameter()) {
    //     Diameter.setValue(opt.value());
    // }
}

App::DocumentObjectExecReturn* Thread::execute()
{
    Base::Console().message("THREAD EXECUTED\n");
    // TODO: verify if this is needed for feature threading
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    // TODO: verify if this is needed for feature threading
    Part::TopoShape TopShape;
    try {
        TopShape = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    // TODO: verify if this is needed for feature threading
    TopShape.setTransform(Base::Matrix4D());


    try {

        // these
        // const int threadDiameter = ThreadDiameter.getValue();
        // int threadType = ThreadType.getValue();
        // int threadSize = ThreadSize.getValue();
        // bool leftHanded = (bool)ThreadDirection.getValue();

        // double Rmaj = threadDescription[threadType][threadSize].diameter / 2;
        // double Pitch = getThreadPitch();
        // double clearance;  // clearance to be added on the diameter
        // double RmajC = Rmaj + clearance;
        // double marginZ = 0.001;
        // double H;

        // this
        //  double threadDepth = ThreadDepth.getValue();

        // double helixLength = threadDepth + Pitch / 2;
        // double holeDepth = Depth.getValue();

        // these
        // std::string threadDepthMethod(ThreadDepthType.getValueAsString());
        // std::string depthMethod(DepthType.getValueAsString());

        // double helixAngle = Tapered.getValue() ? TaperedAngle.getValue() - 90 : 0.0;

        // validate parameters (only cylinders and cones)

        // double length = 0.0;

        //  if (method == "Dimension") {
        //     length = Depth.getValue();
        // }
        // else if (method == "UpToFirst") {
        //     /* TODO */
        // }
        // else if (method == "ThroughAll") {
        // length = getThroughAllLength();
        // }
        // else {
        // return new App::DocumentObjectExecReturn(
        // QT_TRANSLATE_NOOP("Exception", "Hole error: Unsupported length specification")
        // );
        // }

        // if (length <= 0.0) {
        // return new App::DocumentObjectExecReturn(
        // QT_TRANSLATE_NOOP("Exception", "Hole error: Invalid hole depth")
        // );
        // }

        // gp_Vec zDir(SketchVector.x, SketchVector.y, SketchVector.z);
        // zDir.Transform(invObjLoc.Transformation());
        // gp_Vec xDir = computePerpendicular(zDir);

        // TopoDS_Shape protoThread = makeThread(xDir, zDir, length);
        gp_Vec emptyXDir;
        gp_Vec emptyZDir;
        double testLength = 10.0;

        TopoDS_Shape thread = threadUtils.makeThread(emptyXDir, emptyZDir, testLength);
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    return new App::DocumentObjectExecReturn(
        QT_TRANSLATE_NOOP("Exception", "Thread failed: thread not implemented")
    );
}

void Thread::onChanged(const App::Property* prop)
{
    if (prop == &ThreadType) {
        if (ThreadType.isValid()) {
            // type = ThreadType.getValueAsString();
            ThreadSize.setEnums(threadUtils.getThreadDesignations(ThreadType.getValue()));
            // if (type != "None") {
                // findClosestDesignation();
            // }
        }
    }
    
    DressUp::onChanged(prop);
}

void Thread::addThreadType()
{}

}  // namespace PartDesign
