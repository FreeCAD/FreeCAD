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

#include "src/App/InitApplication.h"  // same helper the QuantitySpinBox tests use

class testDlgAddProperty: public QObject
{
    Q_OBJECT

public:
    testDlgAddProperty()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void init()
    {
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
        Gui::Dialog::DlgAddProperty dlg(nullptr, varSet);  // <-- check ctor signature

        auto* comboType = dlg.findChild<QComboBox*>("comboBoxType");
        auto* lineEditName = dlg.findChild<QLineEdit*>("lineEditName");
        QVERIFY(comboType);
        QVERIFY(lineEditName);

        const int idx = comboType->findText(QStringLiteral("App::PropertyString"));
        QVERIFY(idx >= 0);
        comboType->setCurrentIndex(idx);
        QTest::keyClicks(lineEditName, QStringLiteral("Foo"));

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
