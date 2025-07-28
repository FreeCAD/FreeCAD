#include <gtest/gtest.h>
#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Solid.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <QObject>

#include <src/App/InitApplication.h>


QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wextra-semi")
QT_WARNING_DISABLE_CLANG("-Woverloaded-virtual")
#include <SMESH_Version.h>
#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <StdMeshers_LocalLength.hxx>
#include <StdMeshers_Regular_1D.hxx>
#include <StdMeshers_MEFISTO_2D.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
QT_WARNING_POP

class SMesh: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};
// NOLINTBEGIN
TEST_F(SMesh, testMefisto)
{
    TopoDS_Solid box = BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Solid();

    SMESH_Gen* gen = new SMESH_Gen();
#if SMESH_VERSION_MAJOR >= 9
    SMESH_Mesh* mesh = gen->CreateMesh(true);

    StdMeshers_LocalLength* hyp1d = new StdMeshers_LocalLength(0, gen);
    hyp1d->SetLength(1.0);
    StdMeshers_Regular_1D* algo1d = new StdMeshers_Regular_1D(1, gen);

    StdMeshers_QuadranglePreference* hyp2d = new StdMeshers_QuadranglePreference(2, gen);
    StdMeshers_MEFISTO_2D* algo2d = new StdMeshers_MEFISTO_2D(3, gen);
#else
    SMESH_Mesh* mesh = gen->CreateMesh(0, true);

    StdMeshers_LocalLength* hyp1d = new StdMeshers_LocalLength(0, 0, gen);
    hyp1d->SetLength(1.0);
    StdMeshers_Regular_1D* algo1d = new StdMeshers_Regular_1D(1, 0, gen);

    StdMeshers_QuadranglePreference* hyp2d = new StdMeshers_QuadranglePreference(2, 0, gen);
    StdMeshers_MEFISTO_2D* algo2d = new StdMeshers_MEFISTO_2D(3, 0, gen);
#endif

    mesh->ShapeToMesh(box);
    mesh->AddHypothesis(box, 0);
    mesh->AddHypothesis(box, 1);
    mesh->AddHypothesis(box, 2);
    mesh->AddHypothesis(box, 3);

    bool success = gen->Compute(*mesh, box);
    EXPECT_EQ(success, true);

    EXPECT_EQ(mesh->NbNodes(), 1244);
    EXPECT_EQ(mesh->NbTriangles(), 2484);
    EXPECT_EQ(mesh->NbQuadrangles(), 0);

    delete hyp1d;
    delete algo1d;
    delete hyp2d;
    delete algo2d;
    delete mesh;
    delete gen;
}

TEST_F(SMesh, testStdMeshers)
{
    TopoDS_Solid box = BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Solid();

    TopExp_Explorer exp = TopExp_Explorer(box, TopAbs_EDGE);
    const TopoDS_Shape& edge = exp.Current();

    SMESH_Gen* gen = new SMESH_Gen();
#if SMESH_VERSION_MAJOR >= 9
    SMESH_Mesh* mesh = gen->CreateMesh(true);

    StdMeshers_LocalLength* hyp1d = new StdMeshers_LocalLength(0, gen);
    hyp1d->SetLength(0.1);
    StdMeshers_Regular_1D* algo1d = new StdMeshers_Regular_1D(1, gen);
#else
    SMESH_Mesh* mesh = gen->CreateMesh(0, true);

    StdMeshers_LocalLength* hyp1d = new StdMeshers_LocalLength(0, 0, gen);
    hyp1d->SetLength(0.1);
    StdMeshers_Regular_1D* algo1d = new StdMeshers_Regular_1D(1, 0, gen);
#endif

    mesh->ShapeToMesh(box);
    mesh->AddHypothesis(edge, 0);
    mesh->AddHypothesis(edge, 1);

    bool success = gen->Compute(*mesh, box);
    EXPECT_EQ(success, true);

    EXPECT_EQ(mesh->NbNodes(), 107);

    delete hyp1d;
    delete algo1d;
    delete mesh;
    delete gen;
}
// NOLINTEND
