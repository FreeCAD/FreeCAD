// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <BRepAdaptor_Surface.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <cmath>
#include <filesystem>
#include <limits>

#include <App/Part.h>
#include <src/App/InitApplication.h>
#include <src/TempDirectory.h>
#include <Mod/Part/App/FeaturePartCut.h>
#include <Mod/Part/App/FeatureSectionAnalysis.h>

#include "PartTestHelpers.h"

using namespace PartTestHelpers;

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
class FeatureSectionAnalysisTest: public ::testing::Test, public PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
        // _boxes are 1 x 2 x 3, box 0 is at the origin and box 2 clear of it at y = 3
        _section = _doc->addObject<Part::SectionAnalysis>();
        _section->Source.setValues({_boxes[0]});
        _section->PlaneNormal.setValue(Base::Vector3d(0, 0, 1));
        _section->PlaneOffset.setValue(1.5);
    }

    /// Faces of the section, in the order they appear in the Shape
    static std::vector<TopoDS_Face> faces(const Part::SectionAnalysis* sa)
    {
        std::vector<TopoDS_Face> out;
        const TopoDS_Shape shape = sa->Shape.getShape().getShape();
        if (shape.IsNull()) {
            return out;
        }
        for (TopExp_Explorer xp(shape, TopAbs_FACE); xp.More(); xp.Next()) {
            out.push_back(TopoDS::Face(xp.Current()));
        }
        return out;
    }

    /// Which source object each face came from, in face order. This is the
    /// contract the per-body colouring depends on; how the feature encodes it
    /// is an implementation detail, this rebuilds it from what is published.
    static std::vector<App::DocumentObject*> faceSources(const Part::SectionAnalysis* sa)
    {
        std::vector<App::DocumentObject*> out;
        const auto& parts = sa->SourceParts.getValues();
        for (long pi : sa->FaceSourceIndex.getValues()) {
            out.push_back((pi >= 0 && pi < static_cast<long>(parts.size())) ? parts[pi] : nullptr);
        }
        return out;
    }

    /// Face normal with the face's own orientation applied
    static Base::Vector3d effectiveNormal(const TopoDS_Face& face)
    {
        BRepAdaptor_Surface adapt(face);
        gp_Dir dir = adapt.Plane().Axis().Direction();
        if (face.Orientation() == TopAbs_REVERSED) {
            dir.Reverse();
        }
        return Base::Vector3d(dir.X(), dir.Y(), dir.Z());
    }

    /// "Save" the document, close it, reopen it and hand back the section.
    /// <reopened> receives the new document so the caller can close it.
    Part::SectionAnalysis* saveAndReopen(const std::string& dir, App::Document*& reopened)
    {
        const std::string name = _section->getNameInDocument();
        const std::string path = (std::filesystem::path(dir) / "section.FCStd").string();
        if (!_doc->saveAs(path.c_str())) {
            return nullptr;
        }

        App::GetApplication().closeDocument(_docName.c_str());
        reopened = App::GetApplication().openDocument(path.c_str());
        return reopened ? dynamic_cast<Part::SectionAnalysis*>(reopened->getObject(name.c_str()))
                        : nullptr;
    }

    Part::SectionAnalysis* _section = nullptr;  // NOLINT Can't be private in a test framework
};

TEST_F(FeatureSectionAnalysisTest, testSectionsASingleSolid)
{
    // Act
    _section->execute();

    // Assert - one cap face, the full 1 x 2 cross-section of the box
    const auto f = faces(_section);
    ASSERT_EQ(f.size(), 1);
    EXPECT_NEAR(getArea(_section->Shape.getShape().getShape()), 2.0, Base::Precision::Confusion());
}

TEST_F(FeatureSectionAnalysisTest, testCapLiesOnTheCuttingPlane)
{
    // Act
    _section->execute();

    // Assert - every vertex of the cap sits at the plane offset
    const Base::BoundBox3d bb = _section->Shape.getShape().getBoundBox();
    EXPECT_NEAR(bb.MinZ, 1.5, Base::Precision::Confusion());
    EXPECT_NEAR(bb.MaxZ, 1.5, Base::Precision::Confusion());
}

TEST_F(FeatureSectionAnalysisTest, testCapFacesAlongTheCutNormal)
{
    // Act
    _section->execute();

    // Assert - the cap is oriented along the cutting plane normal
    const auto f = faces(_section);
    ASSERT_EQ(f.size(), 1);
    EXPECT_NEAR(effectiveNormal(f[0]).z, 1.0, Base::Precision::Confusion());
}

