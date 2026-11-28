// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <sstream>
#include <string>
#include <vector>

#include <src/App/InitApplication.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/PropertyLinks.h>
#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/Part/App/PrimitiveFeature.h>


class TestablePlane: public Part::Plane
{
public:
    bool restoreLegacySupport(Base::XMLReader& reader, const char* typeName)
    {
        return extensionHandleChangedPropertyName(reader, typeName, "Support");
    }
};


template<typename PropertyT>
void restoreLegacySupport(TestablePlane& plane, PropertyT& property)
{
    const std::string typeName(property.getTypeId().getName());
    Base::StringWriter writer;
    property.Save(writer);

    std::stringstream data;
    data << "<?xml version='1.0' encoding='utf-8'?>\n"
         << "<Property name='Support' type='" << typeName << "'>\n"
         << writer.getString() << "</Property>\n";
    Base::XMLReader reader("Document.xml", data);

    EXPECT_TRUE(plane.restoreLegacySupport(reader, typeName.c_str()));
}


template<typename PropertyT>
void restoreEmptyLegacySupport(TestablePlane& plane)
{
    PropertyT property;
    restoreLegacySupport(plane, property);
}


class AttachExtensionTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        Base::Interpreter().loadModule("Part");
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

    App::Document* getDocument() const
    {
        return _doc;
    }

    TestablePlane* createTestablePlane(const char* name)
    {
        auto plane = new TestablePlane();
        getDocument()->addObject(plane, name);
        return plane;
    }

private:
    std::string _docName;
    App::Document* _doc = nullptr;
};

TEST_F(AttachExtensionTest, testPlanePlane)
{
    auto plane1 = getDocument()->addObject<Part::Plane>("Plane1");
    auto plane2 = getDocument()->addObject<Part::Plane>("Plane2");

    ASSERT_TRUE(plane1);
    ASSERT_TRUE(plane2);

    getDocument()->recompute();

    plane2->MapReversed.setValue(false);
    plane2->AttachmentSupport.setValue(plane1);
    plane2->MapPathParameter.setValue(0.0);
    plane2->MapMode.setValue("FlatFace");

    getDocument()->recompute();
    EXPECT_TRUE(true);
}

TEST_F(AttachExtensionTest, testMidPlane)
{
    auto firstPlane = getDocument()->addObject<Part::Plane>("FirstPlane");
    auto secondPlane = getDocument()->addObject<Part::Plane>("SecondPlane");
    auto midPlane = getDocument()->addObject<Part::Plane>("MidPlane");

    ASSERT_TRUE(firstPlane);
    ASSERT_TRUE(secondPlane);
    ASSERT_TRUE(midPlane);

    secondPlane->Placement.setValue(
        Base::Placement(
            Base::Vector3d(0, 100, 20),
            Base::Rotation(Base::Vector3d(1, 0, 0), std::numbers::pi)
        )
    );
    midPlane->AttachmentSupport.setValues({firstPlane, secondPlane}, {"", ""});
    midPlane->MapMode.setValue("MidPlane");
    EXPECT_STREQ(midPlane->MapMode.getValueAsString(), "MidPlane");

    getDocument()->recompute();

    const Base::Placement placement = midPlane->Placement.getValue();
    const Base::Vector3d position = placement.getPosition();
    EXPECT_NEAR(position.x, 50.0, 1e-7);
    EXPECT_NEAR(position.y, 50.0, 1e-7);
    EXPECT_NEAR(position.z, 10.0, 1e-7);

    Base::Vector3d normal;
    placement.getRotation().multVec(Base::Vector3d(0, 0, 1), normal);
    EXPECT_NEAR(normal.x, 0.0, 1e-7);
    EXPECT_NEAR(normal.y, 0.0, 1e-7);
    EXPECT_NEAR(normal.z, 1.0, 1e-7);
}

