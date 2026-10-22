// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cmath>
#include <numbers>

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <gp_Ax2.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/PartDesign/App/FeatureThread.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/Constraint.h>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

static int findEnumIndex(const std::vector<std::string>& enums, const std::string& needle)
{
    for (size_t i = 0; i < enums.size(); ++i) {
        if (enums[i].find(needle) != std::string::npos) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

struct ThreadShapeSignature
{
    double volume {0.0};
    double surface {0.0};
    gp_Pnt centerOfMass;
    double xmin {0.0};
    double ymin {0.0};
    double zmin {0.0};
    double xmax {0.0};
    double ymax {0.0};
    double zmax {0.0};
    int faces {0};
    int edges {0};
    int vertices {0};
};

static ThreadShapeSignature getThreadShapeSignature(const TopoDS_Shape& shape)
{
    GProp_GProps volumeProperties;
    BRepGProp::VolumeProperties(shape, volumeProperties);

    GProp_GProps surfaceProperties;
    BRepGProp::SurfaceProperties(shape, surfaceProperties);

    Bnd_Box box;
    BRepBndLib::Add(shape, box);

    ThreadShapeSignature signature;
    signature.volume = volumeProperties.Mass();
    signature.surface = surfaceProperties.Mass();
    signature.centerOfMass = volumeProperties.CentreOfMass();
    box.Get(
        signature.xmin,
        signature.ymin,
        signature.zmin,
        signature.xmax,
        signature.ymax,
        signature.zmax
    );

    for (TopExp_Explorer explorer(shape, TopAbs_FACE); explorer.More(); explorer.Next()) {
        ++signature.faces;
    }
    for (TopExp_Explorer explorer(shape, TopAbs_EDGE); explorer.More(); explorer.Next()) {
        ++signature.edges;
    }
    for (TopExp_Explorer explorer(shape, TopAbs_VERTEX); explorer.More(); explorer.Next()) {
        ++signature.vertices;
    }
    return signature;
}

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

    static void addProfileLoop(
        Sketcher::SketchObject* sketch,
        const std::vector<Base::Vector3d>& points,
        bool close = true
    )
    {
        const size_t edgeCount = close ? points.size() : points.size() - 1;
        for (size_t i = 0; i < edgeCount; ++i) {
            auto line = std::make_unique<Part::GeomLineSegment>();
            line->setPoints(points[i], points[(i + 1) % points.size()]);
            sketch->addGeometry(std::move(line));
        }
    }

    Sketcher::SketchObject* createThreadProfile(
        const char* name,
        const std::vector<Base::Vector3d>& points,
        bool close = true
    )
    {
        auto* profile = _doc->addObject<Sketcher::SketchObject>(name);
        addProfileLoop(profile, points, close);
        _doc->recompute();
        return profile;
    }

    static TopoDS_Wire getProfileWire(const Sketcher::SketchObject* profile)
    {
        return TopoDS::Wire(profile->Shape.getShape().getShape());
    }

    static TopoDS_Wire makeRoundedWhitworthRootProfile()
    {
        constexpr double height = 0.960491;
        constexpr double radius = 0.137329;
        constexpr double tangentOffsetFactor = 0.58284013094;

        const double finishedRoot = -2.0 * height / 3.0;
        const double rootCenter = finishedRoot + radius;
        const double rootTangent = finishedRoot + radius * tangentOffsetFactor;
        const double tangentAngle = std::atan2(1.0 / 8.0, rootTangent - rootCenter);
        const double lowerMidAngle = (std::numbers::pi + tangentAngle) / 2.0;
        const double hiddenRoot = finishedRoot - 0.1;

        const auto circlePoint = [rootCenter, radius](double centerY, double angle) {
            return gp_Pnt(rootCenter + radius * std::cos(angle), centerY + radius * std::sin(angle), 0.0);
        };

        const gp_Pnt rootStart(finishedRoot, 0.0, 0.0);
        const gp_Pnt lowerMid = circlePoint(0.0, lowerMidAngle);
        const gp_Pnt lowerTangent(rootTangent, 1.0 / 8.0, 0.0);
        const gp_Pnt crest(0.0, 0.5, 0.0);
        const gp_Pnt upperTangent(rootTangent, 7.0 / 8.0, 0.0);
        const gp_Pnt upperMid = circlePoint(1.0, -lowerMidAngle);
        const gp_Pnt rootEnd(finishedRoot, 1.0, 0.0);
        const gp_Pnt hiddenEnd(hiddenRoot, 1.0, 0.0);
        const gp_Pnt hiddenStart(hiddenRoot, 0.0, 0.0);

        BRepBuilderAPI_MakeWire builder;
        builder.Add(
            BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(rootStart, lowerMid, lowerTangent).Value()).Edge()
        );
        builder.Add(BRepBuilderAPI_MakeEdge(lowerTangent, crest).Edge());
        builder.Add(BRepBuilderAPI_MakeEdge(crest, upperTangent).Edge());
        builder.Add(
            BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(upperTangent, upperMid, rootEnd).Value()).Edge()
        );
        builder.Add(BRepBuilderAPI_MakeEdge(rootEnd, hiddenEnd).Edge());
        builder.Add(BRepBuilderAPI_MakeEdge(hiddenEnd, hiddenStart).Edge());
        builder.Add(BRepBuilderAPI_MakeEdge(hiddenStart, rootStart).Edge());
        builder.Build();
        return builder.Wire();
    }

    PartDesign::Thread* createExtentThread(const gp_Dir& direction = gp_Dir(0.0, 0.0, 1.0))
    {
        auto* base = _doc->addObject<PartDesign::Feature>("Base");
        _body->addObject(base);
        base->Shape.setValue(
            BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(2.0, 3.0, 4.0), direction), 12.0, 30.0).Shape()
        );
        auto* thread = _doc->addObject<PartDesign::Thread>("Thread");
        _body->addObject(thread);
        thread->BaseFeature.setValue(base);
        for (TopExp_Explorer explorer(base->Shape.getValue(), TopAbs_FACE); explorer.More();
             explorer.Next()) {
            const auto face = TopoDS::Face(explorer.Current());
            if (BRepAdaptor_Surface(face).GetType() == GeomAbs_Cylinder) {
                thread->LateralFace.setValue(
                    base,
                    {"Face" + std::to_string(base->Shape.getShape().findShape(face))}
                );
                break;
            }
        }
        const int type = findEnumIndex(thread->ThreadType.getEnumVector(), "ISOMetricProfile");
        if (type < 0) {
            return nullptr;
        }
        thread->ThreadType.setValue(type);
        const int size = findEnumIndex(thread->ThreadSize.getEnumVector(), "24");
        if (size < 0) {
            return nullptr;
        }
        thread->ThreadSize.setValue(size);
        thread->UseCustomThreadClearance.setValue(true);
        thread->CustomThreadClearance.setValue(0.0);
        thread->ModelThread.setValue(false);
        return thread;
    }

    PartDesign::Feature* createExtentPoint(const char* name, const gp_Pnt& point)
    {
        auto* feature = _doc->addObject<PartDesign::Feature>(name);
        feature->Shape.setValue(BRepBuilderAPI_MakeVertex(point).Shape());
        return feature;
    }

    PartDesign::Pad* createCylinderPad(double length = 30.0, double radius = 10.0)
    {
        auto doc = getDocument();
        auto body = getBody();
        auto sketch = getSketch();

        sketch->Geometry.setValues({});
        sketch->Constraints.setValues({});

        Part::GeomCircle circle;
        circle.setRadius(radius);
        sketch->addGeometry(&circle, false);

        auto pad = doc->addObject<PartDesign::Pad>("Pad");
        body->addObject(pad);
        pad->Profile.setValue(sketch, {""});
        pad->Direction.setValue(0.0, 0.0, 1.0);
        pad->Length.setValue(length);
        pad->Midplane.setValue(false);

        doc->recompute();
        return pad;
    }

    PartDesign::Pad* createAnnularPad(double length, double outerRadius, double innerRadius)
    {
        auto doc = getDocument();
        auto body = getBody();
        auto sketch = getSketch();

        sketch->Geometry.setValues({});
        sketch->Constraints.setValues({});

        Part::GeomCircle outerCircle;
        outerCircle.setRadius(outerRadius);
        sketch->addGeometry(&outerCircle, false);

        Part::GeomCircle innerCircle;
        innerCircle.setRadius(innerRadius);
        sketch->addGeometry(&innerCircle, false);

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

    std::optional<std::string> getLateralFaceName(PartDesign::Pad* pad, double expectedRadius)
    {
        const TopoDS_Shape& shape = Part::Feature::getShape(pad, Part::ShapeOption::NoFlag);
        const Part::TopoShape& topo = pad->Shape.getShape();

        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            const TopoDS_Face face = TopoDS::Face(exp.Current());
            const BRepAdaptor_Surface surface(face);
            if (surface.GetType() != GeomAbs_Cylinder
                || std::abs(surface.Cylinder().Radius() - expectedRadius) > 1e-7) {
                continue;
            }

            const int idx = topo.findShape(face);
            if (idx > 0) {
                return "Face" + std::to_string(idx);
            }
        }

        return std::nullopt;
    }

