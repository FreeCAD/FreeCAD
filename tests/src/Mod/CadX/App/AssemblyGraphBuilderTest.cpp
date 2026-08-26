// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Mod/CadX/App/AssemblyGraphBuilder.h>

TEST(CadXAssemblyGraphBuilder, RejectsAChangedCaptureWithoutPublishing)
{
    CadX::AssemblyCapture capture;
    capture.documentUid = "doc:test";
    capture.activeAssemblyObjectName = "Assembly";
    capture.startGuardMatches = true;
    capture.endGuardMatches = false;

    const auto result = CadX::AssemblyGraphBuilder().build(capture);
    EXPECT_FALSE(result);
    EXPECT_EQ(result.errorCode, "CADX_CAPTURE_CHANGED");
}

TEST(CadXAssemblyGraphBuilder, NormalizesLabelsAndBuildsStableRevision)
{
    CadX::AssemblyCapture capture;
    capture.documentUid = "doc:test";
    capture.documentName = "Test";
    capture.activeAssemblyObjectName = "Assembly";

    CadX::NodeRecord root;
    root.id = "assembly";
    root.kind = CadX::NodeKind::AssemblyDefinition;
    root.native = {"doc:test", "Assembly", "Assembly::AssemblyObject"};
    root.display.label = "Main Assembly";
    capture.activeAssemblyNodeId = root.id;
    capture.nodes.push_back(root);

    const auto result = CadX::AssemblyGraphBuilder().build(capture);
    ASSERT_TRUE(result) << result.diagnostic;
    EXPECT_EQ(result.snapshot->nodes().front().display.normalizedLabel, "main assembly");
    EXPECT_TRUE(result.snapshot->header().graphRevision.starts_with("sha256:"));
}