TEST_F(FeatureSectionAnalysisTest, testFlipCutReversesTheCapOrientation)
{
    // Arrange
    _section->FlipCut.setValue(true);

    // Act
    _section->execute();

    // Assert - same geometry, opposite facing, because the surviving half swapped
    const auto f = faces(_section);
    ASSERT_EQ(f.size(), 1);
    EXPECT_NEAR(getArea(_section->Shape.getShape().getShape()), 2.0, Base::Precision::Confusion());
    EXPECT_NEAR(effectiveNormal(f[0]).z, -1.0, Base::Precision::Confusion());
}

TEST_F(FeatureSectionAnalysisTest, testSectionsSeveralSources)
{
    // Arrange - two boxes that do not touch, both crossed by the plane
    _section->Source.setValues({_boxes[0], _boxes[2]});

    // Act
    _section->execute();

    // Assert - one cap per box, each attributed to the box it came from
    const auto f = faces(_section);
    ASSERT_EQ(f.size(), 2);
    EXPECT_NEAR(getArea(_section->Shape.getShape().getShape()), 4.0, Base::Precision::Confusion());

    const std::vector<App::DocumentObject*> expected = {_boxes[0], _boxes[2]};
    EXPECT_EQ(faceSources(_section), expected);
    EXPECT_EQ(_section->SourceParts.getValues(), expected);
}

TEST_F(FeatureSectionAnalysisTest, testOneSourceContributingSeveralSolidsSharesItsEntry)
{
    // Arrange - the same object listed twice must not gain a second entry
    _section->Source.setValues({_boxes[0], _boxes[0]});

    // Act
    _section->execute();

    // Assert
    const std::vector<App::DocumentObject*> expected = {_boxes[0]};
    EXPECT_EQ(_section->SourceParts.getValues(), expected);
    for (auto* src : faceSources(_section)) {
        EXPECT_EQ(src, _boxes[0]);
    }
}

TEST_F(FeatureSectionAnalysisTest, testPlaneMissingTheGeometryIsNotAnError)
{
    // Arrange - well clear of the box, which spans z 0..3
    _section->PlaneOffset.setValue(99.0);

    // Act
    auto* result = _section->execute();

    // Assert - an empty section, not a failure
    EXPECT_EQ(result, App::DocumentObject::StdReturn);
    EXPECT_TRUE(_section->Shape.getShape().getShape().IsNull());
}

TEST_F(FeatureSectionAnalysisTest, testHiddenSourceIsSkipped)
{
    // Arrange - the section shows what the user sees
    _boxes[0]->Visibility.setValue(false);

    // Act
    auto* result = _section->execute();

    // Assert - publishes an empty section rather than erroring out
    EXPECT_EQ(result, App::DocumentObject::StdReturn);
    EXPECT_TRUE(_section->Shape.getShape().getShape().IsNull());
    EXPECT_TRUE(_section->SourceParts.getValues().empty());
}

TEST_F(FeatureSectionAnalysisTest, testHidingOneOfSeveralSourcesKeepsTheRest)
{
    // Arrange
    _section->Source.setValues({_boxes[0], _boxes[2]});
    _boxes[0]->Visibility.setValue(false);

    // Act
    _section->execute();

    // Assert - only the visible box contributes
    const std::vector<App::DocumentObject*> expected = {_boxes[2]};
    ASSERT_EQ(faces(_section).size(), 1);
    EXPECT_EQ(_section->SourceParts.getValues(), expected);
    EXPECT_EQ(faceSources(_section), expected);
}

TEST_F(FeatureSectionAnalysisTest, testDegenerateNormalIsAnError)
{
    // Arrange
    _section->PlaneNormal.setValue(Base::Vector3d(0, 0, 0));

    // Act
    auto* result = _section->execute();

    // Assert
    EXPECT_NE(result, App::DocumentObject::StdReturn);
    delete result;
}

TEST_F(FeatureSectionAnalysisTest, testNoSourceIsAnError)
{
    // Arrange
    _section->Source.setValues({});

    // Act
    auto* result = _section->execute();

    // Assert
    EXPECT_NE(result, App::DocumentObject::StdReturn);
    delete result;
}

