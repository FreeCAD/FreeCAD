// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QCoreApplication>
#include <QWidget>
#include <QtTest/QTest>

#include "Gui/TaskView/TaskView.h"
#include <src/App/InitApplication.h>

class testTaskBox: public QObject
{
    Q_OBJECT

public:
    testTaskBox()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void test_expansionStateSurvivesHiddenParent()  // NOLINT
    {
        QWidget parent;
        parent.resize(200, 100);

        Gui::TaskView::TaskBox taskBox(QStringLiteral("Group"), true, &parent);
        taskBox.resize(200, 100);
        taskBox.show();
        parent.show();
        QCoreApplication::processEvents();

        QVERIFY(taskBox.isVisible());
        QVERIFY(taskBox.isExpanded());

        parent.hide();
        QCoreApplication::processEvents();

        QVERIFY(!taskBox.isVisible());
        QVERIFY(taskBox.isExpanded());
    }

    void test_showHideUsesExpansionStateWhenParentIsHidden()  // NOLINT
    {
        QWidget parent;
        parent.resize(200, 100);

        Gui::TaskView::TaskBox taskBox(QStringLiteral("Group"), true, &parent);
        auto content = new QWidget();
        content->setMinimumSize(20, 20);
        QVERIFY(taskBox.addWidget(content));

        taskBox.show();
        parent.show();
        QCoreApplication::processEvents();

        QVERIFY(taskBox.isExpanded());

        parent.hide();
        QCoreApplication::processEvents();

        QVERIFY(!taskBox.isVisible());
        QVERIFY(taskBox.isExpanded());

        taskBox.showHide();
        QTest::qWait(400);

        QVERIFY(!taskBox.isExpanded());

        parent.show();
        QCoreApplication::processEvents();

        QVERIFY(taskBox.isVisible());
        QVERIFY(!content->isVisible());
    }

    void test_hideGroupBoxUpdatesExpansionState()  // NOLINT
    {
        Gui::TaskView::TaskBox taskBox(QStringLiteral("Group"), true);

        QVERIFY(taskBox.isExpanded());

        taskBox.hideGroupBox();

        QVERIFY(!taskBox.isExpanded());
    }

    void test_setExpandedUpdatesStateAndContents()  // NOLINT
    {
        QWidget parent;
        parent.resize(200, 100);

        Gui::TaskView::TaskBox taskBox(QStringLiteral("Group"), true, &parent);
        auto content = new QWidget();
        content->setMinimumSize(20, 20);
        QVERIFY(taskBox.addWidget(content));

        taskBox.show();
        parent.show();
        QCoreApplication::processEvents();

        QVERIFY(taskBox.isExpanded());
        QVERIFY(content->isVisible());

        taskBox.setExpanded(false);
        QCoreApplication::processEvents();

        QVERIFY(!taskBox.isExpanded());
        QVERIFY(!content->isVisible());

        taskBox.setExpanded(true);
        QCoreApplication::processEvents();

        QVERIFY(taskBox.isExpanded());
        QVERIFY(content->isVisible());
    }

    void test_setExpandedCancelsPendingFoldTimers()  // NOLINT
    {
        QWidget parent;
        parent.resize(200, 100);

        Gui::TaskView::TaskBox taskBox(QStringLiteral("Group"), true, &parent);
        auto content = new QWidget();
        content->setMinimumSize(20, 20);
        QVERIFY(taskBox.addWidget(content));

        taskBox.show();
        parent.show();
        QCoreApplication::processEvents();

        QVERIFY(taskBox.isExpanded());

        taskBox.showHide();
        taskBox.setExpanded(true);

        QTest::qWait(400);

        QVERIFY(taskBox.isExpanded());
        QVERIFY(content->isVisible());
        QCOMPARE(taskBox.maximumHeight(), QWIDGETSIZE_MAX);
    }
};

QTEST_MAIN(testTaskBox)

#include "TaskBox.moc"
