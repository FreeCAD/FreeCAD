// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTest>
#include <QWidget>

#include <memory>
#include <string>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyStandard.h>
#include <App/VarSet.h>

#include "Gui/Application.h"
#include "Gui/ExpressionCompleter.h"
#include "Gui/Dialogs/DlgExpressionInput.h"
#include "Gui/MainWindow.h"

#include <src/App/InitApplication.h>

class testDlgExpressionInput: public QObject
{
    Q_OBJECT

public:
    testDlgExpressionInput()
    {
        tests::initApplication();
        ensureGuiApplication();
    }

private Q_SLOTS:
    void init()  // NOLINT
    {
        docName = App::GetApplication().getUniqueDocumentName("testDlgExpressionInput");
        App::DocumentInitFlags flags;
        flags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", flags);

        target = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Target"));
        QVERIFY(target != nullptr);
        targetProp = freecad_cast<App::PropertyFloat*>(
            target->addDynamicProperty("App::PropertyFloat", "TargetValue", "Variables")
        );
        QVERIFY(targetProp != nullptr);
    }

    void cleanup()  // NOLINT
    {
        if (doc) {
            App::GetApplication().closeDocument(docName.c_str());
        }
        doc = nullptr;
        target = nullptr;
        targetProp = nullptr;
        docName.clear();
    }

    void defaultAssignment_createsParametersVarSet()  // NOLINT
    {
        QVERIFY(doc->getObject("Parameters") == nullptr);

        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("x=42"));
        QVERIFY(okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(parameters->getPropertyByName("x"));
        QVERIFY(x != nullptr);
        QCOMPARE(x->getValue(), 42.0);
    }

    void defaultAssignment_updatesExistingProperty()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(
            parameters->addDynamicProperty("App::PropertyFloat", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(5);

        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("x=42"));
        QVERIFY(okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));
        QCOMPARE(x->getValue(), 42.0);
    }

    void qualifiedAssignmentByName_usesExistingVarSetOnly()  // NOLINT
    {
        auto* myPartVarSet = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "myPartVarSet"));
        QVERIFY(myPartVarSet != nullptr);

        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("myPartVarSet.x=42"));
        QVERIFY(okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));
        QVERIFY(doc->getObject("Parameters") == nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(myPartVarSet->getPropertyByName("x"));
        QVERIFY(x != nullptr);
        QCOMPARE(x->getValue(), 42.0);
    }

    void qualifiedAssignmentByName_fallsBackToUniqueLabel()  // NOLINT
    {
        auto* myPartVarSet = freecad_cast<App::VarSet*>(
            doc->addObject("App::VarSet", "myPartVarSet001")
        );
        QVERIFY(myPartVarSet != nullptr);
        myPartVarSet->Label.setValue("myPartVarSet");

        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("myPartVarSet.x=42"));
        QVERIFY(okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));
        QVERIFY(doc->getObject("Parameters") == nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(myPartVarSet->getPropertyByName("x"));
        QVERIFY(x != nullptr);
        QCOMPARE(x->getValue(), 42.0);
    }

    void qualifiedAssignmentByLabel_usesExistingVarSetOnly()  // NOLINT
    {
        auto* myPartVarSet = freecad_cast<App::VarSet*>(
            doc->addObject("App::VarSet", "myPartVarSet001")
        );
        QVERIFY(myPartVarSet != nullptr);
        myPartVarSet->Label.setValue("myPartVarSet");

        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("<<myPartVarSet>>.x=42"));
        QVERIFY(okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));
        QVERIFY(doc->getObject("Parameters") == nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(myPartVarSet->getPropertyByName("x"));
        QVERIFY(x != nullptr);
        QCOMPARE(x->getValue(), 42.0);
    }

    void rhsExpression_supportsLabelReference()  // NOLINT
    {
        auto* myPartVarSet = freecad_cast<App::VarSet*>(
            doc->addObject("App::VarSet", "myPartVarSet001")
        );
        QVERIFY(myPartVarSet != nullptr);
        myPartVarSet->Label.setValue("myPartVarSet");
        auto* seed = freecad_cast<App::PropertyFloat*>(
            myPartVarSet->addDynamicProperty("App::PropertyFloat", "seed", "Variables")
        );
        QVERIFY(seed != nullptr);
        seed->setValue(41);

        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("x=<<myPartVarSet>>.seed + 1"));
        QVERIFY(okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(parameters->getPropertyByName("x"));
        QVERIFY(x != nullptr);

        auto info = parameters->getExpression(App::ObjectIdentifier(*x));
        QVERIFY(info.expression != nullptr);
        parameters->ExpressionEngine.execute();
        QCOMPARE(x->getValue(), 42.0);
    }

    void missingQualifiedVarSet_reportsError()  // NOLINT
    {
        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("myPartVarSet.x=42"));

        QVERIFY(!okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Rejected));
        QVERIFY(doc->getObject("myPartVarSet") == nullptr);
        QVERIFY(doc->getObject("Parameters") == nullptr);
    }

    void defaultAssignment_namedParametersNonVarSet_reportsError()  // NOLINT
    {
        auto* nonVarSet = doc->addObject("App::DocumentObjectGroup", "Parameters");
        QVERIFY(nonVarSet != nullptr);

        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("x=42"));
        QVERIFY(okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Rejected));
        QVERIFY(doc->getObject("Parameters") != nullptr);
        QVERIFY(!doc->getObject("Parameters")->isDerivedFrom(App::VarSet::getClassTypeId()));
    }

    void defaultAssignment_labeledParametersNonVarSet_reportsError()  // NOLINT
    {
        auto* nonVarSet = doc->addObject("App::DocumentObjectGroup", "Group");
        QVERIFY(nonVarSet != nullptr);
        nonVarSet->Label.setValue("Parameters");

        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("x=42"));
        QVERIFY(okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Rejected));
        QVERIFY(doc->getObject("Parameters") == nullptr);
    }

    void qualifiedAssignmentByName_nonVarSet_reportsError()  // NOLINT
    {
        auto* nonVarSet = doc->addObject("App::DocumentObjectGroup", "myPartVarSet");
        QVERIFY(nonVarSet != nullptr);

        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("myPartVarSet.x=42"));

        QVERIFY(!okButton(dlg)->isEnabled());
        QVERIFY(doc->getObject("Parameters") == nullptr);
    }

    void qualifiedAssignmentByLabel_missing_reportsError()  // NOLINT
    {
        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("<<myPartVarSet>>.x=42"));

        QVERIFY(!okButton(dlg)->isEnabled());
        QVERIFY(doc->getObject("Parameters") == nullptr);
    }

    void assignment_invalidIdentifier_reportsError()  // NOLINT
    {
        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("1x=42"));

        QVERIFY(!okButton(dlg)->isEnabled());
        QVERIFY(doc->getObject("Parameters") == nullptr);
    }

    void assignment_unitName_reportsError()  // NOLINT
    {
        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("mm=42"));

        QVERIFY(!okButton(dlg)->isEnabled());
        QVERIFY(doc->getObject("Parameters") == nullptr);
    }

    void assignment_rejectsLeadingFormulaEquals()  // NOLINT
    {
        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("=x=42"));
        QVERIFY(!okButton(dlg)->isEnabled());
        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Rejected));
        QVERIFY(doc->getObject("Parameters") == nullptr);
    }

    void assignment_integerTarget_writesIntegerValue()  // NOLINT
    {
        auto* targetInt = freecad_cast<App::PropertyInteger*>(
            target->addDynamicProperty("App::PropertyInteger", "TargetInt", "Variables")
        );
        QVERIFY(targetInt != nullptr);

        QWidget parent;
        Gui::Dialog::DlgExpressionInput
            dlg(App::ObjectIdentifier(*targetInt), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("x=42"));
        QVERIFY(okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));
        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyInteger*>(parameters->getPropertyByName("x"));
        QVERIFY(x != nullptr);
        QCOMPARE(x->getValue(), 42L);
    }

    void assignment_reusesExistingIntegerPropertyType()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyInteger*>(
            parameters->addDynamicProperty("App::PropertyInteger", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(5);

        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("x=42"));
        QVERIFY(okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));

        auto* sameX = parameters->getPropertyByName("x");
        QVERIFY(sameX == x);
        QCOMPARE(x->getValue(), 42L);
    }

    void assignment_rejectsNonIntegerForExistingIntegerProperty()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyInteger*>(
            parameters->addDynamicProperty("App::PropertyInteger", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(5);

        QWidget parent;
        Gui::Dialog::DlgExpressionInput dlg(path(), nullptr, Base::Unit::One, &parent);
        setInputText(dlg, QStringLiteral("x=42.5"));
        QVERIFY(okButton(dlg)->isEnabled());

        dlg.accept();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Rejected));
        QCOMPARE(x->getValue(), 5L);
    }

