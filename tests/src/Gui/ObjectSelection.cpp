// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QTest>
#include <QTreeWidget>

#include <App/Application.h>
#include <App/Document.h>
#include <App/PropertyLinks.h>
#include <src/App/InitApplication.h>

#include "Gui/Application.h"
#include "Gui/Dialogs/DlgObjectSelection.h"
#include "Gui/MetaTypes.h"

class ObjectSelectionTest: public QObject
{
    Q_OBJECT

public:
    ObjectSelectionTest()
    {
        tests::initApplication();
        // Gui::Application::Instance must not be nullptr when the dialog calls getViewProvider()
        if (!Gui::Application::Instance) {
            new Gui::Application(false);
        }
    }

private Q_SLOTS:

    void init()
    {
        docName = App::GetApplication().getUniqueDocumentName("test");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
    }

    void cleanup()
    {
        App::GetApplication().closeDocument(docName.c_str());
    }

    void testUncheckPropagatesToAllInstances()
    {
        createLinkedPair();
        Gui::DlgObjectSelection dialog({dependency, dependent});
        auto items = instancesOf(dialog, dependency);
        QCOMPARE(items.size(), 2);
        assertAllHaveState(items, Qt::Checked);

        toggle(items.first(), Qt::Unchecked);

        assertAllHaveState(items, Qt::Unchecked);
    }

    void testCheckPropagatesToAllInstances()
    {
        createLinkedPair();
        Gui::DlgObjectSelection dialog({dependency, dependent});
        auto items = instancesOf(dialog, dependency);
        QCOMPARE(items.size(), 2);
        toggle(items.first(), Qt::Unchecked);
        assertAllHaveState(items, Qt::Unchecked);

        toggle(items.first(), Qt::Checked);

        assertAllHaveState(items, Qt::Checked);
    }

    /// The nested instance must drive the top level one, not just the other way around
    void testNestedInstancePropagatesToAllInstances()
    {
        createLinkedPair();
        Gui::DlgObjectSelection dialog({dependency, dependent});
        auto items = instancesOf(dialog, dependency);
        QCOMPARE(items.size(), 2);

        toggle(items.last(), Qt::Unchecked);
        assertAllHaveState(items, Qt::Unchecked);

        toggle(items.last(), Qt::Checked);
        assertAllHaveState(items, Qt::Checked);
    }

    /// Toggling one object must not drag along objects that merely happen to be selected in the
    /// dependency lists
    void testToggleLeavesOtherObjectsAlone()
    {
        auto* first = doc->addObject("App::FeaturePython", "First");
        auto* second = doc->addObject("App::FeaturePython", "Second");
        doc->recompute();

        Gui::DlgObjectSelection dialog({first, second});
        auto* depList = dialog.findChild<QTreeWidget*>(QStringLiteral("depList"));
        QVERIFY(depList);
        for (int i = 0; i < depList->topLevelItemCount(); ++i) {
            depList->topLevelItem(i)->setSelected(true);
        }

        auto firstItems = instancesOf(dialog, first);
        auto secondItems = instancesOf(dialog, second);
        QCOMPARE(firstItems.size(), 1);
        QCOMPARE(secondItems.size(), 1);

        toggle(firstItems.first(), Qt::Unchecked);

        assertAllHaveState(firstItems, Qt::Unchecked);
        assertAllHaveState(secondItems, Qt::Checked);
    }

private:
    /// Makes "Dependent" link to "Dependency" so that the dependency shows up in the tree twice:
    /// once at the top level and once as a child of the object that depends on it
    void createLinkedPair()
    {
        dependency = doc->addObject("App::FeaturePython", "Dependency");
        dependent = doc->addObject("App::FeaturePython", "Dependent");
        auto* link = static_cast<App::PropertyLink*>(
            dependent->addDynamicProperty("App::PropertyLink", "Source")
        );
        link->setValue(dependency);
        doc->recompute();
    }

    /// Every tree item that stands for @p obj. The nested items are created lazily, so the tree
    /// has to be expanded before the duplicates exist at all.
    static QList<QTreeWidgetItem*> instancesOf(const QWidget& dialog, App::DocumentObject* obj)
    {
        auto* tree = dialog.findChild<QTreeWidget*>(QStringLiteral("treeWidget"));
        if (!tree) {
            return {};
        }
        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            tree->expandItem(tree->topLevelItem(i));
        }

        QList<QTreeWidgetItem*> items;
        std::function<void(QTreeWidgetItem*)> collect = [&](QTreeWidgetItem* item) {
            if (qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole)).getObject() == obj) {
                items.append(item);
            }
            for (int i = 0; i < item->childCount(); ++i) {
                collect(item->child(i));
            }
        };
        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            collect(tree->topLevelItem(i));
        }
        return items;
    }

    /// Clicking a checkbox, plus the wait for the dialog's deferred consistency pass
    static void toggle(QTreeWidgetItem* item, Qt::CheckState state)
    {
        item->setCheckState(0, state);
        QTest::qWait(deferredUpdateDelay);
    }

    static void assertAllHaveState(const QList<QTreeWidgetItem*>& items, Qt::CheckState state)
    {
        for (auto* item : items) {
            QCOMPARE(item->checkState(0), state);
        }
    }

    /// The dialog reconciles the check states from a 10 ms single shot timer
    static constexpr int deferredUpdateDelay = 50;

    std::string docName;
    App::Document* doc = nullptr;
    App::DocumentObject* dependency = nullptr;
    App::DocumentObject* dependent = nullptr;
};

QTEST_MAIN(ObjectSelectionTest)
#include "ObjectSelection.moc"
