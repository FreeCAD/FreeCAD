// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "App/Application.h"
#include "App/Document.h"
#include "App/PropertyStandard.h"
#include "App/StringHasher.h"
#include "Base/Writer.h"
#include <src/App/InitApplication.h>

using ::testing::Eq;
using ::testing::Ne;

// NOLINTBEGIN(readability-magic-numbers)

class FakeWriter: public Base::Writer
{
    void writeFiles() override
    {}
    std::ostream& Stream() override
    {
        return std::cout;
    }
};

class DocumentTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
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

    App::Document* doc()
    {
        return _doc;
    }

private:
    std::string _docName;
    App::Document* _doc {};
};


TEST_F(DocumentTest, addStringHasherIndicatesUnwrittenWhenNew)
{
    // Arrange
    App::StringHasherRef hasher(new App::StringHasher);

    // Act
    auto addResult = doc()->addStringHasher(hasher);

    // Assert
    EXPECT_TRUE(addResult.first);
    EXPECT_THAT(addResult.second, Ne(-1));
}

TEST_F(DocumentTest, addStringHasherIndicatesAlreadyWritten)
{
    // Arrange
    App::StringHasherRef hasher(new App::StringHasher);
    doc()->addStringHasher(hasher);

    // Act
    auto addResult = doc()->addStringHasher(hasher);

    // Assert
    EXPECT_FALSE(addResult.first);
}

TEST_F(DocumentTest, getStringHasherGivesExpectedHasher)
{
    // Arrange
    App::StringHasherRef hasher(new App::StringHasher);
    auto pair = doc()->addStringHasher(hasher);
    int index = pair.second;

    // Act
    auto foundHasher = doc()->getStringHasher(index);

    // Assert
    EXPECT_EQ(hasher, foundHasher);
}

TEST_F(DocumentTest, dynamicPropertyAdditionCanBeUndoneAndRedone)
{
    doc()->openTransaction("Add document property");
    doc()->addDynamicProperty("App::PropertyInteger", "DynamicInteger");
    doc()->commitTransaction();

    ASSERT_NE(doc()->getDynamicPropertyByName("DynamicInteger"), nullptr);
    EXPECT_TRUE(doc()->undo());
    EXPECT_EQ(doc()->getDynamicPropertyByName("DynamicInteger"), nullptr);
    EXPECT_TRUE(doc()->redo());
    EXPECT_NE(doc()->getDynamicPropertyByName("DynamicInteger"), nullptr);
}

TEST_F(DocumentTest, dynamicPropertyRemovalCanBeUndoneAndRedone)
{
    doc()->addDynamicProperty("App::PropertyInteger", "DynamicInteger");
    doc()->clearUndos();

    doc()->openTransaction("Remove document property");
    doc()->removeDynamicProperty("DynamicInteger");
    doc()->commitTransaction();

    ASSERT_EQ(doc()->getDynamicPropertyByName("DynamicInteger"), nullptr);
    EXPECT_TRUE(doc()->undo());
    EXPECT_NE(doc()->getDynamicPropertyByName("DynamicInteger"), nullptr);
    EXPECT_TRUE(doc()->redo());
    EXPECT_EQ(doc()->getDynamicPropertyByName("DynamicInteger"), nullptr);
}

TEST_F(DocumentTest, dynamicPropertyValueCanBeUndoneAndRedone)
{
    auto* property = freecad_cast<App::PropertyInteger*>(
        doc()->addDynamicProperty("App::PropertyInteger", "DynamicInteger")
    );
    ASSERT_NE(property, nullptr);
    property->setValue(1);
    doc()->clearUndos();

    doc()->openTransaction("Change document property");
    property->setValue(2);
    doc()->commitTransaction();

    EXPECT_TRUE(doc()->undo());
    EXPECT_EQ(property->getValue(), 1);
    EXPECT_TRUE(doc()->redo());
    EXPECT_EQ(property->getValue(), 2);
}

TEST_F(DocumentTest, staticPropertyValueCanBeUndoneAndRedone)
{
    doc()->Label.setValue("Before");
    doc()->clearUndos();

    doc()->openTransaction("Change document label");
    doc()->Label.setValue("After");
    doc()->commitTransaction();

    EXPECT_TRUE(doc()->undo());
    EXPECT_STREQ(doc()->Label.getValue(), "Before");
    EXPECT_TRUE(doc()->redo());
    EXPECT_STREQ(doc()->Label.getValue(), "After");
}

TEST_F(DocumentTest, dynamicPropertyRenameCanBeUndoneAndRedone)
{
    auto* property = doc()->addDynamicProperty("App::PropertyInteger", "OriginalName");
    doc()->clearUndos();

    doc()->openTransaction("Rename document property");
    ASSERT_TRUE(doc()->renameDynamicProperty(property, "NewName"));
    doc()->commitTransaction();

    EXPECT_TRUE(doc()->undo());
    EXPECT_EQ(doc()->getDynamicPropertyByName("NewName"), nullptr);
    EXPECT_EQ(doc()->getDynamicPropertyByName("OriginalName"), property);
    EXPECT_TRUE(doc()->redo());
    EXPECT_EQ(doc()->getDynamicPropertyByName("OriginalName"), nullptr);
    EXPECT_EQ(doc()->getDynamicPropertyByName("NewName"), property);
}

TEST_F(DocumentTest, lockedPropertyRemovalDoesNotCreateUndoEntry)
{
    auto* property = doc()->addDynamicProperty("App::PropertyInteger", "DynamicInteger");
    property->setStatus(App::Property::LockDynamic, true);
    doc()->clearUndos();

    doc()->openTransaction("Remove locked document property");
    EXPECT_THROW(doc()->removeDynamicProperty("DynamicInteger"), Base::RuntimeError);
    doc()->commitTransaction();

    EXPECT_EQ(doc()->getDynamicPropertyByName("DynamicInteger"), property);
    EXPECT_FALSE(doc()->undo());
}

TEST_F(DocumentTest, documentAndObjectPropertyAdditionsShareTransaction)
{
    auto* object = doc()->addObject("App::FeaturePython", "Feature");
    doc()->clearUndos();

    doc()->openTransaction("Add properties");
    doc()->addDynamicProperty("App::PropertyInteger", "DocumentInteger");
    object->addDynamicProperty("App::PropertyInteger", "ObjectInteger");
    doc()->commitTransaction();

    EXPECT_TRUE(doc()->undo());
    EXPECT_EQ(doc()->getDynamicPropertyByName("DocumentInteger"), nullptr);
    EXPECT_EQ(object->getDynamicPropertyByName("ObjectInteger"), nullptr);
    EXPECT_TRUE(doc()->redo());
    EXPECT_NE(doc()->getDynamicPropertyByName("DocumentInteger"), nullptr);
    EXPECT_NE(object->getDynamicPropertyByName("ObjectInteger"), nullptr);
}

// NOLINTEND(readability-magic-numbers)
