// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QDebug>
#include <QLineEdit>
#include <QTest>
#include <QSignalSpy>

#include <App/Application.h>
#include <Base/UnitsApi.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>
#include <App/Property.h>

#include "Gui/QuantitySpinBox.h"
#include "Gui/PrefWidgets.h"
#include <src/LocaleTestHelpers.h>
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

/// Gives the tests access to the line edit, so that input can be fed in the same way the user
/// types it. This is what fills the validated string that isNormalized() inspects.
class QuantitySpinBoxWithLineEdit: public Gui::QuantitySpinBox
{
public:
    using QAbstractSpinBox::lineEdit;
};

namespace
{
/// Schema names as registered in Base::UnitsSchemasData
const QString standardSchema = QStringLiteral("Internal");
const QString usCustomarySchema = QStringLiteral("Imperial");
const QString imperialDecimalSchema = QStringLiteral("ImperialDecimal");
const QString buildingUsSchema = QStringLiteral("ImperialBuilding");
class ScopedExpressionOwner
{
public:
    ScopedExpressionOwner()
        : documentName {App::GetApplication().getUniqueDocumentName("quantity_spinbox")}
        , document {App::GetApplication().newDocument(documentName.c_str(), "testUser")}
        , object {document->addObject("App::VarSet", "VarSet")}
        , property {object->addDynamicProperty("App::PropertyFloat", "Value", "Test")}
    {}

    ~ScopedExpressionOwner()
    {
        App::GetApplication().closeDocument(documentName.c_str());
    }

    App::ObjectIdentifier getPath() const
    {
        return App::ObjectIdentifier(*property);
    }

private:
    std::string documentName;
    App::Document* document;
    App::DocumentObject* object;
    App::Property* property;
};
}  // namespace

class testQuantitySpinBox: public QObject
{
    Q_OBJECT

public:
    testQuantitySpinBox()
    {
        tests::initApplication();
        qsb = std::make_unique<Gui::QuantitySpinBox>();
    }

private Q_SLOTS:

    void initTestCase()
    {
        Base::UnitsApi::setSchema("Internal");
    }

    void init()
    {}

    void cleanup()
    {
        // some tests switch the unit schema, the next one has to start from the default again
        Base::UnitsApi::setSchema("Internal");
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

    void test_isNormalized_data()  // NOLINT
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("normalized");

        // A bare number is already a solution, whatever its formatting
        QTest::newRow("integer") << "5" << true;
        QTest::newRow("zero") << "0" << true;
        QTest::newRow("decimals") << "5.00" << true;

        // Attaching a unit needs no calculation either, and neither does the whitespace
        // between the number and its unit
        QTest::newRow("number with unit") << "5.00mm" << true;
        QTest::newRow("number with spaced unit") << "5.00 mm" << true;

        // A leading sign is part of the value, not something to be calculated. Whitespace
        // around it is irrelevant even though the canonical form has none.
        QTest::newRow("negative number") << "-5.00" << true;
        QTest::newRow("positive number") << "+5.00" << true;
        QTest::newRow("negative number with unit") << "-5.00mm" << true;
        QTest::newRow("positive number with unit") << "+5.00mm" << true;
        QTest::newRow("negative number with spaced unit") << "-5.00 mm" << true;
        QTest::newRow("negative number with spaced sign") << "- 5.00 mm" << true;
        QTest::newRow("positive number with spaced sign") << "+ 5.00 mm" << true;

        // Everything that can still be simplified has to be reported as not normalized, so
        // that the result gets shown before it is accepted
        QTest::newRow("multiplication") << "5*2" << false;
        QTest::newRow("multiplication with unit") << "5.00mm * 2" << false;
        QTest::newRow("division") << "10/2" << false;
        QTest::newRow("division with unit") << "5.00mm/2" << false;
        QTest::newRow("unit in denominator") << "1/10mm" << false;
        QTest::newRow("addition") << "5+2" << false;
        QTest::newRow("addition with unit") << "5mm + 2mm" << false;
        QTest::newRow("subtraction") << "5-2" << false;
        QTest::newRow("subtraction with unit") << "5mm - 2mm" << false;
        QTest::newRow("power") << "2^3" << false;
        QTest::newRow("negated product") << "-5*2" << false;
        QTest::newRow("negated sum in parentheses") << "-(5+2)" << false;
    }