TEST_F(AttachExtensionTest, testMidPlaneBisectsFaceAngle)
{
    auto firstPlane = getDocument()->addObject<Part::Plane>("FirstPlane");
    auto secondPlane = getDocument()->addObject<Part::Plane>("SecondPlane");
    auto midPlane = getDocument()->addObject<Part::Plane>("MidPlane");

    ASSERT_TRUE(firstPlane);
    ASSERT_TRUE(secondPlane);
    ASSERT_TRUE(midPlane);

    const Base::Placement secondPlacement(
        Base::Vector3d(0, 0, 10),
        Base::Rotation(Base::Vector3d(1, 0, 0), std::numbers::pi / 2.0)
    );
    secondPlane->Placement.setValue(secondPlacement);
    midPlane->AttachmentSupport.setValues({firstPlane, secondPlane}, {"", ""});
    midPlane->MapMode.setValue("MidPlane");

    getDocument()->recompute();

    const Base::Vector3d position = midPlane->Placement.getValue().getPosition();
    EXPECT_NEAR(position.x, 50.0, 1e-7);
    EXPECT_NEAR(position.y, 27.5, 1e-7);
    EXPECT_NEAR(position.z, 27.5, 1e-7);

    Base::Vector3d secondNormal;
    secondPlacement.getRotation().multVec(Base::Vector3d(0, 0, 1), secondNormal);
    Base::Vector3d expectedNormal(0, 0, 1);
    expectedNormal += secondNormal;
    expectedNormal.Normalize();

    Base::Vector3d normal;
    midPlane->Placement.getValue().getRotation().multVec(Base::Vector3d(0, 0, 1), normal);
    EXPECT_NEAR(normal.x, expectedNormal.x, 1e-7);
    EXPECT_NEAR(normal.y, expectedNormal.y, 1e-7);
    EXPECT_NEAR(normal.z, expectedNormal.z, 1e-7);

    // The faces are z = 0 and y = 0, so they intersect on the X axis.
    EXPECT_NEAR(normal.Dot(Base::Vector3d(0, 0, 0) - position), 0.0, 1e-7);
    EXPECT_NEAR(normal.Dot(Base::Vector3d(10, 0, 0) - position), 0.0, 1e-7);
}

TEST_F(AttachExtensionTest, testMidPlaneBisectsConnectedFaces)
{
    auto box = getDocument()->addObject<Part::Box>("Box");
    auto midPlane = getDocument()->addObject<Part::Plane>("MidPlane");

    ASSERT_TRUE(box);
    ASSERT_TRUE(midPlane);

    box->Length.setValue(100.0);
    box->Width.setValue(60.0);
    box->Height.setValue(20.0);
    midPlane->AttachmentSupport.setValues({box, box}, {"Face6", "Face4"});
    midPlane->MapMode.setValue("MidPlane");

    getDocument()->recompute();

    const Base::Placement placement = midPlane->Placement.getValue();
    const Base::Vector3d position = placement.getPosition();
    EXPECT_NEAR(position.x, 50.0, 1e-7);
    EXPECT_NEAR(position.y, 50.0, 1e-7);
    EXPECT_NEAR(position.z, 10.0, 1e-7);

    Base::Vector3d normal;
    placement.getRotation().multVec(Base::Vector3d(0, 0, 1), normal);
    EXPECT_NEAR(normal.x, 0.0, 1e-7);
    EXPECT_NEAR(normal.y, -std::sqrt(0.5), 1e-7);
    EXPECT_NEAR(normal.z, std::sqrt(0.5), 1e-7);

    // Face6 (z = Height) and Face4 (y = Width) meet at the edge y = 60, z = 20.
    EXPECT_NEAR(normal.Dot(Base::Vector3d(0, 60, 20) - position), 0.0, 1e-7);
    EXPECT_NEAR(normal.Dot(Base::Vector3d(100, 60, 20) - position), 0.0, 1e-7);
}

