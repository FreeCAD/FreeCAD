// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <array>
#include <boost/core/ignore_unused.hpp>

#include <App/Application.h>
#include <App/ComplexGeoData.h>
#include <Base/BoundBox.h>
#include <Base/Writer.h>
#include <src/App/InitApplication.h>


class ConcreteComplexGeoDataForTesting: public Data::ComplexGeoData
{
public:
    std::vector<const char*> getElementTypes() const override
    {
        return {"EDGE"};
    }

    unsigned long countSubElements(const char* Type) const override
    {
        (void)Type;
        return 0;
    }

    Data::Segment* getSubElement(const char* Type, unsigned long number) const override
    {
        (void)number;
        (void)Type;
        return nullptr;
    }

    void setTransform(const Base::Matrix4D& rclTrf) override
    {
        (void)rclTrf;
    }

    Base::Matrix4D getTransform() const override
    {
        return {};
    }

    void transformGeometry(const Base::Matrix4D& rclMat) override
    {
        (void)rclMat;
    }

    Base::BoundBox3d getBoundBox() const override
    {
        return Base::BoundBox3d();
    }
};

class ComplexGeoDataTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    ConcreteComplexGeoDataForTesting& cgd()
    {
        return _complexGeoData;
    }

    std::tuple<Data::IndexedName, Data::MappedName> createMappedName(const std::string& name)
    {
        auto elementMap = std::make_shared<Data::ElementMap>();
        cgd().resetElementMap(elementMap);
        auto mappedName = Data::MappedName(name.c_str());
        auto indexedName = Data::IndexedName(_complexGeoData.getElementTypes().front(), 1);
        elementMap->setElementName(indexedName, mappedName, 0);
        return std::make_tuple(indexedName, mappedName);
    }

private:
    ConcreteComplexGeoDataForTesting _complexGeoData;
    std::string _docName;
};

// NOLINTBEGIN(readability-magic-numbers)

TEST_F(ComplexGeoDataTest, getIndexedNameNoName)  // NOLINT
{
    // Arrange & Act
    auto result = cgd().getIndexedName(Data::MappedName());

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ComplexGeoDataTest, getIndexedNameNoElementMap)  // NOLINT
{
    // Arrange & Act
    auto result = cgd().getIndexedName(Data::MappedName("TestName"));

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ComplexGeoDataTest, getMappedNameNoElement)  // NOLINT
{
    // Arrange & Act
    auto result = cgd().getMappedName(Data::IndexedName {});

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ComplexGeoDataTest, getMappedNameDisallowUnmappedNoMap)  // NOLINT
{
    // Arrange & Act
    auto result = cgd().getMappedName(Data::IndexedName {"TestName"}, false);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ComplexGeoDataTest, getMappedNameDisallowUnmappedWithMap)  // NOLINT
{
    // Arrange
    auto elementMap = std::make_shared<Data::ElementMap>();
    cgd().resetElementMap(elementMap);
    auto mappedName = Data::MappedName("TestMappedName");
    auto indexedName = Data::IndexedName("EDGE", 1);
    elementMap->setElementName(indexedName, mappedName, 0);

    // Act
    auto result = cgd().getMappedName(indexedName, false);

    // Assert
    EXPECT_TRUE(result);
    EXPECT_EQ(mappedName, result);
}

TEST_F(ComplexGeoDataTest, getMappedNameDisallowUnmappedMissingFromMap)  // NOLINT
{
    // Arrange
    auto mappedName = Data::MappedName("NotTheTestName");
    cgd().getIndexedName(mappedName);  // Put it in the map

    // Act
    auto result = cgd().getMappedName(Data::IndexedName {"TestName"}, false);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ComplexGeoDataTest, getMappedNameAllowUnmapped)  // NOLINT
{
    // Arrange & Act
    auto result = cgd().getMappedName(Data::IndexedName {"TestName"}, true);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ComplexGeoDataTest, getElementNameGivenIndexedName)  // NOLINT
{
    // Arrange
    const char* name {"EDGE123"};
    auto indexedName = cgd().getIndexedName(Data::MappedName(name));

    // Act
    auto result = cgd().getElementName(name);

    // Assert
    EXPECT_EQ(result.index, indexedName);
}

TEST_F(ComplexGeoDataTest, getElementNameGivenMappedName)  // NOLINT
{
    // Arrange
    const char* name {"EDGE123"};
    auto mappedName = cgd().getMappedName(Data::IndexedName(name));

    // Act
    auto result = cgd().getElementName(name);

    // Assert
    EXPECT_EQ(result.name, mappedName);
}

