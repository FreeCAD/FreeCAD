// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QCoreApplication>
#include <QDebug>
#include <QFocusEvent>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/VarSet.h>

#include "Gui/Application.h"
#include "Gui/MainWindow.h"
#include "Gui/QuantitySpinBox.h"
#include "Gui/SpinBox.h"
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

class testQuantitySpinBox: public QObject
{
    Q_OBJECT

public:
    testQuantitySpinBox()
    {
        tests::initApplication();
        ensureGuiApplication();
    }

private Q_SLOTS:

    void init()
    {
        docName = App::GetApplication().getUniqueDocumentName("testQuantitySpinBox");
        App::DocumentInitFlags flags;
        flags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", flags);

        target = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Target"));
        QVERIFY(target != nullptr);
        targetFloat = freecad_cast<App::PropertyFloat*>(
            target->addDynamicProperty("App::PropertyFloat", "TargetFloat", "Variables")
        );
        QVERIFY(targetFloat != nullptr);
        targetInt = freecad_cast<App::PropertyInteger*>(
            target->addDynamicProperty("App::PropertyInteger", "TargetInt", "Variables")
        );
        QVERIFY(targetInt != nullptr);

        qsb = std::make_unique<Gui::QuantitySpinBox>();
    }

    void cleanup()
    {
        qsb.reset();
        if (doc) {
            App::GetApplication().closeDocument(docName.c_str());
        }
        doc = nullptr;
        target = nullptr;
        targetFloat = nullptr;
        targetInt = nullptr;
        docName.clear();
    }

    void test_SimpleBaseUnit()  // NOLINT
    {
        auto result = qsb->valueFromText("1mm");
        QCOMPARE(result, Base::Quantity(1, "mm"));
    }

    void test_UnitInNumerator()  // NOLINT
    {
        auto result = qsb->valueFromText("1mm/10");
        QCOMPARE(result, Base::Quantity(0.1, "mm"));
    }

    void test_UnitInDenominator()  // NOLINT
    {
        auto result = qsb->valueFromText("1/10mm");
        QCOMPARE(result, Base::Quantity(0.1, "mm"));
    }

    void test_KeepFormat()  // NOLINT
    {
        auto quant = qsb->value();
        auto format = quant.getFormat();
        format.setPrecision(7);
        quant.setFormat(format);

        qsb->setValue(quant);

        auto val1 = qsb->value();
        QCOMPARE(val1.getFormat().getPrecision(), 7);

        // format shouldn't change after setting a double
        qsb->setValue(3.5);
        auto val2 = qsb->value();
        QCOMPARE(val2.getFormat().getPrecision(), 7);
    }

