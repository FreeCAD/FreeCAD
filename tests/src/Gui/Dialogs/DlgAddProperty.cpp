// SPDX-License-Identifier: LGPL-2.1-or-later
// tests/src/Gui/DlgAddProperty.cpp

#include <QComboBox>
#include <QLineEdit>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/PropertyStandard.h>
#include <App/VarSet.h>
#include <Gui/Dialogs/DlgAddProperty.h>
#include <Gui/Application.h>
#include <Gui/propertyeditor/PropertyItem.h>

#include "src/App/InitApplication.h"  // same helper the QuantitySpinBox tests use

class testDlgAddProperty: public QObject
{
    Q_OBJECT

public:
    testDlgAddProperty()
    {
        tests::initApplication();
        // Gui::Application::Instance must not be nullptr: creating the property
        // records a macro line via Application::Instance->macroManager()
        if(!Gui::Application::Instance) {
            new Gui::Application(false);
        }
        // The dialog builds its value editor through PropertyItemFactory, which
        // is only populated during full Gui startup; register the item under test.
        Gui::PropertyEditor::PropertyStringItem::init();
    }

private Q_SLOTS:
    void initTestCase()
    {
        std::unique_ptr<Gui::PropertyEditor::PropertyItem> probe(
            Gui::PropertyEditor::PropertyItemFactory::instance()
                .createPropertyItem("Gui::PropertyEditor::PropertyStringItem"));
        QVERIFY2(probe, "PropertyStringItem not registered with PropertyItemFactory");
    }

    void init()
    {
        // Start from a non-string type so the test drives a real type change to
        // App::PropertyString, independent of the last type used in the GUI.
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/PropertyView")
            ->SetASCII("NewPropertyType", "App::PropertyLength");

        docName = App::GetApplication().getUniqueDocumentName("test");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        varSet = doc->addObject<App::VarSet>("VarSet");
    }

    void cleanup()
    {
        App::GetApplication().closeDocument(docName.c_str());
    }

    // see: https://github.com/FreeCAD/FreeCAD/issues/33394
    void test_StringValueIsWrittenWhileTyping()  // NOLINT
    {
        auto* probe = Gui::PropertyEditor::PropertyItemFactory::instance()
            .createPropertyItem("Gui::PropertyEditor::PropertyStringItem");
        QVERIFY2(probe, "PropertyItemFactory has no PropertyStringItem registered");
        delete probe;

        Gui::Dialog::DlgAddProperty dlg(nullptr, varSet);
        dlg.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dlg));   // works with QT_QPA_PLATFORM=offscreen

        auto dumpEditor = [&dlg](const char* where) {
            auto* w = dlg.findChild<QWidget*>("editor");
            qDebug() << where << "editor:" << (w ? w->metaObject()->className() : "none");
        };

        auto* comboType = dlg.findChild<QComboBox*>("comboBoxType");
        auto* lineEditName = dlg.findChild<QLineEdit*>("lineEditName");
        QVERIFY(comboType);
        QVERIFY(lineEditName);
        dumpEditor("after ctor");

        const int idx = comboType->findText(QStringLiteral("App::PropertyString"));
        QVERIFY(idx >= 0);
        comboType->setCurrentIndex(idx);
        QCoreApplication::processEvents();
        dumpEditor("after type");

        QTest::keyClicks(lineEditName, QStringLiteral("Foo"));
        QCoreApplication::processEvents();
        dumpEditor("after name");

        auto* editor = dlg.findChild<QLineEdit*>("editor");
        QVERIFY2(editor, "no value editor created for App::PropertyString");
        QTest::keyClicks(editor, QStringLiteral("baz"));

        auto* prop = dynamic_cast<App::PropertyString*>(varSet->getPropertyByName("Foo"));
        QVERIFY2(prop, "property Foo was not created on the VarSet");
        QCOMPARE(QString::fromUtf8(prop->getValue()), QStringLiteral("baz"));
    }

private:
    std::string docName;
    App::Document* doc {};
    App::VarSet* varSet {};
};

QTEST_MAIN(testDlgAddProperty)

#include "DlgAddProperty.moc"