TEST_F(ComplexGeoDataTest, getElementMappedNamesNoMapNoUnmapped)  // NOLINT
{
    // Arrange
    // Do not create an element map
    auto indexedName = Data::IndexedName("EDGE1");

    // Act
    auto result = cgd().getElementMappedNames(indexedName, false);

    // Assert
    EXPECT_TRUE(result.empty());
}

TEST_F(ComplexGeoDataTest, getElementMappedNamesWithMapNoUnmapped)  // NOLINT
{
    // Arrange
    auto elementMap = std::make_shared<Data::ElementMap>();
    cgd().resetElementMap(elementMap);
    auto mappedName = Data::MappedName("TestMappedName");
    auto indexedName = Data::IndexedName("EDGE", 1);
    elementMap->setElementName(indexedName, mappedName, 0);

    // Act
    auto result = cgd().getElementMappedNames(indexedName, false);

    // Assert
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result[0].first, mappedName);
}

TEST_F(ComplexGeoDataTest, getElementMappedNamesNoMapWithUnmapped)  // NOLINT
{
    // Do not create an element map
    auto indexedName = Data::IndexedName("EDGE1");

    // Act
    auto result = cgd().getElementMappedNames(indexedName, true);

    // Assert
    EXPECT_FALSE(result.empty());
}


TEST_F(ComplexGeoDataTest, getElementMappedNamesWithMapWithUnmapped)  // NOLINT
{
    // Arrange
    Data::MappedName mappedName;
    Data::IndexedName indexedName;
    std::tie(indexedName, mappedName) = createMappedName("TestMappedName");
    auto unmappedName = Data::IndexedName("EDGE", 2);

    // Act
    auto result = cgd().getElementMappedNames(unmappedName, true);

    // Assert
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result[0].first, mappedName);
}

TEST_F(ComplexGeoDataTest, elementTypeValidIndexName)  // NOLINT
{
    // Arrange
    auto indexedName = Data::IndexedName("EDGE", 1);

    // Act
    char elementType = cgd().elementType(indexedName);

    // Assert
    EXPECT_EQ(elementType, 'E');
}

TEST_F(ComplexGeoDataTest, elementTypeInvalidIndexedName)  // NOLINT
{
    // Arrange
    auto indexedName = Data::IndexedName("INVALID", 1);  // Not in the element type list

    // Act
    char elementType = cgd().elementType(indexedName);

    // Assert
    EXPECT_EQ(elementType, 0);
}

TEST_F(ComplexGeoDataTest, elementTypeCharEmptyName)  // NOLINT
{
    // Arrange & Act
    char elementType = cgd().elementType(nullptr);

    // Assert
    EXPECT_EQ(elementType, 0);
}

TEST_F(ComplexGeoDataTest, elementTypeCharIndexedName)  // NOLINT
{
    // Arrange & Act
    char elementType = cgd().elementType("EDGE1");

    // Assert
    EXPECT_EQ(elementType, 'E');
}

TEST_F(ComplexGeoDataTest, elementTypeCharMappedNameNoPrefix)  // NOLINT
{
    // Arrange
    int size {0};
    Data::MappedName mappedName;
    Data::IndexedName indexedName;
    std::tie(indexedName, mappedName) = createMappedName("TestMappedName:;");

    // Act
    char elementType = cgd().elementType(mappedName.toConstString(0, size));

    // Assert
    EXPECT_EQ(elementType, 'E');
}

TEST_F(ComplexGeoDataTest, elementTypeCharMappedNameWithPrefix)  // NOLINT
{
    // Arrange
    int size {0};
    Data::MappedName mappedName;
    Data::IndexedName indexedName;
    std::string name(Data::ELEMENT_MAP_PREFIX);
    name.append("TestMappedElement:;");
    std::tie(indexedName, mappedName) = createMappedName(name);

    // Act
    char elementType = cgd().elementType(mappedName.toConstString(0, size));

    // Assert
    EXPECT_EQ(elementType, 'E');
}

TEST_F(ComplexGeoDataTest, resetElementMapNoArgument)  // NOLINT
{
    // Arrange & Act
    cgd().resetElementMap();

    // Assert
    EXPECT_EQ(cgd().getElementMapSize(), 0);
}

TEST_F(ComplexGeoDataTest, resetElementMapWithArgument)  // NOLINT
{
    // Arrange
    auto elementMap = std::make_shared<Data::ElementMap>();
    auto mappedName = Data::MappedName("TestMappedName");
    auto indexedName = Data::IndexedName("EDGE", 1);
    elementMap->setElementName(indexedName, mappedName, 0);

    // Act
    cgd().resetElementMap(elementMap);

    // Assert
    EXPECT_EQ(cgd().getElementMapSize(), 1);
}

