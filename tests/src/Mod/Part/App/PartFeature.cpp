// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "Mod/Part/App/FeaturePartCommon.h"
#include <src/App/InitApplication.h>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include "PartTestHelpers.h"

using namespace Part;
using namespace PartTestHelpers;

class FeaturePartTest: public ::testing::Test, public PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }


    void SetUp() override
    {
        createTestDoc();
        _common = dynamic_cast<Common*>(_doc->addObject("Part::Common"));
    }

    void TearDown() override
    {}

    Common* _common = nullptr;  // NOLINT Can't be private in a test framework
};

TEST_F(FeaturePartTest, testGetElementName)
{
    // Arrange
    _boxes[0]->Shape.getShape().Tag = 1L;
    _boxes[1]->Shape.getShape().Tag = 2L;
    _common->Base.setValue(_boxes[0]);
    _common->Tool.setValue(_boxes[1]);

    // Act
    _common->execute();
    const TopoShape& ts = _common->Shape.getShape();

    auto namePair = _common->getElementName("test");
    auto namePairExport = _common->getElementName("test", App::GeoFeature::Export);
    auto namePairSelf = _common->getElementName(nullptr);
    // Assert
    EXPECT_STREQ(namePair.first.c_str(), "");
    EXPECT_STREQ(namePair.second.c_str(), "test");
    EXPECT_STREQ(namePairExport.first.c_str(), "");
    EXPECT_STREQ(namePairExport.second.c_str(), "test");
    EXPECT_STREQ(namePairSelf.first.c_str(), "");
    EXPECT_STREQ(namePairSelf.second.c_str(), "");
#ifndef FC_USE_TNP_FIX
    EXPECT_EQ(ts.getElementMap().size(), 0);
#else
    EXPECT_EQ(ts.getElementMap().size(), 0);  // TODO: Value and code TBD
#endif
    // TBD
}
