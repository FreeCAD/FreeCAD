// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Mod/CadX/App/NativeAssemblyConstraintOperations.h>
#include <src/App/InitApplication.h>
#include <Mod/CadX/App/NativeAssemblyOperations.h>

#include <string>

namespace
{
std::string groundingJson(const std::string& operation, const std::string& components)
{
    return "{\"operation\":\"" + operation
        + "\",\"operation_id\":\"op-1\",\"expected_graph_revision\":\"rev-1\""
          ",\"assembly\":{\"object_name\":\"Assembly\"},\"components\":"
        + components + "}";
}

std::string jointJson(const std::string& extra = {})
{
    return "{\"operation\":\"create\",\"operation_id\":\"op-2\","
           "\"expected_graph_revision\":\"rev-1\","
           "\"assembly\":{\"object_name\":\"Assembly\"},"
           "\"first\":{\"component\":\"Base\",\"connector_type\":\"element\","
           "\"connector\":\"Face1\"},"
           "\"second\":{\"component\":\"Arm\",\"connector_type\":\"interface\","
           "\"connector\":\"Pivot\"},"
           "\"joint_type\":\"revolute\"" + extra + "}";
}
}  // namespace

std::shared_ptr<CadX::GraphSnapshot> publicationSnapshot(const std::string& label)
{
    auto snapshot = std::make_shared<CadX::GraphSnapshot>();
    snapshot->header().graphId = "assembly-graph:publication-test";
    snapshot->header().documentUid = "doc:publication-test";
    snapshot->header().documentName = "PublicationTest";
    snapshot->header().activeAssemblyNodeId = "assembly";
    snapshot->header().activeAssemblyObjectName = "Assembly";
    CadX::NodeRecord root;
    root.id = "assembly";
    root.kind = CadX::NodeKind::AssemblyDefinition;
    root.native = {"doc:publication-test", "Assembly", "Assembly::AssemblyObject"};
    root.display = {label, label};
    snapshot->nodes().push_back(root);
    std::string diagnostic;
    EXPECT_TRUE(snapshot->finalize(diagnostic)) << diagnostic;
    return snapshot;
}

TEST(CadXNativeAssemblyConstraintParser, GroundingRequiresUniqueBoundedComponents)
{
    CadX::GroundingRequest request;
    std::string diagnostic;
    EXPECT_TRUE(CadX::parseGroundingRequest(
        groundingJson("set_grounded", "[\"Base\",\"Arm\"]"),
        "set_grounded", request, diagnostic))
        << diagnostic;
    EXPECT_TRUE(request.grounded);
    EXPECT_EQ(request.components.size(), 2U);

    EXPECT_FALSE(CadX::parseGroundingRequest(
        groundingJson("set_grounded", "[\"Base\",\"Base\"]"),
        "set_grounded", request, diagnostic));
    EXPECT_NE(diagnostic.find("unique"), std::string::npos);
}

TEST(CadXNativeAssemblyConstraintParser, GroundingRejectsMoreThanSixteenComponents)
{
    std::string components = "[";
    for (int i = 0; i != 17; ++i) {
        if (i != 0) {
            components += ",";
        }
        components += "\"C" + std::to_string(i) + "\"";
    }
    components += "]";
    CadX::GroundingRequest request;
    std::string diagnostic;
    EXPECT_FALSE(CadX::parseGroundingRequest(
        groundingJson("set_movable", components), "set_movable", request, diagnostic));
    EXPECT_NE(diagnostic.find("1 to 16"), std::string::npos);
}

TEST(CadXNativeAssemblyConstraintParser, JointParsesConnectorRecordsAndOffsets)
{
    CadX::JointRequest request;
    std::string diagnostic;
    EXPECT_TRUE(CadX::parseJointRequest(
        jointJson(
            ",\"label\":\"Arm pivot\",\"reverse\":true,\"limits\":{"
            "\"minimum_degrees\":-90,\"maximum_degrees\":90}"),
        request, diagnostic))
        << diagnostic;
    EXPECT_EQ(request.jointType, "revolute");
    EXPECT_TRUE(request.reverse);
    EXPECT_TRUE(request.hasLimits);
    EXPECT_DOUBLE_EQ(request.minimumDegrees, -90.0);
    EXPECT_DOUBLE_EQ(request.maximumDegrees, 90.0);
}