TEST_F(ComplexGeoDataTest, setAndGetElementMap)  // NOLINT
{
    // Arrange
    auto elementMap = std::make_shared<Data::ElementMap>();
    cgd().resetElementMap(elementMap);
    std::vector<Data::MappedElement> vecMappedElements;
    auto mappedNameA = Data::MappedName("TestMappedNameA");
    auto indexedNameA = Data::IndexedName("EDGE", 1);
    vecMappedElements.emplace_back(indexedNameA, mappedNameA);
    auto mappedNameB = Data::MappedName("TestMappedNameB");
    auto indexedNameB = Data::IndexedName("EDGE", 2);
    vecMappedElements.emplace_back(indexedNameB, mappedNameB);

    // Act
    auto emptyElementMap = cgd().getElementMap();
    cgd().setElementMap(vecMappedElements);
    auto resultingElementMap = cgd().getElementMap();

    // Assert
    EXPECT_TRUE(emptyElementMap.empty());
    EXPECT_EQ(resultingElementMap.size(), vecMappedElements.size());
}

TEST_F(ComplexGeoDataTest, getElementMapSize)  // NOLINT
{
    // Arrange
    auto elementMap = std::make_shared<Data::ElementMap>();
    auto mappedName = Data::MappedName("TestMappedName");
    auto indexedName = Data::IndexedName("EDGE", 1);
    elementMap->setElementName(indexedName, mappedName, 0);
    cgd().resetElementMap(elementMap);

    // Act
    auto result = cgd().getElementMapSize();

    // Assert
    EXPECT_EQ(result, 1);
}

TEST_F(ComplexGeoDataTest, flushElementMap)  // NOLINT
{
    // Does nothing in the base class
}

TEST_F(ComplexGeoDataTest, saveWithNoElementMap)  // NOLINT
{
    // Arrange
    Base::StringWriter writer;
    cgd().resetElementMap();

    // Act
    cgd().Save(writer);

    // Assert - With no element map, that element of the XML is empty
    EXPECT_TRUE(writer.getString().find("<ElementMap/>") != std::string::npos);
}

TEST_F(ComplexGeoDataTest, saveWithElementMap)
{
    // Arrange
    Base::StringWriter writer;
    auto map = createMappedName("SomeElement");

    // Act
    cgd().Save(writer);

    // Assert
    EXPECT_TRUE(writer.getString().find("<ElementMap ") != std::string::npos);
    EXPECT_TRUE(writer.getString().find("<ElementMap2 ") != std::string::npos);
    EXPECT_TRUE(writer.getString().find(" file=") == std::string::npos);  // No persistence name set
    // TODO: assert that the elementmap was written in "raw" mode
}

TEST_F(ComplexGeoDataTest, saveWithPersistenceFilename)
{
    // Arrange
    Base::StringWriter writer;
    auto map = createMappedName("SomeElement");
    cgd().setPersistenceFileName("some_file_name.txt");

    // Act
    cgd().Save(writer);

    // Assert
    EXPECT_TRUE(writer.getString().find(" file=") != std::string::npos);
    EXPECT_TRUE(writer.getString().find("some_file_name.txt") != std::string::npos);
}

TEST_F(ComplexGeoDataTest, saveDocFileWithNoElementMap)
{
    // Arrange
    Base::StringWriter writer;
    cgd().resetElementMap(nullptr);  // Force undefined map

    // Act
    cgd().SaveDocFile(writer);

    // Assert -- no element map tag if no element map
    EXPECT_TRUE(writer.getString().find("BeginElementMap") == std::string::npos);
}

TEST_F(ComplexGeoDataTest, saveDocFileWithElementMap)
{
    // Arrange
    Base::StringWriter writer;
    auto map = createMappedName("SomeElement");

    // Act
    cgd().SaveDocFile(writer);

    // Assert -- must begin a v1 ElementMap
    EXPECT_TRUE(writer.getString().find("BeginElementMap v1") != std::string::npos);
}

TEST_F(ComplexGeoDataTest, restoreStream)
{}

TEST_F(ComplexGeoDataTest, traceElement)
{  // This test barely scratches the surface; see the ToposhapeExtension traceElement test for more
    Data::MappedName mappedName;
    Data::IndexedName indexedName;
    std::string name(Data::ELEMENT_MAP_PREFIX);
    name.append("TestMappedElement:;");
    std::tie(indexedName, mappedName) = createMappedName(name);

    // Arrange
    Data::TraceCallback cb =
        [name](const Data::MappedName& elementname, int offset, long encodedTag, long tag) {
            boost::ignore_unused(offset);
            boost::ignore_unused(encodedTag);
            boost::ignore_unused(tag);
            EXPECT_STREQ(elementname.toString().c_str(), name.substr(1).c_str());
            return false;
        };
    // Act
    cgd().traceElement(mappedName, cb);
}

// NOLINTEND(readability-magic-numbers)