// --- the cut plane, condition by condition -------------------------------

TEST_F(FeatureSectionAnalysisTest, testNormalNeedNotBeUnitLength)
{
    // Arrange - same plane, expressed with a longer normal
    _section->PlaneNormal.setValue(Base::Vector3d(0, 0, 5));

    // Act
    _section->execute();

    // Assert - the offset is along the *unit* normal, so the cap is unmoved
    const Base::BoundBox3d bb = _section->Shape.getShape().getBoundBox();
    EXPECT_NEAR(bb.MinZ, 1.5, Base::Precision::Confusion());
    EXPECT_NEAR(getArea(_section->Shape.getShape().getShape()), 2.0, Base::Precision::Confusion());
}

TEST_F(FeatureSectionAnalysisTest, testFlipCutKeepsThePlaneInPlace)
{
    // Arrange - flipping swaps the surviving half, it must not move the plane.
    // Only the offset sign convention keeps n * d on the same point, so this
    // is what breaks if normal and offset are ever negated independently.
    _section->FlipCut.setValue(true);

    // Act
    _section->execute();

    // Assert
    const Base::BoundBox3d bb = _section->Shape.getShape().getBoundBox();
    EXPECT_NEAR(bb.MinZ, 1.5, Base::Precision::Confusion());
    EXPECT_NEAR(bb.MaxZ, 1.5, Base::Precision::Confusion());
}

TEST_F(FeatureSectionAnalysisTest, testNegativeOffsetWithFlipStillCuts)
{
    // Arrange - the box spans z 0..3, so a negative offset only intersects
    // once the flip has been applied to the offset as well as the normal
    _section->PlaneNormal.setValue(Base::Vector3d(0, 0, -1));
    _section->PlaneOffset.setValue(-1.5);

    // Act
    _section->execute();

    // Assert
    ASSERT_EQ(faces(_section).size(), 1);
    const Base::BoundBox3d bb = _section->Shape.getShape().getBoundBox();
    EXPECT_NEAR(bb.MinZ, 1.5, Base::Precision::Confusion());
}

TEST_F(FeatureSectionAnalysisTest, testObliquePlaneCutsALargerArea)
{
    // Arrange - 45 degrees about X through the middle of the box
    _section->PlaneNormal.setValue(Base::Vector3d(0, 1, 1));
    _section->PlaneOffset.setValue(Base::Vector3d(0, 1, 1).Normalize() * Base::Vector3d(0.5, 1, 1.5));

    // Act
    _section->execute();

    // Assert - the cap is the 1 wide slice stretched by 1/cos(45)
    ASSERT_EQ(faces(_section).size(), 1);
    EXPECT_NEAR(getArea(_section->Shape.getShape().getShape()), 2.0 * std::sqrt(2.0), 1e-6);
}

// --- source list edge cases ----------------------------------------------

TEST_F(FeatureSectionAnalysisTest, testSourceRejectsNullEntries)
{
    // Assert - the property itself refuses them, so execute() never sees a
    // null in the list and its guard is defence in depth rather than a path
    EXPECT_THROW(_section->Source.setValues({nullptr, _boxes[0]}), Base::ValueError);
}

TEST_F(FeatureSectionAnalysisTest, testSectionDoesNotSectionItself)
{
    // Arrange - a self reference must not recurse or contribute geometry
    _section->Source.setValues({_boxes[0], _section});

    // Act
    _section->execute();

    // Assert
    const std::vector<App::DocumentObject*> expected = {_boxes[0]};
    EXPECT_EQ(_section->SourceParts.getValues(), expected);
    EXPECT_EQ(faces(_section).size(), 1);
}

TEST_F(FeatureSectionAnalysisTest, testHiddenContainerHidesItsContents)
{
    // Arrange - the box stays visible, its container does not. Its own flag is
    // not enough; the whole claiming chain has to be walked.
    auto* container = _doc->addObject<App::Part>();
    container->addObject(_boxes[0]);
    container->Visibility.setValue(false);
    EXPECT_TRUE(_boxes[0]->Visibility.getValue());

    // Act
    auto* result = _section->execute();

    // Assert
    EXPECT_EQ(result, App::DocumentObject::StdReturn);
    EXPECT_TRUE(_section->Shape.getShape().getShape().IsNull());
}

// --- invariants the per-body colouring depends on ------------------------

