/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QHBoxLayout>
#include <QMap>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QSet>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QStyleOption>


#include <algorithm>
#include <tuple>
#include <boost/algorithm/string/predicate.hpp>

#include <Base/Tools.h>

#include "ToolBarManager.h"
#include "ToolBarAreaWidget.h"
#include "Application.h"
#include "Command.h"
#include "MainWindow.h"
#include "OverlayWidgets.h"
#include "Workbench.h"
#include "WorkbenchManager.h"
#include "WidgetFactory.h"


using namespace Gui;

namespace
{
constexpr auto ToolBarPersistenceKeyProperty = "_fc_toolbar_persistence_key";
constexpr auto ToolBarPublicPersistenceKeyProperty = "PersistenceKey";
constexpr auto ToolBarTierProperty = "_fc_toolbar_tier";
constexpr auto ToolBarPublicTierProperty = "Tier";

QStringList splitLayoutState(const std::string& value)
{
    if (value.empty()) {
        return {};
    }

    return QString::fromUtf8(value.c_str()).split(QLatin1Char(','), Qt::SkipEmptyParts);
}

struct ToolBarLayoutEntry
{
    bool toolbarBreak = false;
    QString serializedKey;
    ToolBarManager::PersistenceId id;
};

struct ToolBarLayoutState
{
    bool saved = false;
    QList<ToolBarLayoutEntry> top;
    QList<ToolBarLayoutEntry> left;
    QList<ToolBarLayoutEntry> right;
    QList<ToolBarLayoutEntry> bottom;
};

struct HostedToolBarPlacement
{
    ToolBarArea area = ToolBarArea::NoToolBarArea;
    int index = -1;
};

struct ResolvedToolBarRestoreState
{
    ToolBarLayoutState mainWindowLayout;
    QMap<QString, HostedToolBarPlacement> hostedToolBarPlacements;
    QMap<QString, bool> statusBarWidgetVisibility;
    QMap<QString, bool> menuBarLeftWidgetVisibility;
    QMap<QString, bool> menuBarRightWidgetVisibility;
};

const auto SharedScopeToken = QStringLiteral("shared");
const auto GlobalScopeToken = QStringLiteral("global");
const auto WorkbenchScopeToken = QStringLiteral("wb");
const auto ContextualScopeToken = QStringLiteral("ctx");
const auto ContextualLayoutContextPrefix = QStringLiteral("ctx:");

QString serializeToolBarPersistenceId(const ToolBarManager::PersistenceId& id);
void moveToolBarPreservingVisibility(MainWindow* mainWindow, QToolBar* toolbar, Qt::ToolBarArea area);

ToolBarManager::ToolbarScopeId tryParseContextualLayoutContext(const QString& context)
{
    const auto parts = context.split(QLatin1Char(':'), Qt::KeepEmptyParts);
    if (parts.size() < 3) {
        return {};
    }

    return ToolBarManager::ToolbarScopeId::forContextual(
        parts.at(1),
        parts.mid(2).join(QLatin1Char(':'))
    );
}

ToolBarManager::PersistenceId makeToolBarPersistenceId(const QString& persistenceKey)
{
    ToolBarManager::PersistenceId id;
    if (persistenceKey.isEmpty()) {
        return id;
    }

    const auto parts = persistenceKey.split(QLatin1Char(':'), Qt::KeepEmptyParts);
    if (parts.isEmpty()) {
        return id;
    }

    const auto scope = parts.front();
    if (scope == SharedScopeToken || scope == GlobalScopeToken) {
        id.scopeId.scope = ToolBarManager::Scope::Shared;
        id.toolbar = parts.back();
        id.sharedPrefix = (scope == GlobalScopeToken)
            ? ToolBarManager::PersistenceId::SharedPrefix::Global
            : ToolBarManager::PersistenceId::SharedPrefix::Shared;
        return id;
    }

    if (scope == WorkbenchScopeToken && parts.size() >= 3) {
        id.scopeId.scope = ToolBarManager::Scope::Workbench;
        id.scopeId.workbench = parts.at(1);
        id.toolbar = parts.back();
        return id;
    }

    if (scope == ContextualScopeToken && parts.size() >= 4) {
        id.scopeId.scope = ToolBarManager::Scope::Contextual;
        id.scopeId.workbench = parts.at(1);
        id.scopeId.context = parts.mid(2, parts.size() - 3).join(QLatin1Char(':'));
        id.toolbar = parts.back();
        return id;
    }

    id.toolbar = parts.back();
    return id;
}

QString layoutEntryKey(const ToolBarLayoutEntry& entry)
{
    if (entry.toolbarBreak) {
        return QStringLiteral("Break");
    }

    if (!entry.serializedKey.isEmpty()) {
        return entry.serializedKey;
    }

    return serializeToolBarPersistenceId(entry.id);
}

ToolBarLayoutEntry makeToolBarLayoutEntry(const QString& entry)
{
    if (entry == QStringLiteral("Break")) {
        ToolBarLayoutEntry layoutEntry;
        layoutEntry.toolbarBreak = true;
        return layoutEntry;
    }

    ToolBarLayoutEntry layoutEntry;
    layoutEntry.serializedKey = entry;
    layoutEntry.id = makeToolBarPersistenceId(entry);
    return layoutEntry;
}

ToolBarLayoutEntry makeToolBarLayoutEntry(const ToolBarManager::PersistenceId& id)
{
    ToolBarLayoutEntry layoutEntry;
    layoutEntry.id = id;
    return layoutEntry;
}

QList<ToolBarLayoutEntry> readToolBarLayoutEntries(const std::string& value)
{
    QList<ToolBarLayoutEntry> entries;
    for (const auto& entry : splitLayoutState(value)) {
        entries.push_back(makeToolBarLayoutEntry(entry));
    }
    return entries;
}

void writeToolBarLayoutEntries(
    const ParameterGrp::handle& group,
    const char* key,
    const QList<ToolBarLayoutEntry>& entries
)
{
    QStringList layout;
    for (const auto& entry : entries) {
        const auto serializedKey = layoutEntryKey(entry);
        if (!serializedKey.isEmpty()) {
            layout << serializedKey;
        }
    }
    group->SetASCII(key, layout.join(QLatin1Char(',')).toUtf8().constData());
}

ToolBarLayoutState readToolBarLayoutState(const ParameterGrp::handle& group)
{
    if (!group) {
        return {};
    }

    return {
        .saved = group->GetBool("Saved", false),
        .top = readToolBarLayoutEntries(group->GetASCII("Top")),
        .left = readToolBarLayoutEntries(group->GetASCII("Left")),
        .right = readToolBarLayoutEntries(group->GetASCII("Right")),
        .bottom = readToolBarLayoutEntries(group->GetASCII("Bottom")),
    };
}

void writeToolBarLayoutState(const ParameterGrp::handle& group, const ToolBarLayoutState& state)
{
    if (!group) {
        return;
    }

    group->SetBool("Saved", state.saved);
    writeToolBarLayoutEntries(group, "Top", state.top);
    writeToolBarLayoutEntries(group, "Left", state.left);
    writeToolBarLayoutEntries(group, "Right", state.right);
    writeToolBarLayoutEntries(group, "Bottom", state.bottom);
}

QString toolBarScopeLabel(ToolBarManager::Scope scope)
{
    switch (scope) {
        case ToolBarManager::Scope::Shared:
            return QApplication::translate("MainWindow", "Shared");
        case ToolBarManager::Scope::Workbench:
            return QApplication::translate("MainWindow", "Workbench");
        case ToolBarManager::Scope::Contextual:
            return QApplication::translate("MainWindow", "Contextual");
        case ToolBarManager::Scope::Legacy:
            return QApplication::translate("MainWindow", "Unscoped");
    }

    return {};
}

QString serializeToolBarPersistenceId(const ToolBarManager::PersistenceId& id)
{
    if (id.toolbar.isEmpty()) {
        return {};
    }

    QStringList segments;
    switch (id.scopeId.scope) {
        case ToolBarManager::Scope::Shared:
            segments.push_back(
                id.sharedPrefix == ToolBarManager::PersistenceId::SharedPrefix::Global
                    ? GlobalScopeToken
                    : SharedScopeToken
            );
            break;
        case ToolBarManager::Scope::Workbench:
            if (id.scopeId.workbench.isEmpty()) {
                return {};
            }
            segments.push_back(WorkbenchScopeToken);
            segments.push_back(id.scopeId.workbench);
            break;
        case ToolBarManager::Scope::Contextual:
            if (id.scopeId.workbench.isEmpty() || id.scopeId.context.isEmpty()) {
                return {};
            }
            segments.push_back(ContextualScopeToken);
            segments.push_back(id.scopeId.workbench);
            segments.push_back(id.scopeId.context);
            break;
        case ToolBarManager::Scope::Legacy:
            break;
    }

    segments.push_back(id.toolbar);
    return segments.join(QLatin1Char(':'));
}

QString serializeLayoutContextId(const ToolBarManager::ToolbarScopeId& scopeId)
{
    switch (scopeId.scope) {
        case ToolBarManager::Scope::Workbench:
            if (scopeId.workbench.isEmpty() || !scopeId.context.isEmpty()) {
                return {};
            }
            return scopeId.workbench;
        case ToolBarManager::Scope::Contextual:
            if (scopeId.workbench.isEmpty() || scopeId.context.isEmpty()) {
                return {};
            }
            return QStringLiteral("%1:%2:%3")
                .arg(ContextualScopeToken, scopeId.workbench, scopeId.context);
        case ToolBarManager::Scope::Legacy:
        case ToolBarManager::Scope::Shared:
            return {};
    }

    return {};
}

bool isValidToolbarLayoutContext(const ToolBarManager::ToolbarScopeId& scopeId)
{
    return !serializeLayoutContextId(scopeId).isEmpty();
}

ToolBarManager::ToolbarScopeId usableToolbarLayoutContext(const ToolBarManager::ToolbarScopeId& scopeId)
{
    return isValidToolbarLayoutContext(scopeId) ? scopeId : ToolBarManager::ToolbarScopeId {};
}

bool toolBarParticipatesInLayoutContext(
    const QToolBar* toolbar,
    const ToolBarManager::ToolbarScopeId& context
)
{
    if (!toolbar) {
        return false;
    }

    const auto usableContext = usableToolbarLayoutContext(context);
    if (usableContext.isEmpty()) {
        return true;
    }

    const auto toolbarScope = ToolBarManager::toolBarScopeId(toolbar);
    switch (toolbarScope.scope) {
        case ToolBarManager::Scope::Legacy:
        case ToolBarManager::Scope::Shared:
            return true;
        case ToolBarManager::Scope::Workbench:
        case ToolBarManager::Scope::Contextual:
            return toolbarScope == usableContext;
    }

    return false;
}

ToolBarItem::Tier defaultToolBarTier(ToolBarItem::DefaultVisibility visibility)
{
    switch (visibility) {
        case ToolBarItem::DefaultVisibility::Visible:
            return ToolBarItem::Tier::Recommended;
        case ToolBarItem::DefaultVisibility::Hidden:
            return ToolBarItem::Tier::Secondary;
        case ToolBarItem::DefaultVisibility::Unavailable:
            return ToolBarItem::Tier::Contextual;
    }

    return ToolBarItem::Tier::Recommended;
}

QString toolBarTierName(ToolBarItem::Tier tier)
{
    switch (tier) {
        case ToolBarItem::Tier::Recommended:
            return QStringLiteral("recommended");
        case ToolBarItem::Tier::Secondary:
            return QStringLiteral("secondary");
        case ToolBarItem::Tier::Advanced:
            return QStringLiteral("advanced");
        case ToolBarItem::Tier::Contextual:
            return QStringLiteral("contextual");
    }

    return {};
}

ToolBarItem::Tier parseToolBarTier(const QString& tierName)
{
    if (tierName == QLatin1String("secondary")) {
        return ToolBarItem::Tier::Secondary;
    }
    if (tierName == QLatin1String("advanced")) {
        return ToolBarItem::Tier::Advanced;
    }
    if (tierName == QLatin1String("contextual")) {
        return ToolBarItem::Tier::Contextual;
    }

    return ToolBarItem::Tier::Recommended;
}

QString toolBarTierLabel(ToolBarItem::Tier tier)
{
    switch (tier) {
        case ToolBarItem::Tier::Recommended:
            return QApplication::translate("MainWindow", "Recommended");
        case ToolBarItem::Tier::Secondary:
            return QApplication::translate("MainWindow", "Secondary");
        case ToolBarItem::Tier::Advanced:
            return QApplication::translate("MainWindow", "Advanced");
        case ToolBarItem::Tier::Contextual:
            return QApplication::translate("MainWindow", "Contextual");
    }

    return {};
}

QString decoratedToolBarActionText(const QToolBar* toolbar)
{
    if (!toolbar) {
        return {};
    }

    auto action = toolbar->toggleViewAction();
    if (!action) {
        return {};
    }

    const auto text = action->text();
    const auto tier = ToolBarManager::toolBarTier(toolbar);
    if (tier == ToolBarItem::Tier::Recommended || tier == ToolBarItem::Tier::Contextual) {
        return text;
    }

    const auto tierLabel = ToolBarManager::toolBarTierLabel(tier);
    if (text.isEmpty() || tierLabel.isEmpty()) {
        return text;
    }

    return QApplication::translate("MainWindow", "%1 (%2)").arg(text, tierLabel);
}

QString legacyToolBarKey(const QToolBar* toolbar)
{
    if (!toolbar) {
        return {};
    }

    const auto legacyKey = toolbar->objectName();
    if (legacyKey.isEmpty() || legacyKey == ToolBarManager::toolBarPersistenceKey(toolbar)) {
        return {};
    }

    return legacyKey;
}

QString stringPropertyValue(const QObject* object, const char* propertyName)
{
    if (!object || !propertyName) {
        return {};
    }

    const auto property = object->property(propertyName);
    if (!property.isValid()) {
        return {};
    }

    return property.toString();
}

template<typename T, typename MapGetter>
QMap<QString, T> toLookup(const ParameterGrp::handle& group, MapGetter&& getMap)
{
    QMap<QString, T> values;
    if (!group) {
        return values;
    }

    for (const auto& [key, value] : getMap(group)) {
        values.insert(QString::fromUtf8(key.c_str()), static_cast<T>(value));
    }

    return values;
}

template<typename T>
bool lookupValue(const QMap<QString, T>& values, const QString& key, T* result)
{
    auto it = values.constFind(key);
    if (it == values.cend()) {
        return false;
    }

    if (result) {
        *result = it.value();
    }

    return true;
}

template<typename T>
void overlayLookup(QMap<QString, T>& values, const QMap<QString, T>& overlay)
{
    for (auto it = overlay.cbegin(); it != overlay.cend(); ++it) {
        values.insert(it.key(), it.value());
    }
}

template<typename T>
bool lookupToolBarValue(
    const QMap<QString, T>& primaryValues,
    const QMap<QString, T>& fallbackValues,
    const QToolBar* toolbar,
    T* result
)
{
    if (!toolbar) {
        return false;
    }

    const auto key = ToolBarManager::toolBarPersistenceKey(toolbar);
    if (!key.isEmpty() && lookupValue(primaryValues, key, result)) {
        return true;
    }

    const auto legacyKey = legacyToolBarKey(toolbar);
    if (!legacyKey.isEmpty() && lookupValue(primaryValues, legacyKey, result)) {
        return true;
    }

    if (!key.isEmpty() && lookupValue(fallbackValues, key, result)) {
        return true;
    }

    if (!legacyKey.isEmpty() && lookupValue(fallbackValues, legacyKey, result)) {
        return true;
    }

    return false;
}

template<typename T>
QMap<QString, T> remapLegacyLookup(const QMap<QString, T>& values, const QMap<QString, QString>& aliases)
{
    QMap<QString, T> normalized;
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        normalized.insert(aliases.value(it.key(), it.key()), it.value());
    }
    return normalized;
}

