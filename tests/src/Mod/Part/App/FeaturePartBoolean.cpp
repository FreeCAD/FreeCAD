// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "Mod/Part/App/FeaturePartBoolean.h"
#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"

class FeaturePartBooleanTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }


    void SetUp() override
    {
        // createTestDoc();
        // _boolean = _doc->addObject<Part::Boolean>();
    }

    void TearDown() override
    {}

    Part::Boolean* _boolean;  // NOLINT Can't be private in a test framework
};

// This is completely tested in the FeaturePartCommon, FeaturePartCut, FeaturePartFuse and
// FeaturePartSection subclasses.  This class is unfortunately not usable unless initialized in one
// of those forms, so no testing at this level.