TEST_F(FeatureSectionAnalysisTest, testMappingHasExactlyOneEntryPerFace)
{
    // Arrange
    _section->Source.setValues({_boxes[0], _boxes[2]});

    // Act
    _section->execute();

    // Assert - the array the view provider indexes by face must match the
    // face count exactly, or the materials silently fall back to one colour
    EXPECT_EQ(_section->FaceSourceIndex.getValues().size(), faces(_section).size());
    for (long pi : _section->FaceSourceIndex.getValues()) {
        EXPECT_GE(pi, 0);
        EXPECT_LT(pi, static_cast<long>(_section->SourceParts.getValues().size()));
    }
}

TEST_F(FeatureSectionAnalysisTest, testRepeatedExecuteIsIdempotent)
{
    // Arrange
    _section->Source.setValues({_boxes[0], _boxes[2]});

    // Act - the output arrays are rebuilt, never appended to
    _section->execute();
    const auto firstParts = _section->SourceParts.getValues();
    const auto firstMap = _section->FaceSourceIndex.getValues();
    _section->execute();

    // Assert
    EXPECT_EQ(_section->SourceParts.getValues(), firstParts);
    EXPECT_EQ(_section->FaceSourceIndex.getValues(), firstMap);
    EXPECT_EQ(_section->FaceSourceIndex.getValues().size(), faces(_section).size());
}

TEST_F(FeatureSectionAnalysisTest, testMissingPlaneClearsThePreviousMapping)
{
    // a section that had geometry
    _section->execute();
    ASSERT_FALSE(_section->FaceSourceIndex.getValues().empty());

    // Act - move the plane clear of everything
    _section->PlaneOffset.setValue(99.0);
    _section->execute();

    // Assert - no stale mapping left pointing at faces that no longer exist
    EXPECT_TRUE(_section->FaceSourceIndex.getValues().empty());
}

// --- the cap and the cut must not drift apart ----------------------------

TEST_F(FeatureSectionAnalysisTest, testCapStaysOnThePlaneItClaimsToCut)
{
    // Arrange - the object owns an inherited Placement that positions its Shape.
    // Whatever that placement is, the cap has to end up on the plane cutPlane()
    // reports, or the rendered section sits somewhere the material is not cut.
    _doc->recompute();
    _section->Placement.setValue(Base::Placement(Base::Vector3d(0, 0, 20), Base::Rotation()));
    _section->PlaneOffset.setValue(1.75);  // force a real recompute through mustExecute()
    _doc->recompute();
    EXPECT_TRUE(_section->Placement.getValue().isIdentity())
        << "a placement survived the recompute and will drift the cap";

    // Act - where the feature says it is cutting
    Base::Vector3d n;
    double d = 0.0;
    ASSERT_TRUE(_section->cutPlane(n, d));

    // ...and where the cap ends up once the object's Placement is applied,
    // which is what the viewer draws and what the user sees
    const Base::BoundBox3d local = _section->Shape.getShape().getBoundBox();
    Base::Vector3d capPoint(local.MinX, local.MinY, local.MinZ);
    _section->Placement.getValue().multVec(capPoint, capPoint);

    // Assert
    EXPECT_NEAR(capPoint * n, d, Base::Precision::Confusion())
        << "cap sits at global z=" << capPoint.z << " but the cut plane is at " << d;
}

// --- round trip through the document -------------------------------------

TEST_F(FeatureSectionAnalysisTest, testShapeComesBackWithoutRecomputing)
{
    // Arrange
    tests::TempDirectory tmp;
    _doc->recompute();
    const double area = getArea(_section->Shape.getShape().getShape());

    // Act
    App::Document* reopened = nullptr;
    auto* sa = saveAndReopen(tmp.path().string(), reopened);
    ASSERT_NE(sa, nullptr);

    // Assert - restored from its own .brp entry, no recompute needed
    EXPECT_EQ(faces(sa).size(), 1);
    EXPECT_NEAR(getArea(sa->Shape.getShape().getShape()), area, Base::Precision::Confusion());
    App::GetApplication().closeDocument(reopened->getName());
}

