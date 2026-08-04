// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Base/NumericFormatting.h>
#include <Base/NumericInput.h>
#include <Base/Quantity.h>

namespace
{
Base::NumericLocaleContext locale(
    const char* localeId,
    const char* decimal,
    const char* grouping,
    int primary = 3,
    int secondary = 3
)
{
    return {localeId, decimal, grouping, "+", "-", primary, secondary};
}

void expectComplete(
    const Base::LocalizedNumberResult& result,
    double value,
    std::string_view canonical,
    std::size_t consumed
)
{
    EXPECT_EQ(result.status, Base::LocalizedNumberResult::Status::Complete);
    EXPECT_DOUBLE_EQ(result.value, value);
    EXPECT_EQ(result.canonicalText, canonical);
    EXPECT_EQ(result.consumedBytes, consumed);
    EXPECT_FALSE(result.diagnostic.has_value());
}
}  // namespace

TEST(NumericInputTest, canonicalAndLocalizedDecimals)
{
    const auto de = locale("de_DE", ",", ".");
    expectComplete(
        Base::scanLocalizedNumber("1,25 mm", de, Base::NumericSyntaxContext::Standalone),
        1.25,
        "1.25",
        4
    );
    expectComplete(
        Base::scanLocalizedNumber("12.345,67 mm", de, Base::NumericSyntaxContext::Standalone),
        12345.67,
        "12345.67",
        9
    );
    expectComplete(
        Base::scanLocalizedNumber("1.25 mm", de, Base::NumericSyntaxContext::Standalone),
        1.25,
        "1.25",
        4
    );
}

TEST(NumericInputTest, westernGroupingAndScientificNotation)
{
    const auto en = locale("en_US", ".", ",");
    expectComplete(
        Base::scanLocalizedNumber("1,234.5 mm", en, Base::NumericSyntaxContext::Standalone),
        1234.5,
        "1234.5",
        7
    );
    expectComplete(
        Base::scanLocalizedNumber("1,234e5 mm", en, Base::NumericSyntaxContext::Standalone),
        123400000.0,
        "1234e5",
        7
    );
}

TEST(NumericInputTest, indianGroupingUsesSecondarySize)
{
    const auto enIn = locale("en_IN", ".", ",", 3, 2);
    expectComplete(
        Base::scanLocalizedNumber("12,34,567 mm", enIn, Base::NumericSyntaxContext::Standalone),
        1234567.0,
        "1234567",
        9
    );
}

TEST(NumericInputTest, malformedGroupingIsInvalid)
{
    const auto en = locale("en_US", ".", ",");
    const auto result
        = Base::scanLocalizedNumber("12,34,567", en, Base::NumericSyntaxContext::Standalone);

    EXPECT_EQ(result.status, Base::LocalizedNumberResult::Status::Invalid);
    ASSERT_TRUE(result.diagnostic.has_value());
    EXPECT_EQ(result.diagnostic->kind, Base::NumericDiagnosticKind::InvalidGrouping);
}

TEST(NumericInputTest, multipleDecimalsAndIncompleteExponentsAreDiagnosed)
{
    const auto en = locale("en_US", ".", ",");
    const auto multipleDecimals
        = Base::scanLocalizedNumber("1.2.3", en, Base::NumericSyntaxContext::Standalone);
    EXPECT_EQ(multipleDecimals.status, Base::LocalizedNumberResult::Status::Invalid);

    const auto incompleteExponent
        = Base::scanLocalizedNumber("1e", en, Base::NumericSyntaxContext::Standalone);
    EXPECT_EQ(incompleteExponent.status, Base::LocalizedNumberResult::Status::Incomplete);
    ASSERT_TRUE(incompleteExponent.diagnostic.has_value());
    EXPECT_EQ(incompleteExponent.diagnostic->kind, Base::NumericDiagnosticKind::IncompleteExponent);

    for (const auto input : {"1.23,4", "1,234.5,6", "1e1,000", "1,234e1,000"}) {
        EXPECT_EQ(
            Base::scanLocalizedNumber(input, en, Base::NumericSyntaxContext::Standalone).status,
            Base::LocalizedNumberResult::Status::Invalid
        ) << input;
    }
}

