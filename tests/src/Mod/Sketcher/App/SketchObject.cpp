// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <FCConfig.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/SketchObject.h>

class SketchObjectTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (App::Application::GetARGC() == 0) {
            int argc = 1;
            char* argv[] = {"FreeCAD"};
            App::Application::Config()["ExeName"] = "FreeCAD";
            App::Application::init(argc, argv);
        }
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        auto _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
        // TODO: Do we add a body first, or is just adding sketch sufficient for this test?
        _sketchobj =
            static_cast<Sketcher::SketchObject*>(_doc->addObject("Sketcher::SketchObject"));
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    Sketcher::SketchObject* getObject()
    {
        return _sketchobj;
    }

private:
    // TODO: use shared_ptr or something else here?
    Sketcher::SketchObject* _sketchobj;
    std::string _docName;
    std::vector<const char*> allowedTypes {"Vertex",
                                           "Edge",
                                           "ExternalEdge",
                                           "H_Axis",
                                           "V_Axis",
                                           "RootPoint"};
};

TEST_F(SketchObjectTest, createSketchObject)  // NOLINT
{
    // Arrange

    // Act

    // Assert
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeEdge)
{
    // Arrange
    // TODO: Do we need to separate existing vs non-existing?
    // It would need to be implemented in code as well.
    Data::IndexedName name("Edge", 1);
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, 0);
    EXPECT_EQ(posId, Sketcher::PointPos::none);
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeVertex)
{
    // Arrange
    // For operating on vertices, there is first a check if the vertex exists.
    Base::Vector3d p1(0.0, 0.0, 0.0), p2(1.0, 0.0, 0.0);
    std::unique_ptr<Part::Geometry> geoline(new Part::GeomLineSegment());
    static_cast<Part::GeomLineSegment*>(geoline.get())->setPoints(p1, p2);
    getObject()->addGeometry(geoline.get());
    // TODO: Do we need to separate existing vs non-existing?
    // It would need to be implemented in code as well.
    Data::IndexedName name("Vertex", 1);
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, 0);
    EXPECT_EQ(posId, Sketcher::PointPos::start);
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeExternalEdge)
{
    // Arrange
    // TODO: Do we need to separate existing vs non-existing?
    // It would need to be implemented in code as well.
    Data::IndexedName name("ExternalEdge", 1);
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, Sketcher::GeoEnum::RefExt);
    EXPECT_EQ(posId, Sketcher::PointPos::none);
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeHAxis)
{
    // Arrange
    Data::IndexedName name("H_Axis");
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, Sketcher::GeoEnum::HAxis);
    EXPECT_EQ(posId, Sketcher::PointPos::none);
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeVAxis)
{
    // Arrange
    Data::IndexedName name("V_Axis");
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, Sketcher::GeoEnum::VAxis);
    EXPECT_EQ(posId, Sketcher::PointPos::none);
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeRootPoint)
{
    // Arrange
    Data::IndexedName name("RootPoint");
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, Sketcher::GeoEnum::RtPnt);
    EXPECT_EQ(posId, Sketcher::PointPos::start);
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionNoUnits1)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("180 - 60");
    EXPECT_EQ(expr, std::string("60"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionNoUnits2)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("60");
    EXPECT_EQ(expr, std::string("180 - (60)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionWithUnits1)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("180 ° - 60 °");
    EXPECT_EQ(expr, std::string("60 °"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionWithUnits2)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("60 °");
    EXPECT_EQ(expr, std::string("180 ° - (60 °)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionWithUnits3)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("60 deg");
    EXPECT_EQ(expr, std::string("180 ° - (60 deg)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionWithUnits4)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("1rad");
    EXPECT_EQ(expr, std::string("180 ° - (1rad)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionApplyAndReverse1)
{
    std::string expr = "180";
    expr = Sketcher::SketchObject::reverseAngleConstraintExpression(expr);
    expr = Sketcher::SketchObject::reverseAngleConstraintExpression(expr);
    EXPECT_EQ(expr, std::string("(180)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionApplyAndReverse2)
{
    std::string expr = "(30 + 15) * 2 / 3";
    expr = Sketcher::SketchObject::reverseAngleConstraintExpression(expr);
    expr = Sketcher::SketchObject::reverseAngleConstraintExpression(expr);
    EXPECT_EQ(expr, std::string("((30 + 15) * 2 / 3)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionSimple)
{
    // Arrange
    auto constraint = new Sketcher::Constraint();  // Ownership will be transferred to the sketch
    constraint->Type = Sketcher::ConstraintType::Angle;
    auto id = getObject()->addConstraint(constraint);

    App::ObjectIdentifier path(App::ObjectIdentifier::parse(getObject(), "Constraints[0]"));
    std::shared_ptr<App::Expression> shared_expr(App::Expression::parse(getObject(), "0"));
    getObject()->setExpression(path, shared_expr);

    getObject()->setConstraintExpression(id, "180 - (60)");

    // Act
    getObject()->reverseAngleConstraintToSupplementary(constraint, id);

    // Assert
    EXPECT_EQ(std::string("60"), getObject()->getConstraintExpression(id));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionApplyAndReverse)
{
    // Arrange
    auto constraint = new Sketcher::Constraint();  // Ownership will be transferred to the sketch
    constraint->Type = Sketcher::ConstraintType::Angle;
    auto id = getObject()->addConstraint(constraint);

    App::ObjectIdentifier path(App::ObjectIdentifier::parse(getObject(), "Constraints[0]"));
    std::shared_ptr<App::Expression> shared_expr(App::Expression::parse(getObject(), "0"));
    getObject()->setExpression(path, shared_expr);

    getObject()->setConstraintExpression(id, "32 °");

    // Act
    getObject()->reverseAngleConstraintToSupplementary(constraint, id);
    getObject()->reverseAngleConstraintToSupplementary(constraint, id);

    // Assert
    EXPECT_EQ(std::string("32 °"), getObject()->getConstraintExpression(id));
}
