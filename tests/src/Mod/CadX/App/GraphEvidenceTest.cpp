// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Mod/CadX/App/GraphAudit.h>
#include <Mod/CadX/App/GraphJsonCodec.h>
#include <Mod/CadX/App/GraphRevision.h>
#include <Mod/CadX/App/GraphSnapshot.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
CadX::NodeRecord node(const std::string& id,
                     CadX::NodeKind kind,
                     const std::string& objectName,
                     const std::string& label,
                     CadX::NodePayload payload = {},
                     bool unresolved = false)
{
    CadX::NodeRecord result;
    result.id = id;
    result.kind = kind;
    result.native = {"doc:examples", objectName, std::string("Example::") + objectName};
    result.display = {label, label};
    std::transform(result.display.normalizedLabel.begin(),
                   result.display.normalizedLabel.end(),
                   result.display.normalizedLabel.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    result.provenance = {"freecad-example", "example:" + id};
    result.presentation = {true, false, "view:main"};
    result.payload = std::move(payload);
    result.unresolved = unresolved;
    return result;
}

CadX::EdgeRecord edge(const std::string& id,
                      CadX::EdgeKind kind,
                      const std::string& from,
                      const std::string& to,
                      const std::string& relation = {})
{
    CadX::EdgeRecord result;
    result.id = id;
    result.kind = kind;
    result.from = from;
    result.to = to;
    result.provenance = {"freecad-example", "edge:" + id};
    result.relation = relation;
    return result;
}

CadX::OccurrencePayload occurrence(std::initializer_list<std::string> path,
                                    bool rigid,
                                    const std::string& signature)
{
    CadX::OccurrencePayload result;
    result.occurrencePath = path;
    result.rigid = rigid;
    result.geometry = {true, true, "solid", 1, 1, 6, 12, 8, 10.5, 42.0, signature};
    result.geometry.signature = signature;
    result.geometry.volume = 10.5;
    result.geometry.area = 42.0;
    return result;
}

CadX::GraphSnapshot makeExampleGraph(bool reverseInput = false)
{
    CadX::GraphSnapshot graph;
    graph.header().graphId = "assembly-graph:examples";
    graph.header().documentUid = "doc:examples";
    graph.header().documentName = "Strict Example Assembly";
    graph.header().activeAssemblyNodeId = "assembly";
    graph.header().activeAssemblyObjectName = "Assembly";
    graph.header().activeAssemblyLabel = "Main Assembly";
    graph.header().activeViewId = "view:main";
    graph.header().cameraState = "camera:v1";
    graph.header().complete = true;

    // Fixture coverage: direct primitive, duplicate-link occurrences, nested
    // occurrence, relation, artifact, and unresolved external reference.
    graph.nodes() = {
        node("document", CadX::NodeKind::Document, "Document", "Document"),
        node("assembly", CadX::NodeKind::AssemblyDefinition, "Assembly", "Main Assembly",
             CadX::DefinitionPayload {"assembly", "Assembly", "none", "freecad", "assembly"}),
        node("part", CadX::NodeKind::PartDefinition, "Part", "Bracket",
             CadX::DefinitionPayload {"part", "Part", "solid", "freecad", "mechanical"}),
        node("body", CadX::NodeKind::BodyDefinition, "Body", "Bracket Body",
             CadX::DefinitionPayload {"body", "Body", "solid", "freecad", "mechanical"}),
        node("feature", CadX::NodeKind::FeatureDefinition, "Pad", "Pad Feature"),
        node("occurrence-a", CadX::NodeKind::Occurrence, "BracketInstanceA", "Bracket A",
             occurrence({"Assembly", "BracketInstanceA"}, true, "shape:bracket")),
        node("occurrence-subassembly", CadX::NodeKind::AssemblyOccurrence,
             "SubassemblyInstance", "Subassembly",
             occurrence({"Assembly", "SubassemblyInstance"}, true, "shape:subassembly")),
        node("joint", CadX::NodeKind::Joint, "Joint", "Fixed Joint",
             CadX::RelationPayload {"fixed", false}),
        node("artifact", CadX::NodeKind::AssemblyArtifact, "BomRow", "BOM Row",
             CadX::ArtifactPayload {"bom-row"}),
        node("unresolved", CadX::NodeKind::UnresolvedDefinition, "MissingPart", "Missing Part",
             CadX::UnresolvedPayload {"doc:missing", "MissingPart", "source document unavailable"},
             true),
    };

    for (auto& item : graph.nodes()) {
        if (item.id == "occurrence-a") {
            item.worldPlacement.x = 1.0;
        }
        else if (item.id == "occurrence-subassembly") {
            item.worldPlacement.y = 2.0;
        }
    }

    graph.edges() = {
        edge("e-document-assembly", CadX::EdgeKind::Contains, "document", "assembly"),
        edge("e-assembly-part", CadX::EdgeKind::Contains, "assembly", "part"),
        edge("e-part-body", CadX::EdgeKind::HasBody, "part", "body"),
        edge("e-body-feature", CadX::EdgeKind::HasFeature, "body", "feature"),
        edge("e-assembly-occurrence-a", CadX::EdgeKind::Contains, "assembly", "occurrence-a"),
        edge("e-assembly-subassembly", CadX::EdgeKind::Contains,
             "assembly", "occurrence-subassembly"),
        edge("e-occurrence-a-definition", CadX::EdgeKind::InstanceOf,
             "occurrence-a", "part"),
        edge("e-subassembly-definition", CadX::EdgeKind::InstanceOf,
             "occurrence-subassembly", "assembly"),
        edge("e-subassembly-occurrence-a", CadX::EdgeKind::NestedOccurrence,
             "occurrence-subassembly", "occurrence-a"),
        edge("e-assembly-joint", CadX::EdgeKind::HasJoint, "assembly", "joint"),
        edge("e-joint-endpoint", CadX::EdgeKind::JointEndpoint, "joint", "occurrence-a"),
        edge("e-assembly-artifact", CadX::EdgeKind::HasArtifact, "assembly", "artifact"),
        edge("e-artifact-unresolved", CadX::EdgeKind::UnresolvedSource,
             "artifact", "unresolved"),
        edge("e-assembly-source", CadX::EdgeKind::SourceDocument, "assembly", "document"),
    };

    if (reverseInput) {
        std::reverse(graph.nodes().begin(), graph.nodes().end());
        std::reverse(graph.edges().begin(), graph.edges().end());
    }
    std::string diagnostic;
    if (!graph.finalize(diagnostic)) {
        throw std::runtime_error("strict example fixture is invalid: " + diagnostic);
    }
    return graph;
}

std::string replaceOnce(std::string value,
                        const std::string& search,
                        const std::string& replacement)
{
    const auto position = value.find(search);
    if (position == std::string::npos) {
        return value;
    }
    value.replace(position, search.size(), replacement);
    return value;
}

}  // namespace