TEST_F(AttachExtensionTest, testMidPlaneBisectsObliqueFaceAngle)
{
    auto firstPlane = getDocument()->addObject<Part::Plane>("FirstPlane");
    auto secondPlane = getDocument()->addObject<Part::Plane>("SecondPlane");
    auto midPlane = getDocument()->addObject<Part::Plane>("MidPlane");

    ASSERT_TRUE(firstPlane);
    ASSERT_TRUE(secondPlane);
    ASSERT_TRUE(midPlane);

    // Rotating the second plane about the Y axis through the origin keeps both planes on the
    // Y axis, so the faces intersect there at a 50 degree dihedral angle. The differing sizes
    // put the face centers off the bisector, so the base point has to be corrected onto it.
    const double angle = 50.0 * std::numbers::pi / 180.0;
    secondPlane->Length.setValue(60.0);
    secondPlane->Width.setValue(60.0);
    secondPlane->Placement.setValue(
        Base::Placement(Base::Vector3d(0, 0, 0), Base::Rotation(Base::Vector3d(0, 1, 0), angle))
    );
    midPlane->AttachmentSupport.setValues({firstPlane, secondPlane}, {"", ""});
    midPlane->MapMode.setValue("MidPlane");

    getDocument()->recompute();

    const Base::Placement placement = midPlane->Placement.getValue();
    const Base::Vector3d position = placement.getPosition();
    Base::Vector3d normal;
    placement.getRotation().multVec(Base::Vector3d(0, 0, 1), normal);

    EXPECT_NEAR(normal.x, std::sin(angle / 2.0), 1e-7);
    EXPECT_NEAR(normal.y, 0.0, 1e-7);
    EXPECT_NEAR(normal.z, std::cos(angle / 2.0), 1e-7);

    EXPECT_NEAR(normal.Dot(Base::Vector3d(0, 0, 0) - position), 0.0, 1e-7);
    EXPECT_NEAR(normal.Dot(Base::Vector3d(0, 10, 0) - position), 0.0, 1e-7);
}

TEST_F(AttachExtensionTest, testAttacherEngineType)
{
    auto plane = getDocument()->addObject<Part::Plane>("Plane");
    EXPECT_STREQ(plane->AttacherType.getValue(), "Attacher::AttachEngine3D");
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine 3D");

    plane->AttacherEngine.setValue(1L);
    EXPECT_STREQ(plane->AttacherType.getValue(), "Attacher::AttachEnginePlane");
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Plane");

    plane->AttacherEngine.setValue(2L);
    EXPECT_STREQ(plane->AttacherType.getValue(), "Attacher::AttachEngineLine");
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Line");

    plane->AttacherEngine.setValue(3L);
    EXPECT_STREQ(plane->AttacherType.getValue(), "Attacher::AttachEnginePoint");
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Point");
}

TEST_F(AttachExtensionTest, testAttacherTypeEngine)
{
    auto plane = getDocument()->addObject<Part::Plane>("Plane");
    EXPECT_STREQ(plane->AttacherType.getValue(), "Attacher::AttachEngine3D");
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine 3D");

    plane->AttacherType.setValue("Attacher::AttachEnginePlane");
    plane->onExtendedDocumentRestored();
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Plane");

    plane->AttacherType.setValue("Attacher::AttachEngineLine");
    plane->onExtendedDocumentRestored();
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Line");

    plane->AttacherType.setValue("Attacher::AttachEnginePoint");
    plane->onExtendedDocumentRestored();
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Point");

    plane->AttacherType.setValue("Attacher::AttachEngine3D");
    plane->onExtendedDocumentRestored();
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine 3D");
}

