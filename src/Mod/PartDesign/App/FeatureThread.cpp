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

    ADD_PROPERTY_TYPE(ThreadSizePitch, (0L), "Thread", App::Prop_None, "Thread size");
    ThreadSizePitch.setEnums(threadUtils.getThreadPitches(ThreadType.getValue(), ThreadSize.getValue()));

    ADD_PROPERTY_TYPE(ThreadDirection, (0L), "Thread", App::Prop_None, "Thread direction");
    ThreadDirection.setEnums(threadUtils.getThreadDirectionEnums());
    ThreadDirection.setReadOnly(true);

    ADD_PROPERTY_TYPE(DepthType, (0L), "Thread", App::Prop_None, "Type");
    DepthType.setEnums(threadUtils.getDepthTypeEnums());

    ADD_PROPERTY_TYPE(Depth, (25.0), "Thread", App::Prop_None, "Length");

    ADD_PROPERTY_TYPE(ThreadClass, (0L), "Thread", App::Prop_None, "Thread class");
    ThreadClass.setEnums(threadUtils.getThreadClass_None_Enums());
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
        Base::Console().message("THREAD ONLY REFINED\n");
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

    // Faces where draft should be applied
    // Note: Cannot be const reference currently because of BRepOffsetAPI_DraftAngle::Remove() bug,
    // see below
    std::vector<std::string> SubVals = Base.getSubValuesStartsWith("Face");

    // If no element is selected, then we use a copy of previous feature.
    if (SubVals.empty()) {
        this->positionByBaseFeature();
        this->Shape.setValue(TopShape);
        return App::DocumentObject::StdReturn;
    }

    auto res = threadUtils.validateParameters(LateralFace);
    if (res != App::DocumentObject::StdReturn) {
        return res;
    }

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

        // TopoShape profileshape = getProfileShape(
        // Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
        // | Part::ShapeOption::Transform | Part::ShapeOption::DontSimplifyCompound
        // );

        // Base::Vector3d SketchVector = guessNormalDirection(profileshape);

        // Define this as zDir
        // gp_Vec zDir(SketchVector.x, SketchVector.y, SketchVector.z);
        // zDir.Transform(invObjLoc.Transformation());

        
        gp_Vec zDir = threadUtils.getThreadZAxis(LateralFace);
        // TODO: resolver problema de xDir  gp_Vec::Normalize() - vector has zero norm
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
        else {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Hole error: Unsupported length specification")
            );
        }

        if (length <= 0.0) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Hole error: Invalid hole depth")
            );
        }

        // double length = ThreadDepth.getValue();

        TopoDS_Shape thread = threadUtils.makeThread(emptyXDir, emptyZDir, testLength);
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // return new App::DocumentObjectExecReturn(
    //     QT_TRANSLATE_NOOP("Exception", "Thread failed: thread not implemented")
    // );
    return App::DocumentObject::StdReturn;
}

void Thread::onChanged(const App::Property* prop)
{
    if (prop == &ThreadType) {
        std::string type;

        if (ThreadType.isValid()) {
            type = ThreadType.getValueAsString();
            ThreadSize.setEnums(threadUtils.getThreadDesignations(ThreadType.getValue()));
            // if (type != "None") {
                // findClosestDesignation();
            // }
        }

        if (type == "None") {
            ThreadClass.setEnums(threadUtils.getThreadClass_None_Enums());
            // HoleCutType.setEnums(HoleCutType_None_Enums);
            // Threaded.setValue(false);
            // ModelThread.setValue(false);
            // UseCustomThreadClearance.setValue(false);
            // ThreadFit.setEnums(ClearanceNoneEnums);
        }
        else if (type == "ISOMetricProfile") {
            ThreadClass.setEnums(threadUtils.getThreadClass_ISOmetric_Enums());
            // HoleCutType.setEnums(HoleCutType_ISOmetric_Enums);
            // ThreadFit.setEnums(ClearanceMetricEnums);
        }
        else if (type == "ISOMetricFineProfile") {
            ThreadClass.setEnums(threadUtils.getThreadClass_ISOmetricfine_Enums());
            // HoleCutType.setEnums(HoleCutType_ISOmetricfine_Enums);
            // ThreadFit.setEnums(ClearanceMetricEnums);
        }
        else if (type == "UNC") {
            ThreadClass.setEnums(threadUtils.getThreadClass_UNC_Enums());
            // HoleCutType.setEnums(HoleCutType_UNC_Enums);
            // ThreadFit.setEnums(ClearanceUTSEnums);
        }
        else if (type == "UNF") {
            ThreadClass.setEnums(threadUtils.getThreadClass_UNF_Enums());
            // HoleCutType.setEnums(HoleCutType_UNF_Enums);
            // ThreadFit.setEnums(ClearanceUTSEnums);
        }
        else if (type == "UNEF") {
            ThreadClass.setEnums(threadUtils.getThreadClass_UNEF_Enums());
            // HoleCutType.setEnums(HoleCutType_UNEF_Enums);
            // ThreadFit.setEnums(ClearanceUTSEnums);
        }
        else if (type == "BSP") {
            ThreadClass.setEnums(threadUtils.getThreadClass_None_Enums());
            // HoleCutType.setEnums(HoleCutType_BSP_Enums);
            // ThreadFit.setEnums(ClearanceMetricEnums);
        }
        else if (type == "NPT") {
            ThreadClass.setEnums(threadUtils.getThreadClass_None_Enums());
            // HoleCutType.setEnums(HoleCutType_NPT_Enums);
            // ThreadFit.setEnums(ClearanceUTSEnums);
        }
        else if (type == "BSW") {
            ThreadClass.setEnums(threadUtils.getThreadClass_BSW_Enums());
            // HoleCutType.setEnums(HoleCutType_BSW_Enums);
            // ThreadFit.setEnums(ClearanceOtherEnums);
        }
        else if (type == "BSF") {
            ThreadClass.setEnums(threadUtils.getThreadClass_BSF_Enums());
            // HoleCutType.setEnums(HoleCutType_BSF_Enums);
            // ThreadFit.setEnums(ClearanceOtherEnums);
        }
        else if (type == "ISOTyre") {
            ThreadClass.setEnums(threadUtils.getThreadClass_None_Enums());
            // HoleCutType.setEnums(HoleCutType_None_Enums);
        }

    } else if (prop == &ThreadSize) {
        // Base::Console().message("it was me!\n");
        ThreadSizePitch.setEnums(threadUtils.getThreadPitches(ThreadType.getValue(), ThreadSize.getValue()));
    }
    
    DressUp::onChanged(prop);
}

void Thread::addThreadType()
{}

}  // namespace PartDesign
