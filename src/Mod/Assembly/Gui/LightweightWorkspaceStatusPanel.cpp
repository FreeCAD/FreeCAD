#include "PreCompiled.h"

#ifndef _PreComp_
#include <string>
#endif

#include "LightweightWorkspaceStatusPanel.h"

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/FileInfo.h>
#include <Gui/DockWindowManager.h>
#include <Mod/Import/App/StepLightweightWorkspaceRuntime.h>

#include <QAbstractItemView>
#include <QDockWidget>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStringList>
#include <QShowEvent>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace AssemblyGui;

namespace
{
constexpr auto lightweightWorkspaceDockName = "Lightweight workspace";
constexpr auto refreshIntervalMs = 750;
constexpr auto documentPathRole = Qt::UserRole + 1;
constexpr auto fullyLoadedRole = Qt::UserRole + 2;
constexpr auto pinnedRole = Qt::UserRole + 3;

std::string normalizePathForCompare(const std::string& path)
{
    if (path.empty()) {
        return {};
    }

    return Base::FileInfo::pathToString(Base::FileInfo::stringToPath(path).lexically_normal());
}

QString shardStateText(const Import::StepLightweightWorkspaceShardState& state)
{
    if (state.isOpen) {
        return state.isFullyLoaded
            ? QObject::tr("Loaded")
            : QObject::tr("Partial");
    }

    return QObject::tr("Unloaded");
}

QString boolText(bool value)
{
    return value ? QObject::tr("Yes") : QObject::tr("No");
}
}

LightweightWorkspaceStatusPanel* LightweightWorkspaceStatusPanel::findPanel()
{
    QWidget* widget = Gui::DockWindowManager::instance()->getDockWindow(lightweightWorkspaceDockName);
    return dynamic_cast<LightweightWorkspaceStatusPanel*>(widget);
}

LightweightWorkspaceStatusPanel* LightweightWorkspaceStatusPanel::showPanel(App::Document* workspaceDoc)
{
    auto* dockMgr = Gui::DockWindowManager::instance();
    auto* panel = findPanel();

    if (!panel) {
        panel = new LightweightWorkspaceStatusPanel();
        QDockWidget* dock = dockMgr->addDockWindow(
            lightweightWorkspaceDockName,
            panel,
            Qt::RightDockWidgetArea
        );
        dock->setFeatures(
            QDockWidget::DockWidgetClosable
            | QDockWidget::DockWidgetMovable
            | QDockWidget::DockWidgetFloatable
        );
        dock->setWindowTitle(QObject::tr("Lightweight workspace"));
        dock->show();
    }

    if (workspaceDoc) {
        panel->setWorkspaceDocument(workspaceDoc);
    }

    dockMgr->activate(panel);
    panel->show();
    panel->refreshNow();
    return panel;
}

void LightweightWorkspaceStatusPanel::refreshPanel(App::Document* workspaceDoc)
{
    auto* panel = findPanel();
    if (!panel) {
        return;
    }

    if (workspaceDoc) {
        panel->setWorkspaceDocument(workspaceDoc);
    }
    else {
        panel->refreshNow();
    }
}

