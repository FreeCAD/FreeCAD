// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <string>

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QObject>
#include <QPointer>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>

#include <Gui/TaskView/TaskDialog.h>

#include "Mod/PartDesign/Gui/DeferredDialogRejectUtils.h"
#include <src/App/InitApplication.h>

namespace
{

class TestSender: public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void triggered();
};

class TestReceiver: public QObject
{
    Q_OBJECT

public:
    int callCount = 0;

public Q_SLOTS:
    void onTriggered()
    {
        ++callCount;
    }
};

class TestTaskDialog: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    int closedCount = 0;

    void closed() override
    {
        ++closedCount;
    }
};

}  // namespace

class testDeferredDialogReject final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
    }

    void init()  // NOLINT
    {
        docName = App::GetApplication().getUniqueDocumentName("deferred_reject");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        QVERIFY(doc != nullptr);
    }

    void cleanup()  // NOLINT
    {
        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        doc = nullptr;
        docName.clear();
    }

    void ensureDeferredDialogRejectConnectionIsQueuedAndDeduplicated()  // NOLINT
    {
        PartDesignGui::DeferredDialogRejectState state;
        TestSender sender;
        TestReceiver receiver;

        PartDesignGui::ensureDeferredDialogRejectConnection(
            state,
            &sender,
            &TestSender::triggered,
            &receiver,
            &TestReceiver::onTriggered
        );
        PartDesignGui::ensureDeferredDialogRejectConnection(
            state,
            &sender,
            &TestSender::triggered,
            &receiver,
            &TestReceiver::onTriggered
        );

        QVERIFY(state.connection);

        Q_EMIT sender.triggered();
        QCOMPARE(receiver.callCount, 0);

        QCoreApplication::processEvents();
        QCOMPARE(receiver.callCount, 1);

        Q_EMIT sender.triggered();
        QCoreApplication::processEvents();
        QCOMPARE(receiver.callCount, 2);
    }

    void setDeferredDialogRejectPendingUpdatesStateAndButtons()  // NOLINT
    {
        PartDesignGui::DeferredDialogRejectState state;
        QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        bool updatedPending = false;

        PartDesignGui::setDeferredDialogRejectPending(
            state,
            true,
            &buttonBox,
            [&updatedPending](bool pending) { updatedPending = pending; }
        );

        QVERIFY(state.pending);
        QVERIFY(!buttonBox.isEnabled());
        QVERIFY(updatedPending);

        PartDesignGui::setDeferredDialogRejectPending(
            state,
            false,
            &buttonBox,
            [&updatedPending](bool pending) { updatedPending = pending; }
        );

        QVERIFY(!state.pending);
        QVERIFY(buttonBox.isEnabled());
        QVERIFY(!updatedPending);
    }

    void finishDeferredDialogRejectWaitsUntilReady()  // NOLINT
    {
        PartDesignGui::DeferredDialogRejectState state;
        state.pending = true;
        int rejectCalls = 0;
        int setPendingCalls = 0;

        TestTaskDialog dialog;
        PartDesignGui::finishDeferredDialogReject(
            &dialog,
            state,
            /*readyToClose=*/false,
            [&rejectCalls]() {
                ++rejectCalls;
                return true;
            },
            [&setPendingCalls](bool) { ++setPendingCalls; }
        );

        QVERIFY(state.pending);
        QCOMPARE(rejectCalls, 0);
        QCOMPARE(setPendingCalls, 0);
    }

    void finishDeferredDialogRejectClearsPendingWhenRejectFails()  // NOLINT
    {
        PartDesignGui::DeferredDialogRejectState state;
        state.pending = true;

        QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttonBox.setEnabled(false);
        int rejectCalls = 0;
        int setPendingCalls = 0;
        bool lastPending = true;

        TestTaskDialog dialog;
        PartDesignGui::finishDeferredDialogReject(
            &dialog,
            state,
            /*readyToClose=*/true,
            [&rejectCalls]() {
                ++rejectCalls;
                return false;
            },
            [&](bool pending) {
                ++setPendingCalls;
                lastPending = pending;
                PartDesignGui::setDeferredDialogRejectPending(state, pending, &buttonBox, [](bool) {});
            }
        );

        QCOMPARE(rejectCalls, 1);
        QCOMPARE(setPendingCalls, 1);
        QVERIFY(!lastPending);
        QVERIFY(!state.pending);
        QVERIFY(buttonBox.isEnabled());
    }

    void finishDeferredDialogRejectInvokesCloseCallbackOnSuccess()  // NOLINT
    {
        TestTaskDialog dialog;

        PartDesignGui::DeferredDialogRejectState state;
        state.pending = true;
        state.documentName = doc->getName();

        int rejectCalls = 0;
        int setPendingCalls = 0;
        int closeCalls = 0;
        App::Document* closedDocument = nullptr;
        TestTaskDialog* closedDialog = nullptr;

        PartDesignGui::finishDeferredDialogReject(
            &dialog,
            state,
            /*readyToClose=*/true,
            [&rejectCalls]() {
                ++rejectCalls;
                return true;
            },
            [&setPendingCalls](bool) { ++setPendingCalls; },
            [&](App::Document* document, TestTaskDialog* closeDialog) {
                ++closeCalls;
                closedDocument = document;
                closedDialog = closeDialog;
            }
        );

        QCOMPARE(rejectCalls, 1);
        QCOMPARE(setPendingCalls, 0);
        QCOMPARE(closeCalls, 1);
        QCOMPARE(closedDocument, doc);
        QCOMPARE(closedDialog, &dialog);
    }

    void finishDeferredDialogRejectReturnsWhenRejectDeletesDialog()  // NOLINT
    {
        PartDesignGui::DeferredDialogRejectState state;
        state.pending = true;

        auto* dialog = new TestTaskDialog();
        QPointer<TestTaskDialog> guard(dialog);
        int setPendingCalls = 0;
        int closeCalls = 0;

        PartDesignGui::finishDeferredDialogReject(
            dialog,
            state,
            /*readyToClose=*/true,
            [&guard]() {
                delete guard.data();
                return true;
            },
            [&setPendingCalls](bool) { ++setPendingCalls; },
            [&closeCalls](App::Document*, TestTaskDialog*) { ++closeCalls; }
        );

        QVERIFY(guard.isNull());
        QCOMPARE(setPendingCalls, 0);
        QCOMPARE(closeCalls, 0);
    }

private:
    std::string docName;
    App::Document* doc = nullptr;
};

QTEST_MAIN(testDeferredDialogReject)

#include "DeferredDialogRejectTest.moc"
