// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Mod/CadX/App/AssemblyObjectAdapter.h>

TEST(CadXAssemblyObjectAdapter, TreatsJointGroupsAsNonPhysicalArtifacts)
{
    const auto classification = CadX::AssemblyObjectAdapter::classify("Assembly::JointGroup");

    EXPECT_EQ(classification.nodeKind, CadX::NodeKind::AssemblyArtifact);
    EXPECT_EQ(classification.role, "artifact");
    EXPECT_EQ(classification.containerKind, "joint_group");
}

TEST(CadXAssemblyObjectAdapter, KeepsLinksAsOccurrences)
{
    const auto classification = CadX::AssemblyObjectAdapter::classify("App::Link");

    EXPECT_EQ(classification.nodeKind, CadX::NodeKind::Occurrence);
    EXPECT_EQ(classification.role, "occurrence");
    EXPECT_EQ(classification.provenanceKind, "external_link");
}

TEST(CadXAssemblyObjectAdapter, PreservesUnknownTypesWithDiagnostics)
{
    const auto classification = CadX::AssemblyObjectAdapter::classify("Custom::Feature");

    EXPECT_EQ(classification.nodeKind, CadX::NodeKind::UnresolvedDefinition);
    EXPECT_FALSE(classification.diagnostic.empty());
}