    void test_InlineAssignmentCreatesParametersVarSet()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("x=42"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(parameters->getPropertyByName("x"));
        QVERIFY(x != nullptr);
        QCOMPARE(x->getValue(), 42.0);

        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression != nullptr);
    }

    void test_UnqualifiedNameResolvesInQuantitySpinBox()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(
            parameters->addDynamicProperty("App::PropertyFloat", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(41);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("x+1"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression != nullptr);
        const QString exprText = QString::fromStdString(info.expression->toString());
        QVERIFY(exprText.contains(QStringLiteral("Parameters.x")));
    }

    void test_AssignmentRhsResolvesUnqualifiedName()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(
            parameters->addDynamicProperty("App::PropertyFloat", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(41);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("y=x+1"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* y = freecad_cast<App::PropertyFloat*>(parameters->getPropertyByName("y"));
        QVERIFY(y != nullptr);
        auto yExpr = parameters->getExpression(App::ObjectIdentifier(*y));
        QVERIFY(yExpr.expression != nullptr);

        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression != nullptr);
    }

    void test_EnterCommitEmitsEditingFinished()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(
            parameters->addDynamicProperty("App::PropertyFloat", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(41);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());
        QSignalSpy spy(&spin, &QAbstractSpinBox::editingFinished);

        setEditorText(spin, QStringLiteral("x+1"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QCOMPARE(spy.count(), 1);
    }

    void test_InlineAssignmentWorksInUIntSpinBox()  // NOLINT
    {
        Gui::UIntSpinBox spin;
        spin.bind(pathInt());

        setEditorText(spin, QStringLiteral("n=32"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* n = freecad_cast<App::PropertyInteger*>(parameters->getPropertyByName("n"));
        QVERIFY(n != nullptr);
        QCOMPARE(n->getValue(), 32L);

        auto info = target->getExpression(pathInt());
        QVERIFY(info.expression != nullptr);
    }

    void test_TypedEqualsAllowsInlineAssignmentInQuantitySpinBox()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        auto* edit = spin.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        edit->clear();
        edit->setFocus();
        QTest::keyClicks(edit, QStringLiteral("x=42"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(parameters->getPropertyByName("x"));
        QVERIFY(x != nullptr);
        QCOMPARE(x->getValue(), 42.0);
    }

    void test_TypedEqualsAllowsInlineAssignmentInUIntSpinBox()  // NOLINT
    {
        Gui::UIntSpinBox spin;
        spin.bind(pathInt());

        auto* edit = spin.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        edit->clear();
        edit->setFocus();
        QTest::keyClicks(edit, QStringLiteral("n=32"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* n = freecad_cast<App::PropertyInteger*>(parameters->getPropertyByName("n"));
        QVERIFY(n != nullptr);
        QCOMPARE(n->getValue(), 32L);
    }

    void test_EmptyInputDoesNotEmitValueResetSignal()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);
        spin.setValue(10.0);

        auto* edit = spin.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);

        QSignalSpy spy(&spin, qOverload<double>(&Gui::QuantitySpinBox::valueChanged));
        edit->clear();
        QCoreApplication::processEvents();

        QCOMPARE(spy.count(), 0);
    }

    void test_QuantityAssignmentReusesExistingIntegerVariableType()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyInteger*>(
            parameters->addDynamicProperty("App::PropertyInteger", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(5);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("x=42"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* sameX = parameters->getPropertyByName("x");
        QVERIFY(sameX == x);
        QCOMPARE(x->getValue(), 42L);
        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression != nullptr);
    }

    void test_QuantityAssignmentRejectsNonIntegerForExistingIntegerVariable()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyInteger*>(
            parameters->addDynamicProperty("App::PropertyInteger", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(5);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("x=42.5"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QCOMPARE(x->getValue(), 5L);
        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression == nullptr);
    }

    void test_UIntEnterCommitAppliesExpression()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyInteger*>(
            parameters->addDynamicProperty("App::PropertyInteger", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(41);

        Gui::UIntSpinBox spin;
        spin.bind(pathInt());

        setEditorText(spin, QStringLiteral("x+1"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto info = target->getExpression(pathInt());
        QVERIFY(info.expression != nullptr);
        QVERIFY(
            QString::fromStdString(info.expression->toString()).contains(QStringLiteral("Parameters.x"))
        );
    }

    void test_UIntFocusOutCommitAppliesExpression()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyInteger*>(
            parameters->addDynamicProperty("App::PropertyInteger", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(41);

        Gui::UIntSpinBox spin;
        spin.bind(pathInt());

        setEditorText(spin, QStringLiteral("x+1"));
        QFocusEvent focusOut(QEvent::FocusOut, Qt::TabFocusReason);
        QCoreApplication::sendEvent(&spin, &focusOut);
        QCoreApplication::processEvents();

        auto info = target->getExpression(pathInt());
        QVERIFY(info.expression != nullptr);
        QVERIFY(
            QString::fromStdString(info.expression->toString()).contains(QStringLiteral("Parameters.x"))
        );
    }

    void test_InvalidExpressionIsRejectedOnEnter()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("x+"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression == nullptr);
        QCOMPARE(editorText(spin), QStringLiteral("x+"));
    }

    void test_OvaDistanceAssignmentCapturesExpressionAndCreatesLengthVar()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);

        setEditorText(spin, QStringLiteral("len=25 mm"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        const std::string exprText = spin.takeUnboundExpressionText();
        QVERIFY(!exprText.empty());
        QCOMPARE(spin.takeUnboundExpressionText(), std::string());
        QCOMPARE(QString::fromStdString(exprText), QStringLiteral("Parameters.len"));

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* len = parameters->getPropertyByName("len");
        QVERIFY(len != nullptr);
        QVERIFY(len->getTypeId().isDerivedFrom(App::PropertyLength::getClassTypeId()));
    }

    void test_OvaAngleAssignmentCapturesExpressionAndCreatesAngleVar()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Angle);

        setEditorText(spin, QStringLiteral("ang=30 deg"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        const std::string exprText = spin.takeUnboundExpressionText();
        QVERIFY(!exprText.empty());
        QCOMPARE(spin.takeUnboundExpressionText(), std::string());
        QCOMPARE(QString::fromStdString(exprText), QStringLiteral("Parameters.ang"));

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* ang = parameters->getPropertyByName("ang");
        QVERIFY(ang != nullptr);
        QVERIFY(ang->getTypeId().isDerivedFrom(App::PropertyAngle::getClassTypeId()));
    }

    void test_OvaZeroDistanceAssignmentStillCapturesExpression()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);

        setEditorText(spin, QStringLiteral("w=0 mm"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        const std::string exprText = spin.takeUnboundExpressionText();
        QVERIFY(!exprText.empty());
        QCOMPARE(QString::fromStdString(exprText), QStringLiteral("Parameters.w"));

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* w = parameters->getPropertyByName("w");
        QVERIFY(w != nullptr);
        QVERIFY(w->getTypeId().isDerivedFrom(App::PropertyLength::getClassTypeId()));
    }

    void test_OvaUnqualifiedReferenceStoresQualifiedExpression()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyLength*>(
            parameters->addDynamicProperty("App::PropertyLength", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(42.0);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);

        setEditorText(spin, QStringLiteral("x"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QCOMPARE(
            QString::fromStdString(spin.takeUnboundExpressionText()),
            QStringLiteral("Parameters.x")
        );
    }

    void test_OvaAssignmentFromVariableEvaluatesImmediately()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* y = freecad_cast<App::PropertyLength*>(
            parameters->addDynamicProperty("App::PropertyLength", "y", "Variables")
        );
        QVERIFY(y != nullptr);
        y->setValue(37.0);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);

        setEditorText(spin, QStringLiteral("x=y"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* x = freecad_cast<App::PropertyLength*>(parameters->getPropertyByName("x"));
        QVERIFY(x != nullptr);
        QCOMPARE(x->getValue(), 37.0);
        QVERIFY(parameters->getExpression(App::ObjectIdentifier(*x)).expression != nullptr);
        QCOMPARE(
            QString::fromStdString(spin.takeUnboundExpressionText()),
            QStringLiteral("Parameters.x")
        );
    }

    void test_OvaLeadingEqualsAssignmentRejected()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);

        setEditorText(spin, QStringLiteral("=w=25 mm"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QVERIFY(doc->getObject("Parameters") == nullptr);
        QCOMPARE(spin.takeUnboundExpressionText(), std::string());
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

    App::ObjectIdentifier pathFloat() const
    {
        return App::ObjectIdentifier(*targetFloat);
    }

    App::ObjectIdentifier pathInt() const
    {
        return App::ObjectIdentifier(*targetInt);
    }

    static void setEditorText(QAbstractSpinBox& spin, const QString& text)
    {
        auto* edit = spin.findChild<QLineEdit*>();
        Q_ASSERT(edit);
        edit->setText(text);
    }

    static QString editorText(QAbstractSpinBox& spin)
    {
        auto* edit = spin.findChild<QLineEdit*>();
        Q_ASSERT(edit);
        return edit->text();
    }

    static std::unique_ptr<Gui::Application> guiApp;
    static std::unique_ptr<Gui::MainWindow> mainWindow;
    std::unique_ptr<Gui::QuantitySpinBox> qsb;
    std::string docName;
    App::Document* doc {};
    App::VarSet* target {};
    App::PropertyFloat* targetFloat {};
    App::PropertyInteger* targetInt {};
};

std::unique_ptr<Gui::Application> testQuantitySpinBox::guiApp;
std::unique_ptr<Gui::MainWindow> testQuantitySpinBox::mainWindow;

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testQuantitySpinBox)

#include "QuantitySpinBox.moc"