QList<ToolBarLayoutEntry> remapLegacyLayoutEntries(
    const QList<ToolBarLayoutEntry>& layout,
    const QMap<QString, QString>& aliases
)
{
    QList<ToolBarLayoutEntry> normalized;
    QSet<QString> knownKeys;
    for (const auto& entry : layout) {
        if (entry.toolbarBreak) {
            normalized << entry;
            continue;
        }

        const auto mappedEntry = aliases.value(layoutEntryKey(entry), layoutEntryKey(entry));
        if (!knownKeys.contains(mappedEntry)) {
            normalized << makeToolBarLayoutEntry(mappedEntry);
            knownKeys.insert(mappedEntry);
        }
    }

    return normalized;
}

ToolBarLayoutState remapLegacyLayoutState(
    const ToolBarLayoutState& state,
    const QMap<QString, QString>& aliases
)
{
    return {
        .saved = state.saved,
        .top = remapLegacyLayoutEntries(state.top, aliases),
        .left = remapLegacyLayoutEntries(state.left, aliases),
        .right = remapLegacyLayoutEntries(state.right, aliases),
        .bottom = remapLegacyLayoutEntries(state.bottom, aliases),
    };
}

QSet<QString> layoutStateToolBarKeys(const ToolBarLayoutState& state)
{
    QSet<QString> keys;
    auto rememberKeys = [&keys](const QList<ToolBarLayoutEntry>& layout) {
        for (const auto& entry : layout) {
            const auto key = layoutEntryKey(entry);
            if (key != QStringLiteral("Break")) {
                keys.insert(key);
            }
        }
    };

    rememberKeys(state.top);
    rememberKeys(state.left);
    rememberKeys(state.right);
    rememberKeys(state.bottom);

    return keys;
}

QMap<QString, QString> buildLegacyToolBarAliases(const QList<ToolBar*>& toolbars)
{
    QMap<QString, QString> aliases;
    for (auto toolbar : toolbars) {
        const auto key = ToolBarManager::toolBarPersistenceKey(toolbar);
        if (key.isEmpty()) {
            continue;
        }

        if (const auto legacyKey = legacyToolBarKey(toolbar); !legacyKey.isEmpty()) {
            aliases.insert(legacyKey, key);
        }
    }

    return aliases;
}

void overlayHostedToolBarPlacements(
    QMap<QString, HostedToolBarPlacement>& placements,
    const QMap<QString, int>& values,
    ToolBarArea area
)
{
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        if (it.value() < 0) {
            continue;
        }

        placements.insert(it.key(), HostedToolBarPlacement {area, it.value()});
    }
}

ResolvedToolBarRestoreState resolveToolBarRestoreState(
    const ParameterGrp::handle& workbenchGroup,
    const ParameterGrp::handle& statusBarGroup,
    const ParameterGrp::handle& menuBarLeftGroup,
    const ParameterGrp::handle& menuBarRightGroup,
    const ParameterGrp::handle& globalStatusBarGroup,
    const ParameterGrp::handle& globalMenuBarLeftGroup,
    const ParameterGrp::handle& globalMenuBarRightGroup,
    const QList<ToolBar*>& toolbars
)
{
    ResolvedToolBarRestoreState state;
    const auto legacyAliases = buildLegacyToolBarAliases(toolbars);

    if (workbenchGroup) {
        state.mainWindowLayout
            = remapLegacyLayoutState(readToolBarLayoutState(workbenchGroup), legacyAliases);
    }

    overlayHostedToolBarPlacements(
        state.hostedToolBarPlacements,
        remapLegacyLookup(
            toLookup<int>(globalStatusBarGroup, [](const auto& group) { return group->GetIntMap(); }),
            legacyAliases
        ),
        ToolBarArea::StatusBarToolBarArea
    );
    overlayHostedToolBarPlacements(
        state.hostedToolBarPlacements,
        remapLegacyLookup(
            toLookup<int>(
                globalMenuBarLeftGroup,
                [](const auto& group) { return group->GetIntMap(); }
            ),
            legacyAliases
        ),
        ToolBarArea::LeftMenuToolBarArea
    );
    overlayHostedToolBarPlacements(
        state.hostedToolBarPlacements,
        remapLegacyLookup(
            toLookup<int>(
                globalMenuBarRightGroup,
                [](const auto& group) { return group->GetIntMap(); }
            ),
            legacyAliases
        ),
        ToolBarArea::RightMenuToolBarArea
    );
    overlayHostedToolBarPlacements(
        state.hostedToolBarPlacements,
        remapLegacyLookup(
            toLookup<int>(statusBarGroup, [](const auto& group) { return group->GetIntMap(); }),
            legacyAliases
        ),
        ToolBarArea::StatusBarToolBarArea
    );
    overlayHostedToolBarPlacements(
        state.hostedToolBarPlacements,
        remapLegacyLookup(
            toLookup<int>(menuBarLeftGroup, [](const auto& group) { return group->GetIntMap(); }),
            legacyAliases
        ),
        ToolBarArea::LeftMenuToolBarArea
    );
    overlayHostedToolBarPlacements(
        state.hostedToolBarPlacements,
        remapLegacyLookup(
            toLookup<int>(menuBarRightGroup, [](const auto& group) { return group->GetIntMap(); }),
            legacyAliases
        ),
        ToolBarArea::RightMenuToolBarArea
    );

    overlayLookup(
        state.statusBarWidgetVisibility,
        toLookup<bool>(globalStatusBarGroup, [](const auto& group) { return group->GetBoolMap(); })
    );
    overlayLookup(
        state.statusBarWidgetVisibility,
        toLookup<bool>(statusBarGroup, [](const auto& group) { return group->GetBoolMap(); })
    );
    overlayLookup(
        state.menuBarLeftWidgetVisibility,
        toLookup<bool>(globalMenuBarLeftGroup, [](const auto& group) { return group->GetBoolMap(); })
    );
    overlayLookup(
        state.menuBarLeftWidgetVisibility,
        toLookup<bool>(menuBarLeftGroup, [](const auto& group) { return group->GetBoolMap(); })
    );
    overlayLookup(
        state.menuBarRightWidgetVisibility,
        toLookup<bool>(globalMenuBarRightGroup, [](const auto& group) { return group->GetBoolMap(); })
    );
    overlayLookup(
        state.menuBarRightWidgetVisibility,
        toLookup<bool>(menuBarRightGroup, [](const auto& group) { return group->GetBoolMap(); })
    );

    if (state.mainWindowLayout.saved) {
        for (const auto& key : layoutStateToolBarKeys(state.mainWindowLayout)) {
            state.hostedToolBarPlacements.remove(key);
        }
    }

    return state;
}

