// SPDX-License-Identifier: LGPL-2.1-or-later

#include <memory>

#include <QCheckBox>
#include <QCoreApplication>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/ImagePlane.h>

#include "Gui/Application.h"
#include "Gui/MainWindow.h"
#include "Gui/QuantitySpinBox.h"
#include "Gui/TaskView/TaskImage.h"
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

namespace
{
std::unique_ptr<Gui::Application> guiApp;
std::unique_ptr<Gui::MainWindow> mainWindow;
}

class testTaskImage: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
        Gui::Application::initApplication();
        Gui::Application::initOpenInventor();
        guiApp = std::make_unique<Gui::Application>(true);
        mainWindow = std::make_unique<Gui::MainWindow>();
        mainWindow->show();
        QCoreApplication::processEvents();
    }

    void init()
    {
        docName = App::GetApplication().getUniqueDocumentName("TaskImage");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");

        image = doc->addObject<Image::ImagePlane>("Image");
        image->XSize.setValue(100.0);
        image->YSize.setValue(50.0);

        QVERIFY(Gui::Application::Instance->getViewProvider(image) != nullptr);

        widget = std::make_unique<Gui::TaskImage>(image);
        QCoreApplication::processEvents();
    }

    void cleanup()
    {
        widget.reset();

        App::GetApplication().closeDocument(docName.c_str());
        image = nullptr;
        doc = nullptr;
    }

    void cleanupTestCase()
    {
        mainWindow.reset();
        guiApp.reset();
    }

    void widthChangesUpdateImagePlaneImmediately()
    {
        auto* width = widget->findChild<Gui::QuantitySpinBox*>("spinBoxWidth");
        QVERIFY(width != nullptr);

        QCOMPARE(image->XSize.getValue(), 100.0);

        width->setValue(200.0);
        QCoreApplication::processEvents();

        QCOMPARE(image->XSize.getValue(), 200.0);
    }

    void ratioLockedWidthChangeUpdatesBothSizesImmediately()
    {
        auto* width = widget->findChild<Gui::QuantitySpinBox*>("spinBoxWidth");
        auto* ratio = widget->findChild<QCheckBox*>("checkBoxRatio");

        QVERIFY(width != nullptr);
        QVERIFY(ratio != nullptr);

        ratio->setChecked(true);
        width->setValue(300.0);
        QCoreApplication::processEvents();

        QCOMPARE(image->XSize.getValue(), 300.0);
        QCOMPARE(image->YSize.getValue(), 150.0);
    }

private:
    std::string docName;
    App::Document* doc {};
    Image::ImagePlane* image {};
    std::unique_ptr<Gui::TaskImage> widget;
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testTaskImage)

#include "TaskImage.moc"
