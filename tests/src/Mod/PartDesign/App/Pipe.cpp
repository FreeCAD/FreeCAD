// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <App/Application.h>
#include <App/Document.h>
#include "Mod/PartDesign/App/Body.h"
#include "Mod/PartDesign/App/ShapeBinder.h"
#include "Mod/PartDesign/App/FeaturePipe.h"
#include "Mod/Sketcher/App/SketchObject.h"
#include <BRepGProp.hxx>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

// Uncomment the line below to save temporary test-generated documents
// #define PIPE_SAVE_TEST_FCSTD 1

// from tests/src/Mod/Part/App/TopoShapeExpansion.cpp
double getVolume(const TopoDS_Shape& shape)
{
    GProp_GProps prop;
    BRepGProp::VolumeProperties(shape, prop);
    return abs(prop.Mass());
}

class FeaturePipeTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        docName_ = App::GetApplication().getUniqueDocumentName("testname");
        doc_ = App::GetApplication().newDocument(docName_.c_str(), "testlabel");
        body_ = doc_->addObject<PartDesign::Body>();

        sketch_circle_ = doc_->addObject<Sketcher::SketchObject>("circle");

        // Attach the sketch to body origin
        // reference: tests/src/Mod/Part/Attacher.cpp
        body_->addObject(sketch_circle_);
        sketch_circle_->MapReversed.setValue(false);
        auto origin = body_->getObject("Origin");
        sketch_circle_->AttachmentOffset.setValue(
            Base::Placement(Base::Vector3d(0, 0, 0), Base::Rotation())
        );
        sketch_circle_->AttachmentSupport.setValue(origin, std::vector<std::string> {"XY_Plane."});
        sketch_circle_->MapPathParameter.setValue(0.0);
        sketch_circle_->MapMode.setValue("FlatFace");

        //
        // Create a fully constrained circle sketch

        // first create the circle geometry
        Part::GeomCircle circle;
        Base::Vector3d coordsCenter(0.0, 0.0, 0.0);
        double radius = 1.0;
        circle.setCenter(coordsCenter);
        circle.setRadius(radius);

        // add circle geometry to sketch
        sketch_circle_->addGeometry(&circle);

        // New scope for constraints.  This allows to use the same
        // temporary variable names as the line sketch below
        {
            auto coincident = Sketcher::Constraint();
            coincident.Type = Sketcher::ConstraintType::Coincident;
            coincident.First = 0;
            coincident.FirstPos = Sketcher::PointPos::mid;
            coincident.Second = -1;
            coincident.SecondPos = Sketcher::PointPos::start;
            sketch_circle_->addConstraint(&coincident);

            auto diameter = Sketcher::Constraint();
            diameter.Type = Sketcher::ConstraintType::Diameter;
            diameter.First = 0;
            diameter.setValue(1.0);
            sketch_circle_->addConstraint(&diameter);
        }

        auto obj = std::vector<std::pair<int, long>> {{0, 1}};
        sketch_circle_->setGeometryIds(obj);

        try {
            sketch_circle_->solve();
        }
        catch (const Base::Exception& e) {
            FAIL() << e.what();
        }

        doc_->recompute();

#ifdef PIPE_SAVE_TEST_FCSTD
        {
            auto p = std::filesystem::temp_directory_path();
            doc_->saveAs((p / "FeaturePipeTest-circle.FCStd").c_str());
        }
#endif
        //
        // Create fully constrained line sketch
        // - define the sketch attachment
        // - create the line geometry
        // - add the line geometry to the sketch
        // - add constraints to the sketch

        sketch_line_ = doc_->addObject<Sketcher::SketchObject>("line");

        // Attach the sketch to body origin, reference: tests/src/Mod/Part/Attacher.cpp
        body_->addObject(sketch_line_);
        sketch_line_->MapReversed.setValue(false);
        sketch_line_->AttachmentSupport.setValue(origin, std::vector<std::string> {"XZ_Plane."});
        sketch_line_->MapPathParameter.setValue(0.0);
        sketch_line_->MapMode.setValue("FlatFace");

        Part::GeomLineSegment line;
        Base::Vector3d p1(0.0, 0.0, 0.0);
        Base::Vector3d p2(0.0, 1.0, 0.0);
        line.setPoints(p1, p2);
        sketch_line_->addGeometry(&line);

        {
            auto distance = Sketcher::Constraint();
            distance.Type = Sketcher::ConstraintType::Distance;
            distance.First = 0;
            distance.setValue(1);
            sketch_line_->addConstraint(&distance);

            auto vertical = Sketcher::Constraint();
            vertical.Type = Sketcher::ConstraintType::Vertical;
            vertical.First = 0;
            sketch_line_->addConstraint(&vertical);

            auto coincident = Sketcher::Constraint();
            coincident.Type = Sketcher::ConstraintType::Coincident;
            coincident.First = 0;
            coincident.FirstPos = Sketcher::PointPos::start;
            coincident.Second = -1;
            coincident.SecondPos = Sketcher::PointPos::start;
            sketch_line_->addConstraint(&coincident);
        }
        doc_->recompute();
        sketch_line_->solve();