LightweightWorkspaceStatusPanel::LightweightWorkspaceStatusPanel(QWidget* parent)
    : QWidget(parent)
    , workspaceLabel(new QLabel(this))
    , shardSummaryLabel(new QLabel(this))
    , activitySummaryLabel(new QLabel(this))
    , budgetSpinBox(new QSpinBox(this))
    , applyBudgetButton(new QPushButton(QObject::tr("Apply budget"), this))
    , pinButton(new QPushButton(QObject::tr("Pin selected"), this))
    , unpinButton(new QPushButton(QObject::tr("Unpin selected"), this))
    , loadButton(new QPushButton(QObject::tr("Load selected"), this))
    , prefetchButton(new QPushButton(QObject::tr("Prefetch nearby"), this))
    , unloadButton(new QPushButton(QObject::tr("Unload selected"), this))
    , trimButton(new QPushButton(QObject::tr("Trim to budget"), this))
    , unloadAllButton(new QPushButton(QObject::tr("Unload all"), this))
    , refreshButton(new QPushButton(QObject::tr("Refresh"), this))
    , shardTable(new QTreeWidget(this))
    , refreshTimer(new QTimer(this))
{
    workspaceLabel->setWordWrap(true);
    shardSummaryLabel->setWordWrap(true);
    activitySummaryLabel->setWordWrap(true);
    budgetSpinBox->setRange(0, 4096);
    budgetSpinBox->setAccelerated(true);

    shardTable->setAlternatingRowColors(true);
    shardTable->setRootIsDecorated(false);
    shardTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    shardTable->setSelectionMode(QAbstractItemView::SingleSelection);
    shardTable->setUniformRowHeights(true);
    shardTable->setColumnCount(6);
    shardTable->setHeaderLabels(QStringList {
        QObject::tr("Link"),
        QObject::tr("State"),
        QObject::tr("Pinned"),
        QObject::tr("Load source"),
        QObject::tr("Proxy"),
        QObject::tr("Shard file"),
    });
    auto* header = shardTable->header();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::Stretch);

    pinButton->setToolTip(QObject::tr(
        "Protect the selected shard from automatic trim passes and persist that choice in the lightweight cache."
    ));
    unpinButton->setToolTip(QObject::tr(
        "Allow the selected shard to be unloaded again by automatic trim passes and remove its persisted pin."
    ));
    loadButton->setToolTip(QObject::tr("Load the selected shard into memory for expansion and editing."));
    prefetchButton->setToolTip(QObject::tr(
        "Preload spatially nearby shards around the selected link when the current shard budget "
        "still has free capacity."
    ));
    budgetSpinBox->setToolTip(QObject::tr(
        "Maximum number of fully loaded shard documents to keep resident for lightweight STEP "
        "workspaces."
    ));
    applyBudgetButton->setToolTip(QObject::tr(
        "Apply the new loaded-shard budget immediately and trim this workspace if it currently "
        "exceeds the requested limit."
    ));
    unloadButton->setToolTip(QObject::tr("Unload the selected shard again and keep only its proxy."));
    trimButton->setToolTip(QObject::tr(
        "Unload least-recently-used unpinned shards until the current loaded-shard budget is respected."
    ));
    unloadAllButton->setToolTip(QObject::tr(
        "Unload every fully loaded unpinned shard from this workspace and keep only lightweight proxies."
    ));
    refreshButton->setToolTip(QObject::tr("Refresh the workspace metrics immediately."));

    auto* budgetLayout = new QHBoxLayout();
    budgetLayout->setContentsMargins(0, 0, 0, 0);
    budgetLayout->addWidget(new QLabel(QObject::tr("Loaded-shard budget:"), this));
    budgetLayout->addWidget(budgetSpinBox);
    budgetLayout->addWidget(applyBudgetButton);
    budgetLayout->addStretch(1);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(pinButton);
    buttonLayout->addWidget(unpinButton);
    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(prefetchButton);
    buttonLayout->addWidget(unloadButton);
    buttonLayout->addWidget(trimButton);
    buttonLayout->addWidget(unloadAllButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(refreshButton);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(workspaceLabel);
    layout->addWidget(shardSummaryLabel);
    layout->addWidget(activitySummaryLabel);
    layout->addLayout(budgetLayout);
    layout->addLayout(buttonLayout);
    layout->addWidget(shardTable, 1);
    setLayout(layout);

    connect(refreshTimer, &QTimer::timeout, this, &LightweightWorkspaceStatusPanel::refreshNow);
    connect(refreshButton, &QPushButton::clicked, this, &LightweightWorkspaceStatusPanel::refreshNow);
    connect(applyBudgetButton, &QPushButton::clicked, this, &LightweightWorkspaceStatusPanel::applyShardBudget);
    connect(budgetSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        Q_UNUSED(value)
        updateActionState();
    });
    connect(pinButton, &QPushButton::clicked, this, &LightweightWorkspaceStatusPanel::pinSelectedShard);
    connect(unpinButton, &QPushButton::clicked, this, &LightweightWorkspaceStatusPanel::unpinSelectedShard);
    connect(loadButton, &QPushButton::clicked, this, &LightweightWorkspaceStatusPanel::loadSelectedShard);
    connect(
        prefetchButton,
        &QPushButton::clicked,
        this,
        &LightweightWorkspaceStatusPanel::prefetchSelectedShardNeighbors
    );
    connect(unloadButton, &QPushButton::clicked, this, &LightweightWorkspaceStatusPanel::unloadSelectedShard);
    connect(trimButton, &QPushButton::clicked, this, &LightweightWorkspaceStatusPanel::trimToBudget);
    connect(unloadAllButton, &QPushButton::clicked, this, &LightweightWorkspaceStatusPanel::unloadAllShards);
    connect(
        shardTable,
        &QTreeWidget::itemSelectionChanged,
        this,
        &LightweightWorkspaceStatusPanel::updateActionState
    );
    connect(shardTable, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int) {
        if (!item || item->data(0, fullyLoadedRole).toBool()) {
            return;
        }

        loadSelectedShard();
    });
    refreshTimer->start(refreshIntervalMs);

    setEmptyState(QObject::tr("Select a lightweight workspace link to inspect its shard status."));
}

