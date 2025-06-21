
#include <FCConfig.h>

#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Mod/Sketcher/App/Constraint.h>

#include <gtest/gtest.h>
#include <xercesc/util/PlatformUtils.hpp>
#include <QTemporaryFile>

// Ensure Xerces is initialized before running tests which uses xml
class XercesEnvironment: public ::testing::Environment
{
public:
    void SetUp() override
    {
        try {
            xercesc::XMLPlatformUtils::Initialize();
        }
        catch (const xercesc::XMLException& e) {
            FAIL() << "Xerces init failed: " << xercesc::XMLString::transcode(e.getMessage());
        }
    }

    void TearDown() override
    {
        xercesc::XMLPlatformUtils::Terminate();
    }
};

::testing::Environment* const xercesEnv =
    ::testing::AddGlobalTestEnvironment(new XercesEnvironment);


TEST(ConstraintPointsAccess, testDefaultGeoElementIdsAreSane)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act - no action needed, we are testing the default state

    // Assert
    // Old way of accessing elements
    EXPECT_EQ(constraint.First, Sketcher::GeoEnum::GeoUndef);
    EXPECT_EQ(constraint.FirstPos, Sketcher::PointPos::none);

    EXPECT_EQ(constraint.Second, Sketcher::GeoEnum::GeoUndef);
    EXPECT_EQ(constraint.SecondPos, Sketcher::PointPos::none);

    EXPECT_EQ(constraint.Third, Sketcher::GeoEnum::GeoUndef);
    EXPECT_EQ(constraint.ThirdPos, Sketcher::PointPos::none);

    // New way of accessing elements
    EXPECT_EQ(constraint.getElement(0),
              Sketcher::GeoElementId(Sketcher::GeoEnum::GeoUndef, Sketcher::PointPos::none));
    EXPECT_EQ(constraint.getElement(1),
              Sketcher::GeoElementId(Sketcher::GeoEnum::GeoUndef, Sketcher::PointPos::none));
    EXPECT_EQ(constraint.getElement(2),
              Sketcher::GeoElementId(Sketcher::GeoEnum::GeoUndef, Sketcher::PointPos::none));
}

TEST(ConstraintPointsAccess, testOldWriteIsReadByNew)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act
    constraint.First = 23;
    constraint.FirstPos = Sketcher::PointPos::start;
    constraint.Second = 34;
    constraint.SecondPos = Sketcher::PointPos::end;
    constraint.Third = 45;
    constraint.ThirdPos = Sketcher::PointPos::mid;

    // Assert
    EXPECT_EQ(constraint.getElement(0),
              Sketcher::GeoElementId(Sketcher::GeoElementId(23, Sketcher::PointPos::start)));
    EXPECT_EQ(constraint.getElement(1),
              Sketcher::GeoElementId(Sketcher::GeoElementId(34, Sketcher::PointPos::end)));
    EXPECT_EQ(constraint.getElement(2),
              Sketcher::GeoElementId(Sketcher::GeoElementId(45, Sketcher::PointPos::mid)));
}

TEST(ConstraintPointsAccess, testNewWriteIsReadByOld)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act
    constraint.setElement(0, Sketcher::GeoElementId(23, Sketcher::PointPos::start));
    constraint.setElement(1, Sketcher::GeoElementId(34, Sketcher::PointPos::end));
    constraint.setElement(2, Sketcher::GeoElementId(45, Sketcher::PointPos::mid));
    constraint.First = 23;
    constraint.FirstPos = Sketcher::PointPos::start;
    constraint.Second = 34;
    constraint.SecondPos = Sketcher::PointPos::end;
    constraint.Third = 45;
    constraint.ThirdPos = Sketcher::PointPos::mid;

    // Assert
    EXPECT_EQ(constraint.First, 23);
    EXPECT_EQ(constraint.FirstPos, Sketcher::PointPos::start);
    EXPECT_EQ(constraint.Second, 34);
    EXPECT_EQ(constraint.SecondPos, Sketcher::PointPos::end);
    EXPECT_EQ(constraint.Third, 45);
    EXPECT_EQ(constraint.ThirdPos, Sketcher::PointPos::mid);
}

TEST(ConstraintPointsAccess, testThreeElementsByDefault)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act - no action needed, we are testing the default state

    // Assert
    EXPECT_EQ(constraint.getElementsSize(), 3);
}

TEST(ConstraintPointsAccess, testFourElementsWhenAddingOne)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act
    constraint.addElement(Sketcher::GeoElementId(1, Sketcher::PointPos::start));

    // Assert
    EXPECT_EQ(constraint.getElementsSize(), 4);
}


