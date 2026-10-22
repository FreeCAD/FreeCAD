// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QAbstractItemDelegate>
#include <QApplication>
#include <QCoreApplication>
#include <QFileDialog>
#include <QLineEdit>
#include <QIdentityProxyModel>
#include <QPushButton>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTemporaryFile>
#include <QTimer>

#include <QFocusEvent>
#include <QPointer>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <App/PropertyFile.h>
#include <src/App/InitApplication.h>

#include "Gui/propertyeditor/PropertyEditor.h"

class TestDocument
{
public:
    TestDocument()
    {
        auto& application = App::GetApplication();
        const auto name = application.getUniqueDocumentName("property_editor_test");
        document = application.newDocument(name.c_str(), "testUser");
        object = document->addObject("App::VarSet", "Values");
    }

    ~TestDocument()
    {
        App::GetApplication().closeDocument(document->getName());
    }

    TestDocument(const TestDocument&) = delete;
    TestDocument& operator=(const TestDocument&) = delete;

    App::Document* document;
    App::DocumentObject* object;
};

// The App-only fixture has no GUI command manager to execute property assignments.
class FilePropertyModel: public QIdentityProxyModel
{
public:
    FilePropertyModel(App::PropertyFile* property, QObject* parent)
        : QIdentityProxyModel(parent)
        , property(property)
    {}

    bool setData(const QModelIndex& index, const QVariant& value, int role) override
    {
        if (role != Qt::EditRole) {
            return QIdentityProxyModel::setData(index, value, role);
        }
        property->setValue(value.toString().toUtf8().constData());
        Q_EMIT dataChanged(index, index, {role});
        return true;
    }

private:
    App::PropertyFile* property;
};

class IntegerPropertyModel: public QIdentityProxyModel
{
public:
    using QIdentityProxyModel::QIdentityProxyModel;

    bool setData(const QModelIndex& index, const QVariant& value, int role) override
    {
        if (role != Qt::EditRole) {
            return QIdentityProxyModel::setData(index, value, role);
        }
        auto* item = static_cast<Gui::PropertyEditor::PropertyItem*>(index.internalPointer());
        auto* property = static_cast<App::PropertyInteger*>(item->getPropertyData().front());
        property->setValue(value.toInt());
        Q_EMIT dataChanged(index, index, {role});
        return true;
    }
};

class PaintTrackingPropertyEditor: public Gui::PropertyEditor::PropertyEditor
{
public:
    int paints = 0;

protected:
    bool viewportEvent(QEvent* event) override
    {
        if (event->type() == QEvent::Paint) {
            ++paints;
        }
        return PropertyEditor::viewportEvent(event);
    }
};

class PropertyEditorFocusTest: public QObject
{
    Q_OBJECT

public:
    PropertyEditorFocusTest()
    {
        tests::initApplication();
        Gui::PropertyEditor::PropertyIntegerItem::init();
        Gui::PropertyEditor::PropertyVectorItem::init();
        Gui::PropertyEditor::PropertyFileItem::init();
    }

private Q_SLOTS:
    void refreshRepaintsReadOnlyChanges_data()
    {
        QTest::addColumn<bool>("readOnly");
        QTest::addColumn<QString>("propertyType");
        QTest::newRow("enable") << false << QStringLiteral("App::PropertyInteger");
        QTest::newRow("disable") << true << QStringLiteral("App::PropertyInteger");
        QTest::newRow("enable-children") << false << QStringLiteral("App::PropertyVector");
        QTest::newRow("disable-children") << true << QStringLiteral("App::PropertyVector");
    }