LightweightWorkspaceStatusPanel::~LightweightWorkspaceStatusPanel() = default;

void LightweightWorkspaceStatusPanel::setWorkspaceDocument(App::Document* workspaceDoc)
{
    workspaceDocumentPath.clear();
    if (workspaceDoc) {
        workspaceDocumentPath = workspaceDoc->FileName.getValue();
    }

    refreshNow();
}

void LightweightWorkspaceStatusPanel::refreshNow()
{
    auto* workspaceDoc = resolveWorkspaceDocument();
    if (!workspaceDoc) {
        setEmptyState(QObject::tr("The tracked lightweight workspace is no longer open."));
        return;
    }

    updateSummary(*workspaceDoc);
    populateShardTable(*workspaceDoc);
}

void LightweightWorkspaceStatusPanel::showEvent(QShowEvent* event)
{
    refreshTimer->start(refreshIntervalMs);
    refreshNow();
    QWidget::showEvent(event);
}

void LightweightWorkspaceStatusPanel::hideEvent(QHideEvent* event)
{
    refreshTimer->stop();
    QWidget::hideEvent(event);
}

QTreeWidgetItem* LightweightWorkspaceStatusPanel::selectedShardItem() const
{
    const auto selected = shardTable->selectedItems();
    return selected.isEmpty() ? nullptr : selected.front();
}

App::Document* LightweightWorkspaceStatusPanel::resolveWorkspaceDocument() const
{
    if (workspaceDocumentPath.empty()) {
        return nullptr;
    }

    return App::GetApplication().getDocumentByPath(
        workspaceDocumentPath.c_str(),
        App::Application::PathMatchMode::MatchCanonicalWarning
    );
}

App::DocumentObject* LightweightWorkspaceStatusPanel::resolveSelectedShardLink() const
{
    auto* selectedItem = selectedShardItem();
    auto* workspaceDoc = resolveWorkspaceDocument();
    if (!selectedItem || !workspaceDoc) {
        return nullptr;
    }

    const std::string targetPath = normalizePathForCompare(
        selectedItem->data(0, documentPathRole).toString().toStdString()
    );
    for (auto* object : workspaceDoc->getObjects()) {
        if (!object) {
            continue;
        }

        const auto state = Import::StepLightweightWorkspaceRuntime::inspectLinkedShard(*object);
        if (state.isWorkspaceShard
            && normalizePathForCompare(state.documentPath) == targetPath) {
            return object;
        }
    }

    return nullptr;
}

void LightweightWorkspaceStatusPanel::loadSelectedShard()
{
    auto* linkObject = resolveSelectedShardLink();
    if (!linkObject) {
        return;
    }

    auto state = Import::StepLightweightWorkspaceRuntime::inspectLinkedShard(*linkObject);
    if (state.isOpen && state.isFullyLoaded) {
        return;
    }

    auto* linkedObject = Import::StepLightweightWorkspaceRuntime::loadLinkedShard(*linkObject);
    if (linkedObject && linkedObject->getDocument()) {
        Import::StepLightweightWorkspaceRuntime::noteDocumentAccess(*linkedObject->getDocument());
    }
    refreshNow();
}

