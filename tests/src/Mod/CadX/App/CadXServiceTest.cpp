// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <QJsonDocument>
#include <QJsonObject>

#include <Mod/CadX/App/CadXService.h>
#include <Mod/CadX/App/GraphJsonCodec.h>

#include <algorithm>

namespace
{
CadX::AssemblyCapture makeCapture()
{
    CadX::AssemblyCapture capture;
    capture.documentUid = "doc:service";
    capture.documentName = "ServiceTest";
    capture.activeAssemblyObjectName = "Assembly";
    capture.activeAssemblyLabel = "Assembly";
    capture.activeAssemblyNodeId = "assembly";

    CadX::NodeRecord root;
    root.id = "assembly";
    root.kind = CadX::NodeKind::AssemblyDefinition;
    root.native = {"doc:service", "Assembly", "Assembly::AssemblyObject"};
    root.display = {"Assembly", "assembly"};
    capture.nodes.push_back(root);

    CadX::NodeRecord part;
    part.id = "part";
    part.kind = CadX::NodeKind::PartDefinition;
    part.native = {"doc:service", "Part", "Part::Feature"};
    part.display = {"Bracket", "bracket"};
    capture.nodes.push_back(part);

    CadX::NodeRecord occurrence;
    occurrence.id = "occurrence";
    occurrence.kind = CadX::NodeKind::Occurrence;
    occurrence.native = part.native;
    occurrence.display = part.display;
    occurrence.presentation.visible = true;
    occurrence.payload = CadX::OccurrencePayload {{"Assembly", "Part"}, false, false, {}};
    capture.nodes.push_back(occurrence);
    auto secondOccurrence = occurrence;
    secondOccurrence.id = "occurrence-two";
    secondOccurrence.payload = CadX::OccurrencePayload {{"Assembly", "PartTwo"}, false, false, {}};
    capture.nodes.push_back(secondOccurrence);
    capture.edges.push_back({"contains", CadX::EdgeKind::Contains, "assembly", "occurrence", {}, ""});
    capture.edges.push_back({"instance", CadX::EdgeKind::InstanceOf, "occurrence", "part", {}, ""});
    capture.edges.push_back({"contains-two", CadX::EdgeKind::Contains, "assembly", "occurrence-two", {}, ""});
    capture.edges.push_back({"instance-two", CadX::EdgeKind::InstanceOf, "occurrence-two", "part", {}, ""});
    return capture;
}

QJsonObject payload(const CadX::ToolResult& result)
{
    return QJsonDocument::fromJson(QByteArray::fromStdString(result.toJson())).object();
}
}  // namespace

TEST(CadXService, ExecutesAllBoundedGraphQueryOperations)
{
    CadX::CadXService service;
    const auto capture = makeCapture();
    const auto built = CadX::AssemblyGraphBuilder().build(capture);
    ASSERT_TRUE(built) << built.diagnostic;
    ASSERT_TRUE(service.publishCapture(capture).ok);

    const auto graphId = built.snapshot->header().graphId;
    const auto revision = built.snapshot->header().graphRevision;
    const auto evidence = service.exportGraphEvidence(graphId, revision);
    ASSERT_TRUE(evidence.ok) << evidence.message;
    const auto decoded = CadX::GraphJsonCodec::decode(evidence.toJson());
    ASSERT_TRUE(decoded) << decoded.errorCode << ": " << decoded.diagnostic;
    EXPECT_EQ(decoded.snapshot->header().graphRevision, revision);
    const auto base = "\"graph_id\":\"" + graphId + "\",\"graph_revision\":\""
        + revision + "\",";

    const auto summary = service.executeTool(
        "assembly.graph_query", "{" + base + "\"operation\":\"summary\"}");
    ASSERT_TRUE(summary.ok) << summary.message;
    EXPECT_EQ(payload(summary).value("operation").toString(), "summary");

    const auto find = service.executeTool(
        "assembly.graph_query",
        "{" + base + "\"operation\":\"find_nodes\",\"node_kinds\":[\"Occurrence\"],\"limit\":1}");
    ASSERT_TRUE(find.ok) << find.message;
    EXPECT_EQ(payload(find).value("returned_node_count").toInt(), 1);
    const auto cursor = payload(find).value("next_cursor").toString().toStdString();
    ASSERT_FALSE(cursor.empty());
    const auto next = service.executeTool(
        "assembly.graph_query",
        "{" + base + "\"operation\":\"find_nodes\",\"node_kinds\":[\"Occurrence\"],\"limit\":1,\"cursor\":\""
            + cursor + "\"}");
    ASSERT_TRUE(next.ok) << next.message;
    EXPECT_EQ(payload(next).value("returned_node_count").toInt(), 1);

    const auto neighbors = service.executeTool(
        "assembly.graph_query",
        "{" + base + "\"operation\":\"neighbors\",\"start_node_ids\":[\"occurrence\"],\"direction\":\"outgoing\"}");
    ASSERT_TRUE(neighbors.ok) << neighbors.message;
    EXPECT_EQ(payload(neighbors).value("returned_edge_count").toInt(), 1);

    const auto subgraph = service.executeTool(
        "assembly.graph_query",
        "{" + base + "\"operation\":\"subgraph\",\"start_node_ids\":[\"occurrence\"],\"max_depth\":1,\"edge_kinds\":[\"INSTANCE_OF\"]}");
    ASSERT_TRUE(subgraph.ok) << subgraph.message;
    EXPECT_EQ(payload(subgraph).value("returned_edge_count").toInt(), 1);

    const auto path = service.executeTool(
        "assembly.graph_query",
        "{" + base + "\"operation\":\"shortest_path\",\"start_node_id\":\"occurrence\",\"target_node_id\":\"part\",\"max_depth\":1,\"edge_kinds\":[\"INSTANCE_OF\"]}");
    ASSERT_TRUE(path.ok) << path.message;
    EXPECT_EQ(payload(path).value("returned_node_count").toInt(), 2);
}

TEST(CadXService, MutationSchemasMatchCreateAndJointRuntimeShapes)
{
    CadX::CadXService service;
    const auto definitions = service.toolRegistry().definitions();
    const auto create = std::find_if(definitions.begin(), definitions.end(),
                                     [](const CadX::ToolDefinition& definition) {
                                         return definition.name == "assembly.create";
                                     });
    const auto joint = std::find_if(definitions.begin(), definitions.end(),
                                    [](const CadX::ToolDefinition& definition) {
                                        return definition.name == "assembly.joint";
                                    });
    if (create == definitions.end() || joint == definitions.end()) {
        GTEST_SKIP() << "Assembly mutation tools are unavailable in this build";
    }

    const auto createSchema = QJsonDocument::fromJson(
        QByteArray::fromStdString(create->inputSchemaJson)).object();
    const auto createProperties = createSchema.value("properties").toObject();
    EXPECT_FALSE(createProperties.contains("parent_assembly"));

    const auto jointSchema = QJsonDocument::fromJson(
        QByteArray::fromStdString(joint->inputSchemaJson)).object();
    const auto connector = jointSchema.value("$defs").toObject().value("connector").toObject();
    const auto offset = connector.value("properties").toObject().value("offset").toObject();
    EXPECT_EQ(offset.value("type").toString(), "object");
    EXPECT_TRUE(offset.value("properties").toObject().contains("translation_mm"));
    EXPECT_TRUE(offset.value("properties").toObject().contains("rotation_axis"));
    EXPECT_TRUE(offset.value("properties").toObject().contains("rotation_degrees"));
}