TEST(CadXGraphEvidence, RoundTripPreservesEveryStrictExampleRecord)
{
    const auto original = makeExampleGraph();
    const auto evidence = CadX::GraphJsonCodec::encode(original);
    const auto decoded = CadX::GraphJsonCodec::decode(evidence);

    ASSERT_TRUE(decoded) << decoded.errorCode << ": " << decoded.diagnostic;
    ASSERT_EQ(decoded.snapshot->nodes().size(), original.nodes().size());
    ASSERT_EQ(decoded.snapshot->edges().size(), original.edges().size());
    EXPECT_EQ(CadX::canonicalSemantic(*decoded.snapshot), CadX::canonicalSemantic(original));
    EXPECT_EQ(CadX::canonicalPresentation(*decoded.snapshot),
              CadX::canonicalPresentation(original));
    EXPECT_EQ(CadX::GraphJsonCodec::encode(*decoded.snapshot), evidence);
    EXPECT_EQ(decoded.snapshot->header().graphRevision, original.header().graphRevision);
    EXPECT_EQ(decoded.snapshot->header().presentationRevision,
              original.header().presentationRevision);
}

TEST(CadXGraphEvidence, InsertionOrderDoesNotChangeRevisionOrEvidence)
{
    const auto forward = makeExampleGraph();
    const auto reverse = makeExampleGraph(true);

    EXPECT_EQ(forward.header().graphRevision, reverse.header().graphRevision);
    EXPECT_EQ(forward.header().presentationRevision, reverse.header().presentationRevision);
    EXPECT_EQ(CadX::GraphJsonCodec::encode(forward), CadX::GraphJsonCodec::encode(reverse));
}