void restoreMainWindowToolBarLayout(
    const ToolBarLayoutState& layoutState,
    const QList<ToolBar*>& currentToolbars,
    MainWindow* mainWindow,
    const ToolBarManager::ToolbarScopeId& context
)
{
    if (!mainWindow || !layoutState.saved) {
        return;
    }

    QMap<QString, ToolBar*> mainWindowToolbars;
    for (auto toolbar : std::as_const(currentToolbars)) {
        auto key = ToolBarManager::toolBarPersistenceKey(toolbar);
        if (key.isEmpty() || toolbar->isFloating() || toolbar->parentWidget() != mainWindow) {
            continue;
        }
        if (!toolBarParticipatesInLayoutContext(toolbar, context)) {
            continue;
        }

        mainWindowToolbars.insert(key, toolbar);
    }

    if (mainWindowToolbars.isEmpty()) {
        return;
    }

    // Saved break entries are authoritative; clear startup/default breaks before replaying them.
    for (auto it = mainWindowToolbars.cbegin(); it != mainWindowToolbars.cend(); ++it) {
        mainWindow->removeToolBarBreak(it.value());
    }

    auto top = layoutState.top;
    auto left = layoutState.left;
    auto right = layoutState.right;
    auto bottom = layoutState.bottom;

    QSet<QString> knownKeys = layoutStateToolBarKeys(layoutState);
    auto appendMissing = [&mainWindowToolbars,
                          &knownKeys,
                          &currentToolbars,
                          mainWindow](QList<ToolBarLayoutEntry>& layout, Qt::ToolBarArea area) {
        for (auto toolbar : currentToolbars) {
            auto key = ToolBarManager::toolBarPersistenceKey(toolbar);
            if (!mainWindowToolbars.contains(key) || knownKeys.contains(key)) {
                continue;
            }
            if (mainWindow->toolBarArea(toolbar) == area) {
                layout << makeToolBarLayoutEntry(ToolBarManager::toolBarPersistenceId(toolbar));
                knownKeys.insert(key);
            }
        }
    };
    appendMissing(top, Qt::TopToolBarArea);
    appendMissing(left, Qt::LeftToolBarArea);
    appendMissing(right, Qt::RightToolBarArea);
    appendMissing(bottom, Qt::BottomToolBarArea);

    auto restore = [&mainWindowToolbars,
                    mainWindow](const QList<ToolBarLayoutEntry>& layout, Qt::ToolBarArea area) {
        for (const auto& entry : layout) {
            const auto key = layoutEntryKey(entry);
            if (key == QStringLiteral("Break")) {
                mainWindow->addToolBarBreak(area);
                continue;
            }

            auto toolbar = mainWindowToolbars.value(key);
            if (!toolbar) {
                continue;
            }

            moveToolBarPreservingVisibility(mainWindow, toolbar, area);
        }
    };

    restore(top, Qt::TopToolBarArea);
    restore(left, Qt::LeftToolBarArea);
    restore(right, Qt::RightToolBarArea);
    restore(bottom, Qt::BottomToolBarArea);
}

void moveToolBarPreservingVisibility(MainWindow* mainWindow, QToolBar* toolbar, Qt::ToolBarArea area)
{
    bool visible = toolbar->isVisible();
    if (!visible) {
        // QMainWindow does not reliably re-place hidden toolbars when restoring a saved layout.
        toolbar->setVisible(true);
    }

    mainWindow->addToolBar(area, toolbar);
    toolbar->setVisible(visible);
}
}  // namespace

ToolBarItem::ToolBarItem()
    : visibilityPolicy(DefaultVisibility::Visible)
    , _tier(defaultToolBarTier(visibilityPolicy))
{}

ToolBarItem::ToolBarItem(ToolBarItem* item, DefaultVisibility visibilityPolicy)
    : visibilityPolicy(visibilityPolicy)
    , _tier(defaultToolBarTier(visibilityPolicy))
{
    if (item) {
        item->appendItem(this);
    }
}

ToolBarItem::~ToolBarItem()
{
    clear();
}

void ToolBarItem::setCommand(const std::string& name)
{
    _name = name;
}

const std::string& ToolBarItem::command() const
{
    return _name;
}

bool ToolBarItem::hasPersistenceKey() const
{
    return !_persistenceKey.empty();
}

void ToolBarItem::setPersistenceKey(const std::string& key)
{
    _persistenceKey = key;
}

const std::string& ToolBarItem::persistenceKey() const
{
    if (_persistenceKey.empty()) {
        return _name;
    }

    return _persistenceKey;
}

void ToolBarItem::setTier(Tier tier)
{
    _tier = tier;
}

ToolBarItem::Tier ToolBarItem::tier() const
{
    return _tier;
}

bool ToolBarItem::hasItems() const
{
    return !_items.isEmpty();
}

ToolBarItem* ToolBarItem::findItem(const std::string& name)
{
    if (_name == name) {
        return this;
    }

    for (auto it : std::as_const(_items)) {
        if (it->_name == name) {
            return it;
        }
    }

    return nullptr;
}

ToolBarItem* ToolBarItem::copy() const
{
    auto root = new ToolBarItem;
    root->setCommand(command());
    if (!_persistenceKey.empty()) {
        root->setPersistenceKey(_persistenceKey);
    }
    root->setTier(_tier);

    QList<ToolBarItem*> items = getItems();
    for (auto it : items) {
        root->appendItem(it->copy());
    }

    return root;
}

uint ToolBarItem::count() const
{
    return _items.count();
}

void ToolBarItem::appendItem(ToolBarItem* item)
{
    _items.push_back(item);
}

bool ToolBarItem::insertItem(ToolBarItem* before, ToolBarItem* item)
{
    int pos = _items.indexOf(before);
    if (pos != -1) {
        _items.insert(pos, item);
        return true;
    }

    return false;
}

void ToolBarItem::removeItem(ToolBarItem* item)
{
    int pos = _items.indexOf(item);
    if (pos != -1) {
        _items.removeAt(pos);
    }
}

void ToolBarItem::clear()
{
    for (auto it : std::as_const(_items)) {
        delete it;
    }

    _items.clear();
}

ToolBarItem& ToolBarItem::operator<<(ToolBarItem* item)
{
    appendItem(item);
    return *this;
}

ToolBarItem& ToolBarItem::operator<<(const std::string& command)
{
    auto item = new ToolBarItem(this);
    item->setCommand(command);
    return *this;
}

QList<ToolBarItem*> ToolBarItem::getItems() const
{
    return _items;
}

// -----------------------------------------------------------

ToolBar::ToolBar()
    : QToolBar()
{
    setupConnections();
}

ToolBar::ToolBar(QWidget* parent)
    : QToolBar(parent)
{
    setupConnections();
}

void ToolBar::undock()
{
    {
        // We want to block only some signals - topLevelChanged should still be propagated
        QSignalBlocker blocker(this);

        if (auto area = ToolBarManager::getInstance()->toolBarAreaWidget(this)) {
            area->removeWidget(this);
            getMainWindow()->addToolBar(this);
        }

        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
        adjustSize();
        setVisible(true);
    }

    Q_EMIT topLevelChanged(true);
}

void ToolBar::updateCustomGripVisibility()
{
    auto area = ToolBarManager::getInstance()->toolBarAreaWidget(this);
    auto grip = findChild<ToolBarGrip*>();

    auto customGripIsRequired = isMovable() && area;

    if (grip && !customGripIsRequired) {
        grip->detach();
        grip->deleteLater();
    }
    else if (!grip && customGripIsRequired) {
        grip = new ToolBarGrip(this);
        grip->attach();
    }
    else {
        // either grip is present and should be present
        // or is not present and should not be - nothing to do
        return;
    }
}

void Gui::ToolBar::setupConnections()
{
    connect(this, &QToolBar::topLevelChanged, this, &ToolBar::updateCustomGripVisibility);
    connect(this, &QToolBar::movableChanged, this, &ToolBar::updateCustomGripVisibility);
}

// -----------------------------------------------------------

ToolBarGrip::ToolBarGrip(QToolBar* parent)
    : QWidget(parent)
{
    updateSize();
}

void ToolBarGrip::attach()
{
    if (isAttached()) {
        return;
    }

    auto parent = qobject_cast<ToolBar*>(parentWidget());

    if (!parent) {
        return;
    }

    auto actions = parent->actions();

    _action = parent->insertWidget(
        // ensure that grip is always placed as the first widget in the toolbar
        actions.isEmpty() ? nullptr : actions[0],
        this
    );

    setCursor(Qt::OpenHandCursor);
    setMouseTracking(true);
    setVisible(true);
}

void ToolBarGrip::detach()
{
    if (!isAttached()) {
        return;
    }

    auto parent = qobject_cast<ToolBar*>(parentWidget());

    if (!parent) {
        return;
    }

    parent->removeAction(_action);
}

bool ToolBarGrip::isAttached() const
{
    return _action != nullptr;
}

void ToolBarGrip::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    if (auto toolbar = qobject_cast<ToolBar*>(parentWidget())) {
        QStyle* style = toolbar->style();
        QStyleOptionToolBar opt;

        toolbar->initStyleOption(&opt);

        opt.features = QStyleOptionToolBar::Movable;
        opt.rect = rect();

        style->drawPrimitive(QStyle::PE_IndicatorToolBarHandle, &opt, &painter, toolbar);
    }
}

