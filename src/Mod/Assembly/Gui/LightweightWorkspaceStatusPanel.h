#pragma once

#include <string>

#include <QWidget>

#include <Mod/Assembly/AssemblyGlobal.h>

namespace App
{
class Document;
class DocumentObject;
}  // namespace App

class QHideEvent;
class QLabel;
class QPushButton;
class QShowEvent;
class QSpinBox;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;
class QString;

namespace AssemblyGui
{

class AssemblyGuiExport LightweightWorkspaceStatusPanel: public QWidget
{
public:
    static LightweightWorkspaceStatusPanel* showPanel(App::Document* workspaceDoc);
    static void refreshPanel(App::Document* workspaceDoc = nullptr);

    explicit LightweightWorkspaceStatusPanel(QWidget* parent = nullptr);
    ~LightweightWorkspaceStatusPanel() override;

    void setWorkspaceDocument(App::Document* workspaceDoc);
    void refreshNow();

protected:
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    static LightweightWorkspaceStatusPanel* findPanel();

    QTreeWidgetItem* selectedShardItem() const;
    App::Document* resolveWorkspaceDocument() const;
    App::DocumentObject* resolveSelectedShardLink() const;
    void applyShardBudget();
    void loadSelectedShard();
    void prefetchSelectedShardNeighbors();
    void unloadSelectedShard();
    void pinSelectedShard();
    void unpinSelectedShard();
    void trimToBudget();
    void unloadAllShards();
    void populateShardTable(App::Document& workspaceDoc);
    void setEmptyState(const QString& message);
    void updateSummary(App::Document& workspaceDoc);
    void updateActionState();

    std::string workspaceDocumentPath;
    QLabel* workspaceLabel;
    QLabel* shardSummaryLabel;
    QLabel* activitySummaryLabel;
    QSpinBox* budgetSpinBox;
    QPushButton* applyBudgetButton;
    QPushButton* pinButton;
    QPushButton* unpinButton;
    QPushButton* loadButton;
    QPushButton* prefetchButton;
    QPushButton* unloadButton;
    QPushButton* trimButton;
    QPushButton* unloadAllButton;
    QPushButton* refreshButton;
    QTreeWidget* shardTable;
    QTimer* refreshTimer;
};

}  // namespace AssemblyGui