TEST_F(FeatureSectionAnalysisTest, testPlaneSettingsComeBack)
{
    // Arrange
    tests::TempDirectory tmp;
    _section->FlipCut.setValue(true);
    _doc->recompute();

    // Act
    App::Document* reopened = nullptr;
    auto* sa = saveAndReopen(tmp.path().string(), reopened);
    ASSERT_NE(sa, nullptr);

    // Assert
    EXPECT_TRUE(sa->FlipCut.getValue());
    EXPECT_NEAR(sa->PlaneOffset.getValue(), 1.5, Base::Precision::Confusion());
    EXPECT_NEAR(sa->PlaneNormal.getValue().z, 1.0, Base::Precision::Confusion());
    App::GetApplication().closeDocument(reopened->getName());
}

TEST_F(FeatureSectionAnalysisTest, testSourceLinksResolveIntoTheReopenedDocument)
{
    // Arrange
    tests::TempDirectory tmp;
    _section->Source.setValues({_boxes[0], _boxes[2]});
    const std::string box0 = _boxes[0]->getNameInDocument();
    const std::string box2 = _boxes[2]->getNameInDocument();
    _doc->recompute();

    // Act
    App::Document* reopened = nullptr;
    auto* sa = saveAndReopen(tmp.path().string(), reopened);
    ASSERT_NE(sa, nullptr);

    // Assert - pointing at the new document's objects, not dangling
    const auto sources = sa->Source.getValues();
    ASSERT_EQ(sources.size(), 2);
    EXPECT_EQ(sources[0], reopened->getObject(box0.c_str()));
    EXPECT_EQ(sources[1], reopened->getObject(box2.c_str()));
    App::GetApplication().closeDocument(reopened->getName());
}

TEST_F(FeatureSectionAnalysisTest, testFaceMappingComesBack)
{
    // Arrange
    tests::TempDirectory tmp;
    _section->Source.setValues({_boxes[0], _boxes[2]});
    _doc->recompute();
    const auto expectedMap = _section->FaceSourceIndex.getValues();
    ASSERT_EQ(expectedMap.size(), 2);

    // Act
    App::Document* reopened = nullptr;
    auto* sa = saveAndReopen(tmp.path().string(), reopened);
    ASSERT_NE(sa, nullptr);

    // Assert
    EXPECT_EQ(sa->FaceSourceIndex.getValues(), expectedMap);
    EXPECT_EQ(sa->SourceParts.getValues().size(), 2);
    App::GetApplication().closeDocument(reopened->getName());
}

TEST_F(FeatureSectionAnalysisTest, testMovingThePlaneAfterReloadStillCuts)
{
    // Arrange
    tests::TempDirectory tmp;
    _doc->recompute();

    // Act
    App::Document* reopened = nullptr;
    auto* sa = saveAndReopen(tmp.path().string(), reopened);
    ASSERT_NE(sa, nullptr);
    sa->PlaneOffset.setValue(2.5);
    sa->execute();

    // Assert - the restored links are live, not just present
    EXPECT_NEAR(sa->Shape.getShape().getBoundBox().MinZ, 2.5, Base::Precision::Confusion());
    App::GetApplication().closeDocument(reopened->getName());
}

// --- the source bounding box ---------------------------------------------

TEST_F(FeatureSectionAnalysisTest, testSourceBoundingBoxCoversEverySource)
{
    // Arrange - box 0 spans y 0..2, box 2 spans y 3..5
    _section->Source.setValues({_boxes[0], _boxes[2]});

    // Act
    Bnd_Box bbox;
    ASSERT_TRUE(_section->sourceBoundingBox(bbox));

    // Assert
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    EXPECT_NEAR(ymin, 0.0, 1e-6);
    EXPECT_NEAR(ymax, 5.0, 1e-6);
}

TEST_F(FeatureSectionAnalysisTest, testSourceBoundingBoxSkipsHiddenSources)
{
    // Arrange - the section only cuts what is shown, so anything sized from
    // this box (the plane visual, the preset offset) has to agree
    _section->Source.setValues({_boxes[0], _boxes[2]});
    _boxes[2]->Visibility.setValue(false);

    // Act
    Bnd_Box bbox;
    ASSERT_TRUE(_section->sourceBoundingBox(bbox));

    // Assert - only the visible box 0, spanning y 0..2
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    EXPECT_NEAR(ymin, 0.0, 1e-6);
    EXPECT_NEAR(ymax, 2.0, 1e-6);
}

TEST_F(FeatureSectionAnalysisTest, testSourceBoundingBoxIsEmptyWhenAllHidden)
{
    // Arrange
    _boxes[0]->Visibility.setValue(false);

    // Act
    Bnd_Box bbox;

    // Assert - nothing to size anything from, matching the empty section
    EXPECT_FALSE(_section->sourceBoundingBox(bbox));
}

