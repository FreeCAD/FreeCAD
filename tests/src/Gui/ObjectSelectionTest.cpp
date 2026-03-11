// SPDX-License-Identifier: LGPL-2.1-or-later
#include <QTest>
#include <QTreeWidget>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Part.h>
#include <App/PropertyLinks.h>
#include <src/App/InitApplication.h>

#include "Gui/Application.h"
#include "Gui/Dialogs/DlgObjectSelection.h"
#include "Gui/MetaTypes.h"

class ObjectSelectionTest : public QObject
{
    Q_OBJECT

public:
    ObjectSelectionTest()
    {
        tests::initApplication();
        // Gui::Application::Instance must not be nullptr when the dialog calls getViewProvider()
        if (!Gui::Application::Instance)
            new Gui::Application(false);
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
        // Child depends on parent, so it appears multiple times in the tree
        auto* parent = doc->addObject("App::Part", "Parent");
        auto* child  = doc->addObject("App::FeaturePython", "Child");

        auto* linkProp = static_cast<App::PropertyLink*>(
            child->addDynamicProperty("App::PropertyLink", "Source"));
        linkProp->setValue(parent);
        doc->recompute();

        std::vector<App::DocumentObject*> objs = {parent, child};
        Gui::DlgObjectSelection dlg(objs);

        auto* treeWidget = dlg.findChild<QTreeWidget*>("treeWidget");
        QVERIFY(treeWidget);

        // Collect all tree items representing 'child'
        QList<QTreeWidgetItem*> childItems;
        std::function<void(QTreeWidgetItem*)> findItems = [&](QTreeWidgetItem* item) {
            if (qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole)).getObject() == child)
                childItems.append(item);
            for (int i = 0; i < item->childCount(); ++i)
                findItems(item->child(i));
        };
        for (int i = 0; i < treeWidget->topLevelItemCount(); ++i)
            findItems(treeWidget->topLevelItem(i));

        QVERIFY(!childItems.isEmpty());

        // Uncheck one instance and verify all instances follow
        childItems.first()->setCheckState(0, Qt::Unchecked);
        QCoreApplication::processEvents();

        for (auto* item : childItems)
            QCOMPARE(item->checkState(0), Qt::Unchecked);
    }

private:
    std::string docName;
    App::Document* doc = nullptr;
};

QTEST_MAIN(ObjectSelectionTest)
#include "ObjectSelectionTest.moc"