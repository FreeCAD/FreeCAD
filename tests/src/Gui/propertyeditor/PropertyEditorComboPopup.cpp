// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QAbstractItemView>
#include <QComboBox>
#include <QLineEdit>
#include <QPointer>
#include <QtTest/QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>

#include "Gui/SoFCDB.h"
#include "Gui/propertyeditor/PropertyEditor.h"
#include "Gui/propertyeditor/PropertyModel.h"

#include <src/App/InitApplication.h>

using namespace Gui::PropertyEditor;

// Investigation harness for PR #32007.
//
// Results with the PR applied (debug, QT_QPA_PLATFORM=offscreen, Qt 6.8.3):
//
//  - the enum drop-down really is auto-opened by PropertyItemDelegate on FocusIn;
//  - opening it moves the focus onto the popup's view, so PropertyItemDelegate::eventFilter()
//    clears PropertyEditor::activeEditor. Instrumenting releaseEditorFocus() showed it entered
//    with activeEditor == nullptr on every call and never reached its hidePopup() branch;
//  - test_rebuildClosesTheComboPopup passes both with and without the PR, because
//    QAbstractItemViewPrivate::releaseEditor() hides the editor before deleting it and
//    QComboBox::hideEvent() closes the popup;
//  - test_pendingStringEditIsNotCommittedByRebuild passes on main and, with the PR, reaches
//    Gui::Command::_runCommand from inside buildUp() (it faults here only because a unit test
//    has no Gui::Application; in the application the assignment would be executed).

class testPropertyEditorComboPopup: public QObject
{
    Q_OBJECT

private:
    App::Document* document = nullptr;
    App::DocumentObject* object = nullptr;
    App::Property* enumProperty = nullptr;
    std::unique_ptr<PropertyEditor> editor;

    static bool popupVisible(const QComboBox* combo)
    {
        return combo && combo->view() && combo->view()->window()
            && combo->view()->window()->isVisible();
    }

    QModelIndex findValueIndex(
        const QAbstractItemModel* model,
        const QModelIndex& parent,
        const QString& name = QLatin1String("Mode")
    ) const
    {
        for (int row = 0; row < model->rowCount(parent); ++row) {
            QModelIndex nameIndex = model->index(row, 0, parent);
            if (nameIndex.data(Qt::DisplayRole).toString() == name) {
                return model->index(row, 1, parent);
            }
            QModelIndex found = findValueIndex(model, nameIndex, name);
            if (found.isValid()) {
                return found;
            }
        }
        return {};
    }

    QComboBox* openEnumEditor()
    {
        PropertyModel::PropertyList props;
        props.emplace_back("Mode", std::vector<App::Property*> {enumProperty});
        editor->buildUp(std::move(props));

        QModelIndex index = findValueIndex(editor->model(), QModelIndex());
        if (!index.isValid()) {
            return nullptr;
        }
        editor->expandAll();
        editor->openEditor(index);
        QTest::qWait(50);
        return editor->viewport()->findChild<QComboBox*>();
    }

private Q_SLOTS:

    void initTestCase()
    {
        tests::initApplication();
        // Registers the PropertyItem types with the factory used by PropertyModel.
        Gui::SoFCDB::init();
        document = App::GetApplication().newDocument("comboPopupTest");
        object = document->addObject("App::DocumentObjectGroup", "grp");
        QVERIFY(object != nullptr);
        enumProperty = object->addDynamicProperty("App::PropertyEnumeration", "Mode");
        QVERIFY(enumProperty != nullptr);
        auto* enumeration = static_cast<App::PropertyEnumeration*>(enumProperty);
        enumeration->setEnumVector({"Alpha", "Beta", "Gamma"});
    }

    void init()
    {
        editor = std::make_unique<PropertyEditor>();
        editor->resize(400, 300);
        editor->show();
        QVERIFY(QTest::qWaitForWindowExposed(editor.get()));
    }

    void cleanup()
    {
        editor.reset();
    }

    // Establishes the premise: an enum property really is edited with a QComboBox, and the
    // drop-down really can be open while the editor is alive.
    void test_enumEditorIsAComboBoxWhosePopupCanBeOpen()
    {
        QComboBox* combo = openEnumEditor();
        QVERIFY2(combo != nullptr, "no QComboBox editor was created for the enum property");

        qWarning() << "popup auto-opened by the delegate:" << popupVisible(combo);
        combo->showPopup();
        QTest::qWait(50);
        QVERIFY2(popupVisible(combo), "the drop-down did not open on the offscreen platform");
    }