#ifdef PIPE_SAVE_TEST_FCSTD
        {
            auto p = std::filesystem::temp_directory_path();
            doc_->saveAs((p / "FeaturePipeTest-circle-line.FCStd").c_str());
        }
#endif

        ASSERT_TRUE(sketch_circle_->FullyConstrained.getValue());
        ASSERT_TRUE(sketch_line_->FullyConstrained.getValue());
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(docName_.c_str());
    }

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)

    App::Document* doc_ = nullptr;
    std::string docName_;
    Sketcher::SketchObject* sketch_circle_ = nullptr;
    Sketcher::SketchObject* sketch_line_ = nullptr;
    PartDesign::Body* body_ = nullptr;
    PartDesign::ShapeBinder* binder_circle_ = nullptr;
    PartDesign::ShapeBinder* binder_line_ = nullptr;

    // Volume of a cylinder is
    // V = pi * r^2 * h
    // calculated for the values in this unit test
    static constexpr double volume_expected_ = M_PI * 0.5 * 0.5 * 1;

    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

TEST_F(FeaturePipeTest, SketchProfileSketchSpine)
{
    auto pipe = doc_->addObject<PartDesign::AdditivePipe>("pipe");
    body_->addObject(pipe);

    pipe->Spine.setValue(sketch_line_);
    pipe->Profile.setValue(sketch_circle_);
    doc_->recompute();
#ifdef PIPE_SAVE_TEST_FCSTD
    {
        auto p = std::filesystem::temp_directory_path();
        doc_->saveAs((p / "FeaturePipeTest-SketchProfileSketchSpine.FCStd").c_str());
    }
#endif

    auto shape = pipe->Shape.getShape().getShape();
    auto volume_measured = getVolume(shape);
    ASSERT_NEAR(volume_expected_, volume_measured, 0.001);
}

TEST_F(FeaturePipeTest, SketchProfileBinderSpine)
{
    auto binderspine = doc_->addObject<PartDesign::SubShapeBinder>("bind-spine");
    binderspine->Shape.setValue(sketch_line_->Shape.getShape());
    body_->addObject(binderspine);

    auto pipe = doc_->addObject<PartDesign::AdditivePipe>("pipe");
    body_->addObject(pipe);

    pipe->Spine.setValue(binderspine);
    pipe->Profile.setValue(sketch_circle_);
    doc_->recompute();
#ifdef PIPE_SAVE_TEST_FCSTD
    {
        auto p = std::filesystem::temp_directory_path();
        doc_->saveAs((p / "FeaturePipeTest-SketchProfileBinderSpine.FCStd").c_str());
    }
#endif

    auto shape = pipe->Shape.getShape().getShape();
    auto volume_measured = getVolume(shape);
    ASSERT_NEAR(volume_expected_, volume_measured, 0.001);
}

TEST_F(FeaturePipeTest, BinderProfileSketchSpine)
{
    auto binderprofile = doc_->addObject<PartDesign::SubShapeBinder>("bind-profile");
    binderprofile->Shape.setValue(sketch_circle_->Shape.getShape());
    body_->addObject(binderprofile);

    auto pipe = doc_->addObject<PartDesign::AdditivePipe>("pipe");
    body_->addObject(pipe);

    pipe->Spine.setValue(sketch_line_);
    pipe->Profile.setValue(binderprofile);
    doc_->recompute();
#ifdef PIPE_SAVE_TEST_FCSTD
    {
        auto p = std::filesystem::temp_directory_path();
        doc_->saveAs((p / "FeaturePipeTest-BinderProfileSketchSpine.FCStd").c_str());
    }
#endif
    auto shape = pipe->Shape.getShape().getShape();
    auto volume_measured = getVolume(shape);
    ASSERT_NEAR(volume_expected_, volume_measured, 0.001);
}

TEST_F(FeaturePipeTest, BinderProfileBinderSpine)
{
    auto binderprofile = doc_->addObject<PartDesign::SubShapeBinder>("bind-profile");
    binderprofile->Shape.setValue(sketch_circle_->Shape.getShape());
    body_->addObject(binderprofile);

    auto binderspine = doc_->addObject<PartDesign::SubShapeBinder>("bind-spine");
    binderspine->Shape.setValue(sketch_line_->Shape.getShape());
    body_->addObject(binderspine);

    auto pipe = doc_->addObject<PartDesign::AdditivePipe>("pipe");
    body_->addObject(pipe);

    pipe->Spine.setValue(binderspine);
    pipe->Profile.setValue(binderprofile);
    doc_->recompute();
#ifdef PIPE_SAVE_TEST_FCSTD
    {
        auto p = std::filesystem::temp_directory_path();
        doc_->saveAs((p / "FeaturePipeTest-BinderProfileBinderSpine.FCStd").c_str());
    }
#endif
    auto shape = pipe->Shape.getShape().getShape();
    auto volume_measured = getVolume(shape);
    ASSERT_NEAR(volume_expected_, volume_measured, 0.001);
}
// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