TEST(NumericInputTest, DotGroupingAndCanonicalDecimalHaveAnExplicitPolicy)
{
    const auto de = locale("de_DE", ",", ".");
    expectComplete(
        Base::scanLocalizedNumber("1.234", de, Base::NumericSyntaxContext::Standalone),
        1.234,
        "1.234",
        5
    );
    expectComplete(
        Base::scanLocalizedNumber("1.234,5", de, Base::NumericSyntaxContext::Standalone),
        1234.5,
        "1234.5",
        7
    );
    expectComplete(
        Base::scanLocalizedNumber("1.234.567", de, Base::NumericSyntaxContext::Standalone),
        1234567.0,
        "1234567",
        9
    );
    expectComplete(
        Base::scanLocalizedNumber("12.345 mm", de, Base::NumericSyntaxContext::Standalone),
        12.345,
        "12.345",
        6
    );
}

TEST(NumericInputTest, negativeAndLocalizedSigns)
{
    const auto en = locale("en_US", ".", ",");
    expectComplete(
        Base::scanLocalizedNumber("-1,234.5 mm", en, Base::NumericSyntaxContext::Standalone),
        -1234.5,
        "-1234.5",
        8
    );

    auto fa = Base::createNumericLocaleContext("fa_IR");
    const std::string input = fa.negativeSign + "1" + fa.decimalSeparator + "25 mm";
    const auto localized = Base::scanLocalizedNumber(input, fa, Base::NumericSyntaxContext::Standalone);
    EXPECT_EQ(localized.status, Base::LocalizedNumberResult::Status::Complete);
    EXPECT_DOUBLE_EQ(localized.value, -1.25);
    EXPECT_EQ(localized.canonicalText, "-1.25");
}

TEST(NumericInputTest, formattedNativeDigitsAreAccepted)
{
    for (const auto localeId : {"fa_IR", "ar_EG"}) {
        const auto localized = Base::createNumericLocaleContext(localeId);
        const auto formatted = Base::formatNumericValue(
            1234.5,
            Base::QuantityFormat(Base::QuantityFormat::Fixed, 1),
            localized
        );
        const auto result
            = Base::scanLocalizedNumber(formatted, localized, Base::NumericSyntaxContext::Standalone);
        expectComplete(result, 1234.5, "1234.5", formatted.size());
    }
}

TEST(NumericInputTest, InvalidUtf8IsNotALocalizedDigit)
{
    const auto locale = Base::createNumericLocaleContext("fa_IR");
    for (const auto input : {"\xC0\x80", "\xED\xA0\x80", "\xF4\x90\x80\x80"}) {
        int digit = 0;
        std::size_t consumed = 0;
        EXPECT_FALSE(Base::localizedDigitAt(input, 0, locale, digit, consumed)) << input;
        EXPECT_EQ(consumed, 0U);
    }
}

TEST(NumericInputTest, positiveSignsAreAcceptedByCanonicalConversion)
{
    const Base::NumericLocaleContext en = locale("en_US", ".", ",");
    expectComplete(
        Base::scanLocalizedNumber("+1.25 mm", en, Base::NumericSyntaxContext::Standalone),
        1.25,
        "1.25",
        5
    );

    const Base::NumericLocaleContext custom {"custom", ".", ",", "plus", "minus", 3, 3};
    expectComplete(
        Base::scanLocalizedNumber("plus1.25 mm", custom, Base::NumericSyntaxContext::Standalone),
        1.25,
        "1.25",
        8
    );
}

TEST(NumericInputTest, OutOfRangeScientificNotationIsDiagnosed)
{
    const Base::NumericLocaleContext en = locale("en_US", ".", ",");
    const auto result = Base::scanLocalizedNumber("1e999", en, Base::NumericSyntaxContext::Standalone);

    EXPECT_EQ(result.status, Base::LocalizedNumberResult::Status::Invalid);
    ASSERT_TRUE(result.diagnostic.has_value());
    EXPECT_EQ(result.diagnostic->kind, Base::NumericDiagnosticKind::OutOfRange);
}

