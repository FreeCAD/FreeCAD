
#include <FCConfig.h>

#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Mod/Sketcher/App/Constraint.h>

#include <gtest/gtest.h>
#include <xercesc/util/PlatformUtils.hpp>
#include <QTemporaryFile>

#include <src/App/InitApplication.h>


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

class ConstraintPointsAccess: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

TEST_F(ConstraintPointsAccess, testDefaultGeoElementIdsAreSane)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act - no action needed, we are testing the default state

    // Assert
#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
    // Old way of accessing elements
    EXPECT_EQ(constraint.First, Sketcher::GeoEnum::GeoUndef);
    EXPECT_EQ(constraint.FirstPos, Sketcher::PointPos::none);

    EXPECT_EQ(constraint.Second, Sketcher::GeoEnum::GeoUndef);
    EXPECT_EQ(constraint.SecondPos, Sketcher::PointPos::none);

    EXPECT_EQ(constraint.Third, Sketcher::GeoEnum::GeoUndef);
    EXPECT_EQ(constraint.ThirdPos, Sketcher::PointPos::none);

    // New way of accessing elements
#endif
    EXPECT_EQ(constraint.getElement(0),
              Sketcher::GeoElementId(Sketcher::GeoEnum::GeoUndef, Sketcher::PointPos::none));
    EXPECT_EQ(constraint.getElement(1),
              Sketcher::GeoElementId(Sketcher::GeoEnum::GeoUndef, Sketcher::PointPos::none));
    EXPECT_EQ(constraint.getElement(2),
              Sketcher::GeoElementId(Sketcher::GeoEnum::GeoUndef, Sketcher::PointPos::none));
}

#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
TEST_F(ConstraintPointsAccess, testOldWriteIsReadByNew)  // NOLINT
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

TEST_F(ConstraintPointsAccess, testNewWriteIsReadByOld)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act
    constraint.setElement(0, Sketcher::GeoElementId(23, Sketcher::PointPos::start));
    constraint.setElement(1, Sketcher::GeoElementId(34, Sketcher::PointPos::end));
    constraint.setElement(2, Sketcher::GeoElementId(45, Sketcher::PointPos::mid));

    // Assert
    EXPECT_EQ(constraint.First, 23);
    EXPECT_EQ(constraint.FirstPos, Sketcher::PointPos::start);
    EXPECT_EQ(constraint.Second, 34);
    EXPECT_EQ(constraint.SecondPos, Sketcher::PointPos::end);
    EXPECT_EQ(constraint.Third, 45);
    EXPECT_EQ(constraint.ThirdPos, Sketcher::PointPos::mid);
}
#endif

TEST_F(ConstraintPointsAccess, testThreeElementsByDefault)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act - no action needed, we are testing the default state

    // Assert
    EXPECT_EQ(constraint.getElementsSize(), 3);
}

TEST_F(ConstraintPointsAccess, testFourElementsWhenAddingOne)  // NOLINT
{
    // Arrange
    auto constraint = Sketcher::Constraint();

    // Act
    constraint.addElement(Sketcher::GeoElementId(1, Sketcher::PointPos::start));

    // Assert
    EXPECT_EQ(constraint.getElementsSize(), 4);
}

#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
TEST_F(ConstraintPointsAccess, testElementSerializationWhenAccessingOldWay)  // NOLINT
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
#endif

TEST_F(ConstraintPointsAccess, testElementSerializationWhenAccessingNewWay)  // NOLINT
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

#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
TEST_F(ConstraintPointsAccess, testElementSerializationWhenMixingOldAndNew)  // NOLINT
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
#endif

TEST_F(ConstraintPointsAccess, testElementsRestoredFromSerialization)  // NOLINT
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

