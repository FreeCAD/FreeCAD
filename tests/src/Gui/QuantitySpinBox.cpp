// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QDebug>
#include <QTest>

#include <App/Application.h>

#include "Gui/QuantitySpinBox.h"
#include <src/LocaleTestHelpers.h>
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

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

    void init()
    {}

    void cleanup()
    {}

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

    void test_MismatchedFormatterAndWidgetLocaleDoesNotMutateValue()  // NOLINT
    {
        tests::ScopedNumericLocaleState localeState {"da_DK", "en_US", "fr_FR"};

        Gui::QuantitySpinBox spinBox;
        spinBox.setValue(Base::Quantity(10, "mm"));

        QCOMPARE(spinBox.value(), Base::Quantity(10, "mm"));
        QCOMPARE(spinBox.rawValue(), 10.0);
    }

    void test_GroupedLocaleNumberIsNormalizedBeforeParse()  // NOLINT
    {
        tests::ScopedNumericLocaleState localeState {"da_DK", "en_US", "fr_FR"};

        Gui::QuantitySpinBox spinBox;
        const auto result = spinBox.valueFromText("1.000,00 mm");

        QCOMPARE(result, Base::Quantity(1000, "mm"));
    }

    void test_CanonicalDecimalPointRemainsAcceptedInCommaLocale()  // NOLINT
    {
        tests::ScopedNumericLocaleState localeState {"da_DK", "en_US", "fr_FR"};

        Gui::QuantitySpinBox spinBox;
        const auto result = spinBox.valueFromText("10.00 mm");

        QCOMPARE(result, Base::Quantity(10, "mm"));
    }

    void test_GroupedEditDoesNotCorruptRawValue()  // NOLINT
    {
        tests::ScopedNumericLocaleState localeState {"en_US", "en_US", "en_US"};

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

    void test_EffectiveSeparatorsOverrideFormattingLocaleId()  // NOLINT
    {
        tests::ScopedQtDefaultLocale qtLocale {"pt_PT"};
        tests::ScopedCurrentNumericFormattingLocale formattingLocale {"en_US"};
        tests::ScopedCurrentNumericFormattingSeparators separators {QLocale()};

        Gui::QuantitySpinBox spinBox;
        Base::Quantity quantity(1.5, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        spinBox.setValue(quantity);

        QCOMPARE(spinBox.text(), QStringLiteral("1,50 mm"));
        QCOMPARE(spinBox.text().at(1), QLocale().decimalPoint());
    }

private:
    std::unique_ptr<Gui::QuantitySpinBox> qsb;
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testQuantitySpinBox)

#include "QuantitySpinBox.moc"
