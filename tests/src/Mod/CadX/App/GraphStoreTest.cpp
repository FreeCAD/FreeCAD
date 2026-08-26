// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Mod/CadX/App/GraphStore.h>

#include <thread>

namespace
{
std::shared_ptr<CadX::GraphSnapshot> makeSnapshot(const std::string& revisionLabel)
{
    auto snapshot = std::make_shared<CadX::GraphSnapshot>();
    snapshot->header().graphId = "assembly-graph:test";
    snapshot->header().documentUid = "doc:test";
    snapshot->header().documentName = "Test";
    snapshot->header().activeAssemblyNodeId = "assembly";
    snapshot->header().activeAssemblyObjectName = "Assembly";

    CadX::NodeRecord root;
    root.id = "assembly";
    root.kind = CadX::NodeKind::AssemblyDefinition;
    root.native = {"doc:test", "Assembly", "Assembly::AssemblyObject"};
    root.display = {"Main Assembly", "main assembly"};
    root.provenance.kind = revisionLabel;
    snapshot->nodes().push_back(root);

    std::string diagnostic;
    EXPECT_TRUE(snapshot->finalize(diagnostic)) << diagnostic;
    return snapshot;
}
}  // namespace

TEST(CadXGraphStore, PublishesAndLooksUpExactRevision)
{
    CadX::GraphStore store;
    auto snapshot = makeSnapshot("first");
    ASSERT_FALSE(snapshot->header().graphRevision.empty());
    std::string diagnostic;
    EXPECT_EQ(store.publish({"doc:test", "Assembly"}, snapshot, diagnostic), CadX::StoreError::None)
        << diagnostic;

    const auto lookup = store.lookup("assembly-graph:test", snapshot->header().graphRevision);
    ASSERT_TRUE(lookup);
    EXPECT_EQ(lookup.snapshot->header().documentUid, "doc:test");
}

TEST(CadXGraphStore, StaleGraphsAreRejectedUntilExplicitlyAllowed)
{
    CadX::GraphStore store;
    auto snapshot = makeSnapshot("first");
    std::string diagnostic;
    ASSERT_EQ(store.publish({"doc:test", "Assembly"}, snapshot, diagnostic), CadX::StoreError::None);
    ASSERT_TRUE(store.markStale("assembly-graph:test", "source changed"));

    EXPECT_EQ(
        store.lookup("assembly-graph:test", snapshot->header().graphRevision).error,
        CadX::StoreError::GraphStale
    );
    EXPECT_TRUE(store.lookup("assembly-graph:test", snapshot->header().graphRevision, true));
}

TEST(CadXGraphStore, SourceDocumentChangesInvalidateDependentGraphs)
{
    CadX::GraphStore store;
    auto snapshot = makeSnapshot("source");
    std::string diagnostic;
    ASSERT_EQ(store.publish({"doc:test", "Assembly"}, snapshot, diagnostic), CadX::StoreError::None);

    EXPECT_TRUE(store.markSourceDocumentStale("doc:test", "linked source changed"));
    EXPECT_EQ(
        store.lookup("assembly-graph:test", snapshot->header().graphRevision).error,
        CadX::StoreError::GraphStale
    );
}

TEST(CadXGraphStore, CompareAndSwapRejectsStaleWriter)
{
    CadX::GraphStore store;
    auto first = makeSnapshot("first");
    std::string diagnostic;
    ASSERT_EQ(store.publish({"doc:test", "Assembly"}, first, diagnostic), CadX::StoreError::None);
    auto second = makeSnapshot("second");
    const auto parent = first->header().graphRevision;
    ASSERT_EQ(
        store.publishIfCurrent({"doc:test", "Assembly"}, second, parent, diagnostic),
        CadX::StoreError::None
    );
    auto stale = makeSnapshot("stale");
    EXPECT_EQ(
        store.publishIfCurrent({"doc:test", "Assembly"}, stale, parent, diagnostic),
        CadX::StoreError::RevisionMismatch
    );
    EXPECT_EQ(
        store.current({"doc:test", "Assembly"}).snapshot->header().graphRevision,
        second->header().graphRevision
    );
}

TEST(CadXGraphStore, CompareAndSwapEmptyRevisionOnlyPublishesAnInitialGraph)
{
    CadX::GraphStore store;
    auto first = makeSnapshot("first");
    std::string diagnostic;
    ASSERT_EQ(
        store.publishIfCurrent({"doc:test", "Assembly"}, first, {}, diagnostic),
        CadX::StoreError::None
    ) << diagnostic;

    auto second = makeSnapshot("second");
    EXPECT_EQ(
        store.publishIfCurrent({"doc:test", "Assembly"}, second, {}, diagnostic),
        CadX::StoreError::RevisionMismatch
    );
    EXPECT_EQ(
        store.current({"doc:test", "Assembly"}).snapshot->header().graphRevision,
        first->header().graphRevision
    );
}

TEST(CadXGraphStore, InitialCompareAndSwapValidatesSnapshotScope)
{
    CadX::GraphStore store;
    auto snapshot = makeSnapshot("scope-mismatch");
    std::string diagnostic;

    EXPECT_EQ(
        store.publishIfCurrent({"doc:other", "Assembly"}, snapshot, {}, diagnostic),
        CadX::StoreError::RevisionMismatch
    );
    EXPECT_EQ(diagnostic, "graph scope does not match the snapshot header");
    EXPECT_EQ(store.graphCount(), 0U);
}

TEST(CadXGraphStore, InitialCompareAndSwapCannotReplaceAStaleHandle)
{
    CadX::GraphStore store;
    auto first = makeSnapshot("stale-initial");
    std::string diagnostic;
    ASSERT_EQ(store.publish({"doc:test", "Assembly"}, first, diagnostic), CadX::StoreError::None);
    ASSERT_TRUE(store.markStale("assembly-graph:test", "source changed"));

    auto replacement = makeSnapshot("replacement");
    EXPECT_EQ(
        store.publishIfCurrent({"doc:test", "Assembly"}, replacement, {}, diagnostic),
        CadX::StoreError::RevisionMismatch
    );
    EXPECT_EQ(
        store.current({"doc:test", "Assembly"}, true).snapshot->header().graphRevision,
        first->header().graphRevision
    );
}

TEST(CadXGraphStore, CompareAndSwapAllowsOnlyOneConcurrentWriter)
{
    CadX::GraphStore store;
    auto first = makeSnapshot("first");
    std::string diagnostic;
    ASSERT_EQ(store.publish({"doc:test", "Assembly"}, first, diagnostic), CadX::StoreError::None);
    const auto parent = first->header().graphRevision;
    auto left = makeSnapshot("left");
    auto right = makeSnapshot("right");
    CadX::StoreError leftResult = CadX::StoreError::None;
    CadX::StoreError rightResult = CadX::StoreError::None;
    std::thread leftThread([&] {
        std::string local;
        leftResult = store.publishIfCurrent({"doc:test", "Assembly"}, left, parent, local);
    });
    std::thread rightThread([&] {
        std::string local;
        rightResult = store.publishIfCurrent({"doc:test", "Assembly"}, right, parent, local);
    });
    leftThread.join();
    rightThread.join();
    EXPECT_NE(leftResult == CadX::StoreError::None, rightResult == CadX::StoreError::None);
}
