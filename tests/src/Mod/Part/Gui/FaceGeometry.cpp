// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Trsf.hxx>

#include <Inventor/SoDB.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>

#include <Gui/SoFCDB.h>
#include <Mod/Part/Gui/ViewProviderExt.h>

class FaceGeometryTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        SoDB::init();
        if (!Gui::SoFCDB::isInitialized()) {
            Gui::SoFCDB::init();
        }
        if (PartGui::SoBrepFaceSet::getClassTypeId().isBad()) {
            PartGui::SoBrepFaceSet::initClass();
            PartGui::SoBrepEdgeSet::initClass();
            PartGui::SoBrepPointSet::initClass();
        }
    }
};

TEST_F(FaceGeometryTest, PreservesShapePlacementWithoutChangingTheSourceMesh)
{
    TopoDS_Shape shape = BRepPrimAPI_MakeBox(2.0, 3.0, 4.0).Shape();
    gp_Trsf placement;
    placement.SetTranslation(gp_Vec(10.0, -20.0, 30.0));
    shape.Location(TopLoc_Location(placement));
    const auto scene = PartGui::ViewProviderPartExt::createFaceGeometry(shape, 0.2, 15.0);
    SoGetBoundingBoxAction boundsAction(SbViewportRegion(1, 1));
    boundsAction.apply(scene);
    const auto bounds = boundsAction.getBoundingBox();
    ASSERT_FALSE(bounds.isEmpty());
    EXPECT_EQ(bounds.getMin(), SbVec3f(10.0F, -20.0F, 30.0F));
    EXPECT_EQ(bounds.getMax(), SbVec3f(12.0F, -17.0F, 34.0F));

    for (TopExp_Explorer faces(shape, TopAbs_FACE); faces.More(); faces.Next()) {
        TopLoc_Location location;
        EXPECT_TRUE(BRep_Tool::Triangulation(TopoDS::Face(faces.Current()), location).IsNull());
    }
    EXPECT_TRUE(
        shape.Location().Transformation().TranslationPart().IsEqual(placement.TranslationPart(), 1.0e-12)
    );
}

TEST_F(FaceGeometryTest, EmptyShapeProducesAnEmptyScene)
{
    const auto scene = PartGui::ViewProviderPartExt::createFaceGeometry(TopoDS_Shape(), 0.2, 15.0);
    SoGetBoundingBoxAction boundsAction(SbViewportRegion(1, 1));
    boundsAction.apply(scene);
    EXPECT_TRUE(boundsAction.getBoundingBox().isEmpty());
}