TEST(ConstraintPointsAccess, testElementSerializationWhenAccessingOldWay)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act
    constraint.First = 23;
    constraint.FirstPos = Sketcher::PointPos::start;
    constraint.Second = 34;
    constraint.SecondPos = Sketcher::PointPos::end;
    constraint.Third = 45;
    constraint.ThirdPos = Sketcher::PointPos::mid;

    Base::StringWriter writer = {};
    constraint.Save(writer);

    // Assert
    std::string serialized = writer.getString();
    EXPECT_TRUE(serialized.find("First=\"23\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("FirstPos=\"1\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("Second=\"34\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("SecondPos=\"2\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("Third=\"45\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("ThirdPos=\"3\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("ElementIds=\"23 34 45\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("ElementPositions=\"1 2 3\"") != std::string::npos);
}


TEST(ConstraintPointsAccess, testElementSerializationWhenAccessingNewWay)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act
    constraint.setElement(0, Sketcher::GeoElementId(23, Sketcher::PointPos::start));
    constraint.setElement(1, Sketcher::GeoElementId(34, Sketcher::PointPos::end));
    constraint.setElement(2, Sketcher::GeoElementId(45, Sketcher::PointPos::mid));

    Base::StringWriter writer = {};
    constraint.Save(writer);

    // Assert
    std::string serialized = writer.getString();
    EXPECT_TRUE(serialized.find("First=\"23\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("FirstPos=\"1\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("Second=\"34\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("SecondPos=\"2\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("Third=\"45\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("ThirdPos=\"3\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("ElementIds=\"23 34 45\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("ElementPositions=\"1 2 3\"") != std::string::npos);
}

TEST(ConstraintPointsAccess, testElementSerializationWhenMixingOldAndNew)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act
    constraint.setElement(0, Sketcher::GeoElementId(23, Sketcher::PointPos::start));
    constraint.setElement(1, Sketcher::GeoElementId(34, Sketcher::PointPos::end));
    constraint.Second = 45;  // Old way
    constraint.SecondPos = Sketcher::PointPos::mid;

    Base::StringWriter writer = {};
    constraint.Save(writer);

    // Assert
    std::string serialized = writer.getString();
    EXPECT_TRUE(serialized.find("First=\"23\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("FirstPos=\"1\"") != std::string::npos);

    // Old way wrote this data
    // ensure mid is 3 for next test
    EXPECT_EQ(Sketcher::PointPos::mid, static_cast<Sketcher::PointPos>(3));
    EXPECT_TRUE(serialized.find("SecondPos=\"3\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("Second=\"45\"") != std::string::npos);

    EXPECT_TRUE(serialized.find("Third=\"-2000\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("ThirdPos=\"0\"") != std::string::npos);

    // Second and SecondPos is reflected in the elements data too
    EXPECT_TRUE(serialized.find("ElementIds=\"23 45 -2000\"") != std::string::npos);
    EXPECT_TRUE(serialized.find("ElementPositions=\"1 3 0\"") != std::string::npos);
}

TEST(ConstraintPointsAccess, testElementsRestoredFromSerialization)  // NOLINT
{
    // Arrange
    Sketcher::Constraint constraint;
    constraint.setElement(0, Sketcher::GeoElementId(23, Sketcher::PointPos::start));
    constraint.setElement(1, Sketcher::GeoElementId(34, Sketcher::PointPos::end));
    constraint.setElement(2, Sketcher::GeoElementId(45, Sketcher::PointPos::mid));

    Base::StringWriter writer;
    writer.Stream() << "<root>\n";  // Wrap in a root element to make constraint.Save happy
    constraint.Save(writer);
    writer.Stream() << "</root>";

    // Write to temporary file
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    ASSERT_TRUE(tempFile.open());
    tempFile.write(writer.getString().c_str(), writer.getString().size());
    tempFile.flush();

    // Open with std::ifstream and parse
    std::string filename = tempFile.fileName().toStdString();
    std::cout << "Temporary file created: " << filename << std::endl;
    std::ifstream inputFile(filename);
    ASSERT_TRUE(inputFile.is_open());

    Base::XMLReader reader(tempFile.fileName().toStdString().c_str(), inputFile);
    Sketcher::Constraint restoredConstraint;
    restoredConstraint.Restore(reader);

    // Assert
    EXPECT_EQ(restoredConstraint.getElement(0),
              Sketcher::GeoElementId(23, Sketcher::PointPos::start));
    EXPECT_EQ(restoredConstraint.getElement(1),
              Sketcher::GeoElementId(34, Sketcher::PointPos::end));
    EXPECT_EQ(restoredConstraint.getElement(2),
              Sketcher::GeoElementId(45, Sketcher::PointPos::mid));

    inputFile.close();
}