    // Commit 907ca26f61 "Close combo popup on the main property-editor rebuild path".
    void test_rebuildClosesTheComboPopup()
    {
        QComboBox* combo = openEnumEditor();
        QVERIFY(combo != nullptr);
        combo->showPopup();
        QTest::qWait(50);
        QVERIFY(popupVisible(combo));

        QPointer<QComboBox> guard(combo);
        editor->buildUp();

        QVERIFY2(!guard.isNull(), "the combo was destroyed synchronously by buildUp()");
        QVERIFY2(
            !popupVisible(guard),
            "buildUp() destroyed the enum editor with its drop-down still open"
        );
    }

    // Commit 37104644a0 "Fix property-editor UAF from combo-box popup destroyed by rebuild".
    // This is the row-removal path, not the model-reset path: PropertyModel::buildUp() only
    // resets the model when the list is empty, otherwise removeChildren() removes rows and
    // PropertyEditor::rowsAboutToBeRemoved() is what calls releaseEditorFocus().
    void test_rowRemovalClosesTheComboPopup()
    {
        PropertyModel::PropertyList props;
        props.emplace_back("Label", std::vector<App::Property*> {&object->Label});
        props.emplace_back("Mode", std::vector<App::Property*> {enumProperty});
        editor->buildUp(std::move(props));

        QModelIndex index = findValueIndex(editor->model(), QModelIndex());
        QVERIFY(index.isValid());
        editor->expandAll();
        editor->openEditor(index);
        QTest::qWait(50);

        auto* combo = editor->viewport()->findChild<QComboBox*>();
        QVERIFY(combo != nullptr);
        combo->showPopup();
        QTest::qWait(50);
        QVERIFY(popupVisible(combo));

        // Rebuild with a list that still has Label but no longer has Mode: the enum row, and
        // with it the editor, is removed through beginRemoveRows()/endRemoveRows().
        PropertyModel::PropertyList fewer;
        fewer.emplace_back("Label", std::vector<App::Property*> {&object->Label});

        QPointer<QComboBox> guard(combo);
        editor->buildUp(std::move(fewer));

        qWarning() << "combo alive after row removal:" << !guard.isNull()
                   << " popup still visible:" << popupVisible(guard);
        QVERIFY2(!guard.isNull(), "the combo was destroyed synchronously by the row removal");
        QVERIFY2(
            !popupVisible(guard),
            "the enum editor was torn down with its drop-down still open"
        );
    }

    // Does a rebuild commit an edit the user typed but never confirmed? PropertyStringItem is
    // the only item whose commitOnEditorClose() is true, so it is the one that can be affected
    // by the commitData() that releaseEditorFocus()'s setFocus() provokes.
    void test_pendingStringEditIsNotCommittedByRebuild()
    {
        PropertyModel::PropertyList props;
        props.emplace_back("Label", std::vector<App::Property*> {&object->Label});
        editor->buildUp(std::move(props));

        QModelIndex index = findValueIndex(editor->model(), QModelIndex(), QLatin1String("Label"));
        QVERIFY(index.isValid());
        editor->expandAll();
        editor->openEditor(index);
        QTest::qWait(50);

        auto* lineEdit = editor->viewport()->findChild<QLineEdit*>();
        QVERIFY2(lineEdit != nullptr, "no line edit was created for the string property");
        qWarning() << "line edit has focus:" << lineEdit->hasFocus();

        lineEdit->setText(QLatin1String("TypedButNeverConfirmed"));
        editor->buildUp();
        QTest::qWait(50);

        qWarning() << "Label after rebuild:" << QString::fromUtf8(object->Label.getValue());
        QCOMPARE(QString::fromUtf8(object->Label.getValue()), QLatin1String("grp"));
    }

    // Commit 8731e9763f "Close combo popup on the closeEditor() teardown path".
    void test_closeEditorClosesTheComboPopup()
    {
        QSKIP(
            "PropertyEditor::closeEditor() dereferences Gui::MainWindow::getInstance() "
            "unconditionally in its Q_OS_MACOS block, so it segfaults without a MainWindow. "
            "Pre-existing, reproduces on main as well."
        );
        QComboBox* combo = openEnumEditor();
        QVERIFY(combo != nullptr);
        combo->showPopup();
        QTest::qWait(50);
        QVERIFY(popupVisible(combo));

        QPointer<QComboBox> guard(combo);
        editor->closeEditor();

        QVERIFY2(!guard.isNull(), "the combo was destroyed synchronously by closeEditor()");
        QVERIFY2(
            !popupVisible(guard),
            "closeEditor() destroyed the enum editor with its drop-down still open"
        );
    }
};

QTEST_MAIN(testPropertyEditorComboPopup)

#include "PropertyEditorComboPopup.moc"