    /// an issue was observed during manual testing where changing a Pad's direction from two sides
    /// to symmetric didn't re-render the Length2 panel's read-only state
    void refreshRepaintsReadOnlyChanges()
    {
        QFETCH(bool, readOnly);
        QFETCH(QString, propertyType);
        TestDocument fixture;
        auto* object = fixture.object;
        auto* property
            = object->addDynamicProperty(propertyType.toUtf8().constData(), "Length2", "Side2");
        property->setStatus(App::Property::ReadOnly, !readOnly);

        PaintTrackingPropertyEditor view;
        view.buildUp({{"Length2", {property}}});
        view.expandAll();
        view.show();
        QTRY_VERIFY(view.paints > 0);
        QTest::qWait(20);
        view.paints = 0;

        property->setStatus(App::Property::ReadOnly, readOnly);
        view.buildUp({{"Length2", {property}}});

        const auto nameIndex = view.model()->index(0, 0, view.model()->index(0, 0));
        const auto valueIndex = view.model()->buddy(nameIndex);
        QCOMPARE(bool(valueIndex.flags() & Qt::ItemIsEditable), !readOnly);
        for (int row = 0; row < view.model()->rowCount(nameIndex); ++row) {
            QCOMPARE(
                bool(view.model()->index(row, 1, nameIndex).flags() & Qt::ItemIsEditable),
                !readOnly
            );
        }
        QTRY_VERIFY(view.paints > 0);
    }

    /// a background recompute should not alter the state of an active editor
    void refreshPreservesPendingEditorValue()
    {
        TestDocument fixture;
        auto* object = fixture.object;
        auto* property = static_cast<App::PropertyInteger*>(
            object->addDynamicProperty("App::PropertyInteger", "Size", "Values")
        );
        property->setValue(3);

        Gui::PropertyEditor::PropertyEditor view;
        view.buildUp({{"Size", {property}}});
        const auto index = view.model()->index(0, 1, view.model()->index(0, 0));
        view.openEditor(index);
        auto* editor = qobject_cast<QSpinBox*>(view.indexWidget(index));
        QVERIFY(editor);
        const QSignalBlocker blocker(editor);
        editor->setValue(23);
        QCOMPARE(property->getValue(), 3L);

        view.buildUp({{"Size", {property}}});
        QCOMPARE(view.indexWidget(index), editor);
        QCOMPARE(editor->value(), 23);
    }

    /// Covers editor closing on Return, related to https://github.com/FreeCAD/FreeCAD/issues/14350.
    /// possibly related to a tab-switch report with a hidden status bar:
    /// https://github.com/FreeCAD/FreeCAD/issues/24660. The macOS document-tab switch in
    /// https://github.com/FreeCAD/FreeCAD/issues/14350 requires the full main window and is not
    /// reproduced here.
    void ordinaryEditorClosesOnReturn()
    {
        TestDocument fixture;
        auto* object = fixture.object;
        auto* property = object->addDynamicProperty("App::PropertyInteger", "Size", "Values");

        Gui::PropertyEditor::PropertyEditor view;
        view.buildUp({{"Size", {property}}});
        const auto index = view.model()->index(0, 1, view.model()->index(0, 0));
        view.openEditor(index);
        QPointer<QWidget> editor = view.indexWidget(index);
        QVERIFY(editor);
        QVERIFY(!view.isPersistentEditorOpen(index));
        QTest::keyClick(editor, Qt::Key_Return);
        QTRY_VERIFY(!editor);
        QVERIFY(!view.indexWidget(index));
        QCOMPARE(view.currentIndex(), index);
    }

    void tabSkipsReadOnlyProperties()
    {
        TestDocument fixture;
        auto* object = fixture.object;
        auto* first = object->addDynamicProperty("App::PropertyInteger", "First", "Values");
        auto* readOnly = object->addDynamicProperty("App::PropertyInteger", "ReadOnly", "Values");
        readOnly->setStatus(App::Property::ReadOnly, true);
        auto* last = object->addDynamicProperty("App::PropertyInteger", "Last", "Values");

        Gui::PropertyEditor::PropertyEditor view;
        view.buildUp({{"First", {first}}, {"ReadOnly", {readOnly}}, {"Last", {last}}});
        const auto group = view.model()->index(0, 0);
        view.expand(group);
        const auto firstIndex = view.model()->index(0, 1, group);
        const auto lastIndex = view.model()->index(2, 1, group);
        view.openEditor(firstIndex);
        QPointer<QWidget> editor = view.indexWidget(firstIndex);
        QVERIFY(editor);
        QTest::keyClick(editor, Qt::Key_Tab);
        QTRY_VERIFY(view.indexWidget(lastIndex));
        QTest::keyClick(view.indexWidget(lastIndex), Qt::Key_Backtab);
        QTRY_VERIFY(view.indexWidget(firstIndex));
    }