TEST_F(AttachExtensionTest, testEmptyLegacySupportDoesNotClearAttachmentSupport)
{
    auto object = createTestablePlane("Object");
    auto support = getDocument()->addObject<Part::Plane>("Support");
    ASSERT_TRUE(object);
    ASSERT_TRUE(support);

    object->AttachmentSupport.setValue(support, "Face1");
    object->MapMode.setValue("Deactivated");

    restoreEmptyLegacySupport<App::PropertyLinkSub>(*object);
    restoreEmptyLegacySupport<App::PropertyLinkSubList>(*object);

    ASSERT_EQ(object->AttachmentSupport.getValues().size(), 1);
    EXPECT_EQ(object->AttachmentSupport.getValues().front(), support);
    ASSERT_EQ(object->AttachmentSupport.getSubValues().size(), 1);
    EXPECT_EQ(object->AttachmentSupport.getSubValues().front(), "Face1");
    EXPECT_STREQ(object->MapMode.getValueAsString(), "Deactivated");
}

TEST_F(AttachExtensionTest, testNonEmptyLegacySupportReplacesAttachmentSupport)
{
    auto object = createTestablePlane("Object");
    auto currentSupport = getDocument()->addObject<Part::Plane>("CurrentSupport");
    auto legacySupport = getDocument()->addObject<Part::Plane>("LegacySupport");
    ASSERT_TRUE(object);
    ASSERT_TRUE(currentSupport);
    ASSERT_TRUE(legacySupport);

    object->AttachmentSupport.setValue(currentSupport, "Face1");
    object->MapMode.setValue("Deactivated");

    App::PropertyLinkSub oldLinkSub;
    oldLinkSub.setContainer(object);
    oldLinkSub.setValue(legacySupport, std::vector<std::string> {"Face2"});
    restoreLegacySupport(*object, oldLinkSub);

    ASSERT_EQ(object->AttachmentSupport.getValues().size(), 1);
    EXPECT_EQ(object->AttachmentSupport.getValues().front(), legacySupport);
    EXPECT_STREQ(object->MapMode.getValueAsString(), "FlatFace");

    object->AttachmentSupport.setValue(currentSupport, "Face1");
    object->MapMode.setValue("Deactivated");

    App::PropertyLinkSubList oldLinkSubList;
    oldLinkSubList.setContainer(object);
    oldLinkSubList.setValues(
        std::vector<App::DocumentObject*> {legacySupport},
        std::vector<std::string> {"Face2"}
    );
    restoreLegacySupport(*object, oldLinkSubList);

    ASSERT_EQ(object->AttachmentSupport.getValues().size(), 1);
    EXPECT_EQ(object->AttachmentSupport.getValues().front(), legacySupport);
    ASSERT_EQ(object->AttachmentSupport.getSubValues().size(), 1);
    EXPECT_EQ(object->AttachmentSupport.getSubValues().front(), "Face2");
    EXPECT_STREQ(object->MapMode.getValueAsString(), "Deactivated");
}

namespace
{
class OpenWarningObserver: public Base::ILogger
{
public:
    OpenWarningObserver()
    {
        Base::Console().attachObserver(this);
    }

    ~OpenWarningObserver() override
    {
        Base::Console().detachObserver(this);
    }

    const char* name() override
    {
        return "AttachOpenWarningObserver";
    }

    void sendLog(
        const std::string& notifier,
        const std::string& message,
        Base::LogStyle level,
        Base::IntendedRecipient,
        Base::ContentType
    ) override
    {
        if (level == Base::LogStyle::Warning
            && message.find("Placement changed on open") != std::string::npos) {
            labels.push_back(notifier.substr(notifier.find('#') + 1));
        }
    }