private:
    App::Document* _doc = nullptr;
    PartDesign::Body* _body = nullptr;
    Sketcher::SketchObject* _sketch = nullptr;
};

TEST_F(ThreadTest, ThreadProfileLoaderCopiesValidNormalizedProfile)
{
    createThreadProfile(
        "ThreadProfile",
        {
            Base::Vector3d(-0.75, 0.001, 0.0),
            Base::Vector3d(0.0, 0.4375, 0.0),
            Base::Vector3d(0.0, 0.5625, 0.0),
            Base::Vector3d(-0.75, 0.999, 0.0),
        }
    );

    PartDesign::ThreadUtils::ThreadDefinition definition;
    PartDesign::ThreadUtils::findThreadProfiles(getDocument(), definition, "profile-test");

    using Status = PartDesign::ThreadUtils::ThreadDefinition::ProfileStatus;
    ASSERT_EQ(definition.profileStatus, Status::Valid) << definition.profileDiagnostic;
    ASSERT_TRUE(definition.profile.has_value());
    EXPECT_EQ(definition.externalProfileStatus, Status::Missing);
    EXPECT_TRUE(definition.externalProfileDiagnostic.find("was not found") != std::string::npos);
    EXPECT_NEAR(definition.profile->minX, -0.75, 1e-9);
    EXPECT_NEAR(definition.profile->maxX, 0.0, 1e-9);
    EXPECT_NEAR(definition.profile->minY, 0.001, 1e-9);
    EXPECT_NEAR(definition.profile->maxY, 0.999, 1e-9);
    EXPECT_TRUE(definition.profile->wire.isClosed());

    getDocument()->removeObject("ThreadProfile");
    getDocument()->recompute();
    EXPECT_FALSE(definition.profile->wire.isNull());
    EXPECT_TRUE(definition.profile->wire.isValid());
}

