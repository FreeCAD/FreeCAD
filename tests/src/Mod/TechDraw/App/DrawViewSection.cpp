// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <string>

#include <BRepPrimAPI_MakeBox.hxx>

#include <src/App/InitApplication.h>
#include <App/Document.h>
#include <Base/Interpreter.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/TechDraw/App/DrawViewSection.h>
#include <Mod/TechDraw/App/Geometry.h>

class TestDrawViewSection: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        Base::Interpreter().loadModule("TechDraw");
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    App::Document* document() const
    {
        return _doc;
    }

private:
    std::string _docName;
    App::Document* _doc {};
};

// The section line is placed where the user clicked, which is generally not the
// centre of the cut. The view must report the section origin where the drawn
// geometry actually puts it, so that placement aligns it with the section line.
TEST_F(TestDrawViewSection, sectionOriginInViewMatchesGeometry)
{
    constexpr double scale = 2.0;
    auto* box = document()->addObject<Part::Feature>("Box");
    box->Shape.setValue(BRepPrimAPI_MakeBox(100.0, 100.0, 100.0).Solid());

    auto* base = document()->addObject<TechDraw::DrawViewPart>("View");
    base->overrideKeepUpdated(true);
    base->Source.setValues({box});
    base->Direction.setValue(0.0, 0.0, 1.0);
    base->XDirection.setValue(1.0, 0.0, 0.0);

    auto* section = document()->addObject<TechDraw::DrawViewSection>("Section");
    section->overrideKeepUpdated(true);
    section->ScaleType.setValue("Custom");
    section->Scale.setValue(scale);
    section->BaseView.setValue(base);
    section->Source.setValues({box});
    section->SectionNormal.setValue(0.0, -1.0, 0.0);
    section->XDirection.setValue(1.0, 0.0, 0.0);
    // Off centre in both directions of the section view: the cut centroid is
    // at x = 50, z = 50.
    section->SectionOrigin.setValue(20.0, 50.0, 30.0);
    document()->recompute();

    const auto vertices = section->getVertexGeometry();
    ASSERT_FALSE(vertices.empty());
    double xMin = std::numeric_limits<double>::max();
    double xMax = std::numeric_limits<double>::lowest();
    double yMin = std::numeric_limits<double>::max();
    double yMax = std::numeric_limits<double>::lowest();
    for (const auto& vertex : vertices) {
        // Geometry is stored with Y inverted.
        xMin = std::min(xMin, vertex->x());
        xMax = std::max(xMax, vertex->x());
        yMin = std::min(yMin, -vertex->y());
        yMax = std::max(yMax, -vertex->y());
    }

    // The section looks along -Y with X to the right, so Z is up.
    const Base::Vector3d origin = section->getSectionOriginInView() * scale;
    constexpr double tolerance = 1.0e-6;
    EXPECT_NEAR(xMin, origin.x - 20.0 * scale, tolerance);
    EXPECT_NEAR(xMax, origin.x + 80.0 * scale, tolerance);
    EXPECT_NEAR(yMin, origin.y - 30.0 * scale, tolerance);
    EXPECT_NEAR(yMax, origin.y + 70.0 * scale, tolerance);
}