void ToolBarGrip::mouseMoveEvent(QMouseEvent* me)
{
    auto toolbar = qobject_cast<ToolBar*>(parentWidget());
    if (!toolbar) {
        return;
    }

    auto area = ToolBarManager::getInstance()->toolBarAreaWidget(toolbar);
    if (!area) {
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint pos = me->globalPos();
#else
    QPoint pos = me->globalPosition().toPoint();
#endif
    QRect rect(toolbar->mapToGlobal(QPoint(0, 0)), toolbar->size());

    // if mouse did not leave the area of toolbar do not continue with undocking it
    if (rect.contains(pos)) {
        return;
    }

    toolbar->undock();

    // After removing from area, this grip will be deleted. In order to
    // continue toolbar dragging (because the mouse button is still pressed),
    // we fake mouse events and send to toolbar. For some reason,
    // send/postEvent() does not work, only timer works.
    QPointer tb(toolbar);
    QTimer::singleShot(0, [tb] {
        auto modifiers = QApplication::queryKeyboardModifiers();
        auto buttons = QApplication::mouseButtons();
        if (buttons != Qt::LeftButton || QWidget::mouseGrabber() || modifiers != Qt::NoModifier
            || !tb) {
            return;
        }

        QPoint pos(10, 10);
        QPoint globalPos(tb->mapToGlobal(pos));
        QMouseEvent mouseEvent(QEvent::MouseButtonPress, pos, globalPos, Qt::LeftButton, buttons, modifiers);
        QApplication::sendEvent(tb, &mouseEvent);

        // Mouse follow the mouse press event with mouse move with some offset
        // in order to activate toolbar dragging.
        QPoint offset(30, 30);
        QMouseEvent mouseMoveEvent(
            QEvent::MouseMove,
            pos + offset,
            globalPos + offset,
            Qt::LeftButton,
            buttons,
            modifiers
        );
        QApplication::sendEvent(tb, &mouseMoveEvent);
    });
}

void ToolBarGrip::mousePressEvent(QMouseEvent*)
{
    setCursor(Qt::ClosedHandCursor);
}

void ToolBarGrip::mouseReleaseEvent(QMouseEvent*)
{
    setCursor(Qt::OpenHandCursor);
}

void ToolBarGrip::updateSize()
{
    auto parent = qobject_cast<ToolBar*>(parentWidget());

    if (!parent) {
        return;
    }

    QStyle* style = parent->style();
    QStyleOptionToolBar opt;

    parent->initStyleOption(&opt);
    opt.features = QStyleOptionToolBar::Movable;

    setFixedWidth(style->subElementRect(QStyle::SE_ToolBarHandle, &opt, parent).width() + 4);
}

// -----------------------------------------------------------

ToolBarManager* ToolBarManager::_instance = nullptr;  // NOLINT

ToolBarManager* ToolBarManager::getInstance()
{
    if (!_instance) {
        _instance = new ToolBarManager;
    }
    return _instance;
}

QString ToolBarManager::toolBarPersistenceKey(const ToolBarItem* item)
{
    if (!item) {
        return {};
    }

    return QString::fromUtf8(item->persistenceKey().c_str());
}

QString ToolBarManager::toolBarPersistenceKey(const QToolBar* toolbar)
{
    if (!toolbar) {
        return {};
    }

    for (const auto* propertyName :
         {ToolBarPublicPersistenceKeyProperty, ToolBarPersistenceKeyProperty}) {
        const auto key = stringPropertyValue(toolbar, propertyName);
        if (!key.isEmpty()) {
            return key;
        }
    }

    return toolbar->objectName();
}

ToolBarManager::PersistenceId ToolBarManager::toolBarPersistenceId(const QString& persistenceKey)
{
    return makeToolBarPersistenceId(persistenceKey);
}

ToolBarManager::PersistenceId ToolBarManager::toolBarPersistenceId(const ToolBarItem* item)
{
    return toolBarPersistenceId(toolBarPersistenceKey(item));
}

ToolBarManager::PersistenceId ToolBarManager::toolBarPersistenceId(const QToolBar* toolbar)
{
    return toolBarPersistenceId(toolBarPersistenceKey(toolbar));
}

ToolBarManager::ToolbarScopeId ToolBarManager::layoutContextId(const QString& context)
{
    if (context.isEmpty()) {
        return {};
    }

    if (context.startsWith(ContextualLayoutContextPrefix)) {
        return tryParseContextualLayoutContext(context);
    }

    return ToolbarScopeId::forWorkbench(context);
}

QString ToolBarManager::makeToolBarLayoutContext(const ToolbarScopeId& scopeId)
{
    return serializeLayoutContextId(scopeId);
}

QString ToolBarManager::makeToolBarPersistenceKey(const PersistenceId& id)
{
    return serializeToolBarPersistenceId(id);
}

ToolBarManager::ToolbarScopeId ToolBarManager::toolBarScopeId(const QString& persistenceKey)
{
    return toolBarPersistenceId(persistenceKey).toolbarScopeId();
}

ToolBarManager::ToolbarScopeId ToolBarManager::toolBarScopeId(const ToolBarItem* item)
{
    return toolBarScopeId(toolBarPersistenceKey(item));
}

ToolBarManager::ToolbarScopeId ToolBarManager::toolBarScopeId(const QToolBar* toolbar)
{
    return toolBarScopeId(toolBarPersistenceKey(toolbar));
}

QString ToolBarManager::toolBarScopeLabel(const QString& persistenceKey)
{
    return ::toolBarScopeLabel(toolBarScopeId(persistenceKey).scope);
}

QString ToolBarManager::toolBarScopeLabel(const ToolBarItem* item)
{
    return toolBarScopeLabel(toolBarPersistenceKey(item));
}

QString ToolBarManager::toolBarScopeLabel(const QToolBar* toolbar)
{
    return toolBarScopeLabel(toolBarPersistenceKey(toolbar));
}

ToolBarItem::Tier ToolBarManager::toolBarTier(const ToolBarItem* item)
{
    if (!item) {
        return ToolBarItem::Tier::Recommended;
    }

    return item->tier();
}

ToolBarItem::Tier ToolBarManager::toolBarTier(const QToolBar* toolbar)
{
    if (!toolbar) {
        return ToolBarItem::Tier::Recommended;
    }

    auto property = toolbar->property(ToolBarTierProperty);
    if (property.isValid()) {
        return static_cast<ToolBarItem::Tier>(property.toInt());
    }

    auto publicProperty = toolbar->property(ToolBarPublicTierProperty);
    if (publicProperty.isValid()) {
        return parseToolBarTier(publicProperty.toString());
    }

    auto scope = toolBarScopeId(toolbar).scope;
    if (scope == Scope::Contextual) {
        return ToolBarItem::Tier::Contextual;
    }

    return ToolBarItem::Tier::Recommended;
}

ToolBarItem::Tier ToolBarManager::normalizeCustomToolBarTier(ToolBarItem::Tier tier)
{
    switch (tier) {
        case ToolBarItem::Tier::Recommended:
        case ToolBarItem::Tier::Secondary:
        case ToolBarItem::Tier::Advanced:
            return tier;
        case ToolBarItem::Tier::Contextual:
            return ToolBarItem::Tier::Secondary;
    }

    return ToolBarItem::Tier::Secondary;
}

ToolBarItem::Tier ToolBarManager::customToolBarTierFromName(const QString& tierName)
{
    if (tierName.isEmpty()) {
        return ToolBarItem::Tier::Secondary;
    }

    return normalizeCustomToolBarTier(parseToolBarTier(tierName));
}

ToolBarItem::Tier ToolBarManager::toolBarTierFromName(const QString& tierName)
{
    return parseToolBarTier(tierName);
}

QString ToolBarManager::toolBarTierName(ToolBarItem::Tier tier)
{
    return ::toolBarTierName(tier);
}

QString ToolBarManager::toolBarTierLabel(ToolBarItem::Tier tier)
{
    return ::toolBarTierLabel(tier);
}

QString ToolBarManager::toolBarTierLabel(const ToolBarItem* item)
{
    return toolBarTierLabel(toolBarTier(item));
}

QString ToolBarManager::toolBarTierLabel(const QToolBar* toolbar)
{
    return toolBarTierLabel(toolBarTier(toolbar));
}

void ToolBarManager::setToolBarPersistenceKey(QToolBar* toolbar, const QString& key)
{
    if (!toolbar) {
        return;
    }

    toolbar->setProperty(ToolBarPersistenceKeyProperty, key);
    toolbar->setProperty(ToolBarPublicPersistenceKeyProperty, key);
    toolbar->toggleViewAction()->setProperty(ToolBarPublicPersistenceKeyProperty, key);
}

void ToolBarManager::setToolBarTier(QToolBar* toolbar, ToolBarItem::Tier tier)
{
    if (!toolbar) {
        return;
    }

    toolbar->setProperty(ToolBarTierProperty, static_cast<int>(tier));
    toolbar->setProperty(ToolBarPublicTierProperty, toolBarTierName(tier));
    toolbar->toggleViewAction()->setProperty(ToolBarPublicTierProperty, toolBarTierName(tier));
}

void ToolBarManager::destruct()
{
    delete _instance;
    _instance = nullptr;
}

ToolBarManager::ToolBarManager()
{
    setupParameters();
    setupStatusBar();
    setupMenuBar();

    setupSizeTimer();
    setupResizeTimer();
    setupConnection();
    setupTimer();
    setupMenuBarTimer();

    setupWidgetProducers();
}

ToolBarManager::~ToolBarManager() = default;

void ToolBarManager::setupParameters()
{
    auto& mgr = App::GetApplication().GetUserParameter();
    hGeneral = mgr.GetGroup("BaseApp/Preferences/General");
    hMainWindow = mgr.GetGroup("BaseApp/Preferences/MainWindow");
    hWorkbenchLayouts = mgr.GetGroup("BaseApp/MainWindow/WorkbenchLayouts");
    hGlobalStatusBar = mgr.GetGroup("BaseApp/MainWindow/StatusBar");
    hGlobalMenuBarRight = mgr.GetGroup("BaseApp/MainWindow/MenuBarRight");
    hGlobalMenuBarLeft = mgr.GetGroup("BaseApp/MainWindow/MenuBarLeft");
    hStatusBar = hGlobalStatusBar;
    hMenuBarRight = hGlobalMenuBarRight;
    hMenuBarLeft = hGlobalMenuBarLeft;
    hPref = mgr.GetGroup("BaseApp/MainWindow/Toolbars");
}

void ToolBarManager::setupStatusBar()
{
    if (auto sb = getMainWindow()->statusBar()) {
        sb->installEventFilter(this);
        statusBarAreaWidget = new ToolBarAreaWidget(
            sb,
            ToolBarArea::StatusBarToolBarArea,
            hStatusBar,
            paramHandlers.connection()
        );
        statusBarAreaWidget->setObjectName(QStringLiteral("StatusBarArea"));
        sb->insertPermanentWidget(2, statusBarAreaWidget);
        statusBarAreaWidget->show();
    }
}

void ToolBarManager::setupMenuBar()
{
    if (auto mb = getMainWindow()->menuBar()) {
        mb->installEventFilter(this);
        menuBarLeftAreaWidget = new ToolBarAreaWidget(
            mb,
            ToolBarArea::LeftMenuToolBarArea,
            hMenuBarLeft,
            paramHandlers.connection(),
            &menuBarTimer
        );
        menuBarLeftAreaWidget->setObjectName(QStringLiteral("MenuBarLeftArea"));
        mb->setCornerWidget(menuBarLeftAreaWidget, Qt::TopLeftCorner);
        menuBarLeftAreaWidget->show();
        menuBarRightAreaWidget = new ToolBarAreaWidget(
            mb,
            ToolBarArea::RightMenuToolBarArea,
            hMenuBarRight,
            paramHandlers.connection(),
            &menuBarTimer
        );
        menuBarRightAreaWidget->setObjectName(QStringLiteral("MenuBarRightArea"));
        mb->setCornerWidget(menuBarRightAreaWidget, Qt::TopRightCorner);
        menuBarRightAreaWidget->show();
    }
}

void ToolBarManager::setupConnection()
{
    auto refreshParams = [this](const char* name) {
        bool sizeChanged = false;
        if (!name || boost::equals(name, "ToolbarIconSize")) {
            _toolBarIconSize = hGeneral->GetInt("ToolbarIconSize", 24);
            sizeChanged = true;
        }
        if (!name || boost::equals(name, "StatusBarIconSize")) {
            _statusBarIconSize = hGeneral->GetInt("StatusBarIconSize", 0);
            sizeChanged = true;
        }
        if (!name || boost::equals(name, "MenuBarIconSize")) {
            _menuBarIconSize = hGeneral->GetInt("MenuBarIconSize", 0);
            sizeChanged = true;
        }
        if (sizeChanged) {
            sizeTimer.start(100);
        }
    };

    refreshParams(nullptr);
    paramHandlers.addHandler(hGeneral, "ToolbarIconSize", [refreshParams](const ParamKey* key) {
        refreshParams(key ? key->key : nullptr);
    });
    paramHandlers.addHandler(hGeneral, "StatusBarIconSize", [refreshParams](const ParamKey* key) {
        refreshParams(key ? key->key : nullptr);
    });
    paramHandlers.addHandler(hGeneral, "MenuBarIconSize", [refreshParams](const ParamKey* key) {
        refreshParams(key ? key->key : nullptr);
    });
    paramHandlers.addGroupHandler(hPref, [this](const ParamKey* key) {
        onToolbarParametersChanged(key);
    });
    paramHandlers.addGroupHandler(hStatusBar, [this](const ParamKey* key) {
        onToolbarParametersChanged(key);
    });
    paramHandlers.addGroupHandler(hMenuBarRight, [this](const ParamKey* key) {
        onToolbarParametersChanged(key);
    });
    paramHandlers.addGroupHandler(hMenuBarLeft, [this](const ParamKey* key) {
        onToolbarParametersChanged(key);
    });
}

void ToolBarManager::onToolbarParametersChanged(const ParamKey*)
{
    if (blockRestore) {
        blockRestore = false;
    }
    else {
        timer.start(100);
    }
}

void ToolBarManager::setupTimer()
{
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, [this] { onTimer(); });
}

void ToolBarManager::setupSizeTimer()
{
    sizeTimer.setSingleShot(true);
    connect(&sizeTimer, &QTimer::timeout, [this] { setupToolBarIconSize(); });
}

void ToolBarManager::setupResizeTimer()
{
    resizeTimer.setSingleShot(true);
    connect(&resizeTimer, &QTimer::timeout, [this] {
        for (const auto& [toolbar, guard] : resizingToolbars) {
            if (guard) {
                setToolBarIconSize(toolbar);
            }
        }
        resizingToolbars.clear();
    });
}

void ToolBarManager::setupMenuBarTimer()
{
    menuBarTimer.setSingleShot(true);
    QObject::connect(&menuBarTimer, &QTimer::timeout, [] {
        if (auto menuBar = getMainWindow()->menuBar()) {
            menuBar->adjustSize();
        }
    });
}

void Gui::ToolBarManager::setupWidgetProducers()
{
    new WidgetProducer<Gui::ToolBar>;
}

ToolBarManager::ToolbarScopeId ToolBarManager::activeToolbarLayoutContext() const
{
    auto active = WorkbenchManager::instance()->active();
    if (!active) {
        return {};
    }

    return ToolbarScopeId::forWorkbench(QString::fromUtf8(active->name().c_str()));
}

