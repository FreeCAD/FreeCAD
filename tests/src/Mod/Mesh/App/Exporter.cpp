#include "gtest/gtest.h"
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/Part.h>
#include <src/App/InitApplication.h>
#include <Mod/Mesh/App/Exporter.h>
#include <Mod/Mesh/App/FeatureMeshSolid.h>
#include <Mod/Mesh/App/Mesh.h>

class ExporterTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        document = App::GetApplication().newDocument("MeshExport");
        fileInfo.setFile(Base::FileInfo::getTempFileName() + ".stl");

        const double value = 10.0;
        cube1 = dynamic_cast<Mesh::Cube*>(document->addObject("Mesh::Cube", "Cube1"));
        cube1->Length.setValue(value);
        cube1->Width.setValue(value);
        cube1->Height.setValue(value);
        cube2 = dynamic_cast<Mesh::Cube*>(document->addObject("Mesh::Cube", "Cube2"));
        cube2->Length.setValue(value);
        cube2->Width.setValue(value);
        cube2->Height.setValue(value);

        document->recompute();
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(document->getName());
        fileInfo.deleteFile();
    }

    App::Document* getDocument() const
    {
        return document;
    }

    std::vector<App::DocumentObject*> getObjects() const
    {
        return {cube1, cube2};
    }

    App::DocumentObject* getMesh2() const
    {
        return cube2;
    }

    void setPlacementTo2ndCube(const Base::Placement& plm)
    {
        cube2->Placement.setValue(plm);
    }

    std::string getFileName() const
    {
        return fileInfo.filePath();
    }

private:
    Base::FileInfo fileInfo;
    App::Document* document {};
    Mesh::Cube* cube1 {};
    Mesh::Cube* cube2 {};
};

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST_F(ExporterTest, TestSingleMesh)
{
    Base::Placement plm;
    plm.setPosition(Base::Vector3d(10, 5, 2));
    setPlacementTo2ndCube(plm);

    // add extra scope because the file will be written when destroying the exporter
    {
        Mesh::MergeExporter exporter(getFileName(), MeshCore::MeshIO::Format::STL);
        exporter.addObject(getMesh2(), 0.1F);
    }

    Mesh::MeshObject kernel;
    EXPECT_TRUE(kernel.load(getFileName().c_str()));
    auto bbox = kernel.getBoundBox();
    EXPECT_DOUBLE_EQ(bbox.MinX, 5.0);
    EXPECT_DOUBLE_EQ(bbox.MaxX, 15.0);
    EXPECT_DOUBLE_EQ(bbox.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bbox.MaxY, 10.0);
    EXPECT_DOUBLE_EQ(bbox.MinZ, -3.0);
    EXPECT_DOUBLE_EQ(bbox.MaxZ, 7.0);
}

// Test for https://github.com/FreeCAD/FreeCAD/pull/11539
TEST_F(ExporterTest, TestMultipleMeshes)
{
    Base::Placement plm;
    plm.setPosition(Base::Vector3d(10, 5, 2));
    setPlacementTo2ndCube(plm);

    // add extra scope because the file will be written when destroying the exporter
    {
        Mesh::MergeExporter exporter(getFileName(), MeshCore::MeshIO::Format::STL);
        for (auto obj : getObjects()) {
            exporter.addObject(obj, 0.1F);
        }
    }

    Mesh::MeshObject kernel;
    EXPECT_TRUE(kernel.load(getFileName().c_str()));
    auto bbox = kernel.getBoundBox();
    EXPECT_DOUBLE_EQ(bbox.MinX, -5.0);
    EXPECT_DOUBLE_EQ(bbox.MaxX, 15.0);
    EXPECT_DOUBLE_EQ(bbox.MinY, -5.0);
    EXPECT_DOUBLE_EQ(bbox.MaxY, 10.0);
    EXPECT_DOUBLE_EQ(bbox.MinZ, -5.0);
    EXPECT_DOUBLE_EQ(bbox.MaxZ, 7.0);
}

TEST_F(ExporterTest, TestMeshesInPart)
{
    Base::Placement plm;
    plm.setPosition(Base::Vector3d(10, 5, 2));
    setPlacementTo2ndCube(plm);

    // add extra scope because the file will be written when destroying the exporter
    {
        auto part = dynamic_cast<App::Part*>(getDocument()->addObject("App::Part", "Part"));
        part->placement().setValue(plm);
        part->addObjects(getObjects());
        Mesh::MergeExporter exporter(getFileName(), MeshCore::MeshIO::Format::STL);
        exporter.addObject(part, 0.1F);
    }

    Mesh::MeshObject kernel;
    EXPECT_TRUE(kernel.load(getFileName().c_str()));
    auto bbox = kernel.getBoundBox();
    EXPECT_DOUBLE_EQ(bbox.MinX, 5.0);
    EXPECT_DOUBLE_EQ(bbox.MaxX, 25.0);
    EXPECT_DOUBLE_EQ(bbox.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bbox.MaxY, 15.0);
    EXPECT_DOUBLE_EQ(bbox.MinZ, -3.0);
    EXPECT_DOUBLE_EQ(bbox.MaxZ, 9.0);
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
