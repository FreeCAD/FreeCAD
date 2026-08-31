// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QDebug>
#include <QLineEdit>
#include <QTest>

#include <App/Application.h>
#include <Base/UnitsApi.h>

#include "Gui/QuantitySpinBox.h"
#include "TestSupport.h"

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
}  // namespace

class testQuantitySpinBox: public QObject
{
    Q_OBJECT

public:
    testQuantitySpinBox()
    {
        GuiTest::ensureGuiApplication();
        qsb = std::make_unique<Gui::QuantitySpinBox>();
    }

private Q_SLOTS:

    void init()
    {}

    void cleanup()
    {
        // some tests switch the unit schema, the next one has to start from the default again
        Base::UnitsApi::setSchema(Base::UnitsApi::getDefSchemaNum());
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