TEST(CadXGraphEvidence, RoundTripPreservesTypedMutationPayloads)
{
    auto graph = makeExampleGraph();
    for (auto& item : graph.nodes()) {
        if (item.id == "feature") {
            item.payload = CadX::PrimitivePayload {"box", 10.0, 20.0, 30.0, 0.0, 360.0};
        }
        else if (item.id == "joint") {
            CadX::JointPayload joint {"revolute", true, true, -15.0, 45.0};
            joint.first = {"BracketInstanceA", "element", "Face1",
                           CadX::Placement {1.0, 2.0, 3.0, 0.0, 0.0, 0.6, 0.8}, true};
            joint.second = {"BracketInstanceA", "interface", "Pivot",
                            CadX::Placement {-4.0, 5.0, 6.0, 0.0, 0.0, 0.0, 1.0}, true};
            item.payload = joint;
        }
    }
    graph.nodes().push_back(node(
        "ground", CadX::NodeKind::GroundConstraint, "GroundedJoint", "Grounded",
        CadX::GroundConstraintPayload {true, "BracketInstanceA", "GroundedJoint"}));
    graph.edges().push_back(
        edge("e-ground", CadX::EdgeKind::GroundedBy, "ground", "occurrence-a"));
    std::string diagnostic;
    ASSERT_TRUE(graph.finalize(diagnostic)) << diagnostic;

    const auto evidence = CadX::GraphJsonCodec::encode(graph);
    const auto decoded = CadX::GraphJsonCodec::decode(evidence);
    ASSERT_TRUE(decoded) << decoded.errorCode << ": " << decoded.diagnostic;
    EXPECT_EQ(CadX::canonicalSemantic(*decoded.snapshot), CadX::canonicalSemantic(graph));
    EXPECT_EQ(CadX::GraphJsonCodec::encode(*decoded.snapshot), evidence);
}

TEST(CadXGraphEvidence, JointRoundTripPreservesTopologyAndCanonicalRevision)
{
    auto graph = makeExampleGraph();
    CadX::JointPayload joint {"revolute", false, true, -30.0, 60.0};
    joint.first = {"BracketInstanceA", "element", "Face12",
                   CadX::Placement {10.0, -2.0, 0.5, 0.0, 0.0, 0.6, 0.8}, true};
    joint.second = {"BracketInstanceA", "interface", "PivotAxis",
                    CadX::Placement {0.0, 0.0, 7.0, 0.0, 0.0, 0.0, 1.0}, false};
    for (auto& item : graph.nodes()) {
        if (item.id == "joint") {
            item.payload = joint;
        }
    }
    std::string diagnostic;
    ASSERT_TRUE(graph.finalize(diagnostic)) << diagnostic;

    const auto evidence = CadX::GraphJsonCodec::encode(graph);
    EXPECT_NE(evidence.find("\"connector_type\":\"element\""), std::string::npos);
    EXPECT_NE(evidence.find("\"connector\":\"Face12\""), std::string::npos);
    EXPECT_NE(evidence.find("\"has_offset\":false"), std::string::npos);
    EXPECT_NE(evidence.find("\"offset\":[10,-2,0.5,0,0,0.6,0.8]"), std::string::npos);

    const auto decoded = CadX::GraphJsonCodec::decode(evidence);
    ASSERT_TRUE(decoded) << decoded.errorCode << ": " << decoded.diagnostic;
    EXPECT_EQ(decoded.snapshot->header().graphRevision, graph.header().graphRevision);
    EXPECT_EQ(CadX::canonicalSemantic(*decoded.snapshot), CadX::canonicalSemantic(graph));
    const auto* decodedJoint = decoded.snapshot->findNode("joint");
    ASSERT_NE(decodedJoint, nullptr);
    const auto* decodedPayload = std::get_if<CadX::JointPayload>(&decodedJoint->payload);
    ASSERT_NE(decodedPayload, nullptr);
    EXPECT_EQ(decodedPayload->first.componentObject, joint.first.componentObject);
    EXPECT_EQ(decodedPayload->first.connectorType, joint.first.connectorType);
    EXPECT_EQ(decodedPayload->first.connector, joint.first.connector);
    EXPECT_TRUE(decodedPayload->first.hasOffset);
    EXPECT_EQ(decodedPayload->second.connector, joint.second.connector);
    EXPECT_FALSE(decodedPayload->second.hasOffset);
    EXPECT_EQ(CadX::GraphJsonCodec::encode(*decoded.snapshot), evidence);
}