private:
    static void ensureGuiApplication()
    {
        if (!Gui::Application::Instance) {
            Gui::Application::initApplication();
            Gui::Application::initOpenInventor();
            guiApp = std::make_unique<Gui::Application>(true);
        }
        if (!Gui::getMainWindow()) {
            mainWindow = std::make_unique<Gui::MainWindow>();
        }
    }

    App::ObjectIdentifier path() const
    {
        return App::ObjectIdentifier(*targetProp);
    }

    static Gui::ExpressionTextEdit* expressionWidget(Gui::Dialog::DlgExpressionInput& dlg)
    {
        auto* expression = dlg.findChild<Gui::ExpressionTextEdit*>("expression");
        Q_ASSERT(expression);
        return expression;
    }

    static QPushButton* okButton(Gui::Dialog::DlgExpressionInput& dlg)
    {
        auto* buttonBox = dlg.findChild<QDialogButtonBox*>("buttonBox");
        Q_ASSERT(buttonBox);
        auto* button = buttonBox->button(QDialogButtonBox::Ok);
        Q_ASSERT(button);
        return button;
    }

    static void setInputText(Gui::Dialog::DlgExpressionInput& dlg, const QString& text)
    {
        expressionWidget(dlg)->setPlainText(text);
        QCoreApplication::processEvents();
    }

private:
    static std::unique_ptr<Gui::Application> guiApp;
    static std::unique_ptr<Gui::MainWindow> mainWindow;

    std::string docName;
    App::Document* doc {};
    App::VarSet* target {};
    App::PropertyFloat* targetProp {};
};

std::unique_ptr<Gui::Application> testDlgExpressionInput::guiApp;
std::unique_ptr<Gui::MainWindow> testDlgExpressionInput::mainWindow;

QTEST_MAIN(testDlgExpressionInput)

#include "DlgExpressionInput.moc"
