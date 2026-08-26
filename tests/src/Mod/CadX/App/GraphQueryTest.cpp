// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Mod/CadX/App/GraphQuery.h>

namespace
{
std::shared_ptr<CadX::GraphSnapshot> makeAssembly()
{
    auto snapshot = std::make_shared<CadX::GraphSnapshot>();
    snapshot->header().graphId = "assembly-graph:query";
    snapshot->header().documentUid = "doc:query";
    snapshot->header().activeAssemblyNodeId = "assembly";
    snapshot->header().activeAssemblyObjectName = "Assembly";

    CadX::NodeRecord assembly;
    assembly.id = "assembly";
    assembly.kind = CadX::NodeKind::AssemblyDefinition;
    assembly.native = {"doc:query", "Assembly", "Assembly::AssemblyObject"};
    assembly.display = {"Assembly", "assembly"};
    snapshot->nodes().push_back(assembly);

    CadX::NodeRecord part;
    part.id = "part";
    part.kind = CadX::NodeKind::PartDefinition;
    part.native = {"doc:query", "Part", "App::Part"};
    part.display = {"Bracket", "bracket"};
    snapshot->nodes().push_back(part);

    CadX::NodeRecord occurrence;
    occurrence.id = "occurrence";
    occurrence.kind = CadX::NodeKind::Occurrence;
    occurrence.native = part.native;
    occurrence.display = part.display;
    occurrence.presentation.visible = true;
    snapshot->nodes().push_back(occurrence);

    snapshot->edges().push_back({"contains", CadX::EdgeKind::Contains, "assembly", "occurrence", {}, ""});
    snapshot->edges().push_back({"instance", CadX::EdgeKind::InstanceOf, "occurrence", "part", {}, ""});
    std::string diagnostic;
    EXPECT_TRUE(snapshot->finalize(diagnostic)) << diagnostic;
    return snapshot;
}
}

TEST(CadXGraphQuery, FindsVisibleOccurrencesDeterministically)
{
    auto snapshot = makeAssembly();
    CadX::GraphQueryEngine engine;
    CadX::QueryRequest request;
    request.graphId = snapshot->header().graphId;
    request.graphRevision = snapshot->header().graphRevision;
    request.operation = CadX::QueryOperation::FindNodes;
    request.nodeKinds = {CadX::NodeKind::Occurrence};
    request.filterVisible = true;
    request.visible = true;

    const auto result = engine.execute(snapshot, request);
    ASSERT_TRUE(result.ok) << result.diagnostic;
    ASSERT_EQ(result.nodeIds.size(), 1U);
    EXPECT_EQ(result.nodeIds.front(), "occurrence");
}

TEST(CadXGraphQuery, TraversesOnlyAllowlistedEdges)
{
    auto snapshot = makeAssembly();
    CadX::GraphQueryEngine engine;
    CadX::QueryRequest request;
    request.graphId = snapshot->header().graphId;
    request.graphRevision = snapshot->header().graphRevision;
    request.operation = CadX::QueryOperation::Subgraph;
    request.startNodeIds = {"occurrence"};
    request.maxDepth = 1;
    request.edgeKinds = {CadX::EdgeKind::InstanceOf};

    const auto result = engine.execute(snapshot, request);
    ASSERT_TRUE(result.ok) << result.diagnostic;
    EXPECT_EQ(result.nodeIds, std::vector<CadX::NodeId>({"part", "occurrence"}));
    EXPECT_EQ(result.edgeIds, std::vector<CadX::EdgeId>({"instance"}));
}