TEST(CadXGraphEvidence, JointPayloadRejectsMalformedConnectorEvidence)
{
    auto graph = makeExampleGraph();
    CadX::JointPayload joint {"fixed", false, false, 0.0, 0.0};
    joint.first = {"BracketInstanceA", "element", "Face1", {}, true};
    joint.second = {"BracketInstanceA", "interface", "Pivot", {}, false};
    for (auto& item : graph.nodes()) {
        if (item.id == "joint") {
            item.payload = joint;
        }
    }
    std::string diagnostic;
    ASSERT_TRUE(graph.finalize(diagnostic)) << diagnostic;
    const auto evidence = CadX::GraphJsonCodec::encode(graph);

    const auto missingPath = replaceOnce(evidence, "\"connector\":\"Face1\",", "");
    EXPECT_FALSE(CadX::GraphJsonCodec::decode(missingPath));
    const auto badOffset = replaceOnce(evidence, "\"offset\":[0,0,0,0,0,0,1]",
                                       "\"offset\":null");
    EXPECT_FALSE(CadX::GraphJsonCodec::decode(badOffset));
    const auto badFlag = replaceOnce(evidence, "\"has_offset\":true", "\"has_offset\":1");
    EXPECT_FALSE(CadX::GraphJsonCodec::decode(badFlag));
    const auto badQuaternion = replaceOnce(evidence, "\"offset\":[0,0,0,0,0,0,1]",
                                           "\"offset\":[0,0,0,0,0,0,0]");
    EXPECT_FALSE(CadX::GraphJsonCodec::decode(badQuaternion));
    const auto unknownField = replaceOnce(evidence, "\"type\":\"joint\"",
                                          "\"type\":\"joint\",\"unexpected\":true");
    EXPECT_FALSE(CadX::GraphJsonCodec::decode(unknownField));
}

TEST(CadXGraphEvidence, JointOffsetIsNormalizedForDeterministicRoundTrip)
{
    auto graph = makeExampleGraph();
    CadX::JointPayload joint {"revolute", false, false, 0.0, 0.0};
    joint.first = {"BracketInstanceA", "element", "Face1",
                   CadX::Placement {0.0, 0.0, 0.0, 0.0, 0.0, 3.0, 4.0}, true};
    joint.second = {"BracketInstanceA", "interface", "Pivot", {}, false};
    for (auto& item : graph.nodes()) {
        if (item.id == "joint") {
            item.payload = joint;
        }
    }
    std::string diagnostic;
    ASSERT_TRUE(graph.finalize(diagnostic)) << diagnostic;

    const auto evidence = CadX::GraphJsonCodec::encode(graph);
    EXPECT_NE(evidence.find("\"offset\":[0,0,0,0,0,0.6,0.8]"), std::string::npos);
    const auto decoded = CadX::GraphJsonCodec::decode(evidence);
    ASSERT_TRUE(decoded) << decoded.errorCode << ": " << decoded.diagnostic;
    EXPECT_EQ(decoded.snapshot->header().graphRevision, graph.header().graphRevision);
    EXPECT_EQ(CadX::GraphJsonCodec::encode(*decoded.snapshot), evidence);
}

