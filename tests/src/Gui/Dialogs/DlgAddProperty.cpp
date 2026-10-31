// SPDX-License-Identifier: LGPL-2.1-or-later
// tests/src/Gui/DlgAddProperty.cpp

/***************************************************************************
 *   Copyright (c) 2026 Chris Jones github.com/ipatch                      *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

#include <memory>

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

#include "src/App/InitApplication.h"

class testDlgAddProperty: public QObject
{
    Q_OBJECT

public:
    testDlgAddProperty()
    {
        tests::initApplication();
        // Gui::Application::Instance must not be nullptr: creating the property
        // records a macro line via Application::Instance->macroManager()
        if (!Gui::Application::Instance) {
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
            Gui::PropertyEditor::PropertyItemFactory::instance().createPropertyItem(
                "Gui::PropertyEditor::PropertyStringItem"
            )
        );
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

    // A string typed into the Add Property dialog must reach the property while
    // typing, as in 1.1.4. PropertyStringItem defers its commit to the property
    // view's delegate (#29900), which this dialog does not use.
    // see: https://github.com/FreeCAD/FreeCAD/issues/33394
    void test_StringValueIsWrittenWhileTyping()  // NOLINT
    {
        Gui::Dialog::DlgAddProperty dlg(nullptr, varSet);
        dlg.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dlg));

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