void LightweightWorkspaceStatusPanel::applyShardBudget()
{
    auto* workspaceDoc = resolveWorkspaceDocument();
    if (!workspaceDoc) {
        return;
    }

    const int appliedBudget
        = Import::StepLightweightWorkspaceRuntime::setConfiguredMaxLoadedShards(budgetSpinBox->value());
    {
        QSignalBlocker blocker(budgetSpinBox);
        budgetSpinBox->setValue(appliedBudget);
    }
    Import::StepLightweightWorkspaceRuntime::restorePinnedShards(*workspaceDoc);
    refreshNow();
}

void LightweightWorkspaceStatusPanel::unloadSelectedShard()
{
    auto* linkObject = resolveSelectedShardLink();
    if (!linkObject) {
        return;
    }

    Import::StepLightweightWorkspaceRuntime::unloadLinkedShard(*linkObject);
    refreshNow();
}

void LightweightWorkspaceStatusPanel::prefetchSelectedShardNeighbors()
{
    auto* linkObject = resolveSelectedShardLink();
    if (!linkObject) {
        return;
    }

    Import::StepLightweightWorkspaceRuntime::prefetchLinkedShardNeighbors(*linkObject);
    refreshNow();
}

void LightweightWorkspaceStatusPanel::pinSelectedShard()
{
    auto* linkObject = resolveSelectedShardLink();
    if (!linkObject) {
        return;
    }

    Import::StepLightweightWorkspaceRuntime::pinLinkedShard(*linkObject);
    refreshNow();
}

void LightweightWorkspaceStatusPanel::unpinSelectedShard()
{
    auto* linkObject = resolveSelectedShardLink();
    if (!linkObject) {
        return;
    }

    Import::StepLightweightWorkspaceRuntime::unpinLinkedShard(*linkObject);
    refreshNow();
}

void LightweightWorkspaceStatusPanel::trimToBudget()
{
    auto* workspaceDoc = resolveWorkspaceDocument();
    if (!workspaceDoc) {
        return;
    }

    Import::StepLightweightWorkspaceRuntime::trimLoadedShards(*workspaceDoc);
    refreshNow();
}

void LightweightWorkspaceStatusPanel::unloadAllShards()
{
    auto* workspaceDoc = resolveWorkspaceDocument();
    if (!workspaceDoc) {
        return;
    }

    Import::StepLightweightWorkspaceRuntime::trimLoadedShards(*workspaceDoc, 0);
    refreshNow();
}

void LightweightWorkspaceStatusPanel::updateSummary(App::Document& workspaceDoc)
{
    const auto state = Import::StepLightweightWorkspaceRuntime::inspect(workspaceDoc);
    if (!state.isWorkspaceDocument) {
        setEmptyState(QObject::tr("The tracked document is not a lightweight STEP workspace."));
        return;
    }

    const QString workspacePath = QString::fromStdString(
        state.masterDocumentPath.empty() ? workspaceDoc.FileName.getValue() : state.masterDocumentPath
    );
    const int budget = Import::StepLightweightWorkspaceRuntime::configuredMaxLoadedShards();
    {
        QSignalBlocker blocker(budgetSpinBox);
        budgetSpinBox->setValue(budget);
    }

    workspaceLabel->setText(QObject::tr("Workspace: %1").arg(workspacePath));
    shardSummaryLabel->setText(
        QObject::tr(
            "Open shards: %1  •  Fully loaded: %2  •  Unloaded: %3  •  Proxy links: %4  •  Pinned: %5  •  Budget: %6"
        )
            .arg(static_cast<qulonglong>(state.openShardCount))
            .arg(static_cast<qulonglong>(state.fullyLoadedShardCount))
            .arg(static_cast<qulonglong>(state.unloadedShardCount))
            .arg(static_cast<qulonglong>(state.proxyShardCount))
            .arg(static_cast<qulonglong>(state.pinnedShardCount))
            .arg(budget)
    );
    activitySummaryLabel->setText(
        QObject::tr(
            "Initial loads: %1  •  Manual loads: %2 (%3 events)  •  Manual unloads: %4  •  Prefetched: %5 (%6 events)  •  Trims: %7"
        )
            .arg(static_cast<qulonglong>(state.initialLoadedShardCount))
            .arg(static_cast<qulonglong>(state.manualLoadedShardCount))
            .arg(static_cast<qulonglong>(state.manualLoadEventCount))
            .arg(static_cast<qulonglong>(state.manualUnloadEventCount))
            .arg(static_cast<qulonglong>(state.prefetchedShardCount))
            .arg(static_cast<qulonglong>(state.prefetchEventCount))
            .arg(static_cast<qulonglong>(state.trimmedShardEventCount))
    );
}