// --- numeric extremes ----------------------------------------------------

TEST_F(FeatureSectionAnalysisTest, testNotANumberNormalIsRejected)
{
    // Arrange - NaN fails every comparison, so a "< tolerance" guard lets it
    // straight through into the plane construction
    _section->PlaneNormal.setValue(Base::Vector3d(std::numeric_limits<double>::quiet_NaN(), 0, 1));

    // Act
    Base::Vector3d n;
    double d = 0.0;

    // Assert
    EXPECT_FALSE(_section->cutPlane(n, d));
}

TEST_F(FeatureSectionAnalysisTest, testInfiniteNormalIsRejected)
{
    // Arrange
    _section->PlaneNormal.setValue(Base::Vector3d(std::numeric_limits<double>::infinity(), 0, 1));

    // Act
    Base::Vector3d n;
    double d = 0.0;

    // Assert
    EXPECT_FALSE(_section->cutPlane(n, d));
}

TEST_F(FeatureSectionAnalysisTest, testNotANumberOffsetIsRejected)
{
    // Arrange
    _section->PlaneOffset.setValue(std::numeric_limits<double>::quiet_NaN());

    // Act
    Base::Vector3d n;
    double d = 0.0;

    // Assert
    EXPECT_FALSE(_section->cutPlane(n, d));
}

TEST_F(FeatureSectionAnalysisTest, testNormalJustBelowToleranceIsRejected)
{
    // Arrange - length 1e-9, well under Precision::Confusion
    _section->PlaneNormal.setValue(Base::Vector3d(0, 0, 1e-9));

    // Act
    Base::Vector3d n;
    double d = 0.0;

    // Assert
    EXPECT_FALSE(_section->cutPlane(n, d));
}

TEST_F(FeatureSectionAnalysisTest, testTinyButValidNormalIsAccepted)
{
    // Arrange - small, but comfortably above the tolerance
    _section->PlaneNormal.setValue(Base::Vector3d(0, 0, 1e-4));

    // Act
    _section->execute();

    // Assert - normalised, so it cuts exactly where a unit normal would
    ASSERT_EQ(faces(_section).size(), 1);
    const Base::BoundBox3d bb = _section->Shape.getShape().getBoundBox();
    EXPECT_NEAR(bb.MinZ, 1.5, Base::Precision::Confusion());
}

// --- geometric extremes --------------------------------------------------

TEST_F(FeatureSectionAnalysisTest, testPlaneExactlyOnTheBottomFace)
{
    // Arrange - the box spans z 0..3, so this is tangential contact
    _section->PlaneOffset.setValue(0.0);

    // Act
    auto* result = _section->execute();

    // Assert - must not throw or error; an empty or degenerate section is fine
    EXPECT_EQ(result, App::DocumentObject::StdReturn);
}

TEST_F(FeatureSectionAnalysisTest, testPlaneExactlyOnTheTopFace)
{
    // Arrange
    _section->PlaneOffset.setValue(3.0);

    // Act
    auto* result = _section->execute();

    // Assert
    EXPECT_EQ(result, App::DocumentObject::StdReturn);
}

TEST_F(FeatureSectionAnalysisTest, testVeryLargeOffsetIsHandled)
{
    // Arrange
    _section->PlaneOffset.setValue(1e9);

    // Act
    auto* result = _section->execute();

    // Assert - far outside the geometry, so simply empty
    EXPECT_EQ(result, App::DocumentObject::StdReturn);
    EXPECT_TRUE(_section->Shape.getShape().getShape().IsNull());
}

TEST_F(FeatureSectionAnalysisTest, testTinyGeometryStillSections)
{
    // Arrange - a box a tenth of a millimetre across
    auto* small = _doc->addObject<Part::Box>();
    small->Length.setValue(0.1);
    small->Width.setValue(0.1);
    small->Height.setValue(0.1);
    _section->Source.setValues({small});
    _section->PlaneOffset.setValue(0.05);

    // Act
    _section->execute();

    // Assert
    ASSERT_EQ(faces(_section).size(), 1);
    EXPECT_NEAR(getArea(_section->Shape.getShape().getShape()), 0.01, 1e-9);
}

