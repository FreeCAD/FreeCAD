// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QAccessible>
#include <QCoreApplication>
#include <QScopeGuard>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <src/App/InitApplication.h>

#include "Gui/propertyeditor/PropertyEditor.h"

class PropertyEditorAccessibilityTest: public QObject
{
    Q_OBJECT

public:
    PropertyEditorAccessibilityTest()
    {
        tests::initApplication();
        Gui::PropertyEditor::PropertyIntegerItem::init();
    }

private Q_SLOTS:
    void childrenRemainConsistentAfterCollapse()
    {
        const bool wasActive = QAccessible::isActive();
        QAccessible::setActive(true);
        const auto restore = qScopeGuard([wasActive] { QAccessible::setActive(wasActive); });
        QVERIFY(QAccessible::isActive());

        auto& app = App::GetApplication();
        const auto name = app.getUniqueDocumentName("property_accessibility_test");
        auto* document = app.newDocument(name.c_str(), "testUser");
        const auto closeDocument = qScopeGuard([&] { app.closeDocument(document->getName()); });
        auto* object = document->addObject("App::VarSet", "Values");
        auto* first = object->addDynamicProperty("App::PropertyInteger", "First", "1 Mount");
        auto* second = object->addDynamicProperty("App::PropertyInteger", "Second", "2 Shape");
        Gui::PropertyEditor::PropertyEditor view;
        view.buildUp({{"First", {first}}, {"Second", {second}}});
        view.show();
        view.expandAll();
        QCoreApplication::processEvents();
        auto* accessible = QAccessible::queryAccessibleInterface(&view);
        QVERIFY(accessible);
        QVERIFY(accessible->childCount() > 2);
        for (int index = 0; index < accessible->childCount(); ++index) {
            QVERIFY(accessible->child(index));
        }
        const auto group = view.model()->index(0, 0);
        for (int cycle = 0; cycle < 3; ++cycle) {
            view.collapse(group);
            QCoreApplication::processEvents();
            for (int index = 0; index < accessible->childCount(); ++index) {
                auto* child = accessible->child(index);
                QVERIFY(child);
                QCOMPARE(accessible->indexOfChild(child), index);
            }
            view.expand(group);
            QCoreApplication::processEvents();
            for (int index = 0; index < accessible->childCount(); ++index) {
                auto* child = accessible->child(index);
                QVERIFY(child);
                QCOMPARE(accessible->indexOfChild(child), index);
            }
        }
    }
};

QTEST_MAIN(PropertyEditorAccessibilityTest)

#include "PropertyEditorAccessibility.moc"