TEST(CadXNativeAssemblyConstraintParser, JointRejectsInvalidLimitsAndUnknownFields)
{
    CadX::JointRequest request;
    std::string diagnostic;
    EXPECT_FALSE(CadX::parseJointRequest(
        jointJson(",\"limits\":{\"minimum_degrees\":90,\"maximum_degrees\":-90}"),
        request, diagnostic));
    EXPECT_NE(diagnostic.find("must not exceed"), std::string::npos);

    EXPECT_FALSE(CadX::parseJointRequest(
        jointJson(",\"unexpected\":true"), request, diagnostic));
    EXPECT_NE(diagnostic.find("unknown field"), std::string::npos);
}

TEST(CadXNativeAssemblyConstraintParser, JointRejectsMalformedConnectorTopology)
{
    CadX::JointRequest request;
    std::string diagnostic;
    EXPECT_FALSE(CadX::parseJointRequest(
        "{\"operation\":\"create\",\"operation_id\":\"op\","
        "\"expected_graph_revision\":\"rev\",\"assembly\":{\"object_name\":\"Assembly\"},"
        "\"first\":{\"component\":\"Base\",\"connector_type\":\"element\","
        "\"connector\":\"Face0\"},"
        "\"second\":{\"component\":\"Arm\",\"connector_type\":\"element\","
        "\"connector\":\"Face1\"},\"joint_type\":\"fixed\"}",
        request, diagnostic));
    EXPECT_NE(diagnostic.find("FaceN"), std::string::npos);
}

TEST(CadXNativeAssemblyConstraintParser, JointAcceptsObjectAndRejectsArrayOffsetForms)
{
    CadX::JointRequest request;
    std::string diagnostic;
    const auto objectOffset =
        R"json({"operation":"create","operation_id":"op","expected_graph_revision":"rev",
        "assembly":{"object_name":"Assembly"},
        "first":{"component":"Base","connector_type":"element","connector":"Face1",
          "offset":{"translation_mm":[1,2,3],"rotation_axis":[0,0,1],"rotation_degrees":90}},
        "second":{"component":"Arm","connector_type":"interface","connector":"Pivot"},
        "joint_type":"revolute"})json";
    ASSERT_TRUE(CadX::parseJointRequest(objectOffset, request, diagnostic)) << diagnostic;
    EXPECT_TRUE(request.first.hasOffset);
    EXPECT_DOUBLE_EQ(request.first.offset.x, 1.0);
    EXPECT_DOUBLE_EQ(request.first.offset.z, 3.0);

    const auto arrayOffset =
        R"json({"operation":"create","operation_id":"op","expected_graph_revision":"rev",
        "assembly":{"object_name":"Assembly"},
        "first":{"component":"Base","connector_type":"element","connector":"Face1",
          "offset":[1,2,3,0,0,0.7071067811865475,0.7071067811865476]},
        "second":{"component":"Arm","connector_type":"interface","connector":"Pivot"},
        "joint_type":"revolute"})json";
    EXPECT_FALSE(CadX::parseJointRequest(arrayOffset, request, diagnostic));
    EXPECT_NE(diagnostic.find("offset must be an object"), std::string::npos);
}

TEST(CadXNativeAssemblyConstraintParser, UnsupportedBuildReturnsExplicitFailure)
{
#ifdef CADX_HAVE_ASSEMBLY
    tests::initApplication();
#endif
    CadX::GraphStore graphs;
    CadX::GraphAuditLog audit;
    CadX::NativeAssemblyConstraintOperations operations(graphs, audit);
    const auto result = operations.execute(
        "assembly.ground",
        groundingJson("set_grounded", "[\"Base\"]"));
#ifndef CADX_HAVE_ASSEMBLY
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.errorCode, "CADX_UNSUPPORTED_OBJECT");
#else
    SUCCEED();
#endif
}

TEST(CadXNativeMutationPublication, ReconcilesCommittedGraphAfterCompareAndSwapRace)
{
    CadX::GraphStore graphs;
    const CadX::GraphScope scope {"doc:publication-test", "Assembly"};
    auto first = publicationSnapshot("first");
    std::string diagnostic;
    ASSERT_EQ(graphs.publish(scope, first, diagnostic), CadX::StoreError::None) << diagnostic;
    const auto parent = first->header().graphRevision;

    auto concurrent = publicationSnapshot("concurrent");
    ASSERT_EQ(graphs.publish(scope, concurrent, diagnostic), CadX::StoreError::None) << diagnostic;
    auto committed = publicationSnapshot("committed");
    const auto publication = CadX::publishCommittedGraph(
        graphs, scope, committed, parent, diagnostic);

    EXPECT_EQ(publication, CadX::CommitPublication::Reconciled);
    EXPECT_NE(diagnostic.find("SEVERE"), std::string::npos);
    const auto current = graphs.current(scope);
    ASSERT_TRUE(current);
    EXPECT_EQ(current.snapshot->header().graphRevision,
              committed->header().graphRevision);
}