ToolBarManager::ToolbarScopeId ToolBarManager::effectiveToolbarLayoutContext() const
{
    auto activeContext = activeToolbarLayoutContext();
    if (!toolbarLayoutContextOverride.isEmpty() && !toolbarLayoutContextOverrideWorkbench.isEmpty()
        && toolbarLayoutContextOverrideWorkbench == activeContext.workbench) {
        return toolbarLayoutContextOverride;
    }

    return activeContext;
}

ToolBarManager::CurrentLayoutScope ToolBarManager::currentToolbarLayoutScope(
    ToolbarScopeId* layoutContext,
    ToolbarScopeId* activeContext
) const
{
    const auto currentLayoutContext = effectiveToolbarLayoutContext();
    if (layoutContext) {
        *layoutContext = usableToolbarLayoutContext(currentLayoutContext);
    }

    const auto usableContext = usableToolbarLayoutContext(currentLayoutContext);
    if (usableContext.isEmpty()) {
        if (activeContext) {
            *activeContext = {};
        }
        return CurrentLayoutScope::None;
    }

    const auto currentActiveContext = activeToolbarLayoutContext();
    if (activeContext) {
        *activeContext = currentActiveContext;
    }

    return usableContext.scope == Scope::Contextual ? CurrentLayoutScope::Contextual
                                                    : CurrentLayoutScope::Workbench;
}

bool ToolBarManager::rememberToolbarLayoutByWorkbench() const
{
    return hMainWindow->GetBool("RememberToolbarLayoutByWorkbench", false);
}

bool ToolBarManager::hasSavedWorkbenchToolBarLayout(const ToolbarScopeId& context) const
{
    const auto usableContext = usableToolbarLayoutContext(context);
    if (usableContext.isEmpty()) {
        return false;
    }

    auto group = workbenchLayoutGroup(usableContext);
    return group && group->GetBool("Saved", false);
}

bool ToolBarManager::toolbarBelongsToLayoutContext(
    const QToolBar* toolbar,
    const ToolbarScopeId& context
) const
{
    const auto usableContext = usableToolbarLayoutContext(context);
    if (!toolbar || usableContext.isEmpty()) {
        return false;
    }

    const auto toolbarScope = toolBarScopeId(toolbar);
    return toolbarScope == usableContext;
}

void ToolBarManager::initializeUnsavedToolbarLayoutContext(const ToolbarScopeId& context)
{
    const auto usableContext = usableToolbarLayoutContext(context);
    if (!rememberToolbarLayoutByWorkbench() || usableContext.isEmpty()
        || hasSavedWorkbenchToolBarLayout(usableContext)) {
        return;
    }

    Base::ConnectionBlocker block(paramHandlers.connection());
    const QList<ToolBar*> toolbars = toolBars();
    for (const auto& key : toolbarKeys) {
        auto toolbar = findToolBar(toolbars, key);
        if (!toolbar || !toolbarBelongsToLayoutContext(toolbar, usableContext)) {
            continue;
        }

        const auto toolbarKey = toolBarPersistenceKey(toolbar);
        if (toolbarKey.isEmpty()) {
            continue;
        }

        hPref->SetBool(toolbarKey.toUtf8().constData(), recommendedToolBarVisibility(toolbar));
    }
}

ParameterGrp::handle ToolBarManager::workbenchLayoutGroup(const ToolbarScopeId& context) const
{
    const auto usableContext = usableToolbarLayoutContext(context);
    if (!rememberToolbarLayoutByWorkbench() || usableContext.isEmpty()) {
        return {};
    }

    const auto contextKey = makeToolBarLayoutContext(usableContext);
    return hWorkbenchLayouts->GetGroup(contextKey.toUtf8().constData());
}

void ToolBarManager::updateLayoutParameters(const ToolbarScopeId& context)
{
    auto workbenchGroup = workbenchLayoutGroup(context);

    if (workbenchGroup) {
        hStatusBar = workbenchGroup->GetGroup("StatusBar");
        hMenuBarLeft = workbenchGroup->GetGroup("MenuBarLeft");
        hMenuBarRight = workbenchGroup->GetGroup("MenuBarRight");
    }
    else {
        hStatusBar = hGlobalStatusBar;
        hMenuBarLeft = hGlobalMenuBarLeft;
        hMenuBarRight = hGlobalMenuBarRight;
    }

    if (statusBarAreaWidget) {
        statusBarAreaWidget->setParameters(hStatusBar);
    }
    if (menuBarLeftAreaWidget) {
        menuBarLeftAreaWidget->setParameters(hMenuBarLeft);
    }
    if (menuBarRightAreaWidget) {
        menuBarRightAreaWidget->setParameters(hMenuBarRight);
    }
}

void ToolBarManager::saveWorkbenchToolBarLayout(const ToolbarScopeId& context) const
{
    auto group = workbenchLayoutGroup(context);
    if (!group) {
        return;
    }

    struct ToolBarPosition
    {
        int primary;
        int secondary;
        bool toolbarBreak;
        PersistenceId id;
    };

    QList<ToolBarPosition> top;
    QList<ToolBarPosition> left;
    QList<ToolBarPosition> right;
    QList<ToolBarPosition> bottom;

    for (auto toolbar : toolBars()) {
        auto key = toolBarPersistenceKey(toolbar);
        if (key.isEmpty() || toolbar->isFloating() || toolbar->parentWidget() != getMainWindow()) {
            continue;
        }
        if (!toolBarParticipatesInLayoutContext(toolbar, context)) {
            continue;
        }

        QRect geometry = toolbar->geometry();
        bool toolbarBreak = getMainWindow()->toolBarBreak(toolbar);
        switch (getMainWindow()->toolBarArea(toolbar)) {
            case Qt::TopToolBarArea:
                top.push_back(
                    {geometry.y(), geometry.x(), toolbarBreak, toolBarPersistenceId(toolbar)}
                );
                break;
            case Qt::LeftToolBarArea:
                left.push_back(
                    {geometry.x(), geometry.y(), toolbarBreak, toolBarPersistenceId(toolbar)}
                );
                break;
            case Qt::RightToolBarArea:
                right.push_back(
                    {-geometry.x(), geometry.y(), toolbarBreak, toolBarPersistenceId(toolbar)}
                );
                break;
            case Qt::BottomToolBarArea:
                bottom.push_back(
                    {-geometry.y(), geometry.x(), toolbarBreak, toolBarPersistenceId(toolbar)}
                );
                break;
            default:
                break;
        }
    }

    auto save = [](QList<ToolBarPosition>& positions) {
        std::sort(positions.begin(), positions.end(), [](const auto& lhs, const auto& rhs) {
            return std::tie(lhs.primary, lhs.secondary) < std::tie(rhs.primary, rhs.secondary);
        });

        QList<ToolBarLayoutEntry> layout;
        for (const auto& position : std::as_const(positions)) {
            if (position.toolbarBreak) {
                layout << makeToolBarLayoutEntry(QStringLiteral("Break"));
            }
            layout << makeToolBarLayoutEntry(position.id);
        }
        return layout;
    };

    writeToolBarLayoutState(
        group,
        {
            .saved = true,
            .top = save(top),
            .left = save(left),
            .right = save(right),
            .bottom = save(bottom),
        }
    );
}

void ToolBarManager::resetMainWindowToolBarLayout() const
{
    QList<ToolBar*> toolbars = toolBars();
    for (const auto& key : toolbarKeys) {
        auto toolbar = findToolBar(toolbars, key);
        if (!toolbar) {
            continue;
        }

        auto parent = toolbar->parentWidget();
        if (parent == statusBarAreaWidget || parent == menuBarLeftAreaWidget
            || parent == menuBarRightAreaWidget) {
            continue;
        }

        bool visible = toolbar->isVisible();
        getMainWindow()->removeToolBarBreak(toolbar);
        getMainWindow()->removeToolBar(toolbar);
        toolbar->setOrientation(Qt::Horizontal);
        getMainWindow()->addToolBar(Qt::TopToolBarArea, toolbar);
        toolbar->setVisible(visible);
    }
}

bool ToolBarManager::recommendedToolBarVisibility(const QToolBar* toolbar) const
{
    switch (toolBarTier(toolbar)) {
        case ToolBarItem::Tier::Recommended:
        case ToolBarItem::Tier::Contextual:
            return true;
        case ToolBarItem::Tier::Secondary:
        case ToolBarItem::Tier::Advanced:
            return false;
    }

    return true;
}

void ToolBarManager::applyRecommendedToolBarPreferences()
{
    Base::ConnectionBlocker block(paramHandlers.connection());
    QList<ToolBar*> toolbars = toolBars();
    for (const auto& key : toolbarKeys) {
        auto toolbar = findToolBar(toolbars, key);
        if (!toolbar) {
            continue;
        }

        const auto toolbarKey = toolBarPersistenceKey(toolbar);
        if (toolbarKey.isEmpty()) {
            continue;
        }

        hPref->SetBool(toolbarKey.toUtf8().constData(), recommendedToolBarVisibility(toolbar));
    }
}

void ToolBarManager::applyRecommendedToolBarVisibility()
{
    QList<ToolBar*> toolbars = toolBars();
    for (const auto& key : toolbarKeys) {
        auto toolbar = findToolBar(toolbars, key);
        if (!toolbar) {
            continue;
        }

        auto action = toolbar->toggleViewAction();
        if ((!action || !action->isVisible()) && !toolbar->isVisible()) {
            continue;
        }

        toolbar->setVisible(recommendedToolBarVisibility(toolbar));
    }
}

ToolBarArea ToolBarManager::toolBarArea(QWidget* widget) const
{
    if (auto toolBar = qobject_cast<QToolBar*>(widget)) {
        if (toolBar->isFloating()) {
            return ToolBarArea::NoToolBarArea;
        }

        auto qtToolBarArea = getMainWindow()->toolBarArea(toolBar);
        switch (qtToolBarArea) {
            case Qt::LeftToolBarArea:
                return ToolBarArea::LeftToolBarArea;
            case Qt::RightToolBarArea:
                return ToolBarArea::RightToolBarArea;
            case Qt::TopToolBarArea:
                return ToolBarArea::TopToolBarArea;
            case Qt::BottomToolBarArea:
                return ToolBarArea::BottomToolBarArea;
            default:
                // no-op
                break;
        }
    }

    if (auto areaWidget = toolBarAreaWidget(widget)) {
        return areaWidget->area();
    }

    return ToolBarArea::NoToolBarArea;
}

ToolBarAreaWidget* ToolBarManager::toolBarAreaWidget(QWidget* widget) const
{
    for (auto& areaWidget : {statusBarAreaWidget, menuBarLeftAreaWidget, menuBarRightAreaWidget}) {
        if (areaWidget->indexOf(widget) >= 0) {
            return areaWidget;
        }
    }

    return nullptr;
}

namespace
{
QPointer<QWidget> createActionWidget()
{
    static QPointer<QWidget> actionWidget;
    if (!actionWidget) {
        actionWidget = new QWidget(getMainWindow());
        actionWidget->setObjectName(QStringLiteral("_fc_action_widget_"));
        /* TODO This is a temporary hack until a longterm solution
        is found, thanks to @realthunder for this pointer.
        Although actionWidget has zero size, it somehow has a
        'phantom' size without any visible content and will block the top
        left tool buttons and menus of the application main window.
        Therefore it is moved out of the way. */
        actionWidget->move(QPoint(-100, -100));
    }
    else {
        auto actions = actionWidget->actions();
        for (auto action : actions) {
            actionWidget->removeAction(action);
        }
    }

    return actionWidget;
}
}  // namespace

