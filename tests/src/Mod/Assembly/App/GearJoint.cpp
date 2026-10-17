// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <cmath>

#include <App/Application.h>
#include <App/Document.h>
#include <App/FeaturePython.h>
#include <App/PropertyStandard.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/AssemblyUtils.h>
#include <Mod/Assembly/App/Groups.h>
#include <Mod/Part/App/PartFeature.h>
#include <src/App/InitApplication.h>

namespace
{

// Only the native joint properties are needed to exercise solving and dragging.
class GearDragTest: public ::testing::TestWithParam<bool>
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        Base::Interpreter().loadModule("Part");
        Base::Interpreter().runString(
            "class _AssemblyGearTestProxy:\n"
            "    def setJointConnectors(self, *args):\n"
            "        pass\n"
            "_assemblyGearTestProxy = _AssemblyGearTestProxy()\n"
        );
    }

    void SetUp() override
    try {
        docName = App::GetApplication().getUniqueDocumentName("gearDrag");
        doc = App::GetApplication().newDocument(docName.c_str());
        assembly = doc->addObject<Assembly::AssemblyObject>("Assembly");
        joints = assembly->addObject<Assembly::JointGroup>("Joints");
        ground = assembly->addObject<Part::Feature>("Ground");
        gear1 = assembly->addObject<Part::Feature>("Gear1");
        gear2 = assembly->addObject<Part::Feature>("Gear2");
        gear2->Placement.setValue(Base::Placement(Base::Vector3d(2, 0, 0), Base::Rotation()));
        ground->Placement.setStatus(App::Property::ReadOnly, true);
        auto* grounding = doc->addObject("App::FeaturePython", "GroundedJoint");
        joints->addObject(grounding);
        auto* groundedObject = static_cast<App::PropertyLink*>(
            grounding->addDynamicProperty("App::PropertyLink", "ObjectToGround")
        );
        groundedObject->setValue(ground);

        addJoint("Revolute1", Assembly::JointType::Revolute, ground, gear1, Base::Vector3d());
        addJoint("Revolute2", Assembly::JointType::Revolute, ground, gear2, Base::Vector3d(2, 0, 0));
        gearJoint = addJoint(
            "GearJoint",
            GetParam() ? Assembly::JointType::Belt : Assembly::JointType::Gears,
            gear1,
            gear2,
            Base::Vector3d()
        );
        setRadius(1.0);
    }
    catch (const Base::Exception& e) {
        FAIL() << e.what();
    }
    catch (const Py::Exception&) {
        Base::PyGILStateLocker lock;
        PyErr_Print();
        FAIL() << "Python setup error";
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(docName.c_str());
    }

    App::DocumentObject* addJoint(
        const char* name,
        Assembly::JointType type,
        App::DocumentObject* first,
        App::DocumentObject* second,
        Base::Vector3d firstPosition
    )
    {
        Base::PyGILStateLocker lock;
        auto* joint = doc->addObject("App::FeaturePython", name);
        joints->addObject(joint);
        auto* enumeration = static_cast<App::PropertyEnumeration*>(
            joint->addDynamicProperty("App::PropertyEnumeration", "JointType")
        );
        static const char* types[] = {
            "Fixed",
            "Revolute",
            "Cylindrical",
            "Slider",
            "Ball",
            "Distance",
            "Parallel",
            "Perpendicular",
            "Angle",
            "RackPinion",
            "Screw",
            "Gears",
            "Belt",
            nullptr
        };
        enumeration->setEnums(types);
        enumeration->setValue(static_cast<long>(type));
        joint->addDynamicProperty("App::PropertyBool", "Suppressed");
        joint->addDynamicProperty("App::PropertyLength", "Distance");
        joint->addDynamicProperty("App::PropertyLength", "Distance2");
        auto* ref1 = static_cast<App::PropertyXLinkSub*>(
            joint->addDynamicProperty("App::PropertyXLinkSub", "Reference1")
        );
        auto* ref2 = static_cast<App::PropertyXLinkSub*>(
            joint->addDynamicProperty("App::PropertyXLinkSub", "Reference2")
        );
        ref1->setValue(first, std::vector<std::string> {""});
        ref2->setValue(second, std::vector<std::string> {""});
        auto* plc1 = static_cast<App::PropertyPlacement*>(
            joint->addDynamicProperty("App::PropertyPlacement", "Placement1")
        );
        joint->addDynamicProperty("App::PropertyPlacement", "Placement2");
        plc1->setValue(Base::Placement(firstPosition, Base::Rotation()));
        auto* proxy = joint->getPropertyByName<App::PropertyPythonObject>("Proxy");
        proxy->setValue(
            Py::Object(
                PyObject_GetAttrString(PyImport_AddModule("__main__"), "_assemblyGearTestProxy"),
                true
            )
        );
        return joint;
    }

    void setRadius(double radius)
    {
        gearJoint->getPropertyByName<App::PropertyFloat>("Distance")->setValue(radius);
        gearJoint->getPropertyByName<App::PropertyFloat>("Distance2")->setValue(1.0);
    }

    App::Document* doc {};
    Assembly::AssemblyObject* assembly {};
    Assembly::JointGroup* joints {};
    Part::Feature* ground {};
    Part::Feature* gear1 {};
    Part::Feature* gear2 {};
    App::DocumentObject* gearJoint {};
    std::string docName;
};

