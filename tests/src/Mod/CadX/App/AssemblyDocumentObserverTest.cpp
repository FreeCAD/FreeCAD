// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Mod/CadX/App/AssemblyDocumentObserver.h>

namespace
{
std::shared_ptr<CadX::GraphSnapshot> snapshot(const std::string& uid, const std::string& name)
{
    auto result = std::make_shared<CadX::GraphSnapshot>();
    result->header().graphId = "graph:" + uid;
    result->header().documentUid = uid;
    result->header().activeAssemblyObjectName = name;
    result->header().activeAssemblyNodeId = "assembly";
    CadX::NodeRecord root;
    root.id = "assembly";
    root.kind = CadX::NodeKind::AssemblyDefinition;
    root.native = {uid, name, "Assembly::AssemblyObject"};
    root.display = {name, name};
    result->nodes().push_back(root);
    std::string diagnostic;
    EXPECT_TRUE(result->finalize(diagnostic)) << diagnostic;
    return result;
}
}  // namespace

TEST(CadXAssemblyDocumentObserver, InvalidatesRootAndSourceScopes)
{
    CadX::GraphStore store;
    const auto root = snapshot("doc:root", "Assembly");
    const auto source = snapshot("doc:other", "Assembly");
    std::string diagnostic;
    ASSERT_EQ(store.publish({"doc:root", "Assembly"}, root, diagnostic), CadX::StoreError::None);
    ASSERT_EQ(store.publish({"doc:other", "Assembly"}, source, diagnostic), CadX::StoreError::None);

    {
        CadX::AssemblyDocumentObserver observer(store);
        observer.documentChanged({"doc:root", "Assembly"}, "root changed");
        EXPECT_EQ(store.current({"doc:root", "Assembly"}).error, CadX::StoreError::GraphStale);
        observer.sourceDocumentChanged("doc:other", "source changed");
        EXPECT_EQ(store.current({"doc:other", "Assembly"}).error, CadX::StoreError::GraphStale);
    }

    EXPECT_TRUE(store.current({"doc:root", "Assembly"}, true));
    EXPECT_TRUE(store.current({"doc:other", "Assembly"}, true));
}