    void test_isNormalized()  // NOLINT
    {
        QFETCH(QString, input);
        QFETCH(bool, normalized);

        auto spinBox = lengthSpinBox(input);
        QVERIFY(spinBox->hasValidInput());

        QCOMPARE(spinBox->isNormalized(), normalized);
    }

    void test_isNormalizedImperial_data()  // NOLINT
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("normalized");

        // Feet, inches and fractions of an inch are how a value is written down in this
        // schema, so an input already in that form is a solution and not a pending
        // calculation, even though it is spelled with operators
        QTest::newRow("inches") << "6\"" << true;
        QTest::newRow("feet") << "2'" << true;
        QTest::newRow("named inches") << "5 in" << true;
        QTest::newRow("negative named inches") << "-5 in" << true;
        QTest::newRow("feet and inches") << "1' 6\"" << true;
        QTest::newRow("negative feet and inches") << "-1' 6\"" << true;
        QTest::newRow("fraction of an inch") << "1/2\"" << true;
        QTest::newRow("inches and fraction") << "1\" + 1/2\"" << true;

        // Sums and products that do not spell out a single value still have to be calculated
        // first, even when they are written with imperial units
        QTest::newRow("sum of inches") << "6\" + 1\"" << false;
        QTest::newRow("scaled inches") << "2\" * 3" << false;
        QTest::newRow("halved feet") << "1'/2" << false;
    }

    void test_isNormalizedImperial()  // NOLINT
    {
        QFETCH(QString, input);
        QFETCH(bool, normalized);

        Base::UnitsApi::setSchema(buildingUsSchema.toStdString());

        auto spinBox = lengthSpinBox(input);
        QVERIFY(spinBox->hasValidInput());

        QCOMPARE(spinBox->isNormalized(), normalized);
    }

    void test_isNormalizedImperialInMetricSchema()  // NOLINT
    {
        // The standard schema displays this as 457.20 mm, a value the user has not seen yet,
        // so it has to be shown before the input can be accepted
        Base::UnitsApi::setSchema(standardSchema.toStdString());

        auto spinBox = lengthSpinBox(QStringLiteral("1' 6\""));
        QVERIFY(spinBox->hasValidInput());

        QCOMPARE(spinBox->isNormalized(), false);
    }

    void test_isNormalizedDisplayedValue_data()  // NOLINT
    {
        QTest::addColumn<QString>("schema");
        QTest::addColumn<double>("millimetres");

        QTest::newRow("standard") << standardSchema << 38.1;
        QTest::newRow("us customary") << usCustomarySchema << 38.1;
        QTest::newRow("imperial decimal") << imperialDecimalSchema << 38.1;
        QTest::newRow("building us, whole inches") << buildingUsSchema << 25.4;
        QTest::newRow("building us, fraction") << buildingUsSchema << 12.7;
        QTest::newRow("building us, inches and fraction") << buildingUsSchema << 38.1;
        QTest::newRow("building us, feet and inches") << buildingUsSchema << 457.2;
    }

    void test_isNormalizedDisplayedValue()  // NOLINT
    {
        QFETCH(QString, schema);
        QFETCH(double, millimetres);

        Base::UnitsApi::setSchema(schema.toStdString());

        QuantitySpinBoxWithLineEdit spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setValue(Base::Quantity(millimetres, Base::Unit::Length));

        // Whatever the spin box wrote out itself is the value the user is looking at, so
        // there is nothing left to show them and enter can be accepted right away
        QVERIFY(spinBox.hasValidInput());

        QCOMPARE(spinBox.isNormalized(), true);
    }

    void test_isNormalizedSignedConditional()  // NOLINT
    {
        // The operand of a sign is not necessarily a number or another operator, here it is a
        // conditional expression. It still has to be calculated before it can be accepted.
        auto spinBox = lengthSpinBox(QStringLiteral("-(1 > 0 ? 2 : 3)"));
        QVERIFY(spinBox->hasValidInput());

        QCOMPARE(spinBox->isNormalized(), false);
    }

    void test_MismatchedFormatterAndWidgetLocaleDoesNotMutateValue()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "da_DK",
             .formattingLocale = "en_US",
             .icuLocale = "fr_FR",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setValue(Base::Quantity(10, "mm"));

        QCOMPARE(spinBox.value(), Base::Quantity(10, "mm"));
        QCOMPARE(spinBox.rawValue(), 10.0);
    }

    void test_NativeDigitRoundTripUsesTheWidgetLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "fa_IR",
             .formattingLocale = "fa_IR",
             .icuLocale = "fa_IR",
             .useQtSeparators = true}
        };

        const QLocale persian(QStringLiteral("fa_IR"));
        Base::Quantity quantity(1234.5, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 1);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        Gui::QuantitySpinBox unbound;
        unbound.setLocale(persian);
        unbound.setKeyboardTracking(false);
        unbound.setValue(quantity);
        unbound.show();
        auto* unboundEdit = unbound.findChild<QLineEdit*>();
        QVERIFY(unboundEdit != nullptr);
        if (!unboundEdit) {
            return;
        }
        const QString formatted = unbound.text();
        QVERIFY(formatted.contains(QChar(0x06F1)));
        unboundEdit->setText(formatted);
        unbound.selectNumber();
        const int unitStart = formatted.indexOf(QStringLiteral(" mm"));
        QVERIFY(unitStart > 0);
        QCOMPARE(unboundEdit->selectedText(), formatted.left(unitStart));
        QTest::keyClick(&unbound, Qt::Key_Return);
        QCOMPARE(unbound.rawValue(), 1234.5);
        QVERIFY(unbound.hasValidInput());

        ScopedExpressionOwner owner;
        Gui::QuantitySpinBox bound;
        bound.setLocale(persian);
        bound.setKeyboardTracking(false);
        bound.bind(owner.getPath());
        bound.setUnit(Base::Unit::Length);
        bound.setValue(quantity);
        bound.show();
        auto* boundEdit = bound.findChild<QLineEdit*>();
        QVERIFY(boundEdit != nullptr);
        if (!boundEdit) {
            return;
        }
        boundEdit->setText(formatted);
        bound.selectNumber();
        QCOMPARE(boundEdit->selectedText(), formatted.left(unitStart));
        QTest::keyClick(&bound, Qt::Key_Return);
        QCOMPARE(bound.rawValue(), 1234.5);
        QVERIFY(bound.hasValidInput());
    }

    void test_LocaleChangeReformatsAndParsesWithTheNewWidgetLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setLocale(QLocale(QStringLiteral("en_US")));
        Base::Quantity quantity(1234.5, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 1);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);
        spinBox.show();
        QVERIFY(spinBox.text().contains(QStringLiteral("1,234.5")));

        spinBox.setLocale(QLocale(QStringLiteral("de_DE")));
        QCoreApplication::processEvents();
        QVERIFY(spinBox.text().contains(QStringLiteral("1.234,5")));

        spinBox.findChild<QLineEdit*>()->setText(QStringLiteral("2.345,6 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QCOMPARE(spinBox.rawValue(), 2345.6);
        QVERIFY(spinBox.hasValidInput());
    }

    void test_GroupedLocaleNumberIsNormalizedBeforeParse()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "da_DK",
             .formattingLocale = "en_US",
             .icuLocale = "fr_FR",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        const auto result = spinBox.valueFromText("1.000,00 mm");

        QCOMPARE(result, Base::Quantity(1000, "mm"));
    }

    void test_CanonicalDecimalPointRemainsAcceptedInCommaLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "da_DK",
             .formattingLocale = "en_US",
             .icuLocale = "fr_FR",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        const auto result = spinBox.valueFromText("10.00 mm");

        QCOMPARE(result, Base::Quantity(10, "mm"));
    }

    void test_IndianGroupedLocaleNumberIsNormalizedBeforeParse()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_IN",
             .formattingLocale = "en_IN",
             .icuLocale = "en_IN",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        const auto result = spinBox.valueFromText("12,34,567 mm");

        QCOMPARE(result, Base::Quantity(1234567, "mm"));
    }

    void test_GroupedScientificNotationIsNormalizedBeforeParse_data()  // NOLINT
    {
        QTest::addColumn<QString>("input");
        QTest::newRow("lowercase exponent") << QStringLiteral("1,234e5 mm");
        QTest::newRow("uppercase exponent") << QStringLiteral("1,234E5 mm");
    }

    void test_GroupedScientificNotationIsNormalizedBeforeParse()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };
        QFETCH(QString, input);

        Gui::QuantitySpinBox spinBox;
        const auto result = spinBox.valueFromText(input);

        QCOMPARE(result, Base::Quantity(123400000, "mm"));
    }

    void test_GroupedEditDoesNotCorruptRawValue()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        Base::Quantity quantity(10.0, "mm");

        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        spinBox.setValue(quantity);
        spinBox.show();
        spinBox.setFocus();
        spinBox.selectNumber();
        QTest::keyClicks(&spinBox, "1,010.00");
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QCOMPARE(spinBox.rawValue(), 1010.0);
        QCOMPARE(spinBox.text(), QStringLiteral("1,010.00 mm"));
    }

    void test_BoundPrefSpinBoxGroupedDecimalUsesWidgetLocale()  // NOLINT
    {
        // Start aligned so the widget is constructed and initially displayed
        // using US numeric formatting.
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };
        Base::UnitsApi::setSchema("FEM");

        ScopedExpressionOwner owner;

        // Sketcher's datum dialog uses PrefQuantitySpinBox rather than a plain
        // QuantitySpinBox.
        Gui::PrefQuantitySpinBox spinBox;

        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        spinBox.setValue(quantity);
        spinBox.bind(owner.getPath());

        spinBox.show();
        spinBox.setFocus();
        spinBox.selectNumber();

        {
            // Reproduce the suspected real-world mismatch:
            //
            //   widget locale:       en_US, decimal ".", grouping ","
            //   shared parse state:  decimal ",", grouping "."
            //
            // The widget still displays and accepts US-style text, but the
            // current implementation parses through the stale shared state.
            tests::ScopedNumericLocaleContext staleFormatting {
                {"en_US", ",", ".", "+", "-", 3, 3, "0"}
            };

            QTest::keyClicks(&spinBox, "12,345.67");
            QTest::keyClick(&spinBox, Qt::Key_Return);
        }

        QCOMPARE(spinBox.hasValidInput(), true);
        QCOMPARE(spinBox.rawValue(), 12345.67);
        QCOMPARE(spinBox.text(), QStringLiteral("12,345.67 mm"));
    }

    void test_BoundGroupedDecimalEditUsesWidgetLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };
        Base::UnitsApi::setSchema("FEM");
        ScopedExpressionOwner owner;

        Gui::QuantitySpinBox spinBox;
        spinBox.bind(owner.getPath());
        spinBox.setUnit(Base::Unit::Length);

        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        spinBox.setValue(quantity);
        spinBox.show();
        spinBox.setFocus();
        spinBox.selectNumber();

        QTest::keyClicks(&spinBox, "12,345.67");
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QCOMPARE(spinBox.rawValue(), 12345.67);
    }

    void test_EffectiveSeparatorsOverrideFormattingLocaleId()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "pt_PT", .formattingLocale = "en_US", .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        Base::Quantity quantity(1.5, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        spinBox.setValue(quantity);

        QCOMPARE(spinBox.text(), QStringLiteral("1,50 mm"));
        QCOMPARE(spinBox.text().at(1), QLocale().decimalPoint());
    }

    void test_FunctionArgumentSeparatorSurvivesQuantityParsing()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };
        ScopedExpressionOwner owner;

        Gui::QuantitySpinBox spinBox;
        spinBox.bind(owner.getPath());
        spinBox.setUnit(Base::Unit::Length);
        // Use a non-unit function name here. The helper-level min(1,234) case above still covers
        // separator preservation for that spelling, but the real parser tokenizes "min" as minute.
        const auto result = spinBox.valueFromText("pow(1, 234)");

        QCOMPARE(result.getValue(), 1.0);
    }

    void test_InvalidCommittedGroupingRemainsVisibleAndEscapes()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);
        spinBox.show();
        spinBox.setFocus();

        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);
        spinBox.findChild<QLineEdit*>()->setText(QStringLiteral("12,34,567 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QCOMPARE(spinBox.text(), QStringLiteral("12,34,567 mm"));
        QCOMPARE(spinBox.rawValue(), 10.0);
        QVERIFY(!spinBox.hasValidInput());
        QCOMPARE(rejected.count(), 1);
        const auto arguments = rejected.at(0);
        QVERIFY(arguments.at(0).toString().contains(QStringLiteral("Malformed grouping")));
        QVERIFY(arguments.at(1).toInt() >= 0);
        QVERIFY(arguments.at(2).toInt() > 0);

        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }
        QVERIFY(edit->property("numericInputInvalid").toBool());

        QTest::keyClick(&spinBox, Qt::Key_Escape);
        QCOMPARE(spinBox.text(), QStringLiteral("10.00 mm"));
        QCOMPARE(spinBox.rawValue(), 10.0);
        QVERIFY(spinBox.hasValidInput());
        QVERIFY(!edit->property("numericInputInvalid").toBool());
    }

    void test_ReturnSignalsMatchCommitResult()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();

        QSignalSpy changed(
            &spinBox,
            qOverload<const Base::Quantity&>(&Gui::QuantitySpinBox::valueChanged)
        );
        QSignalSpy returnPressed(&spinBox, &Gui::QuantitySpinBox::returnPressed);
        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);
        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }

        edit->setText(QStringLiteral("11 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QCOMPARE(returnPressed.count(), 1);
        QCOMPARE(rejected.count(), 0);
        QCOMPARE(changed.count(), 1);

        edit->setText(QStringLiteral("12,34,567 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QCOMPARE(returnPressed.count(), 1);
        QCOMPARE(rejected.count(), 1);
        QCOMPARE(changed.count(), 1);
    }

    void test_MultibyteDiagnosticSelectsTheCompleteSeparator()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "fr_FR",
             .formattingLocale = "fr_FR",
             .icuLocale = "fr_FR",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setLocale(QLocale(QStringLiteral("fr_FR")));
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();
        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }

        const QString narrowSpace = QString::fromUtf8("\xE2\x80\xAF");
        edit->setText(QStringLiteral("12") + narrowSpace + QStringLiteral("34 mm"));
        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QCOMPARE(rejected.count(), 1);
        const auto args = rejected.at(0);
        const int start = args.at(1).toInt();
        const int length = args.at(2).toInt();
        QCOMPARE(edit->text().mid(start, length), narrowSpace);
        QCOMPARE(length, narrowSpace.size());
        QCOMPARE(length, 1);
        QVERIFY(!edit->text().contains(QChar::ReplacementCharacter));
    }

    void test_ValidEditClearsTheInvalidState()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();
        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }

        QTest::keyClicks(edit, QStringLiteral("12,34,567 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QVERIFY(edit->property("numericInputInvalid").toBool());

        edit->setText(QStringLiteral("11 mm"));
        QVERIFY(!edit->property("numericInputInvalid").toBool());
        QVERIFY(edit->toolTip().isEmpty());
        QVERIFY(spinBox.hasValidInput());
    }

    void test_InvalidFocusLossCommitsOnce()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();

        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);
        spinBox.findChild<QLineEdit*>()->setText(QStringLiteral("12,34,567 mm"));
        QLineEdit other;
        other.show();
        other.setFocus();

        QTRY_COMPARE(rejected.count(), 1);
        QCOMPARE(spinBox.rawValue(), 10.0);
    }

    void test_ValidFocusLossCommitsOnce()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setKeyboardTracking(false);
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();
        QSignalSpy changed(
            &spinBox,
            qOverload<const Base::Quantity&>(&Gui::QuantitySpinBox::valueChanged)
        );
        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);

        spinBox.findChild<QLineEdit*>()->setText(QStringLiteral("11 mm"));
        QLineEdit other;
        other.show();
        other.setFocus();
        QTRY_COMPARE(changed.count(), 1);
        QCOMPARE(rejected.count(), 0);

        QLineEdit another;
        another.show();
        another.setFocus();
        QTest::qWait(0);
        QCOMPARE(changed.count(), 1);
    }

    void test_IncompleteEditHasNoTooltipAndDoesNotCommit()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);
        spinBox.show();
        spinBox.setFocus();

        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }
        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);

        edit->setText(QStringLiteral("-"));

        QVERIFY(!spinBox.hasValidInput());
        QCOMPARE(spinBox.rawValue(), 10.0);
        QVERIFY(edit->toolTip().isEmpty());
        QCOMPARE(rejected.count(), 0);

        QTest::keyClick(&spinBox, Qt::Key_Return);

        QVERIFY(!spinBox.hasValidInput());
        QCOMPARE(spinBox.rawValue(), 10.0);
        QCOMPARE(spinBox.text(), QStringLiteral("-"));
        QCOMPARE(rejected.count(), 1);
        QVERIFY(rejected.at(0).at(0).toString().contains(QStringLiteral("Incomplete number")));

        QTest::keyClick(&spinBox, Qt::Key_Escape);
        QCOMPARE(spinBox.text(), QStringLiteral("10.00 mm"));
        QVERIFY(spinBox.hasValidInput());
    }

    void test_IncompatibleUnitAndRangeDiagnosticsRemainVisible()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();

        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }
        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);

        edit->setText(QStringLiteral("1 s"));
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QVERIFY(!spinBox.hasValidInput());
        QCOMPARE(spinBox.rawValue(), 10.0);
        QCOMPARE(rejected.count(), 1);
        QVERIFY(rejected.at(0).at(0).toString().contains(QStringLiteral("Incompatible unit")));

        QTest::keyClick(&spinBox, Qt::Key_Escape);
        spinBox.setRange(0.0, 10.0);
        edit->setText(QStringLiteral("11 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QVERIFY(!spinBox.hasValidInput());
        QCOMPARE(spinBox.rawValue(), 10.0);
        QCOMPARE(rejected.count(), 2);
        QVERIFY(rejected.at(1).at(0).toString().contains(QStringLiteral("outside the allowed range")));
    }

    void test_ValidGroupedCommitUpdatesQuantity()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };
        Base::UnitsApi::setSchema("FEM");

        Gui::QuantitySpinBox spinBox;
        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);
        spinBox.show();
        spinBox.setFocus();

        spinBox.findChild<QLineEdit*>()->setText(QStringLiteral("12,345.67 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QCOMPARE(spinBox.text(), QStringLiteral("12,345.67 mm"));
        QCOMPARE(spinBox.rawValue(), 12345.67);
        QVERIFY(spinBox.hasValidInput());
    }

private:
    /// Builds a length spin box holding the given input, entered the way the user types it.
    /// isNormalized() inspects the last input accepted by the validator, so callers have to
    /// check hasValidInput() as well, otherwise a rejected string silently leaves the
    /// previous one in place.
    static std::unique_ptr<QuantitySpinBoxWithLineEdit> lengthSpinBox(const QString& input)
    {
        auto spinBox = std::make_unique<QuantitySpinBoxWithLineEdit>();
        spinBox->setUnit(Base::Unit::Length);
        spinBox->lineEdit()->setText(input);

        return spinBox;
    }

    std::unique_ptr<Gui::QuantitySpinBox> qsb;
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testQuantitySpinBox)

#include "QuantitySpinBox.moc"