TEST_F(ThreadTest, ThreadProfileLoaderReadsOptionalExternalProfile)
{
    const std::vector<Base::Vector3d> commonProfile {
        Base::Vector3d(-0.75, 0.001, 0.0),
        Base::Vector3d(0.0, 0.4375, 0.0),
        Base::Vector3d(0.0, 0.5625, 0.0),
        Base::Vector3d(-0.75, 0.999, 0.0),
    };
    createThreadProfile("ThreadProfile", commonProfile);

    auto externalProfile = commonProfile;
    externalProfile[0].x = -0.8;
    externalProfile[3].x = -0.8;
    createThreadProfile("ExternalThreadProfile", externalProfile);

    PartDesign::ThreadUtils::ThreadDefinition definition;
    PartDesign::ThreadUtils::findThreadProfiles(getDocument(), definition, "profile-test");

    using Status = PartDesign::ThreadUtils::ThreadDefinition::ProfileStatus;
    ASSERT_EQ(definition.profileStatus, Status::Valid) << definition.profileDiagnostic;
    ASSERT_EQ(definition.externalProfileStatus, Status::Valid)
        << definition.externalProfileDiagnostic;
    ASSERT_TRUE(definition.externalProfile.has_value());
    EXPECT_NEAR(definition.externalProfile->minX, -0.8, 1e-9);
    EXPECT_EQ(definition.sketches.size(), 2);
}

TEST_F(ThreadTest, ThreadProfileLoaderRejectsOpenProfile)
{
    createThreadProfile(
        "ThreadProfile",
        {
            Base::Vector3d(-0.75, 0.001, 0.0),
            Base::Vector3d(0.0, 0.4375, 0.0),
            Base::Vector3d(0.0, 0.5625, 0.0),
            Base::Vector3d(-0.75, 0.999, 0.0),
        },
        false
    );

    PartDesign::ThreadUtils::ThreadDefinition definition;
    PartDesign::ThreadUtils::findThreadProfiles(getDocument(), definition, "profile-test");

    using Status = PartDesign::ThreadUtils::ThreadDefinition::ProfileStatus;
    EXPECT_EQ(definition.profileStatus, Status::Invalid);
    EXPECT_FALSE(definition.profile.has_value());
    EXPECT_TRUE(definition.profileDiagnostic.find("closed wire") != std::string::npos);
}

TEST_F(ThreadTest, ThreadEndTrimsFollowMetricProfileSupportIntersection)
{
    auto* profile = createThreadProfile(
        "TrimProfile",
        {
            Base::Vector3d(-0.75, 0.001, 0.0),
            Base::Vector3d(0.0, 0.4375, 0.0),
            Base::Vector3d(0.0, 0.5625, 0.0),
            Base::Vector3d(-0.75, 0.999, 0.0),
        }
    );

    constexpr double pitch = 2.0;
    constexpr double expectedPhase = 1.0 / 8.0;
    constexpr double supportX = -0.75 + 0.75 * (expectedPhase - 0.001) / (0.4375 - 0.001);
    const PartDesign::ThreadUtils::NormalizedThreadSupport support {supportX, 0.0};
    const auto trims = PartDesign::ThreadUtils::calculateThreadEndTrims(
        getProfileWire(profile),
        pitch,
        support,
        support
    );
    EXPECT_NEAR(trims.nearEnd, pitch / 8.0, 1e-9);
    EXPECT_NEAR(trims.farEnd, pitch / 8.0, 1e-9);
}