int ToolBarManager::toolBarIconSize(QWidget* widget) const
{
    int s = _toolBarIconSize;
    if (widget) {
        if (widget->parentWidget() == statusBarAreaWidget) {
            if (_statusBarIconSize > 0) {
                s = _statusBarIconSize;
            }
            else {
                s *= 0.6;
            }
        }
        else if (
            widget->parentWidget() == menuBarLeftAreaWidget
            || widget->parentWidget() == menuBarRightAreaWidget
        ) {
            if (_menuBarIconSize > 0) {
                s = _menuBarIconSize;
            }
            else {
                s *= 0.6;
            }
        }
    }
    return std::max(s, 5);
}

void ToolBarManager::setupToolBarIconSize()
{
    int s = toolBarIconSize();
    getMainWindow()->setIconSize(QSize(s, s));
    // Most of the toolbar will have explicit icon size, so the above call
    // to QMainWindow::setIconSize() will have no effect. We need to explicitly
    // change the icon size.
    QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>();
    for (auto toolbar : std::as_const(bars)) {
        setToolBarIconSize(toolbar);
    }
}

void ToolBarManager::setToolBarIconSize(QToolBar* toolbar)
{
    int s = toolBarIconSize(toolbar);
    toolbar->setIconSize(QSize(s, s));
    if (toolbar->parentWidget() == menuBarLeftAreaWidget) {
        menuBarLeftAreaWidget->adjustParent();
    }
    else if (toolbar->parentWidget() == menuBarRightAreaWidget) {
        menuBarRightAreaWidget->adjustParent();
    }
}

void ToolBarManager::setup(ToolBarItem* toolBarItems)
{
    if (!toolBarItems) {
        return;  // empty menu bar
    }

    QPointer<QWidget> actionWidget = createActionWidget();

    const auto nextLayoutContext = effectiveToolbarLayoutContext();
    saveState();
    updateLayoutParameters(nextLayoutContext);
    this->toolbarKeys.clear();

    int max_width = getMainWindow()->width();
    int top_width = 0;

    bool nameAsToolTip = App::GetApplication()
                             .GetUserParameter()
                             .GetGroup("BaseApp")
                             ->GetGroup("Preferences")
                             ->GetGroup("MainWindow")
                             ->GetBool("ToolBarNameAsToolTip", true);

    QList<ToolBarItem*> items = toolBarItems->getItems();
    QList<ToolBar*> toolbars = toolBars();

    for (ToolBarItem* it : items) {
        QString name = QString::fromUtf8(it->command().c_str());
        QString key = toolBarPersistenceKey(it);
        this->toolbarKeys << key;
        ToolBar* toolbar = findToolBar(toolbars, key);
        bool toolbar_added = false;

        if (!toolbar) {
            toolbar = new ToolBar(getMainWindow());
            toolbar->setWindowTitle(QApplication::translate("Workbench", it->command().c_str()));
            toolbar->setObjectName(name);
            setToolBarPersistenceKey(toolbar, key);
            setToolBarTier(toolbar, toolBarTier(it));

            getMainWindow()->addToolBar(toolbar);
            setToolBarIconSize(toolbar);

            if (nameAsToolTip) {
                auto tooltip = QChar::fromLatin1('[')
                    + QApplication::translate("Workbench", it->command().c_str())
                    + QChar::fromLatin1(']');
                toolbar->setToolTip(tooltip);
            }
            toolbar_added = true;
        }
        else {
            setToolBarPersistenceKey(toolbar, key);
            setToolBarTier(toolbar, toolBarTier(it));
            int index = toolbars.indexOf(toolbar);
            toolbars.removeAt(index);
        }

        bool visible = false;

        // If visibility policy is custom, the toolbar is initialised as not visible, and the
        // toggleViewAction to control its visibility is not visible either.
        //
        // Both are managed under the responsibility of the client code
        if (it->visibilityPolicy != ToolBarItem::DefaultVisibility::Unavailable) {
            bool defaultvisibility = it->visibilityPolicy == ToolBarItem::DefaultVisibility::Visible;

            QByteArray toolbarKey = key.toUtf8();
            visible = hPref->GetBool(toolbarKey.constData(), defaultvisibility);

            // Enable automatic handling of visibility via, for example, (contextual) menu
            toolbar->toggleViewAction()->setVisible(true);
        }
        else {
            // ToolBarItem::DefaultVisibility::Unavailable
            // Prevent that the action to show/hide a toolbar appears on the (contextual) menus.
            // This is also managed by the client code for a toolbar with custom policy
            toolbar->toggleViewAction()->setVisible(false);
        }

        // Initialise toolbar item visibility
        toolbar->setVisible(visible);

        // Store item visibility policy within the action
        QAction* toggle = toolbar->toggleViewAction();
        toggle->setProperty("DefaultVisibility", static_cast<int>(it->visibilityPolicy));

        // setup the toolbar
        setup(it, toolbar);
        auto actions = toolbar->actions();
        for (auto action : actions) {
            actionWidget->addAction(action);
        }

        // try to add some breaks to avoid to have all toolbars in one line
        if (toolbar_added) {
            if (top_width > 0 && getMainWindow()->toolBarBreak(toolbar)) {
                top_width = 0;
            }

            // the width() of a toolbar doesn't return useful results so we estimate
            // its size by the number of buttons and the icon size
            QList<QToolButton*> btns = toolbar->findChildren<QToolButton*>();
            top_width += (btns.size() * toolbar->iconSize().width());
            if (top_width > max_width) {
                top_width = 0;
                getMainWindow()->insertToolBarBreak(toolbar);
            }
        }
    }
    // hide all unneeded toolbars
    for (QToolBar* it : std::as_const(toolbars)) {
        // make sure that the main window has the focus when hiding the toolbar with
        // the combo box inside
        QWidget* fw = QApplication::focusWidget();
        while (fw && !fw->isWindow()) {
            if (fw == it) {
                getMainWindow()->setFocus();
                break;
            }
            fw = fw->parentWidget();
        }
        // ignore toolbars which do not belong to the previously active workbench
        // QByteArray toolbarName = it->objectName().toUtf8();
        if (!it->toggleViewAction()->isVisible()) {
            continue;
        }
        // hPref->SetBool(toolbarName.constData(), it->isVisible());
        it->hide();
        it->toggleViewAction()->setVisible(false);
    }

    setMovable(!areToolBarsLocked());
    activateToolbarLayoutContext(nextLayoutContext);
}

void ToolBarManager::setup(ToolBarItem* item, QToolBar* toolbar) const
{
    CommandManager& mgr = Application::Instance->commandManager();
    QList<ToolBarItem*> items = item->getItems();
    QList<QAction*> actions = toolbar->actions();
    QList<QAction*> orderedActions;
    orderedActions.reserve(items.size());
    for (ToolBarItem* it : items) {
        // search for the action item
        QAction* action = findAction(actions, QString::fromLatin1(it->command().c_str()));
        if (!action) {
            if (it->command() == "Separator") {
                action = toolbar->addSeparator();
            }
            else {
                // Check if action was added successfully
                if (mgr.addTo(it->command().c_str(), toolbar)) {
                    action = toolbar->actions().constLast();
                }
            }

            // set the tool button user data
            if (action) {
                action->setData(QString::fromLatin1(it->command().c_str()));
            }
        }
        else {
            int index = actions.indexOf(action);
            actions.removeAt(index);
        }

        if (action) {
            orderedActions.push_back(action);
        }
    }

    // remove all tool buttons which we don't need for the moment
    for (QAction* it : std::as_const(actions)) {
        toolbar->removeAction(it);
    }

    if (toolbar->actions() != orderedActions) {
        // Reinsert in one batch so reused toolbars match the workbench definition again.
        toolbar->setUpdatesEnabled(false);
        for (QAction* action : std::as_const(orderedActions)) {
            toolbar->removeAction(action);
        }
        for (QAction* action : std::as_const(orderedActions)) {
            toolbar->addAction(action);
        }
        toolbar->setUpdatesEnabled(true);
    }
}

void ToolBarManager::onTimer()
{
    restoreState();
}

void ToolBarManager::saveState() const
{
    saveWorkbenchToolBarLayout(toolbarLayoutContext);

    auto ignoreSave = [](QAction* action) {
        // Only save state for toolbars whose toggle action is user-visible.
        return !action->isVisible();
    };

    QList<ToolBar*> toolbars = toolBars();
    for (const QString& it : toolbarKeys) {
        ToolBar* toolbar = findToolBar(toolbars, it);

        if (toolbar) {
            if (ignoreSave(toolbar->toggleViewAction())) {
                continue;
            }

            QByteArray toolbarKey = toolBarPersistenceKey(toolbar).toUtf8();
            hPref->SetBool(toolbarKey.constData(), toolbar->isVisible());
        }
    }
}

void ToolBarManager::restoreState()
{
    activateToolbarLayoutContext(effectiveToolbarLayoutContext());
}

void ToolBarManager::activateToolbarLayoutContext(const ToolbarScopeId& context)
{
    const auto previousLayoutContext = toolbarLayoutContext;
    const auto layoutContext = usableToolbarLayoutContext(context);
    updateLayoutParameters(layoutContext);
    initializeUnsavedToolbarLayoutContext(layoutContext);
    const auto visibilityValues = toLookup<bool>(hPref, [](const auto& group) {
        return group->GetBoolMap();
    });
    QList<ToolBar*> toolbars = toolBars();
    const auto resolvedState = resolveToolBarRestoreState(
        workbenchLayoutGroup(layoutContext),
        hStatusBar,
        hMenuBarLeft,
        hMenuBarRight,
        hGlobalStatusBar,
        hGlobalMenuBarLeft,
        hGlobalMenuBarRight,
        toolbars
    );

    std::map<int, QToolBar*> sbToolBars;
    std::map<int, QToolBar*> mbRightToolBars;
    std::map<int, QToolBar*> mbLeftToolBars;
    for (const QString& it : toolbarKeys) {
        QToolBar* toolbar = findToolBar(toolbars, it);
        if (toolbar) {
            if (!toolBarParticipatesInLayoutContext(toolbar, layoutContext)) {
                deactivateToolBarForScope(toolbar);
                continue;
            }

            if (getToolbarPolicy(toolbar) != ToolBarItem::DefaultVisibility::Unavailable) {
                bool visible = toolbar->isVisible();
                if (lookupToolBarValue(visibilityValues, {}, toolbar, &visible)) {
                    toolbar->setVisible(visible);
                }
            }

            HostedToolBarPlacement hostedPlacement;
            if (lookupToolBarValue(
                    resolvedState.hostedToolBarPlacements,
                    QMap<QString, HostedToolBarPlacement>(),
                    toolbar,
                    &hostedPlacement
                )
                && hostedPlacement.index >= 0) {
                switch (hostedPlacement.area) {
                    case ToolBarArea::StatusBarToolBarArea:
                        sbToolBars[hostedPlacement.index] = toolbar;
                        break;
                    case ToolBarArea::LeftMenuToolBarArea:
                        mbLeftToolBars[hostedPlacement.index] = toolbar;
                        break;
                    case ToolBarArea::RightMenuToolBarArea:
                        mbRightToolBars[hostedPlacement.index] = toolbar;
                        break;
                    default:
                        break;
                }
                continue;
            }
            if (toolbar->parentWidget() != getMainWindow()) {
                moveToolBarToMainWindow(toolbar);
            }
        }
    }

    setMovable(!areToolBarsLocked());

    restoreMainWindowToolBarLayout(resolvedState.mainWindowLayout, toolbars, getMainWindow(), layoutContext);
    statusBarAreaWidget->restoreState(sbToolBars, resolvedState.statusBarWidgetVisibility);
    menuBarRightAreaWidget->restoreState(mbRightToolBars, resolvedState.menuBarRightWidgetVisibility);
    menuBarLeftAreaWidget->restoreState(mbLeftToolBars, resolvedState.menuBarLeftWidgetVisibility);

    toolbarLayoutContext = layoutContext;
    if (previousLayoutContext != layoutContext) {
        Q_EMIT toolbarLayoutContextChanged();
    }
    Q_EMIT toolbarLayoutScopeRestored(layoutContext);
    Q_EMIT toolbarLayoutRestored(makeToolBarLayoutContext(layoutContext));
}

