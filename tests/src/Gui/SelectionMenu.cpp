// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QTest>
#include <QTimer>

#include <memory>

#include <src/App/InitApplication.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionView.h>

namespace
{

class TestableSelectionMenu: public Gui::SelectionMenu
{
public:
    using Gui::SelectionMenu::onHover;

    void populate(const std::vector<Gui::PickData>& selections)
    {
        populateMenu(selections);
    }

    void sendLeave()
    {
        QEvent event(QEvent::Leave);
        leaveEvent(&event);
    }
};

QAction* findActionWithData(QMenu& menu, int expectedData)
{
    for (auto* action : menu.actions()) {
        bool ok = false;
        const int value = action->data().toInt(&ok);
        if (ok && value == expectedData) {
            return action;
        }

        if (auto* childMenu = action->menu()) {
            if (auto* found = findActionWithData(*childMenu, expectedData)) {
                return found;
            }
        }
    }

    return nullptr;
}

}  // namespace

class testSelectionMenu: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
        if (!Gui::Application::Instance) {
            Gui::Application::initApplication();
            _guiApplication = std::make_unique<Gui::Application>(false);
        }
    }

    void init()
    {
        App::DocumentInitFlags createFlags;
        createFlags.createView = false;
        _docName = App::GetApplication().getUniqueDocumentName("selection_menu_test");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser", createFlags);
        _firstObject = _doc->addObject("App::FeatureTest", "First");
        _secondObject = _doc->addObject("App::FeatureTest", "Second");
        Gui::Selection().setClarifySelectionActive(false);
        Gui::Selection().rmvPreselect();
    }

    void cleanup()
    {
        Gui::Selection().rmvPreselect();
        Gui::Selection().setClarifySelectionActive(false);
        Gui::Selection().clearSelection(_docName.c_str());
        if (App::GetApplication().getDocument(_docName.c_str())) {
            App::GetApplication().closeDocument(_docName.c_str());
        }
    }

    void hoverSetsAndReplacesPreselection()
    {
        TestableSelectionMenu menu;
        const auto selections = makeSelections();
        menu.populate(selections);

        auto* firstAction = findActionWithData(menu, 0);
        QVERIFY(firstAction);
        menu.onHover(firstAction);

        QVERIFY(Gui::Selection().hasPreselection());
        const auto& firstPreselection = Gui::Selection().getPreselection();
        QCOMPARE(QString::fromLatin1(firstPreselection.pObjectName), QStringLiteral("First"));
        QCOMPARE(QString::fromLatin1(firstPreselection.pSubName), QStringLiteral("Face1"));

        auto* secondAction = findActionWithData(menu, 1);
        QVERIFY(secondAction);
        menu.onHover(secondAction);

        QVERIFY(Gui::Selection().hasPreselection());
        const auto& secondPreselection = Gui::Selection().getPreselection();
        QCOMPARE(QString::fromLatin1(secondPreselection.pObjectName), QStringLiteral("Second"));
        QCOMPARE(QString::fromLatin1(secondPreselection.pSubName), QStringLiteral("Face2"));
    }

    void leaveClearsPreselectionPreview()
    {
        TestableSelectionMenu menu;
        const auto selections = makeSelections();
        menu.populate(selections);

        auto* firstAction = findActionWithData(menu, 0);
        QVERIFY(firstAction);
        menu.onHover(firstAction);
        QVERIFY(Gui::Selection().hasPreselection());

        menu.sendLeave();

        QVERIFY(!Gui::Selection().hasPreselection());
    }

    void doPickScopesClarifySelectionActiveState()
    {
        TestableSelectionMenu menu;
        bool activeWhileMenuIsOpen = false;

        QTimer::singleShot(0, &menu, [&menu, &activeWhileMenuIsOpen]() {
            activeWhileMenuIsOpen = Gui::Selection().isClarifySelectionActive();
            menu.close();
        });

        const auto picked = menu.doPick(makeSelections(), QPoint(1, 1));

        QVERIFY(!picked.obj);
        QVERIFY(activeWhileMenuIsOpen);
        QVERIFY(!Gui::Selection().isClarifySelectionActive());
        QVERIFY(!Gui::Selection().hasPreselection());
    }

private:
    std::vector<Gui::PickData> makeSelections() const
    {
        return {
            Gui::PickData {
                _firstObject,
                "Face",
                _docName,
                _firstObject->getNameInDocument(),
                "Face1",
            },
            Gui::PickData {
                _secondObject,
                "Face",
                _docName,
                _secondObject->getNameInDocument(),
                "Face2",
            },
        };
    }

    std::string _docName;
    App::Document* _doc {};
    App::DocumentObject* _firstObject {};
    App::DocumentObject* _secondObject {};
    std::unique_ptr<Gui::Application> _guiApplication;
};

QTEST_MAIN(testSelectionMenu)

#include "SelectionMenu.moc"
