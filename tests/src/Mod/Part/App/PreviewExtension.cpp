// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <src/App/InitApplication.h>
#include <App/Document.h>
#include <App/PropertyStandard.h>
#include <Base/Interpreter.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PreviewExtension.h>

/// Feature carrying the preview extension, counting recompute calls so the
/// invalidation rules can be observed.
///
/// Not in an anonymous namespace: PROPERTY_SOURCE defines out-of-class statics
/// that must sit at the same namespace scope as the declaration. Reopening
/// Part rather than using a test-local namespace because PROPERTY_HEADER_WITH_OVERRIDE
/// requires DocumentObject-derived classes under src/Mod/Part/ to be qualified
/// with a namespace matching that directory.
namespace Part
{

class PreviewTestFeature: public Part::Feature, public Part::PreviewExtension
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::PreviewTestFeature);

public:
    PreviewTestFeature()
    {
        ADD_PROPERTY_TYPE(Trigger, (0), "Test", App::Prop_None, "Plain input property");
        ADD_PROPERTY_TYPE(Result, (0), "Test", App::Prop_Output, "Output-status property");
        // extensionOnChanged() checks the Output *status* bit (the same one PreviewShape
        // itself carries), not the Prop_Output *property type* set above by ADD_PROPERTY_TYPE
        // -- those are distinct StatusBits positions, so both must be set here.
        Result.setStatus(App::Property::Output, true);

        // Required for a statically-inherited extension: it wires the extension into the
        // owning object's extension map, which is what routes property-changed notifications
        // to extensionOnChanged() and resolves getExtendedObject(). PartDesign::Feature does
        // the same for this same extension (src/Mod/PartDesign/App/Feature.cpp).
        Part::PreviewExtension::initExtension(this);
    }

    App::PropertyInteger Trigger;
    App::PropertyInteger Result;

    int recomputePreviewCalls {0};

protected:
    App::DocumentObjectExecReturn* recomputePreview() override
    {
        ++recomputePreviewCalls;
        return App::DocumentObject::StdReturn;
    }
};

PROPERTY_SOURCE(Part::PreviewTestFeature, Part::Feature)

}  // namespace Part

using Part::PreviewTestFeature;

class PreviewExtensionTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        // PreviewTestFeature derives from Part::Feature, so its parent type must be
        // registered before init() runs; importing Part triggers that registration.
        Base::Interpreter().runString("import Part");
        PreviewTestFeature::init();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("previewtest");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
        _feature = _doc->addObject<PreviewTestFeature>("Feature");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    PreviewTestFeature* getFeature() const
    {
        return _feature;
    }

private:
    std::string _docName;
    App::Document* _doc = nullptr;
    PreviewTestFeature* _feature = nullptr;
};

TEST_F(PreviewExtensionTest, previewShapeIsOutputTransientAndHidden)
{
    const auto& previewShape = getFeature()->PreviewShape;

    EXPECT_TRUE(previewShape.testStatus(App::Property::Output));
    EXPECT_TRUE(previewShape.testStatus(App::Property::Transient));
    EXPECT_TRUE(previewShape.testStatus(App::Property::Hidden));
}

TEST_F(PreviewExtensionTest, updatePreviewRecomputesOnceThenMarksFresh)
{
    auto* feature = getFeature();
    feature->invalidatePreview();

    feature->updatePreview();

    EXPECT_EQ(feature->recomputePreviewCalls, 1);
    EXPECT_TRUE(feature->isPreviewFresh());
}

TEST_F(PreviewExtensionTest, updatePreviewIsNoOpWhileFresh)
{
    auto* feature = getFeature();
    feature->invalidatePreview();
    feature->updatePreview();

    feature->updatePreview();

    EXPECT_EQ(feature->recomputePreviewCalls, 1);
}

TEST_F(PreviewExtensionTest, changingInputPropertyInvalidates)
{
    auto* feature = getFeature();
    feature->updatePreview();
    ASSERT_TRUE(feature->isPreviewFresh());

    feature->Trigger.setValue(42);

    EXPECT_FALSE(feature->isPreviewFresh());
}

TEST_F(PreviewExtensionTest, changingPreviewShapeDoesNotInvalidate)
{
    auto* feature = getFeature();
    feature->updatePreview();
    ASSERT_TRUE(feature->isPreviewFresh());

    feature->PreviewShape.setValue(Part::TopoShape());

    EXPECT_TRUE(feature->isPreviewFresh());
}

TEST_F(PreviewExtensionTest, changingOutputPropertyDoesNotInvalidate)
{
    auto* feature = getFeature();
    feature->updatePreview();
    ASSERT_TRUE(feature->isPreviewFresh());

    feature->Result.setValue(7);

    EXPECT_TRUE(feature->isPreviewFresh());
}

TEST_F(PreviewExtensionTest, invalidatePreviewForcesRecompute)
{
    auto* feature = getFeature();
    feature->updatePreview();
    const int callsBefore = feature->recomputePreviewCalls;

    feature->invalidatePreview();
    feature->updatePreview();

    EXPECT_EQ(feature->recomputePreviewCalls, callsBefore + 1);
}

TEST_F(PreviewExtensionTest, mustRecomputePreviewFollowsMustRecompute)
{
    auto* feature = getFeature();

    EXPECT_EQ(feature->mustRecomputePreview(), feature->mustRecompute());
}