TEST_F(ConstraintPointsAccess,
       testElementsRestoredFromSerializationWithoutNewElementStorage)  // NOLINT
{
    // Arrange

    // Manually craft a serialized version, only parts in "{}" are important.
    // New way of storing elements is not present, like if it is an older file.
    std::string serializedConstraint = fmt::format("<Constrain "
                                                   R"(Name="" )"
                                                   R"(Type="0" )"
                                                   R"(Value="0" )"
                                                   R"(LabelDistance="10" )"
                                                   R"(LabelPosition="0" )"
                                                   R"(IsDriving="1" )"
                                                   R"(IsInVirtualSpace="0" )"
                                                   R"(IsActive="1" )"

                                                   R"(First="{}" )"
                                                   R"(Second="{}" )"
                                                   R"(Third="{}" )"
                                                   R"(FirstPos="{}" )"
                                                   R"(SecondPos="{}" )"
                                                   R"(ThirdPos="{}" )"

                                                   "/>",

                                                   67,
                                                   78,
                                                   89,
                                                   static_cast<int>(Sketcher::PointPos::mid),
                                                   static_cast<int>(Sketcher::PointPos::start),
                                                   static_cast<int>(Sketcher::PointPos::end));

    Base::StringWriter writer;
    auto& stream {writer.Stream()};
    stream << "<root>\n";  // Wrap in a root element to make constraint.
    stream << serializedConstraint;
    stream << "</root>";

    // Write to temporary file
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    ASSERT_TRUE(tempFile.open());
    tempFile.write(writer.getString().c_str(), writer.getString().size());
    tempFile.flush();

    // Open with std::ifstream and parse
    std::string filename = tempFile.fileName().toStdString();
    std::ifstream inputFile(filename);
    ASSERT_TRUE(inputFile.is_open());

    Base::XMLReader reader(tempFile.fileName().toStdString().c_str(), inputFile);
    Sketcher::Constraint restoredConstraint;
    restoredConstraint.Restore(reader);

    // Assert
    EXPECT_EQ(restoredConstraint.getElement(0),
              Sketcher::GeoElementId(67, Sketcher::PointPos::mid));
    EXPECT_EQ(restoredConstraint.getElement(1),
              Sketcher::GeoElementId(78, Sketcher::PointPos::start));
    EXPECT_EQ(restoredConstraint.getElement(2),
              Sketcher::GeoElementId(89, Sketcher::PointPos::end));

    inputFile.close();
}

