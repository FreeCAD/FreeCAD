// SPDX-License-Identifier: LGPL-2.1-or-later

#include <memory>

#include <QLineEdit>
#include <QListWidget>
#include <QTest>
#include <QToolButton>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionView.h>
#include <src/App/InitApplication.h>

class SelectionCommandLogGuard
{
public:
    SelectionCommandLogGuard()
    {
        Gui::Selection().disableCommandLog();
    }

    ~SelectionCommandLogGuard()
    {
        Gui::Selection().enableCommandLog(true);
    }
};

class testSelectionView final: public QObject
{
    Q_OBJECT

public:
    testSelectionView()
    {
        tests::initApplication();
        if (!Gui::Application::Instance) {
            guiApplication = std::make_unique<Gui::Application>(false);
        }
        if (!Gui::getMainWindow()) {
            mainWindow = std::make_unique<Gui::MainWindow>();
        }
    }

private Q_SLOTS:
    void init()  // NOLINT
    {
        App::DocumentInitFlags createFlags;
        createFlags.createView = false;
        docName = App::GetApplication().getUniqueDocumentName("selection_view_test");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", createFlags);

        alpha = doc->addObject("App::FeatureTest", "AlphaObject");
        alpha->Label.setValue("Alpha target");
        beta = doc->addObject("App::FeatureTest", "BetaObject");
        beta->Label.setValue("Beta target");
    }

    void cleanup()  // NOLINT
    {
        SelectionCommandLogGuard guard;
        Gui::Selection().clearSelection(docName.c_str());
        if (App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }
        doc = nullptr;
        alpha = nullptr;
        beta = nullptr;
    }

    void clearingSearchRestoresCurrentSelection()  // NOLINT
    {
        Gui::DockWnd::SelectionView view(nullptr);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        selectTestObjects();
        QCOMPARE(view.selectionView->count(), 2);

        auto searchBox = view.findChild<QLineEdit*>();
        QVERIFY(searchBox);

        searchBox->setText(QStringLiteral("Alpha"));
        QCOMPARE(view.selectionView->count(), 1);

        searchBox->clear();
        QCOMPARE(view.selectionView->count(), 2);
        QCOMPARE(Gui::Selection().getSelection(docName.c_str(), Gui::ResolveMode::NoResolve).size(), 2U);

        searchBox->clearFocus();
        QCoreApplication::processEvents();
    }

    void clearButtonDoesNotApplyFilteredSelection()  // NOLINT
    {
        Gui::DockWnd::SelectionView view(nullptr);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        selectTestObjects();

        auto searchBox = view.findChild<QLineEdit*>();
        QVERIFY(searchBox);
        auto clearButton = view.findChild<QToolButton*>();
        QVERIFY(clearButton);
        QCOMPARE(clearButton->focusPolicy(), Qt::NoFocus);

        searchBox->setFocus();
        searchBox->setText(QStringLiteral("Alpha"));
        QCOMPARE(view.selectionView->count(), 1);

        QTest::mouseClick(clearButton, Qt::LeftButton);
        QCOMPARE(searchBox->text(), QString());
        QCOMPARE(view.selectionView->count(), 2);
        QCOMPARE(Gui::Selection().getSelection(docName.c_str(), Gui::ResolveMode::NoResolve).size(), 2U);

        searchBox->clearFocus();
        QCoreApplication::processEvents();
    }

private:
    void selectTestObjects()
    {
        SelectionCommandLogGuard guard;
        QVERIFY(Gui::Selection().addSelection(docName.c_str(), alpha->getNameInDocument()));
        QVERIFY(Gui::Selection().addSelection(docName.c_str(), beta->getNameInDocument()));
    }

    std::unique_ptr<Gui::Application> guiApplication;
    std::unique_ptr<Gui::MainWindow> mainWindow;
    std::string docName;
    App::Document* doc {};
    App::DocumentObject* alpha {};
    App::DocumentObject* beta {};
};

QTEST_MAIN(testSelectionView)

#include "SelectionView.moc"