TEST_F(FeatureSectionAnalysisTest, testLargeGeometryStillSections)
{
    // Arrange - a metre-scale part, in millimetres
    auto* big = _doc->addObject<Part::Box>();
    big->Length.setValue(1000);
    big->Width.setValue(2000);
    big->Height.setValue(3000);
    _section->Source.setValues({big});
    _section->PlaneOffset.setValue(1500);

    // Act
    _section->execute();

    // Assert
    ASSERT_EQ(faces(_section).size(), 1);
    EXPECT_NEAR(getArea(_section->Shape.getShape().getShape()), 2.0e6, 1.0);
}

TEST_F(FeatureSectionAnalysisTest, testHollowSolidSectionsAsOneFaceWithAHole)
{
    // Arrange - this is what FaceMakerBullseye exists for: the section of a
    // tube must be one face with a hole, not two disjoint faces
    auto* tool = _doc->addObject<Part::Box>();
    tool->Length.setValue(0.5);
    tool->Width.setValue(0.5);
    tool->Height.setValue(5);
    tool->Placement.setValue(Base::Placement(Base::Vector3d(0.25, 0.75, -1), Base::Rotation()));
    auto* hollow = _doc->addObject<Part::Cut>();
    hollow->Base.setValue(_boxes[0]);
    hollow->Tool.setValue(tool);
    _doc->recompute();

    _section->Source.setValues({hollow});

    // Act
    _section->execute();

    // Assert - one face, its area reduced by the hole
    ASSERT_EQ(faces(_section).size(), 1);
    EXPECT_NEAR(getArea(_section->Shape.getShape().getShape()), 2.0 - 0.25, Base::Precision::Confusion());
}

TEST_F(FeatureSectionAnalysisTest, testNonSolidSourceProducesNothing)
{
    // Arrange - a bare face has no solids to section
    auto* face = _doc->addObject<Part::Feature>();
    auto [rect, wire, e1, e2, e3, e4] = CreateRectFace();
    face->Shape.setValue(rect);
    _section->Source.setValues({face});

    // Act
    auto* result = _section->execute();

    // Assert - not an error, just nothing to cut
    EXPECT_EQ(result, App::DocumentObject::StdReturn);
    EXPECT_TRUE(_section->Shape.getShape().getShape().IsNull());
}

TEST_F(FeatureSectionAnalysisTest, testSourcePlacementIsComposedIntoTheCut)
{
    // Arrange - box 1 is offset to y = 1 by its own Placement, so the section
    // is only correct if that placement is resolved rather than ignored
    _section->Source.setValues({_boxes[1]});

    // Act
    _section->execute();

    // Assert
    ASSERT_EQ(faces(_section).size(), 1);
    const Base::BoundBox3d bb = _section->Shape.getShape().getBoundBox();
    EXPECT_NEAR(bb.MinY, 1.0, Base::Precision::Confusion());
    EXPECT_NEAR(bb.MaxY, 3.0, Base::Precision::Confusion());
}

TEST_F(FeatureSectionAnalysisTest, testManySourcesAllMapCorrectly)
{
    // Arrange - every box the fixture provides, overlapping and not
    _section->Source.setValues({_boxes[0], _boxes[1], _boxes[2], _boxes[3], _boxes[4], _boxes[5]});

    // Act
    _section->execute();

    // Assert - the mapping stays exactly one entry per face however many
    // sources pile up, and every index resolves
    const auto& map = _section->FaceSourceIndex.getValues();
    const auto& parts = _section->SourceParts.getValues();
    EXPECT_EQ(map.size(), faces(_section).size());
    EXPECT_GT(map.size(), 0);
    for (long pi : map) {
        EXPECT_GE(pi, 0);
        EXPECT_LT(pi, static_cast<long>(parts.size()));
    }
}

// --- dragging the gizmo --------------------------------------------------

TEST_F(FeatureSectionAnalysisTest, testDragWithNoInputLeavesThePlaneAlone)
{
    // Act
    Base::Vector3d n;
    double d = 0.0;
    Part::SectionAnalysis::planeAfterDrag(Base::Vector3d(0, 0, 1), 10.0, Base::Rotation(), 0.0, n, d);

    // Assert
    EXPECT_NEAR(n.z, 1.0, 1e-9);
    EXPECT_NEAR(d, 10.0, 1e-9);
}