TEST_F(ThreadTest, ThreadEndTrimsFollowRoundedWhitworthRootIntersection)
{
    constexpr double pitch = 25.4 / 11.0;
    constexpr double radialOverlap = 0.005;
    constexpr double finishedRoot = -2.0 * 0.960491 / 3.0;
    const PartDesign::ThreadUtils::NormalizedThreadSupport support {
        finishedRoot + radialOverlap / pitch,
        0.0
    };
    const TopoDS_Wire profile = makeRoundedWhitworthRootProfile();
    const auto trims
        = PartDesign::ThreadUtils::calculateThreadEndTrims(profile, pitch, support, support);

    BRepTools_WireExplorer explorer(profile);
    ASSERT_TRUE(explorer.More());
    const BRepAdaptor_Curve rootArc(explorer.Current());
    ASSERT_EQ(rootArc.GetType(), GeomAbs_Circle);

    const gp_Circ rootCircle = rootArc.Circle();
    const double radialDistance = support.xAtYZero - rootCircle.Location().X();
    const double intersectionTerm = rootCircle.Radius() * rootCircle.Radius()
        - radialDistance * radialDistance;
    ASSERT_GT(intersectionTerm, 0.0);

    const double expected = (rootCircle.Location().Y() + std::sqrt(intersectionTerm)) * pitch;
    EXPECT_NEAR(trims.nearEnd, expected, 1e-6);
    EXPECT_NEAR(trims.farEnd, expected, 1e-6);
    EXPECT_NEAR(expected, 0.0558318, 1e-6);
}

TEST_F(ThreadTest, ThreadEndTrimsSupportTaperedIntersectionLines)
{
    auto* profile = createThreadProfile(
        "TaperedTrimProfile",
        {
            Base::Vector3d(-0.75, 0.001, 0.0),
            Base::Vector3d(0.0, 0.4375, 0.0),
            Base::Vector3d(0.0, 0.5625, 0.0),
            Base::Vector3d(-0.75, 0.999, 0.0),
        }
    );

    constexpr double pitch = 2.0;
    constexpr double slope = 0.05;
    constexpr double nearPhase = 0.1;
    constexpr double farPhase = 0.85;
    const auto lowerProfileX = [](double phase) {
        return -0.75 + 0.75 * (phase - 0.001) / (0.4375 - 0.001);
    };
    const auto upperProfileX = [](double phase) {
        return -0.75 + 0.75 * (0.999 - phase) / (0.999 - 0.5625);
    };
    const auto trims = PartDesign::ThreadUtils::calculateThreadEndTrims(
        getProfileWire(profile),
        pitch,
        {lowerProfileX(nearPhase) - slope * nearPhase, slope},
        {upperProfileX(farPhase) - slope * farPhase, slope}
    );
    EXPECT_NEAR(trims.nearEnd, nearPhase * pitch, 1e-9);
    EXPECT_NEAR(trims.farEnd, (1.0 - farPhase) * pitch, 1e-9);
}

TEST_F(ThreadTest, ThreadExtentDimensionAndThroughAllRespectStartPlane)
{
    auto* thread = createExtentThread();
    ASSERT_NE(thread, nullptr);
    auto* start = createExtentPoint("Start", gp_Pnt(2.0, 3.0, 9.0));
    thread->StartPlane.setValue(start, {"Vertex1"});
    thread->DepthType.setValue("Dimension");
    thread->Depth.setValue(8.0);

    const auto dimension = thread->resolveThreadExtent();
    EXPECT_NEAR(dimension.origin.Distance(gp_Pnt(2.0, 3.0, 9.0)), 0.0, 1e-7);
    EXPECT_NEAR(dimension.direction.Dot(gp_Dir(0.0, 0.0, 1.0)), 1.0, 1e-7);
    EXPECT_NEAR(dimension.length, 8.0, 1e-7);

    thread->DepthType.setValue("ThroughAll");
    const auto throughAll = thread->resolveThreadExtent();
    EXPECT_NEAR(throughAll.origin.Distance(dimension.origin), 0.0, 1e-7);
    EXPECT_NEAR(throughAll.length, 30.0, 1e-7);
}