    void tabSkipsGroupsAndWraps()
    {
        TestDocument fixture;
        auto* object = fixture.object;
        auto* first = object->addDynamicProperty("App::PropertyInteger", "First", "FirstGroup");
        auto* last = object->addDynamicProperty("App::PropertyInteger", "Last", "LastGroup");

        Gui::PropertyEditor::PropertyEditor view;
        view.buildUp({{"First", {first}}, {"Last", {last}}});
        view.expandAll();
        const auto firstIndex = view.model()->index(0, 1, view.model()->index(0, 0));
        const auto lastIndex = view.model()->index(0, 1, view.model()->index(1, 0));
        view.openEditor(firstIndex);
        QVERIFY(view.indexWidget(firstIndex));
        QTest::keyClick(view.indexWidget(firstIndex), Qt::Key_Tab);
        QTRY_VERIFY(view.indexWidget(lastIndex));
        QTest::keyClick(view.indexWidget(lastIndex), Qt::Key_Tab);
        QTRY_VERIFY(view.indexWidget(firstIndex));
        QTest::keyClick(view.indexWidget(firstIndex), Qt::Key_Backtab);
        QTRY_VERIFY(view.indexWidget(lastIndex));
    }

    void removingAnotherPropertyKeepsEditorOpen()
    {
        TestDocument fixture;
        auto* object = fixture.object;
        auto* first = object->addDynamicProperty("App::PropertyInteger", "First", "Values");
        auto* last = static_cast<App::PropertyInteger*>(
            object->addDynamicProperty("App::PropertyInteger", "Last", "Values")
        );

        Gui::PropertyEditor::PropertyEditor view;
        view.buildUp({{"First", {first}}, {"Last", {last}}});
        auto* model = new IntegerPropertyModel(&view);
        model->setSourceModel(view.model());
        view.setModel(model);
        view.expandAll();
        view.show();
        view.activateWindow();
        const auto group = view.model()->index(0, 0);
        const QPersistentModelIndex index = view.model()->index(1, 1, group);
        view.openEditor(index);
        QPointer<QWidget> editor = view.indexWidget(index);
        QVERIFY(editor);
        QTRY_VERIFY(editor->hasFocus());
        auto* spinBox = qobject_cast<QSpinBox*>(editor);
        QVERIFY(spinBox);
        spinBox->setKeyboardTracking(false);
        auto* lineEdit = spinBox->findChild<QLineEdit*>();
        QVERIFY(lineEdit);
        lineEdit->setText(QStringLiteral("42"));
        QCOMPARE(last->getValue(), 0L);

        view.removeProperty(*first);
        QCoreApplication::processEvents();
        QCOMPARE(view.indexWidget(index), editor.data());
        QVERIFY(editor);
        QVERIFY(editor->hasFocus());
        QCOMPARE(lineEdit->text(), QStringLiteral("42"));
        QCOMPARE(last->getValue(), 0L);
        view.closeEditor();
        QCoreApplication::sendPostedEvents(editor, QEvent::DeferredDelete);
        QVERIFY(!editor);
    }

    void closingEditorFinishesItsOwnDocumentTransaction()
    {
        TestDocument fixture;
        auto* object = fixture.object;
        auto* property = object->addDynamicProperty("App::PropertyInteger", "Size", "Values");

        Gui::PropertyEditor::PropertyEditor view;
        view.setAutomaticDocumentUpdate(true);
        view.buildUp({{"Size", {property}}});
        const auto index = view.model()->index(0, 1, view.model()->index(0, 0));
        view.openEditor(index);
        QVERIFY(view.indexWidget(index));
        QVERIFY(fixture.document->getBookedTransactionID());
        TestDocument other;
        QCOMPARE(App::GetApplication().getActiveDocument(), other.document);
        view.closeEditor();
        QCOMPARE(fixture.document->getBookedTransactionID(), 0);
    }

