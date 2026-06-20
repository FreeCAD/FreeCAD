// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Gui/ToolBarManager.h>


using Gui::ToolBarManager;

namespace
{

void expectPersistenceId(
    const ToolBarManager::PersistenceId& actual,
    ToolBarManager::Scope scope,
    const QString& toolbar,
    const QString& workbench = {},
    const QString& context = {},
    ToolBarManager::PersistenceId::SharedPrefix sharedPrefix
    = ToolBarManager::PersistenceId::SharedPrefix::Shared
)
{
    EXPECT_EQ(actual.scopeId.scope, scope);
    EXPECT_EQ(actual.toolbar, toolbar);
    EXPECT_EQ(actual.scopeId.workbench, workbench);
    EXPECT_EQ(actual.scopeId.context, context);
    EXPECT_EQ(actual.sharedPrefix, sharedPrefix);
}

}  // namespace

TEST(ToolBarManagerPersistenceId, parsesAndSerializesLegacyKey)
{
    const auto id = ToolBarManager::toolBarPersistenceId(QStringLiteral("Sketcher"));

    ASSERT_FALSE(id.isEmpty());
    expectPersistenceId(id, ToolBarManager::Scope::Legacy, QStringLiteral("Sketcher"));
    EXPECT_EQ(ToolBarManager::makeToolBarPersistenceKey(id), QStringLiteral("Sketcher"));
}

TEST(ToolBarManagerPersistenceId, preservesSharedPrefixes)
{
    const auto shared = ToolBarManager::toolBarPersistenceId(QStringLiteral("shared:View"));
    const auto global = ToolBarManager::toolBarPersistenceId(QStringLiteral("global:Custom"));

    ASSERT_FALSE(shared.isEmpty());
    ASSERT_FALSE(global.isEmpty());

    expectPersistenceId(
        shared,
        ToolBarManager::Scope::Shared,
        QStringLiteral("View"),
        {},
        {},
        ToolBarManager::PersistenceId::SharedPrefix::Shared
    );
    expectPersistenceId(
        global,
        ToolBarManager::Scope::Shared,
        QStringLiteral("Custom"),
        {},
        {},
        ToolBarManager::PersistenceId::SharedPrefix::Global
    );

    EXPECT_EQ(ToolBarManager::makeToolBarPersistenceKey(shared), QStringLiteral("shared:View"));
    EXPECT_EQ(ToolBarManager::makeToolBarPersistenceKey(global), QStringLiteral("global:Custom"));
}

TEST(ToolBarManagerPersistenceId, parsesAndSerializesWorkbenchKey)
{
    const auto id = ToolBarManager::toolBarPersistenceId(
        QStringLiteral("wb:SketcherWorkbench:Sketcher")
    );

    ASSERT_FALSE(id.isEmpty());
    expectPersistenceId(
        id,
        ToolBarManager::Scope::Workbench,
        QStringLiteral("Sketcher"),
        QStringLiteral("SketcherWorkbench")
    );
    EXPECT_EQ(
        ToolBarManager::makeToolBarPersistenceKey(id),
        QStringLiteral("wb:SketcherWorkbench:Sketcher")
    );
}

TEST(ToolBarManagerPersistenceId, parsesAndSerializesContextualKey)
{
    const auto id = ToolBarManager::toolBarPersistenceId(
        QStringLiteral("ctx:SketcherWorkbench:task:edit:Constraints")
    );

    ASSERT_FALSE(id.isEmpty());
    expectPersistenceId(
        id,
        ToolBarManager::Scope::Contextual,
        QStringLiteral("Constraints"),
        QStringLiteral("SketcherWorkbench"),
        QStringLiteral("task:edit")
    );
    EXPECT_EQ(
        ToolBarManager::makeToolBarPersistenceKey(id),
        QStringLiteral("ctx:SketcherWorkbench:task:edit:Constraints")
    );
}

TEST(ToolBarManagerPersistenceId, invalidScopedKeysFallBackToLegacyToolbarIdentity)
{
    const auto incompleteWorkbench = ToolBarManager::toolBarPersistenceId(
        QStringLiteral("wb:SketcherWorkbench")
    );
    const auto incompleteContext = ToolBarManager::toolBarPersistenceId(
        QStringLiteral("ctx:SketcherWorkbench:edit")
    );
    const auto empty = ToolBarManager::toolBarPersistenceId(QString());

    ASSERT_FALSE(incompleteWorkbench.isEmpty());
    ASSERT_FALSE(incompleteContext.isEmpty());
    EXPECT_TRUE(empty.isEmpty());

    expectPersistenceId(
        incompleteWorkbench,
        ToolBarManager::Scope::Legacy,
        QStringLiteral("SketcherWorkbench")
    );
    expectPersistenceId(incompleteContext, ToolBarManager::Scope::Legacy, QStringLiteral("edit"));
    expectPersistenceId(empty, ToolBarManager::Scope::Legacy, {});

    EXPECT_EQ(
        ToolBarManager::makeToolBarPersistenceKey(incompleteWorkbench),
        QStringLiteral("SketcherWorkbench")
    );
    EXPECT_EQ(ToolBarManager::makeToolBarPersistenceKey(incompleteContext), QStringLiteral("edit"));
    EXPECT_TRUE(ToolBarManager::makeToolBarPersistenceKey(empty).isEmpty());
}

TEST(ToolBarManagerPersistenceId, parsesAndSerializesLayoutContexts)
{
    const auto workbench = ToolBarManager::layoutContextId(QStringLiteral("SketcherWorkbench"));
    const auto contextual = ToolBarManager::layoutContextId(
        QStringLiteral("ctx:SketcherWorkbench:edit")
    );

    ASSERT_FALSE(workbench.isEmpty());
    EXPECT_EQ(workbench.scope, ToolBarManager::Scope::Workbench);
    EXPECT_EQ(workbench.workbench, QStringLiteral("SketcherWorkbench"));
    EXPECT_TRUE(workbench.context.isEmpty());
    EXPECT_EQ(ToolBarManager::makeToolBarLayoutContext(workbench), QStringLiteral("SketcherWorkbench"));

    ASSERT_FALSE(contextual.isEmpty());
    EXPECT_EQ(contextual.scope, ToolBarManager::Scope::Contextual);
    EXPECT_EQ(contextual.workbench, QStringLiteral("SketcherWorkbench"));
    EXPECT_EQ(contextual.context, QStringLiteral("edit"));
    EXPECT_EQ(
        ToolBarManager::makeToolBarLayoutContext(contextual),
        QStringLiteral("ctx:SketcherWorkbench:edit")
    );
}

TEST(ToolBarManagerPersistenceId, invalidLayoutContextsSerializeToEmpty)
{
    EXPECT_TRUE(
        ToolBarManager::makeToolBarLayoutContext({ToolBarManager::Scope::Legacy, {}, {}}).isEmpty()
    );
    EXPECT_TRUE(
        ToolBarManager::makeToolBarLayoutContext({ToolBarManager::Scope::Shared, {}, {}}).isEmpty()
    );
    EXPECT_TRUE(
        ToolBarManager::makeToolBarLayoutContext(
            {ToolBarManager::Scope::Workbench, {}, QStringLiteral("edit")}
        )
            .isEmpty()
    );
    EXPECT_TRUE(
        ToolBarManager::makeToolBarLayoutContext(
            {ToolBarManager::Scope::Contextual, QStringLiteral("SketcherWorkbench"), {}}
        ).isEmpty()
    );
}