TEST_P(GearDragTest, failedSolveDoesNotStartDrag)
{
    ASSERT_EQ(assembly->getJoints().size(), 3);
    ASSERT_TRUE(assembly->isPartConnected(gear1));
    ASSERT_EQ(assembly->solve(), 0);
    assembly->preDrag({gear1});
    assembly->postDrag();
    setRadius(0.0);
    ASSERT_EQ(assembly->solve(), -1);
    const auto before = gear2->Placement.getValue();
    assembly->preDrag({gear1});
    assembly->doDragStep();
    assembly->postDrag();
    EXPECT_TRUE(gear2->Placement.getValue().isSame(before));
    EXPECT_EQ(assembly->getLastSolverStatus(), -1);
}

TEST_P(GearDragTest, validJointRemainsCoupledDuringDrag)
{
    ASSERT_EQ(assembly->solve(), 0);
    assembly->preDrag({gear1});
    const Base::Rotation rotation(Base::Vector3d::UnitZ, 0.1);
    gear1->Placement.setValue(Base::Placement(Base::Vector3d(), rotation));
    assembly->doDragStep();
    const auto first = gear1->Placement.getValue().getRotation().multVec(Base::Vector3d::UnitX);
    const auto second = gear2->Placement.getValue().getRotation().multVec(Base::Vector3d::UnitX);
    // The solver can adjust both bodies. Check the resulting relationship,
    // rather than assuming the dragged body retains the requested angle.
    EXPECT_GT(first.y, 0.01);
    EXPECT_NEAR(second.x, first.x, 1e-6);
    EXPECT_NEAR(second.y, GetParam() ? first.y : -first.y, 1e-6);
    assembly->postDrag();
}

TEST_P(GearDragTest, dragRecoversAfterCorrectingFailedJoint)
{
    setRadius(0.0);
    assembly->preDrag({gear1});
    ASSERT_EQ(assembly->getLastSolverStatus(), -1);
    assembly->doDragStep();
    assembly->postDrag();

    setRadius(1.0);
    ASSERT_EQ(assembly->solve(), 0);
    assembly->preDrag({gear1});
    gear1->Placement.setValue(
        Base::Placement(Base::Vector3d(), Base::Rotation(Base::Vector3d::UnitZ, 0.1))
    );
    assembly->doDragStep();
    const auto moved = gear2->Placement.getValue().getRotation().multVec(Base::Vector3d::UnitX);
    EXPECT_GT(std::abs(moved.y), 0.01);
    assembly->postDrag();
}

INSTANTIATE_TEST_SUITE_P(
    GearAndBelt,
    GearDragTest,
    ::testing::Bool(),
    [](const ::testing::TestParamInfo<bool>& info) { return info.param ? "Belt" : "Gears"; }
);

}  // namespace