    std::vector<std::string> labels;
};

void runInDocument(const App::Document* doc, const std::string& code)
{
    Base::Interpreter().runString(("import FreeCAD as App\ndoc = App.getDocument('"
                                   + std::string(doc->getName()) + "')\n" + code)
                                      .c_str());
}

std::vector<std::string> openWarningsAs(const App::Document* doc, const std::string& version)
{
    OpenWarningObserver observer;
    runInDocument(doc, "version = '" + version + "'\n" + R"PY(
import os, re, shutil, tempfile, zipfile
directory = tempfile.mkdtemp()
try:
    source = os.path.join(directory, "source.FCStd")
    target = os.path.join(directory, "target.FCStd")
    doc.saveCopy(source)
    with zipfile.ZipFile(source) as zin, zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as zout:
        for item in zin.infolist():
            data = zin.read(item.filename)
            if item.filename == "Document.xml":
                data = re.sub(rb'ProgramVersion="[^"]*"', b'ProgramVersion="' + version.encode() + b'"', data, count=1)
            zout.writestr(item, data)
    App.closeDocument(App.openDocument(target).Name)
finally:
    shutil.rmtree(directory)
)PY");
    std::ranges::sort(observer.labels);
    return observer.labels;
}

const char* lineAttachedPlanes = R"PY(
part = doc.addObject("App::Part", "Part")
part.newObject("Part::LocalCoordinateSystem", "LCS")
line = doc.addObject("Part::DatumLine", "Line")
link = doc.addObject("App::Link", "LineLink")
link.LinkedObject = line
box = doc.addObject("Part::Box", "Box")
supports = {
    "OnLine": [(line, "")],
    "OnLineEdge": [(line, "Edge1")],
    "OnLcsAxis": [(part, "LCS.X_Axis")],
    "OnLink": [(link, "")],
    "OnFace": [(box, "Face6")],
    "Kept": [(line, "")],
}
for label, support in supports.items():
    plane = doc.addObject("Part::DatumPlane", label)
    plane.AttachmentSupport = support
    plane.MapMode = "FlatFace" if label == "OnFace" else "NormalToEdge"
doc.recompute()
for label in supports:
    if label != "Kept":
        doc.getObject(label).Placement = App.Placement(App.Vector(1, 2, 3), App.Rotation(App.Vector(0, 1, 0), 90))
)PY";
}  // namespace

TEST_F(AttachExtensionTest, testWarnWhenLineAttachedPlacementMovesOnOpen)
{
    runInDocument(getDocument(), lineAttachedPlanes);
    const std::vector<std::string> moved {"OnLcsAxis", "OnLine", "OnLineEdge", "OnLink"};

    EXPECT_EQ(openWarningsAs(getDocument(), "1.1R20260725 (Git shallow)"), moved);
    EXPECT_EQ(openWarningsAs(getDocument(), "26.3R48900 (Git)"), moved);
}

TEST_F(AttachExtensionTest, testNoOpenWarningOutsideAffectedVersions)
{
    runInDocument(getDocument(), lineAttachedPlanes);

    EXPECT_TRUE(openWarningsAs(getDocument(), "1.0R39109 (Git)").empty());
    EXPECT_TRUE(openWarningsAs(getDocument(), "27.1R49585 (Git)").empty());
}

TEST_F(AttachExtensionTest, testOpenWarningFarFromOrigin)
{
    runInDocument(getDocument(), R"PY(
for i in range(20):
    line = doc.addObject("Part::DatumLine", "Line")
    line.Placement = App.Placement(App.Vector(2e9 + 1234.5678 * i, 6.1e8 - 98.7 * i, 345.6), App.Rotation(App.Vector(1, i, 2), 17 * i + 3))
    plane = doc.addObject("Part::DatumPlane", "Far%d" % i)
    plane.AttachmentSupport = [(line, "")]
    plane.MapMode = "NormalToEdge"
    plane.AttachmentOffset = App.Placement(App.Vector(123.456, -78.9, 0), App.Rotation(App.Vector(0, 0, 1), 7 * i))
doc.recompute()
far = doc.Far7.Placement
doc.Far7.Placement = App.Placement(far.Base + App.Vector(0.5, 0, 0), far.Rotation)
)PY");

    EXPECT_EQ(
        openWarningsAs(getDocument(), "1.1R20260725 (Git shallow)"),
        std::vector<std::string> {"Far7"}
    );
}
