#include <gtest/gtest.h>
#include <Base/FileInfo.h>
#include <Mod/Mesh/App/Core/IO/Reader3MF.h>
#include <xercesc/util/PlatformUtils.hpp>
#include <zipios++/fcoll.h>

class ImporterTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        XERCES_CPP_NAMESPACE::XMLPlatformUtils::Initialize();
    }
};

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST_F(ImporterTest, Test3MF)
{
    std::string file(DATADIR);
    file.append("/tests/mesh.3mf");

    MeshCore::Reader3MF reader(file);
    EXPECT_EQ(reader.Load(), true);

    std::vector<int> ids = reader.GetMeshIds();
    std::sort(ids.begin(), ids.end());

    EXPECT_EQ(ids.size(), 2);

    const MeshCore::MeshKernel& mesh1 = reader.GetMesh(ids[0]);
    EXPECT_EQ(mesh1.CountPoints(), 8);
    EXPECT_EQ(mesh1.CountEdges(), 18);
    EXPECT_EQ(mesh1.CountFacets(), 12);

    const MeshCore::MeshKernel& mesh2 = reader.GetMesh(ids[1]);
    EXPECT_EQ(mesh2.CountPoints(), 652);
    EXPECT_EQ(mesh2.CountEdges(), 1950);
    EXPECT_EQ(mesh2.CountFacets(), 1300);
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