TEST_F(FeatureSectionAnalysisTest, testDragTranslationSlidesAlongTheNormal)
{
    // Act
    Base::Vector3d n;
    double d = 0.0;
    Part::SectionAnalysis::planeAfterDrag(Base::Vector3d(0, 0, 1), 10.0, Base::Rotation(), 2.5, n, d);

    // Assert
    EXPECT_NEAR(n.z, 1.0, 1e-9);
    EXPECT_NEAR(d, 12.5, 1e-9);
}

TEST_F(FeatureSectionAnalysisTest, testDragRotationKeepsThePlaneThroughTheGizmo)
{
    // Arrange - the gizmo sits on the plane at (0, 0, 10); turning the handle
    // must pivot the plane about that point, not swing it about the origin
    const Base::Vector3d startNormal(0, 0, 1);
    const double startOffset = 10.0;
    const Base::Vector3d pivot = startNormal * startOffset;
    const Base::Rotation turn(Base::Vector3d(1, 0, 0), M_PI / 6);  // 30 degrees

    // Act
    Base::Vector3d n;
    double d = 0.0;
    Part::SectionAnalysis::planeAfterDrag(startNormal, startOffset, turn, 0.0, n, d);

    // Assert - the pivot still lies on the plane
    EXPECT_NEAR(n * pivot, d, 1e-9)
        << "the plane no longer passes through the point the gizmo is on";
}

TEST_F(FeatureSectionAnalysisTest, testDragRotationThroughTheOriginIsUnaffected)
{
    // Arrange - with the plane through the origin the pivot is the origin, so
    // the offset is zero whichever way it is derived. This is why the drift
    // goes unnoticed until the section is moved away from the origin.
    const Base::Rotation turn(Base::Vector3d(1, 0, 0), M_PI / 6);

    // Act
    Base::Vector3d n;
    double d = 0.0;
    Part::SectionAnalysis::planeAfterDrag(Base::Vector3d(0, 0, 1), 0.0, turn, 0.0, n, d);

    // Assert
    EXPECT_NEAR(d, 0.0, 1e-9);
}

TEST_F(FeatureSectionAnalysisTest, testDragRotationKeepsTheNormalUnitLength)
{
    // Act
    Base::Vector3d n;
    double d = 0.0;
    Part::SectionAnalysis::planeAfterDrag(
        Base::Vector3d(0, 0, 1),
        10.0,
        Base::Rotation(Base::Vector3d(1, 1, 0), 0.7),
        0.0,
        n,
        d
    );

    // Assert
    EXPECT_NEAR(n.Length(), 1.0, 1e-9);
}

TEST_F(FeatureSectionAnalysisTest, testDragRotationAndTranslationCombine)
{
    // Arrange
    const Base::Vector3d startNormal(0, 0, 1);
    const double startOffset = 10.0;
    const Base::Vector3d pivot = startNormal * startOffset;
    const Base::Rotation turn(Base::Vector3d(1, 0, 0), M_PI / 6);

    // Act
    Base::Vector3d n;
    double d = 0.0;
    Part::SectionAnalysis::planeAfterDrag(startNormal, startOffset, turn, 3.0, n, d);

    // Assert - pivot about the gizmo first, then slide 3mm along the new normal
    EXPECT_NEAR(d, n * pivot + 3.0, 1e-9);
}

// --- the shared plane frame ----------------------------------------------

TEST_F(FeatureSectionAnalysisTest, testPlaneFrameIsOrthonormalOnBothBranches)
{
    // Arrange - the axis picked to cross with switches on |n.x| < 0.9, so both
    // sides of that decision need to produce a usable frame
    const std::vector<Base::Vector3d> normals = {
        Base::Vector3d(0, 0, 1),
        Base::Vector3d(1, 0, 0),
        Base::Vector3d(-1, 0, 0),
        Base::Vector3d(0.5, 0.5, 0.7071).Normalize(),
    };

    for (const auto& n : normals) {
        // Act
        Base::Vector3d u, v;
        Part::SectionAnalysis::planeFrame(n, u, v);

        // Assert
        EXPECT_NEAR(u.Length(), 1.0, 1e-9);
        EXPECT_NEAR(v.Length(), 1.0, 1e-9);
        EXPECT_NEAR(u * v, 0.0, 1e-9);
        EXPECT_NEAR(u * n, 0.0, 1e-9);
        EXPECT_NEAR(v * n, 0.0, 1e-9);
    }
}
// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