void LightweightWorkspaceStatusPanel::populateShardTable(App::Document& workspaceDoc)
{
    const auto state = Import::StepLightweightWorkspaceRuntime::inspect(workspaceDoc);
    if (!state.isWorkspaceDocument) {
        shardTable->clear();
        updateActionState();
        return;
    }

    const QString selectedDocumentPath = selectedShardItem()
        ? selectedShardItem()->data(0, documentPathRole).toString()
        : QString();
    QTreeWidgetItem* selectedReplacement = nullptr;

    shardTable->setUpdatesEnabled(false);
    QSignalBlocker blocker(shardTable);
    shardTable->clear();

    for (const auto& shard : state.shards) {
        auto* item = new QTreeWidgetItem();
        item->setText(0, QString::fromStdString(shard.objectName));
        item->setText(1, shardStateText(shard));
        item->setText(2, boolText(shard.isPinned));
        item->setText(3, QString::fromStdString(shard.loadSource));
        item->setText(4, boolText(shard.hasProxy));
        item->setText(5, QString::fromStdString(shard.documentPath));
        item->setData(0, documentPathRole, QString::fromStdString(shard.documentPath));
        item->setData(0, fullyLoadedRole, shard.isFullyLoaded);
        item->setData(0, pinnedRole, shard.isPinned);
        shardTable->addTopLevelItem(item);

        if (!selectedDocumentPath.isEmpty()
            && item->data(0, documentPathRole).toString() == selectedDocumentPath) {
            selectedReplacement = item;
        }
    }

    if (selectedReplacement) {
        shardTable->setCurrentItem(selectedReplacement);
        selectedReplacement->setSelected(true);
    }

    shardTable->setUpdatesEnabled(true);
    updateActionState();
}

void LightweightWorkspaceStatusPanel::setEmptyState(const QString& message)
{
    workspaceLabel->setText(message);
    shardSummaryLabel->clear();
    activitySummaryLabel->clear();
    shardTable->clear();
    updateActionState();
}

void LightweightWorkspaceStatusPanel::updateActionState()
{
    auto* workspaceDoc = resolveWorkspaceDocument();
    const bool hasWorkspace = workspaceDoc
        && Import::StepLightweightWorkspaceRuntime::inspect(*workspaceDoc).isWorkspaceDocument;
    const auto state = hasWorkspace
        ? Import::StepLightweightWorkspaceRuntime::inspect(*workspaceDoc)
        : Import::StepLightweightWorkspaceState();
    auto* item = selectedShardItem();
    const bool isPinned = item && item->data(0, pinnedRole).toBool();
    const bool canLoad = item && !item->data(0, fullyLoadedRole).toBool();
    const bool canUnload = item && item->data(0, fullyLoadedRole).toBool();
    const int maxLoadedShards = Import::StepLightweightWorkspaceRuntime::configuredMaxLoadedShards();
    const bool canApplyBudget = hasWorkspace && budgetSpinBox->value() != maxLoadedShards;
    const bool canPrefetch = hasWorkspace && item && state.shards.size() > 1 && maxLoadedShards > 0
        && state.fullyLoadedShardCount < static_cast<std::size_t>(maxLoadedShards);
    const auto trimmableLoadedShardCount = std::count_if(
        state.shards.begin(),
        state.shards.end(),
        [](const auto& shard) { return shard.isFullyLoaded && !shard.isPinned; }
    );

    budgetSpinBox->setEnabled(hasWorkspace);
    applyBudgetButton->setEnabled(canApplyBudget);
    pinButton->setEnabled(item && !isPinned);
    unpinButton->setEnabled(item && isPinned);
    loadButton->setEnabled(canLoad);
    prefetchButton->setEnabled(canPrefetch);
    unloadButton->setEnabled(canUnload);
    trimButton->setEnabled(hasWorkspace && trimmableLoadedShardCount > 0);
    unloadAllButton->setEnabled(hasWorkspace && trimmableLoadedShardCount > 0);
    refreshButton->setEnabled(hasWorkspace);
}
