// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/PartDesign/App/FeatureThread.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/Constraint.h>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class ThreadTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _doc = App::GetApplication().newDocument("Thread_test", "testUser");
        _body = _doc->addObject<PartDesign::Body>();
        _sketch = _doc->addObject<Sketcher::SketchObject>("Sketch");
        _body->addObject(_sketch);

        _sketch->AttachmentSupport.setValue(_doc->getObject("XY_Plane"), "");
        _sketch->MapMode.setValue("FlatFace");

        Part::GeomCircle circle;
        circle.setRadius(10.0);
        _sketch->addGeometry(&circle, false);
    }

    void TearDown() override
    {
        if (_doc) {
            App::GetApplication().closeDocument(_doc->getName());
        }
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

    PartDesign::Pad* createCylinderPad(double length = 30.0)
    {
        auto doc = getDocument();
        auto body = getBody();
        auto sketch = getSketch();

        auto pad = doc->addObject<PartDesign::Pad>("Pad");
        body->addObject(pad);
        pad->Profile.setValue(sketch, {""});
        pad->Direction.setValue(0.0, 0.0, 1.0);
        pad->Length.setValue(length);
        pad->Midplane.setValue(false);

        doc->recompute();
        return pad;
    }

    PartDesign::Pad* createCubePad(double sideLength = 30.0)
    {
        auto doc = getDocument();
        auto body = getBody();
        auto sketch = getSketch();

        sketch->Geometry.setValues({});
        sketch->Constraints.setValues({});

        double half = sideLength / 2.0;

        auto l1 = std::make_unique<Part::GeomLineSegment>();
        l1->setPoints(Base::Vector3d(-half, -half, 0.0), Base::Vector3d(half, -half, 0.0));
        sketch->addGeometry(std::move(l1));

        auto l2 = std::make_unique<Part::GeomLineSegment>();
        l2->setPoints(Base::Vector3d(half, -half, 0.0), Base::Vector3d(half, half, 0.0));
        sketch->addGeometry(std::move(l2));

        auto l3 = std::make_unique<Part::GeomLineSegment>();
        l3->setPoints(Base::Vector3d(half, half, 0.0), Base::Vector3d(-half, half, 0.0));
        sketch->addGeometry(std::move(l3));

        auto l4 = std::make_unique<Part::GeomLineSegment>();
        l4->setPoints(Base::Vector3d(-half, half, 0.0), Base::Vector3d(-half, -half, 0.0));
        sketch->addGeometry(std::move(l4));

        int pairs[4][4] = {{0, 2, 1, 1}, {1, 2, 2, 1}, {2, 2, 3, 1}, {3, 2, 0, 1}};

        for (int i = 0; i < 4; ++i) {
            auto c = new Sketcher::Constraint();
            c->Type = Sketcher::Coincident;
            c->First = pairs[i][0];
            c->FirstPos = static_cast<Sketcher::PointPos>(pairs[i][1]);
            c->Second = pairs[i][2];
            c->SecondPos = static_cast<Sketcher::PointPos>(pairs[i][3]);
            sketch->addConstraint(c);
        }

        auto pad = doc->addObject<PartDesign::Pad>("Pad");
        body->addObject(pad);
        pad->Profile.setValue(sketch, {""});
        pad->Direction.setValue(0.0, 0.0, 1.0);
        pad->Length.setValue(sideLength);
        pad->Midplane.setValue(false);

        doc->recompute();
        return pad;
    }

    std::optional<std::string> getLateralFaceName(PartDesign::Pad* pad)
    {
        const TopoDS_Shape& shape = Part::Feature::getShape(pad, Part::ShapeOption::NoFlag);

        const Part::TopoShape& topo = pad->Shape.getShape();

        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            TopoDS_Face face = TopoDS::Face(exp.Current());

            BRepAdaptor_Surface surface(face);

            if (surface.GetType() == GeomAbs_Cylinder) {
                int idx = topo.findShape(face);
                if (idx > 0) {
                    return "Face" + std::to_string(idx);
                }
            }
        }

        return std::nullopt;
    }

private:
    App::Document* _doc = nullptr;
    PartDesign::Body* _body = nullptr;
    Sketcher::SketchObject* _sketch = nullptr;
};

TEST_F(ThreadTest, ThreadCreationOnCylinder)
{
    auto doc = getDocument();
    auto body = getBody();
    auto pad = createCylinderPad(30.0);
    ASSERT_NE(pad, nullptr);
    auto thread = doc->addObject<PartDesign::Thread>("Thread");
    body->addObject(thread);

    auto lateralFace = getLateralFaceName(pad);
    thread->LateralFace.setValue(pad, {*lateralFace});

    doc->recompute();

    ASSERT_NE(thread, nullptr);
    EXPECT_FALSE(thread->isError())
        << "Feature thread has failed during recompute: " << thread->getStatusString();
    EXPECT_TRUE(thread->isValid()) << "Feature Thread is not valid.";
}

TEST_F(ThreadTest, ThreadCreationOnCube)
{
    auto doc = getDocument();
    auto body = getBody();
    auto pad = createCubePad(30.0);
    ASSERT_NE(pad, nullptr);
    auto thread = doc->addObject<PartDesign::Thread>("Thread");
    body->addObject(thread);
    thread->LateralFace.setValue(pad, {"Face3"});  // any cube face works

    doc->recompute();

    ASSERT_NE(thread, nullptr);
    EXPECT_TRUE(thread->isError())
        << "Feature Thread should have failed for a plane face, but didn't failed.";
    EXPECT_FALSE(thread->isValid());
}

TEST_F(ThreadTest, EmptyThread)
{
    auto doc = getDocument();
    auto body = getBody();
    auto pad = createCylinderPad(30.0);
    ASSERT_NE(pad, nullptr);
    auto thread = doc->addObject<PartDesign::Thread>("Thread");
    body->addObject(thread);

    doc->recompute();

    ASSERT_NE(thread, nullptr);
    EXPECT_FALSE(thread->isError())
        << "Feature thread has failed during recompute " << thread->getStatusString();
    EXPECT_TRUE(thread->isValid()) << "Feature Thread is not valid.";
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
