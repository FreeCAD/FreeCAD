// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QAbstractItemDelegate>
#include <QCoreApplication>
#include <QFocusEvent>
#include <QPointer>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <src/App/InitApplication.h>

#include "Gui/propertyeditor/PropertyEditor.h"

class PropertyEditorFocusTest: public QObject
{
    Q_OBJECT

public:
    PropertyEditorFocusTest()
    {
        tests::initApplication();
        Gui::PropertyEditor::PropertyIntegerItem::init();
    }

private Q_SLOTS:
    /// Regression test for https://github.com/FreeCAD/FreeCAD/issues/23726
    void editorCanCloseDuringFocusOut()
    {
        auto& application = App::GetApplication();
        const auto documentName = application.getUniqueDocumentName("property_editor_focus");
        auto* document = application.newDocument(documentName.c_str(), "testUser");
        auto* object = document->addObject("App::VarSet", "Box");
        auto* property = object->addDynamicProperty("App::PropertyInteger", "Size", "Box");

        {
            Gui::PropertyEditor::PropertyEditor view;
            view.buildUp({{"Size", {property}}});
            const auto group = view.model()->index(0, 0);
            const auto index = view.model()->index(0, 1, group);
            QVERIFY(index.isValid());
            view.openEditor(index);
            QPointer<QWidget> editor = view.indexWidget(index);
            QVERIFY(editor);

            bool closeRequested = false;
            QObject::connect(
                view.itemDelegate(),
                &QAbstractItemDelegate::closeEditor,
                &view,
                [&](QWidget*, QAbstractItemDelegate::EndEditHint) {
                    closeRequested = true;
                    // Reentrant event processing can run deferred deletions before FocusOut returns.
                    QCoreApplication::sendPostedEvents(editor, QEvent::DeferredDelete);
                }
            );

            QFocusEvent focusOut(QEvent::FocusOut, Qt::MouseFocusReason);
            QCoreApplication::sendEvent(editor, &focusOut);
            QVERIFY(closeRequested);
            QVERIFY(!editor);
        }

        application.closeDocument(documentName.c_str());
    }
};

QTEST_MAIN(PropertyEditorFocusTest)

#include "PropertyEditorFocus.moc"
