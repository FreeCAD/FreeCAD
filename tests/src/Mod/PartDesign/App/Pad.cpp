// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/Sketcher/App/SketchObject.h>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class PadTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _doc = App::GetApplication().newDocument("Pad_test", "testUser");
        _body = dynamic_cast<PartDesign::Body*>(_doc->addObject("PartDesign::Body"));
        _sketch = dynamic_cast<Sketcher::SketchObject*>(
            _doc->addObject("Sketcher::SketchObject", "Sketch"));
        _body->addObject(_sketch);

        _sketch->AttachmentSupport.setValue(_doc->getObject("XY_Plane"), "");
        _sketch->MapMode.setValue("FlatFace");
        Part::GeomCircle circle;
        circle.setRadius(10.0);
        _sketch->addGeometry(&circle, false);
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_doc->getName());
    }

    App::Document* getDocument() const
    {
        return _doc;
    }

    PartDesign::Body* getBody() const
    {
        return _body;
    }

    Sketcher::SketchObject* getSketch() const
    {
        return _sketch;
    }

private:
    App::Document* _doc = nullptr;
    PartDesign::Body* _body = nullptr;
    Sketcher::SketchObject* _sketch = nullptr;
};

TEST_F(PadTest, TestMidPlaneTwoLength)
{
    auto doc = getDocument();
    auto body = getBody();
    auto sketch = getSketch();

    doc->recompute();

    auto pad = dynamic_cast<PartDesign::Pad*>(doc->addObject("PartDesign::Pad", "Pad"));
    body->addObject(pad);
    pad->Profile.setValue(sketch, {""});
    pad->Direction.setValue(0.0, 0.0, 1.0);
    pad->Midplane.setValue(true);
    pad->Length.setValue(10.0);
    pad->Length2.setValue(20.0);

    pad->Type.setValue("TwoLengths");

    doc->recompute();

    auto bbox = pad->Shape.getBoundingBox();

    EXPECT_DOUBLE_EQ(bbox.MaxX, 10.0);
    EXPECT_DOUBLE_EQ(bbox.MinX, -10.0);
    EXPECT_DOUBLE_EQ(bbox.MaxY, 10.0);
    EXPECT_DOUBLE_EQ(bbox.MinY, -10.0);
    EXPECT_DOUBLE_EQ(bbox.MaxZ, 10.0);
    EXPECT_DOUBLE_EQ(bbox.MinZ, -20.0);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
