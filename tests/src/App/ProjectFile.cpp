#include "gtest/gtest.h"

#include "InitApplication.h"
#include <App/ProjectFile.h>
#include <App/InventorObject.h>
#include <Base/Type.h>

// NOLINTBEGIN
class ProjectFileTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
    void SetUp() override
    {}
    void TearDown() override
    {}
    std::string fileName() const
    {
        std::string resDir(DATADIR);
        resDir.append("/tests/ProjectTest.FCStd");
        return resDir;
    }
    std::list<std::string> getInventorObjects() const
    {
        return {"Body"};
    }
    Base::Type getInventorId() const
    {
        return App::InventorObject::getClassTypeId();
    }
};

TEST_F(ProjectFileTest, loadInvalid)
{
    App::ProjectFile proj("non-existing.FCStd");
    EXPECT_FALSE(proj.loadDocument());
}

TEST_F(ProjectFileTest, loadDocument)
{
    App::ProjectFile proj(fileName());
    EXPECT_TRUE(proj.loadDocument());
}

TEST_F(ProjectFileTest, getObjects)
{
    App::ProjectFile proj(fileName());
    EXPECT_TRUE(proj.loadDocument());
    std::list<App::ProjectFile::Object> objs = proj.getObjects();
    EXPECT_EQ(objs.size(), 1);
}

TEST_F(ProjectFileTest, getPropertyFiles)
{
    App::ProjectFile proj(fileName());
    EXPECT_TRUE(proj.loadDocument());
    std::list<App::ProjectFile::PropertyFile> files = proj.getPropertyFiles("Body");
    EXPECT_EQ(files.size(), 0);
}

TEST_F(ProjectFileTest, getMetadata)
{
    App::ProjectFile proj(fileName());
    EXPECT_TRUE(proj.loadDocument());
    auto metadata = proj.getMetadata();
    EXPECT_EQ(std::string("No comment"), metadata.comment);
    EXPECT_EQ(std::string("John Doe & Jane Roe"), metadata.company);
    EXPECT_EQ(std::string("John Doe"), metadata.createdBy);
    EXPECT_EQ(std::string("2024-03-08T10:53:31Z"), metadata.creationDate);
    EXPECT_EQ(std::string("ProjectTest"), metadata.label);
    EXPECT_EQ(std::string("John Doe"), metadata.lastModifiedBy);
    EXPECT_EQ(std::string("2024-03-08T11:03:44Z"), metadata.lastModifiedDate);
    EXPECT_EQ(std::string("All rights reserved"), metadata.license);
    EXPECT_EQ(std::string("https://en.wikipedia.org/wiki/All_rights_reserved"),
              metadata.licenseURL);
    EXPECT_EQ(std::string("0.22R36329 (Git)"), metadata.programVersion);
    EXPECT_EQ(std::string("6847155d-dcc3-4dea-92c9-c4d32d6a3055"), metadata.uuid);
}

TEST_F(ProjectFileTest, getObjectsOfType)
{
    App::ProjectFile proj(fileName());
    EXPECT_TRUE(proj.loadDocument());
    std::list<std::string> objs = proj.getObjectsOfType(getInventorId());
    EXPECT_EQ(objs, getInventorObjects());
}

TEST_F(ProjectFileTest, restoreObject)
{
    App::ProjectFile proj(fileName());
    EXPECT_TRUE(proj.loadDocument());
    App::InventorObject obj;
    EXPECT_TRUE(proj.restoreObject("Body", &obj, false));
    EXPECT_EQ(obj.Label.getStrValue(), std::string("Body"));
}

TEST_F(ProjectFileTest, getTypeId)
{
    App::ProjectFile proj(fileName());
    EXPECT_TRUE(proj.loadDocument());
    Base::Type id = proj.getTypeId("Body");
    EXPECT_EQ(id, getInventorId());
}
// NOLINTEND