TEST(NumericInputTest, exactGroupingSymbolsAreRequired)
{
    const auto en = locale("en_US", ".", ",");
    const auto plainSpace
        = Base::scanLocalizedNumber("12 345", en, Base::NumericSyntaxContext::Standalone);
    EXPECT_EQ(plainSpace.status, Base::LocalizedNumberResult::Status::Invalid);
    ASSERT_TRUE(plainSpace.diagnostic.has_value());
    EXPECT_EQ(plainSpace.diagnostic->kind, Base::NumericDiagnosticKind::UnexpectedSeparator);

    const auto narrowSpace = locale("fr_FR", ",", "\xE2\x80\xAF");
    expectComplete(
        Base::scanLocalizedNumber(
            "12\xE2\x80\xAF"
            "345",
            narrowSpace,
            Base::NumericSyntaxContext::Standalone
        ),
        12345.0,
        "12345",
        8
    );

    for (const auto separator : {"\xC2\xA0", " ", "\xE2\x80\x89", "\t", "\n"}) {
        const auto result = Base::scanLocalizedNumber(
            std::string {"12"} + separator + "345",
            narrowSpace,
            Base::NumericSyntaxContext::Standalone
        );
        EXPECT_EQ(result.status, Base::LocalizedNumberResult::Status::Invalid) << separator;
    }

    auto noBreakSpace = narrowSpace;
    noBreakSpace.groupingSeparator = "\xC2\xA0";
    expectComplete(
        Base::scanLocalizedNumber(
            "12\xC2\xA0"
            "345",
            noBreakSpace,
            Base::NumericSyntaxContext::Standalone
        ),
        12345.0,
        "12345",
        7
    );
}

TEST(NumericInputTest, FunctionArgumentsTakePrecedenceOverCommaGrouping)
{
    const auto en = locale("en_US", ".", ",");
    const auto firstArgument
        = Base::scanLocalizedNumber("1,234", en, Base::NumericSyntaxContext::FunctionArgument);
    expectComplete(firstArgument, 1.0, "1", 1);

    const auto groupedArgument = locale("space", ".", " ");
    expectComplete(
        Base::scanLocalizedNumber("1 234; 2", groupedArgument, Base::NumericSyntaxContext::FunctionArgument),
        1234.0,
        "1234",
        5
    );

    const auto de = locale("de_DE", ",", ".");
    expectComplete(
        Base::scanLocalizedNumber("1,234; 2", de, Base::NumericSyntaxContext::FunctionArgument),
        1.234,
        "1.234",
        5
    );
}

TEST(NumericInputTest, GroupingPlacementMutationsFollowLocalePattern)
{
    const auto en = locale("en_US", ".", ",");
    for (const auto input : {"1,234", "12,345", "123,456"}) {
        EXPECT_EQ(
            Base::scanLocalizedNumber(input, en, Base::NumericSyntaxContext::Standalone).status,
            Base::LocalizedNumberResult::Status::Complete
        );
    }
    for (const auto input : {"1234,567", "1,23,456", "1,,234", ",123", "1,234,"}) {
        const auto result
            = Base::scanLocalizedNumber(input, en, Base::NumericSyntaxContext::Standalone);
        EXPECT_NE(result.status, Base::LocalizedNumberResult::Status::Complete) << input;
    }

    const auto enIn = locale("en_IN", ".", ",", 3, 2);
    for (const auto input : {"1,23,456", "12,34,567", "1,23,45,678"}) {
        EXPECT_EQ(
            Base::scanLocalizedNumber(input, enIn, Base::NumericSyntaxContext::Standalone).status,
            Base::LocalizedNumberResult::Status::Complete
        );
    }
    for (const auto input : {"123,45,678", "1,234,567", "1,2,345"}) {
        EXPECT_EQ(
            Base::scanLocalizedNumber(input, enIn, Base::NumericSyntaxContext::Standalone).status,
            Base::LocalizedNumberResult::Status::Invalid
        );
    }
}