    void tabFinishesTransactionBeforeOpeningNextEditor_data()
    {
        QTest::addColumn<int>("key");
        QTest::addColumn<bool>("singleEditableProperty");
        QTest::newRow("tab") << int(Qt::Key_Tab) << false;
        QTest::newRow("backtab") << int(Qt::Key_Backtab) << false;
        QTest::newRow("tab-single") << int(Qt::Key_Tab) << true;
        QTest::newRow("backtab-single") << int(Qt::Key_Backtab) << true;
    }

    void tabFinishesTransactionBeforeOpeningNextEditor()
    {
        QFETCH(int, key);
        QFETCH(bool, singleEditableProperty);
        TestDocument fixture;
        auto* object = fixture.object;
        auto* first = static_cast<App::PropertyInteger*>(
            object->addDynamicProperty("App::PropertyInteger", "First", "Values")
        );
        auto* last = static_cast<App::PropertyInteger*>(
            object->addDynamicProperty("App::PropertyInteger", "Last", "Values")
        );
        last->setStatus(App::Property::ReadOnly, singleEditableProperty);

        Gui::PropertyEditor::PropertyEditor view;
        view.setAutomaticDocumentUpdate(true);
        view.buildUp({{"First", {first}}, {"Last", {last}}});
        auto* model = new IntegerPropertyModel(&view);
        model->setSourceModel(view.model());
        view.setModel(model);
        view.expandAll();
        view.show();
        view.activateWindow();
        const auto group = view.model()->index(0, 0);
        const auto firstIndex = view.model()->index(0, 1, group);
        const auto lastIndex = view.model()->index(1, 1, group);
        const bool backwards = key == Qt::Key_Backtab && !singleEditableProperty;
        const auto startIndex = backwards ? lastIndex : firstIndex;
        const auto nextIndex = singleEditableProperty ? firstIndex
                                                      : (backwards ? firstIndex : lastIndex);
        auto* property = backwards ? last : first;
        view.openEditor(startIndex);
        QPointer<QWidget> editor = view.indexWidget(startIndex);
        QVERIFY(editor);
        QTRY_VERIFY(editor->hasFocus());
        const int transaction = fixture.document->getBookedTransactionID();
        QVERIFY(transaction);
        auto* spinBox = qobject_cast<QSpinBox*>(editor);
        QVERIFY(spinBox);
        spinBox->setKeyboardTracking(false);
        auto* lineEdit = spinBox->findChild<QLineEdit*>();
        QVERIFY(lineEdit);
        lineEdit->selectAll();
        QTest::keyClicks(lineEdit, "17");
        QCOMPARE(property->getValue(), 0L);
        int commits = 0;
        bool nextEditorWasOpenAtCommit = false;
        long valueAtCommit = 0;
        auto connection = fixture.document->signalCommitTransaction.connect([&](const App::Document&) {
            ++commits;
            nextEditorWasOpenAtCommit = view.indexWidget(nextIndex) != nullptr;
            valueAtCommit = property->getValue();
        });
        const auto disconnect = qScopeGuard([&] { connection.disconnect(); });

        QTest::keyClick(editor, static_cast<Qt::Key>(key));
        QTRY_VERIFY(view.indexWidget(nextIndex));
        QCOMPARE(view.currentIndex(), nextIndex);
        QCOMPARE(commits, 1);
        QVERIFY(!nextEditorWasOpenAtCommit);
        QCOMPARE(valueAtCommit, 17L);
        QCOMPARE(property->getValue(), 17L);
        QVERIFY(fixture.document->getBookedTransactionID());
        QVERIFY(fixture.document->getBookedTransactionID() != transaction);
        QTRY_VERIFY(!editor);
        connection.disconnect();
        view.closeEditor();
        QCOMPARE(fixture.document->getBookedTransactionID(), 0);
    }

    void teardownReleasesEditorAndTransaction_data()
    {
        QTest::addColumn<bool>("removeProperty");
        QTest::newRow("remove") << true;
        QTest::newRow("reset") << false;
    }

