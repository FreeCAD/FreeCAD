// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/BoundBox.h>
#include <Base/Vector3D.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/PartDesign/App/FeaturePocket.h>
#include <Mod/Sketcher/App/SketchObject.h>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

namespace
{

double volumeOf(const Part::TopoShape& shape)
{
    if (shape.isNull()) {
        return 0.0;
    }
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape.getShape(), props);
    return props.Mass();
}

}  // namespace

/// Covers FeatureAddSub::updatePreviewShape(), which fills PreviewShape for both
/// the task dialog and a tree preselection preview of a hidden feature.
class FeatureAddSubTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _doc = App::GetApplication().newDocument("FeatureAddSub_test", "testUser");
        _body = _doc->addObject<PartDesign::Body>();

        _baseSketch = _doc->addObject<Sketcher::SketchObject>("BaseSketch");
        _body->addObject(_baseSketch);
        _baseSketch->AttachmentSupport.setValue(_doc->getObject("XY_Plane"), "");
        _baseSketch->MapMode.setValue("FlatFace");
        Part::GeomCircle base;
        base.setRadius(10.0);
        _baseSketch->addGeometry(&base, false);

        _pad = _doc->addObject<PartDesign::Pad>("Pad");
        _body->addObject(_pad);
        _pad->Profile.setValue(_baseSketch, {""});
        _pad->Length.setValue(10.0);
        _doc->recompute();
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_doc->getName());
    }

    /// Add a pocket cutting up into the pad, its axis offset by centerX.
    PartDesign::Pocket* addPocket(double radius, double depth, double centerX = 0.0)
    {
        auto* sketch = _doc->addObject<Sketcher::SketchObject>("PocketSketch");
        _body->addObject(sketch);
        sketch->AttachmentSupport.setValue(_doc->getObject("XY_Plane"), "");
        sketch->MapMode.setValue("FlatFace");
        Part::GeomCircle circle;
        circle.setRadius(radius);
        circle.setCenter(Base::Vector3d(centerX, 0.0, 0.0));
        sketch->addGeometry(&circle, false);

        auto* pocket = _doc->addObject<PartDesign::Pocket>("Pocket");
        _body->addObject(pocket);
        pocket->Profile.setValue(sketch, {""});
        pocket->Reversed.setValue(true);
        pocket->Length.setValue(depth);
        _doc->recompute();
        return pocket;
    }

    PartDesign::Pad* getPad() const
    {
        return _pad;
    }

private:
    App::Document* _doc = nullptr;
    PartDesign::Body* _body = nullptr;
    Sketcher::SketchObject* _baseSketch = nullptr;
    PartDesign::Pad* _pad = nullptr;
};

TEST_F(FeatureAddSubTest, AdditivePreviewIsTheTool)
{
    // an additive feature already is the material it adds, so there is nothing to trim
    getPad()->updatePreviewShape();

    const Part::TopoShape preview = getPad()->PreviewShape.getShape();
    const Part::TopoShape tool = getPad()->AddSubShape.getShape();

    ASSERT_FALSE(preview.isNull());
    EXPECT_TRUE(preview.getShape().IsEqual(tool.getShape()));
}

TEST_F(FeatureAddSubTest, ContainedSubtractivePreviewIsTheWholeTool)
{
    // the tool stays inside the pad, so the removed volume is the tool itself
    PartDesign::Pocket* pocket = addPocket(3.0, 4.0);
    pocket->updatePreviewShape();

    const Part::TopoShape preview = pocket->PreviewShape.getShape();
    const Part::TopoShape tool = pocket->AddSubShape.getShape();

    ASSERT_FALSE(preview.isNull());
    ASSERT_GT(volumeOf(tool), 0.0);
    EXPECT_NEAR(volumeOf(preview), volumeOf(tool), Precision::Confusion());
}

TEST_F(FeatureAddSubTest, OverreachingSubtractivePreviewIsTrimmedToTheBase)
{
    // a tool far deeper than the pad would preview as a long rod hanging below it
    PartDesign::Pocket* pocket = addPocket(3.0, 500.0);
    pocket->updatePreviewShape();

    const Part::TopoShape preview = pocket->PreviewShape.getShape();
    const Part::TopoShape tool = pocket->AddSubShape.getShape();

    ASSERT_FALSE(preview.isNull());
    const double previewHeight = preview.getBoundBox().LengthZ();
    const double toolHeight = tool.getBoundBox().LengthZ();
    const double baseHeight = getPad()->Shape.getBoundingBox().LengthZ();

    // guard the premise: without a real overreach the trim is never exercised
    ASSERT_GT(toolHeight, baseHeight);

    EXPECT_LT(previewHeight, toolHeight);
    EXPECT_LE(previewHeight, baseHeight + Precision::Confusion());
}

TEST_F(FeatureAddSubTest, PreviewFallsBackToTheToolWhenNothingIsRemoved)
{
    // clear of the pad, so there is no removed volume and the trim would draw nothing
    PartDesign::Pocket* pocket = addPocket(3.0, 4.0, 50.0);
    pocket->updatePreviewShape();

    const Part::TopoShape preview = pocket->PreviewShape.getShape();
    const Part::TopoShape tool = pocket->AddSubShape.getShape();

    ASSERT_FALSE(preview.isNull());

    // guard the premise: an overlapping tool would exercise the trim instead
    ASSERT_GT(tool.getBoundBox().MinX, getPad()->Shape.getBoundingBox().MaxX);

    ASSERT_GT(volumeOf(tool), 0.0);
    EXPECT_NEAR(volumeOf(preview), volumeOf(tool), Precision::Confusion());
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