TEST(CadXGraphEvidence, TamperedRevisionIsRejected)
{
    const auto graph = makeExampleGraph();
    const auto evidence = CadX::GraphJsonCodec::encode(graph);
    const auto tampered = replaceOnce(
        evidence,
        "\"graph_revision\":\"" + graph.header().graphRevision + "\"",
        "\"graph_revision\":\"sha256:tampered\"");
    ASSERT_NE(tampered, evidence);

    const auto result = CadX::GraphJsonCodec::decode(tampered);
    EXPECT_FALSE(result);
    EXPECT_EQ(result.errorCode, "CADX_GRAPH_EVIDENCE_MISMATCH");
}

TEST(CadXGraphEvidence, RejectsContainmentCycleAtValidationLayer)
{
    auto graph = makeExampleGraph();
    graph.edges().push_back(
        edge("e-cycle", CadX::EdgeKind::Contains, "occurrence-a", "assembly"));
    std::string diagnostic;

    EXPECT_FALSE(graph.finalize(diagnostic));
    EXPECT_EQ(diagnostic, "graph containment is cyclic");
}

TEST(CadXGraphEvidence, RejectsMissingOccurrenceDefinitionAtValidationLayer)
{
    auto graph = makeExampleGraph();
    graph.edges().erase(
        std::remove_if(graph.edges().begin(), graph.edges().end(), [](const auto& item) {
            return item.id == "e-occurrence-a-definition";
        }),
        graph.edges().end());
    std::string diagnostic;

    EXPECT_FALSE(graph.finalize(diagnostic));
    EXPECT_EQ(diagnostic, "every resolved occurrence must have exactly one INSTANCE_OF edge");
}

TEST(CadXGraphEvidence, RejectsNonNormalizedPlacementAtValidationLayer)
{
    auto graph = makeExampleGraph();
    for (auto& item : graph.nodes()) {
        if (item.id == "occurrence-a") {
            item.localPlacement.qw = 2.0;
        }
    }
    std::string diagnostic;

    EXPECT_FALSE(graph.finalize(diagnostic));
    EXPECT_EQ(diagnostic, "graph contains a non-finite or non-normalized placement");
}

TEST(CadXGraphAudit, WritesOrderedRevisionAndHashCheckpoints)
{
    const std::string path = "/private/tmp/cadx-graph-audit-test.jsonl";
    {
        std::ofstream truncate(path, std::ios::trunc);
        ASSERT_TRUE(truncate);
    }

    const auto graph = makeExampleGraph();
    CadX::GraphAuditLog log(path);
    log.record(CadX::makeGraphAuditEvent("build", "started", nullptr,
                                         "assembly.graph_snapshot"));
    log.record(CadX::makeGraphAuditEvent("build", "passed", &graph,
                                         "assembly.graph_snapshot"));
    log.record(CadX::makeGraphAuditEvent("round_trip", "passed", &graph,
                                         "assembly.graph_snapshot"));
    log.record(CadX::makeGraphAuditEvent("publish", "passed", &graph,
                                         "assembly.graph_snapshot"));

    std::ifstream input(path);
    ASSERT_TRUE(input);
    std::string line;
    int count = 0;
    while (std::getline(input, line)) {
        ++count;
        EXPECT_NE(line.find("\"schema_version\":\"cadx.graph-audit.v1\""),
                  std::string::npos);
        EXPECT_NE(line.find("\"sequence\":" + std::to_string(count)), std::string::npos);
        if (count > 1) {
            EXPECT_NE(line.find("\"graph_revision\":\"" + graph.header().graphRevision + "\""),
                      std::string::npos);
            EXPECT_NE(line.find("\"semantic_hash\":\"sha256:"), std::string::npos);
            EXPECT_NE(line.find("\"presentation_hash\":\"sha256:"), std::string::npos);
        }
    }
    EXPECT_EQ(count, 4);
    EXPECT_TRUE(log.lastError().empty()) << log.lastError();
    input.close();
    std::remove(path.c_str());
}
