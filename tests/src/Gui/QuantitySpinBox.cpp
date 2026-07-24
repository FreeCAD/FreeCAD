// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QDebug>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>
#include <App/Property.h>

#include "Gui/QuantitySpinBox.h"
#include "Gui/PrefWidgets.h"
#include <src/LocaleTestHelpers.h>
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

namespace
{
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
            tests::ScopedNumericFormattingState staleFormatting {",", "."};

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

private:
    std::unique_ptr<Gui::QuantitySpinBox> qsb;
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testQuantitySpinBox)

#include "QuantitySpinBox.moc"
