// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <FCConfig.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/Link.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyLinks.h>
#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/Part/App/LinkArrayLinear.h>
#include <Mod/Assembly/App/AssemblyLink.h>
#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/Groups.h>
#include <src/App/InitApplication.h>

class AssemblyObjectTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        Part::LinearPatternExtension::init();
        Part::LinkArray::init();
        Part::LinkArrayLinear::init();
        Part::AttachExtension::init();
        Part::Primitive::init();
        Part::Box::init();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        auto _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
        _assemblyObj = _doc->addObject<Assembly::AssemblyObject>();
        _jointGroupObj = _assemblyObj->addObject<Assembly::JointGroup>("jointGroupTest");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    Assembly::AssemblyObject* getObject()
    {
        return _assemblyObj;
    }

private:
    // TODO: use shared_ptr or something else here?
    Assembly::AssemblyObject* _assemblyObj;
    Assembly::JointGroup* _jointGroupObj;
    std::string _docName;
};

TEST_F(AssemblyObjectTest, createAssemblyObject)  // NOLINT
{
    // Arrange

    // Act

    // Assert
}

TEST_F(AssemblyObjectTest, assemblyLinkMapsExpandedPartLinkArrayElementReferences)  // NOLINT
{
    auto* doc = getObject()->getDocument();
    auto* sourceAssembly = doc->addObject<Assembly::AssemblyObject>("SourceAssembly");
    auto* box = doc->addObject<Part::Box>("Box");
    auto* array = doc->addObject<Part::LinkArrayLinear>("LinearArray");
    array->LinkedObject.setValue(box);
    array->Occurrences.setValue(2);
    array->Occurrences2.setValue(1);
    array->Length.setValue(10.0);
    array->execute();
    sourceAssembly->addObject(array);

    auto* assemblyLink = doc->addObject<Assembly::AssemblyLink>("AssemblyLink");
    assemblyLink->LinkedObject.setValue(sourceAssembly);
    assemblyLink->synchronizeComponents();

    const auto elements = array->ElementList.getValues();
    ASSERT_EQ(elements.size(), 2);
    auto linkIt = assemblyLink->objLinkMap.find(array);
    ASSERT_NE(linkIt, assemblyLink->objLinkMap.end());
    auto prefixIt = assemblyLink->objSubPrefixMap.find(elements[1]);
    ASSERT_NE(prefixIt, assemblyLink->objSubPrefixMap.end());
    EXPECT_EQ(prefixIt->second, "1");

    auto* joint = doc->addObject("App::FeaturePython", "SourceJoint");
    auto* linkedJoint = doc->addObject("App::FeaturePython", "LinkedJoint");
    auto* sourceRef = dynamic_cast<App::PropertyXLinkSub*>(
        joint->addDynamicProperty("App::PropertyXLinkSub", "Reference1")
    );
    auto* linkedRef = dynamic_cast<App::PropertyXLinkSub*>(
        linkedJoint->addDynamicProperty("App::PropertyXLinkSub", "Reference1")
    );
    ASSERT_NE(sourceRef, nullptr);
    ASSERT_NE(linkedRef, nullptr);
    sourceRef->setValue(elements[1], std::vector<std::string> {"Face1"});

    assemblyLink->handleJointReference(joint, linkedJoint, "Reference1");

    EXPECT_EQ(linkedRef->getValue(), linkIt->second);
    ASSERT_EQ(linkedRef->getSubValues().size(), 1);
    EXPECT_EQ(linkedRef->getSubValues().front(), "1.Face1");
}
