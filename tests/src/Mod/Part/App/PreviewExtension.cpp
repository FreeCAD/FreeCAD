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
/// PROPERTY_HEADER_WITH_OVERRIDE requires the namespace to match the source
/// directory, so this reopens Part rather than using an anonymous or test-local one.
namespace Part
{

class PreviewTestFeature: public Part::Feature, public Part::PreviewExtension
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::PreviewTestFeature);

public:
    PreviewTestFeature()
    {
        ADD_PROPERTY_TYPE(Trigger, (0), "Test", App::Prop_None, "Plain input property");
        ADD_PROPERTY_TYPE(Result, (0), "Test", App::Prop_None, "Output declared by status bit");
        Result.setStatus(App::Property::Output, true);
        ADD_PROPERTY_TYPE(
            TypeOnlyOutput,
            (0),
            "Test",
            App::Prop_Output,
            "Output declared by property type only, as PartDesign::Feature::_Body is"
        );

        // Required for a statically-inherited extension: without it property changes never
        // reach extensionOnChanged() and getExtendedObject() does not resolve.
        Part::PreviewExtension::initExtension(this);
    }

    App::PropertyInteger Trigger;
    App::PropertyInteger Result;
    App::PropertyInteger TypeOnlyOutput;

    int recomputePreviewCalls {0};

protected:
    App::DocumentObjectExecReturn* recomputePreview() override
    {
        ++recomputePreviewCalls;
        return App::DocumentObject::StdReturn;
    }
};

PROPERTY_SOURCE(Part::PreviewTestFeature, Part::Feature)

/// Feature whose preview does not depend on its properties.
class PreviewIndependentFeature: public PreviewTestFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::PreviewIndependentFeature);

public:
    bool mustRecomputePreview() override
    {
        return false;
    }
};

PROPERTY_SOURCE(Part::PreviewIndependentFeature, Part::PreviewTestFeature)

}  // namespace Part

using Part::PreviewIndependentFeature;
using Part::PreviewTestFeature;

class PreviewExtensionTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        // registers Part::Feature, the parent type init() below resolves
        Base::Interpreter().runString("import Part");
        PreviewTestFeature::init();
        PreviewIndependentFeature::init();
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

    App::Document* getDocument() const
    {
        return _doc;
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

    ASSERT_TRUE(feature->Result.testStatus(App::Property::Output));
    feature->Result.setValue(7);

    EXPECT_TRUE(feature->isPreviewFresh());
}

TEST_F(PreviewExtensionTest, changingTypeOnlyOutputPropertyDoesNotInvalidate)
{
    auto* feature = getFeature();
    feature->updatePreview();
    ASSERT_TRUE(feature->isPreviewFresh());

    ASSERT_FALSE(feature->TypeOnlyOutput.testStatus(App::Property::Output));
    feature->TypeOnlyOutput.setValue(7);

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
    ASSERT_EQ(feature->mustRecomputePreview(), feature->mustRecompute());

    feature->Trigger.setValue(42);

    ASSERT_TRUE(feature->mustRecompute());
    EXPECT_EQ(feature->mustRecomputePreview(), feature->mustRecompute());
}

/// Counterpart of changingInputPropertyInvalidates: the same change on a feature
/// that declines the recompute must leave the preview alone.
TEST_F(PreviewExtensionTest, decliningRecomputeKeepsPreviewFreshOnInputChange)
{
    auto* feature = getDocument()->addObject<PreviewIndependentFeature>("Independent");
    feature->updatePreview();
    ASSERT_TRUE(feature->isPreviewFresh());

    feature->Trigger.setValue(42);

    EXPECT_TRUE(feature->isPreviewFresh());
}