void ToolBarManager::moveToolBarToMainWindow(QToolBar* toolbar) const
{
    if (!toolbar) {
        return;
    }

    if (auto areaWidget = toolBarAreaWidget(toolbar)) {
        areaWidget->removeWidget(toolbar);
    }

    if (toolbar->parentWidget() != getMainWindow()) {
        getMainWindow()->addToolBar(toolbar);
    }
}

void ToolBarManager::deactivateToolBarForScope(QToolBar* toolbar) const
{
    if (!toolbar) {
        return;
    }

    moveToolBarToMainWindow(toolbar);
    toolbar->hide();
    toolbar->toggleViewAction()->setVisible(false);
}

bool ToolBarManager::addToolBarToArea(QObject* source, QMouseEvent* ev)
{
    auto statusBar = getMainWindow()->statusBar();
    if (!statusBar || !statusBar->isVisible()) {
        statusBar = nullptr;
    }

    auto menuBar = getMainWindow()->menuBar();
    if (!menuBar || !menuBar->isVisible()) {
        if (!statusBar) {
            return false;
        }
        menuBar = nullptr;
    }

    auto tb = qobject_cast<QToolBar*>(source);
    if (!tb || !tb->isFloating()) {
        return false;
    }

    static QPointer<OverlayDragFrame> tbPlaceholder;
    static QPointer<ToolBarAreaWidget> lastArea;
    static int tbIndex = -1;
    if (ev->type() == QEvent::MouseMove) {
        if (tb->orientation() != Qt::Horizontal || ev->buttons() != Qt::LeftButton) {
            if (tbIndex >= 0) {
                if (lastArea) {
                    lastArea->removeWidget(tbPlaceholder);
                    lastArea = nullptr;
                }
                tbPlaceholder->hide();
                tbIndex = -1;
            }
            return false;
        }
    }

    if (ev->type() == QEvent::MouseButtonRelease && ev->button() != Qt::LeftButton) {
        return false;
    }

    QPoint pos = QCursor::pos();
    ToolBarAreaWidget* area = nullptr;
    if (statusBar) {
        QRect rect(statusBar->mapToGlobal(QPoint(0, 0)), statusBar->size());
        if (rect.contains(pos)) {
            area = statusBarAreaWidget;
        }
    }
    if (!area) {
        if (!menuBar) {
            return false;
        }
        QRect rect(menuBar->mapToGlobal(QPoint(0, 0)), menuBar->size());
        if (rect.contains(pos)) {
            if (pos.x() - rect.left() < menuBar->width() / 2) {
                area = menuBarLeftAreaWidget;
            }
            else {
                area = menuBarRightAreaWidget;
            }
        }
        else {
            if (tbPlaceholder) {
                if (lastArea) {
                    lastArea->removeWidget(tbPlaceholder);
                    lastArea = nullptr;
                }
                tbPlaceholder->hide();
                tbIndex = -1;
            }
            return false;
        }
    }

    int idx = 0;
    for (int c = area->count(); idx < c; ++idx) {
        auto widget = area->widgetAt(idx);
        if (!widget || widget->isHidden()) {
            continue;
        }
        int p = widget->mapToGlobal(widget->rect().center()).x();
        if (pos.x() < p) {
            break;
        }
    }
    if (tbIndex >= 0 && tbIndex == idx - 1) {
        idx = tbIndex;
    }
    if (ev->type() == QEvent::MouseMove) {
        if (!tbPlaceholder) {
            tbPlaceholder = new OverlayDragFrame(getMainWindow());
            tbPlaceholder->hide();
            tbIndex = -1;
        }
        if (tbIndex != idx) {
            tbIndex = idx;
            tbPlaceholder->setSizePolicy(tb->sizePolicy());
            tbPlaceholder->setMinimumWidth(tb->minimumWidth());
            tbPlaceholder->resize(tb->size());
            area->insertWidget(idx, tbPlaceholder);
            lastArea = area;
            tbPlaceholder->adjustSize();
            tbPlaceholder->show();
        }
    }
    else {
        tbIndex = idx;
        QTimer::singleShot(10, tb, [tb]() {
            if (!lastArea) {
                return;
            }

            {
                tbPlaceholder->hide();
                QSignalBlocker block(tb);
                lastArea->removeWidget(tbPlaceholder);
                getMainWindow()->removeToolBar(tb);
                tb->setOrientation(Qt::Horizontal);
                lastArea->insertWidget(tbIndex, tb);
                tb->setVisible(true);
                lastArea = nullptr;
            }

            Q_EMIT tb->topLevelChanged(false);
            tbIndex = -1;
        });
    }
    return false;
}

bool ToolBarManager::showContextMenu(QObject* source)
{
    QMenu menu;
    QLayout* layout = nullptr;
    ToolBarAreaWidget* area = nullptr;
    if (getMainWindow()->statusBar() == source) {
        area = statusBarAreaWidget;
        layout = findLayoutOfObject(source, area);
    }
    else if (getMainWindow()->menuBar() == source) {
        area = findToolBarAreaWidget();
        if (!area) {
            return false;
        }
    }
    else {
        return false;
    }

    if (layout) {
        addToMenu(layout, area, &menu);
    }

    QList<QToolBar*> toolbars;
    area->foreachToolBar([&toolbars](QToolBar* toolbar, int, ToolBarAreaWidget*) {
        toolbars.push_back(toolbar);
    });

    if (!toolbars.isEmpty() && !menu.isEmpty()) {
        menu.addSeparator();
    }
    addToolBarActionsByScope(&menu, toolbars);
    addCurrentToolbarLayoutActions(&menu);

    menu.exec(QCursor::pos());
    return true;
}
void ToolBarManager::populateToolBarMenu(QMenu* menu)
{
    if (!menu) {
        return;
    }

    QList<QToolBar*> allToolBars;
    for (auto toolbar : toolBars()) {
        allToolBars.push_back(toolbar);
    }

    addToolBarActionsByScope(menu, allToolBars);
    addCurrentToolbarLayoutActions(menu);
}

QLayout* ToolBarManager::findLayoutOfObject(QObject* source, QWidget* area) const
{
    QLayout* layout = nullptr;
    auto layouts = source->findChildren<QHBoxLayout*>();
    for (auto l : std::as_const(layouts)) {
        if (l->indexOf(area) >= 0) {
            layout = l;
            break;
        }
    }
    return layout;
}

ToolBarAreaWidget* ToolBarManager::findToolBarAreaWidget() const
{
    ToolBarAreaWidget* area = nullptr;

    QPoint pos = QCursor::pos();
    QRect rect(menuBarLeftAreaWidget->mapToGlobal(QPoint(0, 0)), menuBarLeftAreaWidget->size());
    if (rect.contains(pos)) {
        area = menuBarLeftAreaWidget;
    }
    else {
        rect = QRect(menuBarRightAreaWidget->mapToGlobal(QPoint(0, 0)), menuBarRightAreaWidget->size());
        if (rect.contains(pos)) {
            area = menuBarRightAreaWidget;
        }
    }

    return area;
}

void ToolBarManager::addToMenu(QLayout* layout, QWidget* area, QMenu* menu)
{
    for (int i = 0, c = layout->count(); i < c; ++i) {
        auto widget = layout->itemAt(i)->widget();
        if (!widget || widget == area || widget->objectName().isEmpty()
            || widget->objectName().startsWith(QStringLiteral("*"))) {
            continue;
        }
        QString name = widget->windowTitle();
        if (name.isEmpty()) {
            name = widget->objectName();
            name.replace(QLatin1Char('_'), QLatin1Char(' '));
            name = name.simplified();
        }

        auto action = new QAction(menu);
        action->setText(name);
        action->setCheckable(true);
        action->setChecked(widget->isVisible());
        menu->addAction(action);

        auto onToggle = [widget, this](bool visible) {
            onToggleStatusBarWidget(widget, visible);
        };
        QObject::connect(action, &QAction::triggered, onToggle);
    }
}

void ToolBarManager::onToggleStatusBarWidget(QWidget* widget, bool visible)
{
    Base::ConnectionBlocker block(paramHandlers.connection());
    widget->setVisible(visible);
    hStatusBar->SetBool(widget->objectName().toUtf8().constData(), widget->isVisible());
}

void ToolBarManager::addToolBarActionsByScope(QMenu* menu, const QList<QToolBar*>& toolbars) const
{
    if (!menu) {
        return;
    }

    QList<QAction*> sharedActions;
    QList<QAction*> workbenchActions;
    QList<QAction*> contextualActions;
    QList<QAction*> legacyActions;
    const auto toggleLabel = QApplication::translate("MainWindow", "Toggles this toolbar");
    const auto tierLabelPrefix = QApplication::translate("MainWindow", "Tier: %1");

    for (auto toolbar : toolbars) {
        if (!toolbar) {
            continue;
        }

        auto action = toolbar->toggleViewAction();
        if ((!action->isVisible() && !toolbar->isVisible()) || action->text().isEmpty()) {
            continue;
        }

        auto* menuAction = new QAction(action->icon(), decoratedToolBarActionText(toolbar), menu);
        menuAction->setCheckable(true);
        menuAction->setChecked(toolbar->isVisible());
        menuAction->setEnabled(action->isEnabled());

        const auto tierLabel = toolBarTierLabel(toolbar);
        const auto toolTip = tierLabel.isEmpty()
            ? toggleLabel
            : QStringLiteral("%1. %2").arg(toggleLabel, tierLabelPrefix.arg(tierLabel));
        menuAction->setToolTip(toolTip);
        menuAction->setStatusTip(toolTip);
        menuAction->setWhatsThis(toolTip);
        QObject::connect(menuAction, &QAction::triggered, action, &QAction::trigger);

        switch (toolBarScopeId(toolbar).scope) {
            case Scope::Shared:
                sharedActions.push_back(menuAction);
                break;
            case Scope::Workbench:
                workbenchActions.push_back(menuAction);
                break;
            case Scope::Contextual:
                contextualActions.push_back(menuAction);
                break;
            case Scope::Legacy:
                legacyActions.push_back(menuAction);
                break;
        }
    }

    bool hasSection = false;
    auto addToolbarSection = [&](const QString& title, const QList<QAction*>& actions) {
        if (actions.isEmpty()) {
            return;
        }

        if (hasSection) {
            menu->addSeparator();
        }

        menu->addSection(title);
        for (auto action : actions) {
            menu->addAction(action);
        }
        hasSection = true;
    };

    addToolbarSection(QApplication::translate("MainWindow", "Shared Toolbars"), sharedActions);
    addToolbarSection(QApplication::translate("MainWindow", "Workbench Toolbars"), workbenchActions);
    addToolbarSection(QApplication::translate("MainWindow", "Contextual Toolbars"), contextualActions);
    addToolbarSection(QApplication::translate("MainWindow", "Other Toolbars"), legacyActions);
}