    /// Related teardown coverage for https://github.com/FreeCAD/FreeCAD/issues/23726:
    /// removing or resetting an edited row must release its editor without saving pending input.
    void teardownReleasesEditorAndTransaction()
    {
        QFETCH(bool, removeProperty);
        QTest::failOnWarning(QRegularExpression(QStringLiteral("QAbstractItemView::commitData.*")));
        TestDocument fixture;
        auto* object = fixture.object;
        auto* property = static_cast<App::PropertyInteger*>(
            object->addDynamicProperty("App::PropertyInteger", "Size", "Values")
        );

        Gui::PropertyEditor::PropertyEditor view;
        view.setAutomaticDocumentUpdate(true);
        view.buildUp({{"Size", {property}}});
        auto* model = new IntegerPropertyModel(&view);
        model->setSourceModel(view.model());
        view.setModel(model);
        view.expandAll();
        view.show();
        view.activateWindow();
        const QPersistentModelIndex index = view.model()->index(0, 1, view.model()->index(0, 0));
        view.openEditor(index);
        QPointer<QWidget> editor = view.indexWidget(index);
        QVERIFY(editor);
        const int transaction = fixture.document->getBookedTransactionID();
        QVERIFY(transaction);
        property->setValue(17);
        QTRY_VERIFY(editor->hasFocus());
        auto* spinBox = qobject_cast<QSpinBox*>(editor);
        QVERIFY(spinBox);
        spinBox->setKeyboardTracking(false);
        auto* lineEdit = spinBox->findChild<QLineEdit*>();
        QVERIFY(lineEdit);
        lineEdit->setText(QStringLiteral("42"));

        if (removeProperty) {
            view.removeProperty(*property);
            QVERIFY(!index.isValid());
        }
        else {
            view.reset();
            QVERIFY(!view.indexWidget(index));
        }
        QCOMPARE(fixture.document->getBookedTransactionID(), 0);
        QTRY_VERIFY(!editor);
        QCOMPARE(property->getValue(), 17L);

        if (removeProperty) {
            view.closeEditor();
            QVERIFY(object->removeDynamicProperty("Size"));
        }
        else {
            view.openEditor(index);
            QVERIFY(view.indexWidget(index));
            QVERIFY(fixture.document->getBookedTransactionID());
            QVERIFY(fixture.document->getBookedTransactionID() != transaction);
            view.closeEditor();
            QCOMPARE(fixture.document->getBookedTransactionID(), 0);
        }
    }

    void compoundEditorLifecycle_data()
    {
        QTest::addColumn<bool>("automaticUpdate");
        QTest::addColumn<bool>("openDialog");
        QTest::addColumn<bool>("accept");
        QTest::newRow("accept") << true << true << true;
        QTest::newRow("cancel") << true << true << false;
        QTest::newRow("filename-return") << false << false << false;
        QTest::newRow("filename-return-update") << true << false << false;
    }

    /// Covers child-focus and modal-dialog handling related to the TechDraw template crash:
    /// https://github.com/FreeCAD/FreeCAD/issues/6583
    /// https://forum.freecad.org/viewtopic.php?p=579530#p579530
    /// Also related to the points-editor dialog crash:
    /// https://forum.freecad.org/viewtopic.php?t=66992 Uses a file editor to exercise shared focus
    /// handling; it does not reproduce those workbench dialogs.
    void compoundEditorLifecycle()
    {
        QFETCH(bool, automaticUpdate);
        QFETCH(bool, openDialog);
        QFETCH(bool, accept);
        QTest::failOnWarning(QRegularExpression(QStringLiteral("QAbstractItemView::commitData.*")));
        TestDocument fixture;
        auto* object = fixture.object;
        auto* property = static_cast<App::PropertyFile*>(
            object->addDynamicProperty("App::PropertyFile", "Filename", "Values")
        );
        auto parameters = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Dialog"
        );
        const bool oldNative = parameters->GetBool("DontUseNativeDialog", false);
        const auto restore = qScopeGuard([&] {
            parameters->SetBool("DontUseNativeDialog", oldNative);
        });
        parameters->SetBool("DontUseNativeDialog", true);
        QTemporaryFile original;
        QVERIFY(original.open());
        QTemporaryFile file;
        QVERIFY(file.open());
        property->setValue(original.fileName().toUtf8().constData());