TEST_F(ThreadTest, ThreadExtentUpToGeometryTracksReferencesOnRotatedAxis)
{
    const gp_Dir direction(1.0, 1.0, 0.0);
    auto* thread = createExtentThread(direction);
    ASSERT_NE(thread, nullptr);
    const gp_Pnt origin(2.0, 3.0, 4.0);
    const gp_Pnt startPoint = origin.Translated(gp_Vec(direction) * 5.0);
    auto* start = createExtentPoint("Start", startPoint);
    auto* end = createExtentPoint("End", origin.Translated(gp_Vec(direction) * 17.0));
    thread->StartPlane.setValue(start, {"Vertex1"});
    thread->UpToGeometry.setValue(end, {"Vertex1"});
    thread->DepthType.setValue("UpToGeometry");

    const auto extent = thread->resolveThreadExtent();
    EXPECT_NEAR(extent.origin.Distance(startPoint), 0.0, 1e-7);
    EXPECT_NEAR(extent.direction.Dot(direction), 1.0, 1e-7);
    EXPECT_NEAR(extent.length, 12.0, 1e-7);

    end->Shape.setValue(BRepBuilderAPI_MakeVertex(origin.Translated(gp_Vec(direction) * 20.0)).Shape());
    EXPECT_NEAR(thread->resolveThreadExtent().length, 15.0, 1e-7);
    thread->UpToGeometry.setValue(nullptr);
    EXPECT_NEAR(thread->resolveThreadExtent().length, 25.0, 1e-7);
}

TEST_F(ThreadTest, ThreadExtentUpToFirstUsesBaseAndOffsetStart)
{
    auto* thread = createExtentThread();
    ASSERT_NE(thread, nullptr);
    auto* start = createExtentPoint("Start", gp_Pnt(2.0, 3.0, 9.0));
    thread->StartPlane.setValue(start, {"Vertex1"});
    thread->DepthType.setValue("UpToFirst");
    const auto extent = thread->resolveThreadExtent();
    EXPECT_NEAR(extent.origin.Distance(gp_Pnt(2.0, 3.0, 9.0)), 0.0, 1e-7);
    EXPECT_NEAR(extent.length, 25.0, 1e-7);
}

TEST_F(ThreadTest, ThreadExtentInvalidDepthRecoversAfterCorrection)
{
    auto* thread = createExtentThread();
    ASSERT_NE(thread, nullptr);
    thread->DepthType.setValue("Dimension");
    thread->Depth.setValue(0.0);
    EXPECT_THROW(thread->resolveThreadExtent(), Base::ValueError);
    getDocument()->recompute();
    EXPECT_TRUE(thread->isError());

    thread->Depth.setValue(10.0);
    getDocument()->recompute();
    EXPECT_FALSE(thread->isError()) << thread->getStatusString();
    EXPECT_NEAR(thread->resolveThreadExtent().length, 10.0, 1e-7);
    EXPECT_FALSE(thread->Shape.getShape().isNull());
}