void ToolBarManager::addCurrentToolbarLayoutActions(QMenu* menu)
{
    if (!menu) {
        return;
    }

    const auto showRecommendedOnlyLabel = currentShowRecommendedOnlyLabel();
    const auto resetLabel = currentToolbarLayoutResetLabel();
    const auto recommendedResetLabel = currentRecommendedToolbarLayoutResetLabel();
    if (showRecommendedOnlyLabel.isEmpty() && resetLabel.isEmpty()
        && recommendedResetLabel.isEmpty()) {
        return;
    }

    if (!menu->isEmpty()) {
        menu->addSeparator();
    }

    if (!showRecommendedOnlyLabel.isEmpty()) {
        auto showRecommendedOnlyAction = menu->addAction(showRecommendedOnlyLabel);
        QObject::connect(showRecommendedOnlyAction, &QAction::triggered, [this] {
            showRecommendedToolBarsOnly();
        });
    }

    if (!resetLabel.isEmpty()) {
        auto resetAction = menu->addAction(resetLabel);
        QObject::connect(resetAction, &QAction::triggered, [this] { resetCurrentToolbarLayout(); });
    }

    if (!recommendedResetLabel.isEmpty()) {
        auto recommendedResetAction = menu->addAction(recommendedResetLabel);
        QObject::connect(recommendedResetAction, &QAction::triggered, [this] {
            resetCurrentToolbarLayoutToRecommended();
        });
    }
}
bool ToolBarManager::eventFilter(QObject* source, QEvent* ev)
{
    bool res = false;
    switch (ev->type()) {
        case QEvent::Show:
        case QEvent::Hide:
            if (auto toolbar = qobject_cast<QToolBar*>(source)) {
                auto parent = toolbar->parentWidget();
                if (parent == menuBarLeftAreaWidget || parent == menuBarRightAreaWidget) {
                    menuBarTimer.start(10);
                }
            }
            break;
        case QEvent::MouseButtonRelease: {
            auto mev = static_cast<QMouseEvent*>(ev);
            if (mev->button() == Qt::RightButton) {
                if (showContextMenu(source)) {
                    return true;
                }
            }
        }
        // fall through
        case QEvent::MouseMove:
            res = addToolBarToArea(source, static_cast<QMouseEvent*>(ev));
            break;
        case QEvent::ParentChange:
            if (auto toolbar = qobject_cast<QToolBar*>(source)) {
                resizingToolbars[toolbar] = toolbar;
                resizeTimer.start(100);
            }
            break;
        default:
            break;
    }
    return res;
}

void ToolBarManager::retranslate() const
{
    QList<ToolBar*> toolbars = toolBars();
    for (ToolBar* it : toolbars) {
        QByteArray toolbarName = it->objectName().toUtf8();
        it->setWindowTitle(QApplication::translate("Workbench", (const char*)toolbarName));
    }
}

QString ToolBarManager::currentToolbarLayoutResetLabel() const
{
    if (!rememberToolbarLayoutByWorkbench()) {
        return {};
    }

    switch (currentToolbarLayoutScope()) {
        case CurrentLayoutScope::Contextual:
            return QApplication::translate("MainWindow", "Reset Current Contextual Layout");
        case CurrentLayoutScope::Workbench:
            return QApplication::translate("MainWindow", "Reset Current Workbench Layout");
        case CurrentLayoutScope::None:
            return {};
    }

    return {};
}

QString ToolBarManager::currentRecommendedToolbarLayoutResetLabel() const
{
    if (!rememberToolbarLayoutByWorkbench()) {
        return {};
    }

    switch (currentToolbarLayoutScope()) {
        case CurrentLayoutScope::Contextual:
            return QApplication::translate("MainWindow", "Reset To Recommended Contextual Layout");
        case CurrentLayoutScope::Workbench:
            return QApplication::translate("MainWindow", "Reset To Recommended Workbench Layout");
        case CurrentLayoutScope::None:
            return {};
    }

    return {};
}

QString ToolBarManager::currentShowRecommendedOnlyLabel() const
{
    if (currentToolbarLayoutScope() == CurrentLayoutScope::None) {
        return {};
    }

    return QApplication::translate("MainWindow", "Show Recommended Only");
}

QString ToolBarManager::currentToolbarLayoutScopeLabel() const
{
    switch (currentToolbarLayoutScope()) {
        case CurrentLayoutScope::Contextual:
            return QApplication::translate("MainWindow", "Layout scope: Current contextual mode");
        case CurrentLayoutScope::Workbench:
            return QApplication::translate("MainWindow", "Layout scope: Current workbench");
        case CurrentLayoutScope::None:
            return {};
    }

    return {};
}

void ToolBarManager::resetCurrentToolbarLayout()
{
    ToolbarScopeId layoutContext;
    ToolbarScopeId activeContext;
    const auto scope = currentToolbarLayoutScope(&layoutContext, &activeContext);
    if (scope == CurrentLayoutScope::None || !rememberToolbarLayoutByWorkbench()) {
        return;
    }

    const auto layoutContextKey = makeToolBarLayoutContext(layoutContext);
    if (hWorkbenchLayouts->HasGroup(layoutContextKey.toUtf8().constData())) {
        hWorkbenchLayouts->RemoveGrp(layoutContextKey.toUtf8().constData());
    }

    const bool hasWorkbenchFallback = scope == CurrentLayoutScope::Contextual
        && hasSavedWorkbenchToolBarLayout(activeContext);

    if (hasWorkbenchFallback) {
        const auto fallbackToolbars = toolBars();
        const auto fallbackLayout = remapLegacyLayoutState(
            readToolBarLayoutState(workbenchLayoutGroup(activeContext)),
            buildLegacyToolBarAliases(fallbackToolbars)
        );
        restoreMainWindowToolBarLayout(fallbackLayout, fallbackToolbars, getMainWindow(), activeContext);
    }
    else {
        resetMainWindowToolBarLayout();
    }

    restoreState();
}

void ToolBarManager::resetCurrentToolbarLayoutToRecommended()
{
    if (currentRecommendedToolbarLayoutResetLabel().isEmpty()) {
        return;
    }

    applyRecommendedToolBarPreferences();
    resetCurrentToolbarLayout();
    applyRecommendedToolBarVisibility();
}

void ToolBarManager::showRecommendedToolBarsOnly()
{
    if (currentShowRecommendedOnlyLabel().isEmpty()) {
        return;
    }

    applyRecommendedToolBarPreferences();
    applyRecommendedToolBarVisibility();
}

void ToolBarManager::setToolbarLayoutContextOverride(const QString& workbench, const QString& context)
{
    setToolbarLayoutContextOverride(workbench, layoutContextId(context));
}

void ToolBarManager::setToolbarLayoutContextOverride(
    const QString& workbench,
    const ToolbarScopeId& context
)
{
    const auto usableContext = usableToolbarLayoutContext(context);

    if (toolbarLayoutContextOverrideWorkbench == workbench
        && toolbarLayoutContextOverride == usableContext
        && effectiveToolbarLayoutContext() == usableContext) {
        return;
    }

    bool affectsCurrentLayout = activeToolbarLayoutContext().workbench == workbench;
    if (affectsCurrentLayout) {
        saveState();
    }

    toolbarLayoutContextOverrideWorkbench = workbench;
    toolbarLayoutContextOverride = usableContext;

    if (affectsCurrentLayout) {
        restoreState();
    }
}

void ToolBarManager::clearToolbarLayoutContextOverride(const QString& workbench)
{
    if (toolbarLayoutContextOverrideWorkbench != workbench) {
        return;
    }

    bool affectsCurrentLayout = activeToolbarLayoutContext().workbench == workbench;
    if (affectsCurrentLayout) {
        saveState();
    }

    toolbarLayoutContextOverrideWorkbench.clear();
    toolbarLayoutContextOverride = {};

    if (affectsCurrentLayout) {
        restoreState();
    }
}

bool Gui::ToolBarManager::areToolBarsLocked() const
{
    return hGeneral->GetBool("LockToolBars", false);
}

void Gui::ToolBarManager::setToolBarsLocked(bool locked) const
{
    hGeneral->SetBool("LockToolBars", locked);

    setMovable(!locked);
}

void Gui::ToolBarManager::setMovable(bool movable) const
{
    for (auto& tb : toolBars()) {
        tb->setMovable(movable);
        tb->updateCustomGripVisibility();
    }
}

ToolBar* ToolBarManager::findToolBar(const QList<ToolBar*>& toolbars, const QString& item) const
{
    for (ToolBar* it : toolbars) {
        if (toolBarPersistenceKey(it) == item || it->objectName() == item) {
            return it;
        }
    }

    return nullptr;  // no item with the user data found
}

QAction* ToolBarManager::findAction(const QList<QAction*>& acts, const QString& item) const
{
    for (QAction* it : acts) {
        if (it->data().toString() == item) {
            return it;
        }
    }

    return nullptr;  // no item with the user data found
}

QList<ToolBar*> ToolBarManager::toolBars() const
{
    auto mw = getMainWindow();

    QList<ToolBar*> tb;
    QList<ToolBar*> bars = getMainWindow()->findChildren<ToolBar*>();

    for (ToolBar* it : bars) {
        auto parent = it->parentWidget();
        if (parent == mw || parent == mw->statusBar() || parent == statusBarAreaWidget
            || parent == menuBarLeftAreaWidget || parent == menuBarRightAreaWidget) {
            tb.push_back(it);
            it->installEventFilter(const_cast<ToolBarManager*>(this));
        }
    }

    return tb;
}

ToolBarItem::DefaultVisibility ToolBarManager::getToolbarPolicy(const QToolBar* toolbar) const
{
    auto* action = toolbar->toggleViewAction();

    QVariant property = action->property("DefaultVisibility");
    if (property.isNull()) {
        return ToolBarItem::DefaultVisibility::Visible;
    }

    return static_cast<ToolBarItem::DefaultVisibility>(property.toInt());
}

void ToolBarManager::setState(const QList<QString>& names, State state)
{
    for (auto& name : names) {
        setState(name, state);
    }
}

void ToolBarManager::setState(const QString& name, State state)
{
    QToolBar* tb = findToolBar(toolBars(), name);
    const auto visibilityValues = toLookup<bool>(hPref, [](const auto& group) {
        return group->GetBoolMap();
    });
    auto visibility = [this, name, tb, &visibilityValues](bool defaultvalue) {
        bool value = defaultvalue;
        if (tb && lookupToolBarValue(visibilityValues, {}, tb, &value)) {
            return value;
        }

        return hPref->GetBool(name.toStdString().c_str(), defaultvalue);
    };

    auto saveVisibility = [this, visibility, name](bool value, ToolBarItem::DefaultVisibility policy) {
        auto show = visibility(policy == ToolBarItem::DefaultVisibility::Visible);

        if (show != value) {
            blockRestore = true;
            hPref->SetBool(name.toStdString().c_str(), value);
        }
    };

    auto showhide = [visibility](QToolBar* toolbar, ToolBarItem::DefaultVisibility policy) {
        auto show = visibility(policy == ToolBarItem::DefaultVisibility::Visible);

        if (show) {
            toolbar->show();
        }
        else {
            toolbar->hide();
        }
    };

    if (tb) {

        auto policy = getToolbarPolicy(tb);

        if (state == State::RestoreDefault) {
            if (policy == ToolBarItem::DefaultVisibility::Unavailable) {
                tb->hide();
                tb->toggleViewAction()->setVisible(false);
            }
            else {
                tb->toggleViewAction()->setVisible(true);

                showhide(tb, policy);
            }
        }
        else if (state == State::ForceAvailable) {
            tb->toggleViewAction()->setVisible(true);

            // Unavailable policy defaults to Visible when made available.
            auto show = visibility(
                policy == ToolBarItem::DefaultVisibility::Visible
                || policy == ToolBarItem::DefaultVisibility::Unavailable
            );

            if (show) {
                tb->show();
            }
            else {
                tb->hide();
            }
        }
        else if (state == State::ForceHidden) {
            tb->toggleViewAction()->setVisible(false);  // not visible in context menus
            tb->hide();                                 // toolbar not visible
        }
        else if (state == State::SaveState) {
            auto show = tb->isVisible();
            saveVisibility(show, policy);
        }
    }
}

void ToolBarManager::setState(const QList<PersistenceId>& ids, State state)
{
    for (const auto& id : ids) {
        setState(id, state);
    }
}

void ToolBarManager::setState(const PersistenceId& id, State state)
{
    setState(makeToolBarPersistenceKey(id), state);
}

#include "moc_ToolBarManager.cpp"
