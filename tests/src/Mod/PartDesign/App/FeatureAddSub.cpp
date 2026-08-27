// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <Precision.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/BoundBox.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/PartDesign/App/FeaturePocket.h>
#include <Mod/Sketcher/App/SketchObject.h>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

/// Covers FeatureAddSub::getAddSubPreviewShape(), which decides what a tree
/// preselection draws on top for a hidden feature.
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

    /// Add a pocket of the given radius and depth cutting up into the pad.
    PartDesign::Pocket* addPocket(double radius, double depth)
    {
        auto* sketch = _doc->addObject<Sketcher::SketchObject>("PocketSketch");
        _body->addObject(sketch);
        sketch->AttachmentSupport.setValue(_doc->getObject("XY_Plane"), "");
        sketch->MapMode.setValue("FlatFace");
        Part::GeomCircle circle;
        circle.setRadius(radius);
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

TEST_F(FeatureAddSubTest, AdditiveReturnsToolUnchanged)
{
    // an additive feature already *is* the material it adds, so no trimming
    Part::TopoShape preview = getPad()->getAddSubPreviewShape();
    Part::TopoShape tool = getPad()->AddSubShape.getShape();

    ASSERT_FALSE(preview.isNull());
    EXPECT_TRUE(preview.getShape().IsEqual(tool.getShape()));
}

TEST_F(FeatureAddSubTest, ContainedSubtractiveToolIsNotTrimmed)
{
    // the tool stays inside the pad, so it already is the removed volume
    PartDesign::Pocket* pocket = addPocket(3.0, 4.0);
    Part::TopoShape preview = pocket->getAddSubPreviewShape();
    Part::TopoShape tool = pocket->AddSubShape.getShape();

    ASSERT_FALSE(preview.isNull());
    EXPECT_TRUE(preview.getShape().IsEqual(tool.getShape()));
}

TEST_F(FeatureAddSubTest, OverreachingSubtractiveToolIsTrimmedToTheBase)
{
    // a tool far deeper than the pad would preview as a long rod hanging below
    // it; the trim clips it back to the material actually removed
    PartDesign::Pocket* pocket = addPocket(3.0, 500.0);
    Part::TopoShape preview = pocket->getAddSubPreviewShape();
    Part::TopoShape tool = pocket->AddSubShape.getShape();

    ASSERT_FALSE(preview.isNull());
    const Base::BoundBox3d previewBox = preview.getBoundBox();
    const Base::BoundBox3d toolBox = tool.getBoundBox();
    const Base::BoundBox3d baseBox = getPad()->Shape.getBoundingBox();

    // guard the premise: if the pocket did not actually overreach the pad this
    // test would pass without exercising the trim at all
    ASSERT_GT(toolBox.LengthZ(), baseBox.LengthZ());

    EXPECT_LT(previewBox.LengthZ(), toolBox.LengthZ());
    EXPECT_LE(previewBox.LengthZ(), baseBox.LengthZ() + Precision::Confusion());
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
