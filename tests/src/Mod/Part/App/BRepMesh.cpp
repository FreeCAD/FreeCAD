// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include "Mod/Part/App/BRepMesh.h"

// NOLINTBEGIN
class BRepMeshTest: public ::testing::Test
{
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}

    std::vector<Part::BRepMesh::Domain> getNoDomains() const
    {
        std::vector<Part::BRepMesh::Domain> domains;
        return domains;
    }

    std::vector<Part::BRepMesh::Domain> getEmptyDomains() const
    {
        Part::BRepMesh::Domain domain;
        std::vector<Part::BRepMesh::Domain> domains;
        domains.push_back(domain);
        domains.push_back(domain);
        return domains;
    }

    std::vector<Part::BRepMesh::Domain> getConnectedDomains() const
    {
        Part::BRepMesh::Domain domain1;
        domain1.points.emplace_back(0, 0, 0);
        domain1.points.emplace_back(10, 0, 0);
        domain1.points.emplace_back(10, 10, 0);
        domain1.points.emplace_back(0, 10, 0);

        {
            Part::BRepMesh::Facet f1;
            f1.I1 = 0;
            f1.I2 = 1;
            f1.I3 = 2;
            domain1.facets.emplace_back(f1);
        }
        {
            Part::BRepMesh::Facet f2;
            f2.I1 = 0;
            f2.I2 = 2;
            f2.I3 = 3;
            domain1.facets.emplace_back(f2);
        }

        Part::BRepMesh::Domain domain2;
        domain2.points.emplace_back(0, 0, 0);
        domain2.points.emplace_back(0, 10, 0);
        domain2.points.emplace_back(0, 10, 10);
        domain2.points.emplace_back(0, 0, 10);

        {
            Part::BRepMesh::Facet f1;
            f1.I1 = 0;
            f1.I2 = 1;
            f1.I3 = 2;
            domain2.facets.emplace_back(f1);
        }
        {
            Part::BRepMesh::Facet f2;
            f2.I1 = 0;
            f2.I2 = 2;
            f2.I3 = 3;
            domain2.facets.emplace_back(f2);
        }

        std::vector<Part::BRepMesh::Domain> domains;
        domains.push_back(domain1);
        domains.push_back(domain2);
        return domains;
    }

    std::vector<Part::BRepMesh::Domain> getUnconnectedDomains() const
    {
        double eps = 1.0e-10;
        Part::BRepMesh::Domain domain1;
        domain1.points.emplace_back(eps, eps, eps);
        domain1.points.emplace_back(10, 0, 0);
        domain1.points.emplace_back(10, 10, 0);
        domain1.points.emplace_back(eps, 10, eps);

        {
            Part::BRepMesh::Facet f1;
            f1.I1 = 0;
            f1.I2 = 1;
            f1.I3 = 2;
            domain1.facets.emplace_back(f1);
        }
        {
            Part::BRepMesh::Facet f2;
            f2.I1 = 0;
            f2.I2 = 2;
            f2.I3 = 3;
            domain1.facets.emplace_back(f2);
        }

        Part::BRepMesh::Domain domain2;
        domain2.points.emplace_back(0, 0, 0);
        domain2.points.emplace_back(0, 10, 0);
        domain2.points.emplace_back(0, 10, 10);
        domain2.points.emplace_back(0, 0, 10);

        {
            Part::BRepMesh::Facet f1;
            f1.I1 = 0;
            f1.I2 = 1;
            f1.I3 = 2;
            domain2.facets.emplace_back(f1);
        }
        {
            Part::BRepMesh::Facet f2;
            f2.I1 = 0;
            f2.I2 = 2;
            f2.I3 = 3;
            domain2.facets.emplace_back(f2);
        }

        std::vector<Part::BRepMesh::Domain> domains;
        domains.push_back(domain1);
        domains.push_back(domain2);
        return domains;
    }
};

TEST_F(BRepMeshTest, testNoDomains)
{
    std::vector<Base::Vector3d> points;
    std::vector<Part::BRepMesh::Facet> faces;
    Part::BRepMesh brepMesh;
    brepMesh.getFacesFromDomains(getNoDomains(), points, faces);

    EXPECT_TRUE(points.empty());
    EXPECT_TRUE(faces.empty());
}

TEST_F(BRepMeshTest, testEmptyDomains)
{
    std::vector<Base::Vector3d> points;
    std::vector<Part::BRepMesh::Facet> faces;
    Part::BRepMesh brepMesh;
    brepMesh.getFacesFromDomains(getEmptyDomains(), points, faces);

    EXPECT_TRUE(points.empty());
    EXPECT_TRUE(faces.empty());
}

TEST_F(BRepMeshTest, testConnectedDomains)
{
    std::vector<Base::Vector3d> points;
    std::vector<Part::BRepMesh::Facet> faces;
    Part::BRepMesh brepMesh;
    brepMesh.getFacesFromDomains(getConnectedDomains(), points, faces);

    EXPECT_EQ(points.size(), 6);
    EXPECT_EQ(faces.size(), 4);
}

TEST_F(BRepMeshTest, testUnconnectedDomains)
{
    std::vector<Base::Vector3d> points;
    std::vector<Part::BRepMesh::Facet> faces;
    Part::BRepMesh brepMesh;
    brepMesh.getFacesFromDomains(getUnconnectedDomains(), points, faces);

    EXPECT_EQ(points.size(), 6);
    EXPECT_EQ(faces.size(), 4);
}
// NOLINTEND
