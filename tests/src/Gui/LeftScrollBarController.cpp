// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QDockWidget>
#include <QLayout>
#include <QListWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSplitter>
#include <QTest>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "Gui/LeftScrollBarController.h"

// NOLINTBEGIN(readability-magic-numbers)

namespace
{

QScrollBar* leftBarOf(QAbstractScrollArea* area)
{
    return qobject_cast<QScrollBar*>(qvariant_cast<QObject*>(area->property("LeftScrollBar")));
}

void fill(QTreeWidget* tree)
{
    for (int i = 0; i < 300; ++i) {
        tree->addTopLevelItem(new QTreeWidgetItem({QStringLiteral("Item %1").arg(i)}));
    }
}

void settle()
{
    QTest::qWait(50);
}

/// A main window with a dock holding a tree inside a layout, like the tree view panel does.
struct Fixture
{
    QMainWindow window;
    QDockWidget* dock;
    QTreeWidget* tree;
    QWidget* holder;
    bool wantLeft = true;
    Gui::LeftScrollBarController* controller;

    Fixture()
    {
        holder = new QWidget;
        auto layout = new QVBoxLayout(holder);
        layout->setContentsMargins(0, 0, 0, 0);
        tree = new QTreeWidget;
        fill(tree);
        layout->addWidget(tree);

        dock = new QDockWidget(&window);
        dock->setWidget(holder);
        window.setCentralWidget(new QWidget);
        window.addDockWidget(Qt::LeftDockWidgetArea, dock);
        window.resize(700, 500);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        settle();

        controller = new Gui::LeftScrollBarController(dock, [this]() { return wantLeft; });
    }

    static int xIn(QWidget* ancestor, QWidget* widget)
    {
        return widget->mapTo(ancestor, QPoint(0, 0)).x();
    }
};

}  // namespace

class LeftScrollBarControllerTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void movesBarToTheLeft()
    {
        Fixture f;
        QVERIFY(f.tree->verticalScrollBar()->maximum() > 0);

        f.controller->update();
        settle();

        QScrollBar* bar = leftBarOf(f.tree);
        QVERIFY(bar);
        QCOMPARE(f.tree->verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
        QVERIFY(bar->isVisible());
        QVERIFY(Fixture::xIn(&f.window, bar) < Fixture::xIn(&f.window, f.tree));
    }

    void keepsBarsInSync()
    {
        Fixture f;
        f.controller->update();
        settle();
        QScrollBar* bar = leftBarOf(f.tree);
        QVERIFY(bar);
        QCOMPARE(bar->maximum(), f.tree->verticalScrollBar()->maximum());
        QCOMPARE(bar->pageStep(), f.tree->verticalScrollBar()->pageStep());

        f.tree->verticalScrollBar()->setValue(25);
        QCOMPARE(bar->value(), 25);

        bar->setValue(40);
        QCOMPARE(f.tree->verticalScrollBar()->value(), 40);
    }

    void followsContentChanges()
    {
        Fixture f;
        f.controller->update();
        settle();
        QScrollBar* bar = leftBarOf(f.tree);
        QVERIFY(bar);
        QVERIFY(bar->isVisible());

        f.tree->clear();
        settle();
        QVERIFY(!bar->isVisible());

        fill(f.tree);
        settle();
        QVERIFY(bar->isVisible());
        QCOMPARE(bar->maximum(), f.tree->verticalScrollBar()->maximum());
    }

    void restoresNativeBarWhenNoLongerWanted()
    {
        Fixture f;
        f.controller->update();
        settle();
        QVERIFY(leftBarOf(f.tree)->isVisible());

        f.wantLeft = false;
        f.controller->update();
        settle();

        QVERIFY(!leftBarOf(f.tree)->isVisible());
        QCOMPARE(f.tree->verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
        QVERIFY(f.tree->verticalScrollBar()->isVisible());

        f.wantLeft = true;
        f.controller->update();
        settle();
        QVERIFY(leftBarOf(f.tree)->isVisible());
        QCOMPARE(f.tree->verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    }

    void restoresTheOriginalPolicy()
    {
        Fixture f;
        f.tree->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

        f.controller->update();
        QCOMPARE(f.tree->verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);

        f.wantLeft = false;
        f.controller->update();
        QCOMPARE(f.tree->verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOn);
    }

    void doesNothingWhileNotWanted()
    {
        Fixture f;
        f.wantLeft = false;

        f.controller->update();
        settle();

        QVERIFY(!leftBarOf(f.tree));
        QCOMPARE(f.tree->parentWidget(), f.holder);
        QCOMPARE(f.tree->verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    }

    void wrapsOnlyOnce()
    {
        Fixture f;
        f.controller->update();
        QWidget* container = f.tree->parentWidget();
        QVERIFY(container != f.holder);

        f.controller->update();
        f.controller->update();

        QCOMPARE(f.tree->parentWidget(), container);
        QCOMPARE(container->layout()->count(), 2);
        QCOMPARE(f.holder->layout()->count(), 1);
        QCOMPARE(f.holder->layout()->itemAt(0)->widget(), container);
    }

    void leavesAreasWithHiddenNativeBarAlone()
    {
        Fixture f;
        // this is how the overlay panels hide the bar
        f.tree->verticalScrollBar()->setStyleSheet(QStringLiteral("*{width:0}"));

        f.controller->update();
        settle();

        QVERIFY(!leftBarOf(f.tree));
        QCOMPARE(f.tree->parentWidget(), f.holder);
        QCOMPARE(f.tree->verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    }

    void removesLeftBarWhenNativeBarGetsHidden()
    {
        Fixture f;
        f.controller->update();
        settle();
        QVERIFY(leftBarOf(f.tree)->isVisible());

        f.tree->verticalScrollBar()->setStyleSheet(QStringLiteral("*{width:0}"));
        f.controller->update();
        settle();

        QVERIFY(!leftBarOf(f.tree)->isVisible());
        QCOMPARE(f.tree->verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    }

    void ignoresScrollAreasInsideScrollAreas()
    {
        Fixture f;
        auto edit = new QTextEdit;
        f.tree->setItemWidget(f.tree->topLevelItem(0), 0, edit);

        f.controller->update();

        QVERIFY(leftBarOf(f.tree));
        QVERIFY(!leftBarOf(edit));
        QCOMPARE(edit->verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    }

    void leavesDirectDockContentAlone()
    {
        QMainWindow window;
        auto dock = new QDockWidget(&window);
        auto edit = new QPlainTextEdit;
        dock->setWidget(edit);
        window.addDockWidget(Qt::LeftDockWidgetArea, dock);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        auto controller = new Gui::LeftScrollBarController(dock, []() { return true; });
        controller->update();
        settle();

        QVERIFY(!leftBarOf(edit));
        QCOMPARE(edit->parentWidget(), dock);
        QCOMPARE(edit->verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    }

    void wrapsAreasInSplitters()
    {
        QMainWindow window;
        auto dock = new QDockWidget(&window);
        auto splitter = new QSplitter(Qt::Vertical);
        auto first = new QTreeWidget;
        auto second = new QTreeWidget;
        fill(first);
        fill(second);
        splitter->addWidget(first);
        splitter->addWidget(second);
        dock->setWidget(splitter);
        window.setCentralWidget(new QWidget);
        window.addDockWidget(Qt::LeftDockWidgetArea, dock);
        window.resize(700, 500);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        settle();

        auto controller = new Gui::LeftScrollBarController(dock, []() { return true; });
        controller->update();
        settle();

        QCOMPARE(splitter->count(), 2);
        for (auto tree : {first, second}) {
            QScrollBar* bar = leftBarOf(tree);
            QVERIFY(bar);
            QVERIFY(bar->isVisible());
            QVERIFY(splitter->indexOf(tree->parentWidget()) >= 0);
            QVERIFY(Fixture::xIn(&window, bar) < Fixture::xIn(&window, tree));
        }
    }

    void updatesItselfWhenTheDockIsShown()
    {
        Fixture f;
        QVERIFY(!leftBarOf(f.tree));

        f.dock->hide();
        f.dock->show();
        QTRY_VERIFY(leftBarOf(f.tree));
        QTRY_VERIFY(leftBarOf(f.tree)->isVisible());
    }
};

QTEST_MAIN(LeftScrollBarControllerTest)

#include "LeftScrollBarController.moc"

// NOLINTEND(readability-magic-numbers)