        Gui::PropertyEditor::PropertyEditor view;
        view.setAutomaticDocumentUpdate(automaticUpdate);
        view.buildUp({{"Filename", {property}}});
        auto* model = new FilePropertyModel(property, &view);
        model->setSourceModel(view.model());
        view.setModel(model);
        view.expandAll();
        view.show();
        view.activateWindow();
        const auto index = view.model()->index(0, 1, view.model()->index(0, 0));
        view.openEditor(index);
        QPointer<QWidget> editor = view.indexWidget(index);
        QVERIFY(editor);
        auto* lineEdit = editor->findChild<QLineEdit*>();
        auto* button = editor->findChild<QPushButton*>();
        QVERIFY(lineEdit);
        QVERIFY(button);
        const int transaction = fixture.document->getBookedTransactionID();
        QCOMPARE(transaction != 0, automaticUpdate);
        QCOMPARE(lineEdit->text(), original.fileName());
        lineEdit->setFocus();
        QCoreApplication::processEvents();
        if (openDialog) {
            button->setFocus();
            QCoreApplication::processEvents();
            QCOMPARE(view.indexWidget(index), editor.data());
            QCOMPARE(fixture.document->getBookedTransactionID(), transaction);
            const std::string before = property->getValue();
            bool sawDialog = false;
            bool editorSurvivedDialog = false;
            QStringList selectedFiles;
            QTimer action;
            action.setInterval(10);
            connect(&action, &QTimer::timeout, &view, [&] {
                auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
                if (!dialog) {
                    return;
                }
                action.stop();
                sawDialog = true;
                editorSurvivedDialog = editor && view.indexWidget(index) == editor.data()
                    && fixture.document->getBookedTransactionID() == transaction;
                if (accept) {
                    // selectFile() does not replace the filename while its field has focus.
                    if (auto* focus = dialog->focusWidget()) {
                        focus->clearFocus();
                    }
                    dialog->selectFile(file.fileName());
                    selectedFiles = dialog->selectedFiles();
                    QMetaObject::invokeMethod(dialog, "accept");
                }
                else {
                    dialog->reject();
                }
            });
            QTimer watchdog;
            watchdog.setSingleShot(true);
            connect(&watchdog, &QTimer::timeout, &view, [] {
                if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
                    dialog->reject();
                }
            });
            action.start();
            watchdog.start(5000);
            QTest::mouseClick(button, Qt::LeftButton);
            watchdog.stop();
            action.stop();
            QVERIFY(sawDialog);
            QVERIFY(editorSurvivedDialog);
            if (accept) {
                QCOMPARE(selectedFiles, QStringList {file.fileName()});
            }
            QCOMPARE(view.indexWidget(index), editor.data());
            QCOMPARE(fixture.document->getBookedTransactionID(), transaction);
            QCOMPARE(std::string(property->getValue()), accept ? file.fileName().toStdString() : before);
        }
        lineEdit->setText(file.fileName());
        lineEdit->setFocus();
        // Return can close the editor before the filename field emits editingFinished (#21272).
        // https://github.com/FreeCAD/FreeCAD/issues/21272
        QTest::keyClick(lineEdit, Qt::Key_Return);
        QTRY_VERIFY(!editor);
        QCOMPARE(std::string(property->getValue()), file.fileName().toStdString());
        QCOMPARE(fixture.document->getBookedTransactionID(), 0);
    }

    /// Regression test for https://github.com/FreeCAD/FreeCAD/issues/23726.
    /// Also related to the Pad length crash: https://github.com/FreeCAD/FreeCAD/issues/30820.
    /// Forces editor deletion during FocusOut without reproducing the CAM recompute itself.
    void editorCanCloseDuringFocusOut()
    {
        TestDocument fixture;
        auto* object = fixture.object;
        auto* property = object->addDynamicProperty("App::PropertyInteger", "Size", "Box");

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
};

QTEST_MAIN(PropertyEditorFocusTest)

#include "PropertyEditorFocus.moc"
