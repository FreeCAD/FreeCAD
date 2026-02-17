// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QDebug>
#include <QKeyEvent>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTest>

#include <memory>
#include <string>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Base/Exception.h>
#include <Base/Interpreter.h>

#include "Gui/QuantitySpinBox.h"
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

class testQuantitySpinBox: public QObject
{
    Q_OBJECT

public:
    testQuantitySpinBox()
    {
        tests::initApplication();
    }

private Q_SLOTS:

    void init()
    {
        docName = App::GetApplication().getUniqueDocumentName("qsbtest");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        App::GetApplication().setActiveDocument(doc);

        spreadsheetAvailable = false;
        try {
            Base::Interpreter().runString("import Spreadsheet");

            if (auto* sheet = doc->addObject("Spreadsheet::Sheet", "__qsb_sheet_probe")) {
                doc->removeObject(sheet->getNameInDocument());
                spreadsheetAvailable = true;
            }
        }
        catch (const Base::Exception&) {
            spreadsheetAvailable = false;
        }

        qsb = std::make_unique<Gui::QuantitySpinBox>();
    }

    void cleanup()
    {
        qsb.reset();

        if (!docName.empty()) {
            App::GetApplication().closeDocument(docName.c_str());
            docName.clear();
            doc = nullptr;
        }
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

    void test_AssignmentCreatesParametersAlias()  // NOLINT
    {
        if (!spreadsheetAvailable) {
            QSKIP("Spreadsheet::Sheet is unavailable in this test build");
        }
        QVERIFY(commitInlineText("width=10 mm"));

        QCOMPARE(qsb->takeUnboundExpressionText(), std::string("Parameters.width"));
        QVERIFY(doc->getObject("Parameters") != nullptr);
        QVERIFY(doc->getObject("Variables") == nullptr);
    }

    void test_BareAliasResolvesToParameters()  // NOLINT
    {
        if (!spreadsheetAvailable) {
            QSKIP("Spreadsheet::Sheet is unavailable in this test build");
        }
        QVERIFY(commitInlineText("width=10 mm"));
        qsb->takeUnboundExpressionText();

        QVERIFY(commitInlineText("width"));
        QCOMPARE(qsb->takeUnboundExpressionText(), std::string("Parameters.width"));
    }

    void test_FunctionQualificationResolvesAlias()  // NOLINT
    {
        if (!spreadsheetAvailable) {
            QSKIP("Spreadsheet::Sheet is unavailable in this test build");
        }
        QVERIFY(commitInlineText("width=10 mm"));
        qsb->takeUnboundExpressionText();

        QVERIFY(commitInlineText("sin(width)"));
        const std::string expression = qsb->takeUnboundExpressionText();
        QVERIFY(!expression.empty());
        QVERIFY(expression.find("Parameters.width") != std::string::npos);
    }

    void test_ZeroAssignmentStaysExpressionBound()  // NOLINT
    {
        if (!spreadsheetAvailable) {
            QSKIP("Spreadsheet::Sheet is unavailable in this test build");
        }
        QVERIFY(commitInlineText("x0=0 mm"));
        QCOMPARE(qsb->takeUnboundExpressionText(), std::string("Parameters.x0"));

        QVERIFY(commitInlineText("x0"));
        QCOMPARE(qsb->takeUnboundExpressionText(), std::string("Parameters.x0"));
        QCOMPARE(qsb->rawValue(), 0.0);
    }

    void test_DimensionlessAssignmentInheritsInputUnit()  // NOLINT
    {
        if (!spreadsheetAvailable) {
            QSKIP("Spreadsheet::Sheet is unavailable in this test build");
        }

        qsb->setUnitText(QStringLiteral("mm"));

        QVERIFY(commitInlineText("x=32"));
        QCOMPARE(qsb->takeUnboundExpressionText(), std::string("Parameters.x"));
        QVERIFY(!qsb->value().isDimensionless());
    }

    void test_UnitTokenIsNotQualifiedAsAlias()  // NOLINT
    {
        if (!spreadsheetAvailable) {
            QSKIP("Spreadsheet::Sheet is unavailable in this test build");
        }

        QVERIFY(commitInlineText("width=10 mm"));
        qsb->takeUnboundExpressionText();

        QVERIFY(commitInlineText("width + 1 mm"));
        const std::string expression = qsb->takeUnboundExpressionText();
        QVERIFY(!expression.empty());
        QVERIFY(expression.find("Parameters.width") != std::string::npos);
        QVERIFY(expression.find("Parameters.mm") == std::string::npos);
    }

    void test_BoundEqualsOpensFormulaDialog()  // NOLINT
    {
        auto* object = doc->addObject("App::DocumentObjectGroup", "__qsb_bound_obj");
        QVERIFY(object != nullptr);

        auto* property
            = object->addDynamicProperty("App::PropertyLength", "DrivenLength", "Base", "test");
        QVERIFY(property != nullptr);

        const QString boundPath = QString::fromLatin1(object->getNameInDocument())
            + QLatin1Char('.') + QString::fromLatin1(property->getName());
        qsb->setBoundToByName(boundPath);
        QVERIFY(qsb->isBound());

        QSignalSpy formulaDialogSpy(qsb.get(), &Gui::QuantitySpinBox::showFormulaDialog);

        auto* editor = qsb->findChild<QLineEdit*>();
        QVERIFY(editor != nullptr);
        editor->clear();

        QKeyEvent pressEquals(QEvent::KeyPress, Qt::Key_Equal, Qt::NoModifier, QStringLiteral("="));
        QCoreApplication::sendEvent(qsb.get(), &pressEquals);

        QVERIFY(!formulaDialogSpy.isEmpty());
        QCOMPARE(formulaDialogSpy.first().at(0).toBool(), true);
    }

    void test_ExpressionOverwrittenByPlainNumberInput()  // NOLINT
    {
        if (!spreadsheetAvailable) {
            QSKIP("Spreadsheet::Sheet is unavailable in this test build");
        }
        QVERIFY(commitInlineText("width=10 mm"));

        auto* editor = qsb->findChild<QLineEdit*>();
        QVERIFY(editor != nullptr);
        editor->setText(QStringLiteral("12 mm"));

        QVERIFY(!qsb->commitInlineExpressionTextForUi());
        QCOMPARE(qsb->takeUnboundExpressionText(), std::string());
    }

private:
    bool commitInlineText(const QString& text)
    {
        auto* editor = qsb->findChild<QLineEdit*>();
        if (!editor) {
            return false;
        }

        editor->setText(text);
        return qsb->commitInlineExpressionTextForUi();
    }

    std::unique_ptr<Gui::QuantitySpinBox> qsb;
    App::Document* doc {};
    std::string docName;
    bool spreadsheetAvailable {false};
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testQuantitySpinBox)

#include "QuantitySpinBox.moc"