TEST_F(ConstraintPointsAccess,
       testLegacyIsPreferedDuringSerializationWithoutLegacyElementStorage)  // NOLINT
{
    // Arrange

    // Manually craft a serialized version, only parts in "{}" are important.
    // Only new way of storing elements is present.
    std::string serializedConstraint = fmt::format("<Constrain "
                                                   R"(Name="" )"
                                                   R"(Type="0" )"
                                                   R"(Value="0" )"
                                                   R"(LabelDistance="10" )"
                                                   R"(LabelPosition="0" )"
                                                   R"(IsDriving="1" )"
                                                   R"(IsInVirtualSpace="0" )"
                                                   R"(IsActive="1" )"

                                                   // New way
                                                   R"(ElementIds="{} {} {}" )"
                                                   R"(ElementPositions="{} {} {}" )"

                                                   "/>",
                                                   // New way data
                                                   23,
                                                   34,
                                                   45,
                                                   static_cast<int>(Sketcher::PointPos::start),
                                                   static_cast<int>(Sketcher::PointPos::end),
                                                   static_cast<int>(Sketcher::PointPos::mid));

    Base::StringWriter writer;
    auto& stream {writer.Stream()};
    stream << "<root>\n";  // Wrap in a root element to make constraint.
    stream << serializedConstraint;
    stream << "</root>";

    // Write to temporary file
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    ASSERT_TRUE(tempFile.open());
    tempFile.write(writer.getString().c_str(), writer.getString().size());
    tempFile.flush();

    // Open with std::ifstream and parse
    std::string filename = tempFile.fileName().toStdString();
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

TEST_F(ConstraintPointsAccess, testLegacyIsPreferedDuringSerializationIfContradicting)  // NOLINT
{
    // Arrange

    // Manually craft a serialized version, only parts in "{}" are important.
    // It is not important if legacy is included before or after, legacy should always be preferred.
    std::string serializedConstraint =
        fmt::format("<Constrain "
                    R"(Name="" )"
                    R"(Type="0" )"
                    R"(Value="0" )"
                    R"(LabelDistance="10" )"
                    R"(LabelPosition="0" )"
                    R"(IsDriving="1" )"
                    R"(IsInVirtualSpace="0" )"
                    R"(IsActive="1" )"

                    // New way
                    R"(ElementIds="{} {} {}" )"
                    R"(ElementPositions="{} {} {}" )"

                    // Legacy
                    R"(First="{}" )"
                    R"(Second="{}" )"
                    R"(Third="{}" )"
                    R"(FirstPos="{}" )"
                    R"(SecondPos="{}" )"
                    R"(ThirdPos="{}" )"

                    "/>",
                    // New way data
                    23,
                    34,
                    45,
                    static_cast<int>(Sketcher::PointPos::start),
                    static_cast<int>(Sketcher::PointPos::end),
                    static_cast<int>(Sketcher::PointPos::mid),

                    // Contradicting legacy data, this should be preferred if available
                    67,
                    78,
                    89,
                    static_cast<int>(Sketcher::PointPos::mid),
                    static_cast<int>(Sketcher::PointPos::start),
                    static_cast<int>(Sketcher::PointPos::end));

    Base::StringWriter writer;
    auto& stream {writer.Stream()};
    stream << "<root>\n";  // Wrap in a root element to make constraint.
    stream << serializedConstraint;
    stream << "</root>";

    // Write to temporary file
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    ASSERT_TRUE(tempFile.open());
    tempFile.write(writer.getString().c_str(), writer.getString().size());
    tempFile.flush();

    // Open with std::ifstream and parse
    std::string filename = tempFile.fileName().toStdString();
    std::ifstream inputFile(filename);
    ASSERT_TRUE(inputFile.is_open());

    Base::XMLReader reader(tempFile.fileName().toStdString().c_str(), inputFile);
    Sketcher::Constraint restoredConstraint;
    restoredConstraint.Restore(reader);

    // Assert
    EXPECT_EQ(restoredConstraint.getElement(0),
              Sketcher::GeoElementId(67, Sketcher::PointPos::mid));
    EXPECT_EQ(restoredConstraint.getElement(1),
              Sketcher::GeoElementId(78, Sketcher::PointPos::start));
    EXPECT_EQ(restoredConstraint.getElement(2),
              Sketcher::GeoElementId(89, Sketcher::PointPos::end));

    inputFile.close();
}

TEST_F(ConstraintPointsAccess, testSubstituteIndex)  // NOLINT
{
    // Arrange
    Sketcher::Constraint constraint;
    constraint.setElement(0, Sketcher::GeoElementId(10, Sketcher::PointPos::start));
    constraint.setElement(1, Sketcher::GeoElementId(20, Sketcher::PointPos::end));
    constraint.setElement(2,
                          Sketcher::GeoElementId(10, Sketcher::PointPos::mid));  // same GeoId as 0

    // Act
    constraint.substituteIndex(10, 99);

    // Assert
    EXPECT_EQ(constraint.getElement(0), Sketcher::GeoElementId(99, Sketcher::PointPos::start));
    EXPECT_EQ(constraint.getElement(1), Sketcher::GeoElementId(20, Sketcher::PointPos::end));
    EXPECT_EQ(constraint.getElement(2), Sketcher::GeoElementId(99, Sketcher::PointPos::mid));
}

TEST_F(ConstraintPointsAccess, testSubstituteIndexAndPos)  // NOLINT
{
    // Arrange
    Sketcher::Constraint constraint;
    constraint.setElement(0, Sketcher::GeoElementId(10, Sketcher::PointPos::start));
    constraint.setElement(1, Sketcher::GeoElementId(20, Sketcher::PointPos::start));
    constraint.setElement(2, Sketcher::GeoElementId(10, Sketcher::PointPos::mid));

    // Act
    constraint.substituteIndexAndPos(10, Sketcher::PointPos::start, 42, Sketcher::PointPos::end);

    // Assert
    EXPECT_EQ(constraint.getElement(0), Sketcher::GeoElementId(42, Sketcher::PointPos::end));
    EXPECT_EQ(constraint.getElement(1), Sketcher::GeoElementId(20, Sketcher::PointPos::start));
    EXPECT_EQ(constraint.getElement(2),
              Sketcher::GeoElementId(10, Sketcher::PointPos::mid));  // unchanged
}

TEST_F(ConstraintPointsAccess, testInvolvesGeoId)  // NOLINT
{
    // Arrange
    Sketcher::Constraint constraint;
    constraint.setElement(0, Sketcher::GeoElementId(10, Sketcher::PointPos::start));
    constraint.setElement(1, Sketcher::GeoElementId(20, Sketcher::PointPos::end));

    // Act & Assert
    EXPECT_TRUE(constraint.involvesGeoId(10));
    EXPECT_TRUE(constraint.involvesGeoId(20));
    EXPECT_FALSE(constraint.involvesGeoId(99));
}

TEST_F(ConstraintPointsAccess, testInvolvesGeoIdAndPosId)  // NOLINT
{
    // Arrange
    Sketcher::Constraint constraint;
    constraint.setElement(0, Sketcher::GeoElementId(10, Sketcher::PointPos::start));
    constraint.setElement(1, Sketcher::GeoElementId(20, Sketcher::PointPos::mid));
    constraint.setElement(2, Sketcher::GeoElementId(30, Sketcher::PointPos::end));

    // Act & Assert
    EXPECT_TRUE(constraint.involvesGeoIdAndPosId(10, Sketcher::PointPos::start));
    EXPECT_TRUE(constraint.involvesGeoIdAndPosId(20, Sketcher::PointPos::mid));
    EXPECT_FALSE(constraint.involvesGeoIdAndPosId(20, Sketcher::PointPos::start));
    EXPECT_FALSE(constraint.involvesGeoIdAndPosId(99, Sketcher::PointPos::end));
}

#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
TEST_F(ConstraintPointsAccess, testLegacyWriteReflectedInInvolvesAndSubstitute)  // NOLINT
{
    // Arrange
    Sketcher::Constraint constraint;
    constraint.First = 10;
    constraint.FirstPos = Sketcher::PointPos::start;
    constraint.Second = 20;
    constraint.SecondPos = Sketcher::PointPos::end;

    // Act & Assert
    EXPECT_TRUE(constraint.involvesGeoId(10));
    EXPECT_TRUE(constraint.involvesGeoIdAndPosId(20, Sketcher::PointPos::end));

    // Substitute the legacy-indexed element
    constraint.substituteIndex(10, 99);

    // Should now reflect the substituted value
    EXPECT_TRUE(constraint.involvesGeoId(99));
    EXPECT_FALSE(constraint.involvesGeoId(10));
}

TEST_F(ConstraintPointsAccess, testSubstituteUpdatesLegacyFieldsToo)  // NOLINT
{
    // Arrange
    Sketcher::Constraint constraint;
    constraint.setElement(0, Sketcher::GeoElementId(10, Sketcher::PointPos::start));

    // Act
    constraint.substituteIndex(10, 42);

    // Assert
    EXPECT_EQ(constraint.getElement(0), Sketcher::GeoElementId(42, Sketcher::PointPos::start));
    EXPECT_EQ(constraint.First, 42);
    EXPECT_EQ(constraint.FirstPos, Sketcher::PointPos::start);
}
#endif