TEST_F(ThreadTest, ThreadCreationOnCube)
{
    auto doc = getDocument();
    auto body = getBody();
    auto pad = createCubePad(30.0);
    ASSERT_NE(pad, nullptr);
    auto thread = doc->addObject<PartDesign::Thread>("Thread");
    body->addObject(thread);
    thread->LateralFace.setValue(pad, {"Face3"});

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

TEST_F(ThreadTest, InternalThreadModeledM30x3_5)
{
    constexpr double length = 24.0;
    constexpr double minorDiameter = 26.211;

    auto doc = getDocument();
    auto body = getBody();
    auto pad = createAnnularPad(length, 20.0, minorDiameter / 2.0);
    ASSERT_NE(pad, nullptr);
    ASSERT_FALSE(pad->Shape.getValue().IsNull());

    GProp_GProps padVolume;
    BRepGProp::VolumeProperties(pad->Shape.getValue(), padVolume);

    auto thread = doc->addObject<PartDesign::Thread>("Thread");
    body->addObject(thread);

    auto lateralFace = getLateralFaceName(pad, minorDiameter / 2.0);
    ASSERT_TRUE(lateralFace.has_value());
    thread->LateralFace.setValue(pad, {*lateralFace});

    thread->DepthType.setValue(0L);
    thread->Depth.setValue(length);

    const int typeIdx = findEnumIndex(thread->ThreadType.getEnumVector(), "ISOMetricProfile");
    ASSERT_GE(typeIdx, 0) << "ISOMetricProfile was not found in enum";
    thread->ThreadType.setValue(typeIdx);

    const int sizeIdx = findEnumIndex(thread->ThreadSize.getEnumVector(), "30");
    ASSERT_GE(sizeIdx, 0) << "30mm Diameter was not found in enum";
    thread->ThreadSize.setValue(sizeIdx);

    const int pitchIdx = findEnumIndex(thread->ThreadSizePitch.getEnumVector(), "3.5");
    ASSERT_GE(pitchIdx, 0) << "3.5mm Pitch was not found in enum";
    thread->ThreadSizePitch.setValue(pitchIdx);

    thread->UseCustomThreadClearance.setValue(true);
    thread->CustomThreadClearance.setValue(0.0);
    thread->ModelThread.setValue(true);
    thread->CosmeticThread.setValue(false);

    doc->recompute();

    ASSERT_FALSE(thread->isError()) << thread->getStatusString();
    ASSERT_TRUE(thread->isValid());
    EXPECT_TRUE(thread->IsInternal.getValue());
    EXPECT_EQ(thread->getAddSubType(), PartDesign::FeatureAddSub::Subtractive);
    ASSERT_FALSE(thread->Shape.getValue().IsNull());
    EXPECT_FALSE(thread->AddSubShape.getValue().IsNull());

    GProp_GProps threadVolume;
    BRepGProp::VolumeProperties(thread->Shape.getValue(), threadVolume);
    EXPECT_LT(threadVolume.Mass(), padVolume.Mass());
}

TEST_F(ThreadTest, ExternalCosmeticThreadPreservesBase)
{
    auto doc = getDocument();
    auto body = getBody();
    auto pad = createCylinderPad(24.0, 12.0);
    ASSERT_NE(pad, nullptr);

    auto thread = doc->addObject<PartDesign::Thread>("Thread");
    body->addObject(thread);

    auto lateralFace = getLateralFaceName(pad);
    ASSERT_TRUE(lateralFace.has_value());
    thread->LateralFace.setValue(pad, {*lateralFace});
    thread->DepthType.setValue(0L);
    thread->Depth.setValue(24.0);

    int typeIdx = findEnumIndex(thread->ThreadType.getEnumVector(), "ISOMetricProfile");
    ASSERT_GE(typeIdx, 0);
    thread->ThreadType.setValue(typeIdx);

    int sizeIdx = findEnumIndex(thread->ThreadSize.getEnumVector(), "24");
    ASSERT_GE(sizeIdx, 0);
    thread->ThreadSize.setValue(sizeIdx);

    int pitchIdx = findEnumIndex(thread->ThreadSizePitch.getEnumVector(), "3");
    ASSERT_GE(pitchIdx, 0);
    thread->ThreadSizePitch.setValue(pitchIdx);

    thread->ModelThread.setValue(false);
    thread->CosmeticThread.setValue(true);
    doc->recompute();

    ASSERT_FALSE(thread->isError()) << thread->getStatusString();
    ASSERT_TRUE(thread->isValid());
    EXPECT_FALSE(thread->IsInternal.getValue());
    ASSERT_FALSE(thread->Shape.getValue().IsNull());
    EXPECT_TRUE(thread->AddSubShape.getValue().IsNull());
    EXPECT_TRUE(thread->getReducedBasePreviewShape().isNull());

    const auto baseSignature = getThreadShapeSignature(pad->Shape.getValue());
    const auto threadSignature = getThreadShapeSignature(thread->Shape.getValue());
    EXPECT_NEAR(threadSignature.centerOfMass.Distance(baseSignature.centerOfMass), 0.0, 1e-9);
    EXPECT_NEAR(threadSignature.xmin, baseSignature.xmin, 1e-9);
    EXPECT_NEAR(threadSignature.ymin, baseSignature.ymin, 1e-9);
    EXPECT_NEAR(threadSignature.zmin, baseSignature.zmin, 1e-9);
    EXPECT_NEAR(threadSignature.xmax, baseSignature.xmax, 1e-9);
    EXPECT_NEAR(threadSignature.ymax, baseSignature.ymax, 1e-9);
    EXPECT_NEAR(threadSignature.zmax, baseSignature.zmax, 1e-9);
    EXPECT_EQ(threadSignature.faces, baseSignature.faces);
    EXPECT_EQ(threadSignature.edges, baseSignature.edges);
    EXPECT_EQ(threadSignature.vertices, baseSignature.vertices);

    GProp_GProps padVolume;
    GProp_GProps threadVolume;
    BRepGProp::VolumeProperties(pad->Shape.getValue(), padVolume);
    BRepGProp::VolumeProperties(thread->Shape.getValue(), threadVolume);
    EXPECT_NEAR(threadVolume.Mass(), padVolume.Mass(), 1e-9);

    GProp_GProps padSurface;
    GProp_GProps threadSurface;
    BRepGProp::SurfaceProperties(pad->Shape.getValue(), padSurface);
    BRepGProp::SurfaceProperties(thread->Shape.getValue(), threadSurface);
    EXPECT_NEAR(threadSurface.Mass(), padSurface.Mass(), 1e-9);
}

TEST_F(ThreadTest, ExternalThreadModeledOnCone)
{
    constexpr double length = 12.0;

    auto doc = getDocument();
    auto body = getBody();
    auto base = doc->addObject<PartDesign::Feature>("Base");
    body->addObject(base);
    base->Shape.setValue(BRepPrimAPI_MakeCone(12.0, 12.375, length).Shape());
    doc->recompute();

    auto thread = doc->addObject<PartDesign::Thread>("Thread");
    body->addObject(thread);
    thread->BaseFeature.setValue(base);

    std::optional<std::string> lateralFace;
    for (TopExp_Explorer explorer(base->Shape.getValue(), TopAbs_FACE); explorer.More();
         explorer.Next()) {
        const auto face = TopoDS::Face(explorer.Current());
        if (BRepAdaptor_Surface(face).GetType() == GeomAbs_Cone) {
            const int index = base->Shape.getShape().findShape(face);
            ASSERT_GT(index, 0);
            lateralFace = "Face" + std::to_string(index);
            break;
        }
    }
    ASSERT_TRUE(lateralFace.has_value());
    thread->LateralFace.setValue(base, {*lateralFace});

    const int typeIdx = findEnumIndex(thread->ThreadType.getEnumVector(), "ISOMetricProfile");
    ASSERT_GE(typeIdx, 0);
    thread->ThreadType.setValue(typeIdx);

    const int sizeIdx = findEnumIndex(thread->ThreadSize.getEnumVector(), "24");
    ASSERT_GE(sizeIdx, 0);
    thread->ThreadSize.setValue(sizeIdx);

    const int pitchIdx = findEnumIndex(thread->ThreadSizePitch.getEnumVector(), "3");
    ASSERT_GE(pitchIdx, 0);
    thread->ThreadSizePitch.setValue(pitchIdx);

    thread->DepthType.setValue("Dimension");
    thread->Depth.setValue(length);
    thread->UseCustomThreadClearance.setValue(true);
    thread->CustomThreadClearance.setValue(0.0);
    thread->ModelThread.setValue(true);
    thread->CosmeticThread.setValue(false);
    doc->recompute();

    ASSERT_FALSE(thread->isError()) << thread->getStatusString();
    ASSERT_TRUE(thread->isValid());
    EXPECT_FALSE(thread->IsInternal.getValue());

    const auto& shape = thread->Shape.getShape();
    ASSERT_FALSE(shape.isNull());
    EXPECT_TRUE(shape.isValid());
    int solids = 0;
    for (TopExp_Explorer explorer(shape.getShape(), TopAbs_SOLID); explorer.More(); explorer.Next()) {
        ++solids;
    }
    EXPECT_EQ(solids, 1);
    ASSERT_FALSE(thread->AddSubShape.getShape().isNull());

    const auto& reducedBase = thread->getReducedBasePreviewShape();
    ASSERT_FALSE(reducedBase.isNull());
    GProp_GProps baseVolume;
    GProp_GProps reducedBaseVolume;
    GProp_GProps threadVolume;
    BRepGProp::VolumeProperties(base->Shape.getValue(), baseVolume);
    BRepGProp::VolumeProperties(reducedBase.getShape(), reducedBaseVolume);
    BRepGProp::VolumeProperties(shape.getShape(), threadVolume);
    EXPECT_GT(threadVolume.Mass(), reducedBaseVolume.Mass());
    EXPECT_LT(threadVolume.Mass(), baseVolume.Mass());
}

TEST_F(ThreadTest, ExternalThreadModeledM24Signature)
{
    auto doc = getDocument();
    auto body = getBody();
    auto pad = createCylinderPad(24.0, 12.0);
    ASSERT_NE(pad, nullptr);

    auto thread = doc->addObject<PartDesign::Thread>("Thread");
    body->addObject(thread);

    auto lateralFace = getLateralFaceName(pad);
    ASSERT_TRUE(lateralFace.has_value());
    thread->LateralFace.setValue(pad, {*lateralFace});

    thread->DepthType.setValue(0L);
    thread->Depth.setValue(24.0);

    int typeIdx = findEnumIndex(thread->ThreadType.getEnumVector(), "ISOMetricProfile");
    ASSERT_GE(typeIdx, 0) << "ISOMetricProfile was not found in enum";
    thread->ThreadType.setValue(typeIdx);

    int sizeIdx = findEnumIndex(thread->ThreadSize.getEnumVector(), "24");
    ASSERT_GE(sizeIdx, 0) << "24mm Diameter was not found in enum";
    thread->ThreadSize.setValue(sizeIdx);

    int pitchIdx = findEnumIndex(thread->ThreadSizePitch.getEnumVector(), "3");
    ASSERT_GE(pitchIdx, 0) << "3mm Pitch was not found in enum";
    thread->ThreadSizePitch.setValue(pitchIdx);

    thread->UseCustomThreadClearance.setValue(true);
    thread->CustomThreadClearance.setValue(0.0);

    thread->ModelThread.setValue(true);
    thread->CosmeticThread.setValue(false);

    doc->recompute();

    ASSERT_FALSE(thread->isError()) << thread->getStatusString();
    ASSERT_TRUE(thread->isValid());

    const TopoDS_Shape& shape = thread->Shape.getValue();
    const ThreadShapeSignature signature = getThreadShapeSignature(shape);

    EXPECT_NEAR(signature.volume, 9185.12278706965, 1e-3);
    EXPECT_NEAR(signature.surface, 3299.5060565756, 1e-3);

    EXPECT_NEAR(signature.centerOfMass.X(), -0.0280986964238836, 1e-3);
    EXPECT_NEAR(signature.centerOfMass.Y(), -0.0280862441415311, 1e-3);
    EXPECT_NEAR(signature.centerOfMass.Z(), 12.0000048858924, 1e-3);

    EXPECT_NEAR(signature.xmin, -12.0000407872795, 1e-3);
    EXPECT_NEAR(signature.xmax, 13.0767182177214, 1e-3);
    EXPECT_NEAR(signature.ymin, -13.4439233177845, 1e-3);
    EXPECT_NEAR(signature.ymax, 13.4439619163563, 1e-3);
    EXPECT_NEAR(signature.zmin, -0.374583462836048, 1e-3);
    EXPECT_NEAR(signature.zmax, 24.3745345419815, 1e-3);

    EXPECT_EQ(signature.faces, 36);
    EXPECT_EQ(signature.edges, 142);
    EXPECT_EQ(signature.vertices, 284);

    EXPECT_NEAR(thread->Diameter.getValue(), 24.0, 1e-9);
    EXPECT_FALSE(thread->IsInternal.getValue());
    EXPECT_EQ(std::string(thread->ThreadDesignation.getValue()), "M24x3");
}

TEST_F(ThreadTest, ExternalThreadModeledBSPHalfInchSignature)
{
    constexpr double majorDiameter = 20.955;
    constexpr double length = 24.0;

    auto doc = getDocument();
    auto body = getBody();
    auto pad = createCylinderPad(length, majorDiameter / 2.0);
    ASSERT_NE(pad, nullptr);

    auto thread = doc->addObject<PartDesign::Thread>("Thread");
    body->addObject(thread);

    auto lateralFace = getLateralFaceName(pad);
    ASSERT_TRUE(lateralFace.has_value());
    thread->LateralFace.setValue(pad, {*lateralFace});

    thread->DepthType.setValue(0L);
    thread->Depth.setValue(length);

    int typeIdx = findEnumIndex(thread->ThreadType.getEnumVector(), "BSP");
    ASSERT_GE(typeIdx, 0) << "BSP was not found in enum";
    thread->ThreadType.setValue(typeIdx);

    int sizeIdx = findEnumIndex(thread->ThreadSize.getEnumVector(), "1/2");
    ASSERT_GE(sizeIdx, 0) << "BSP 1/2 was not found in enum";
    thread->ThreadSize.setValue(sizeIdx);

    int pitchIdx = findEnumIndex(thread->ThreadSizePitch.getEnumVector(), "1.814");
    ASSERT_GE(pitchIdx, 0) << "BSP 1/2 pitch was not found in enum";
    thread->ThreadSizePitch.setValue(pitchIdx);

    thread->UseCustomThreadClearance.setValue(true);
    thread->CustomThreadClearance.setValue(0.0);
    thread->ModelThread.setValue(true);
    thread->CosmeticThread.setValue(false);

    doc->recompute();

    ASSERT_FALSE(thread->isError()) << thread->getStatusString();
    ASSERT_TRUE(thread->isValid());

    const TopoDS_Shape& shape = thread->Shape.getValue();
    const ThreadShapeSignature signature = getThreadShapeSignature(shape);

    EXPECT_NEAR(signature.volume, 7338.01619667068, 1e-3);
    EXPECT_NEAR(signature.surface, 3018.00537029963, 1e-3);

    EXPECT_NEAR(signature.centerOfMass.X(), -0.0132596030688335, 1e-3);
    EXPECT_NEAR(signature.centerOfMass.Y(), -0.0165485382413947, 1e-3);
    EXPECT_NEAR(signature.centerOfMass.Z(), 12.0000064330152, 1e-3);

    EXPECT_NEAR(signature.xmin, -10.5118406489529, 1e-3);
    EXPECT_NEAR(signature.ymin, -11.7766673958061, 1e-3);
    EXPECT_NEAR(signature.zmin, -0.0494698965083938, 1e-3);
    EXPECT_NEAR(signature.xmax, 10.9834152303562, 1e-3);
    EXPECT_NEAR(signature.ymax, 10.7476245833315, 1e-3);
    EXPECT_NEAR(signature.zmax, 24.0494393526971, 1e-3);

    EXPECT_EQ(signature.faces, 83);
    EXPECT_EQ(signature.edges, 334);
    EXPECT_EQ(signature.vertices, 668);

    EXPECT_NEAR(thread->Diameter.getValue(), majorDiameter, 1e-9);
    EXPECT_FALSE(thread->IsInternal.getValue());
    EXPECT_FALSE(shape.IsNull());
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
