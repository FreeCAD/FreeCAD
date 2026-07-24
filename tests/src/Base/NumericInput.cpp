// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <unicode/dcfmtsym.h>
#include <unicode/locid.h>

#include <Base/Exception.h>
#include <Base/NumericFormatting.h>
#include <Base/NumericInput.h>
#include <Base/Quantity.h>

namespace
{
Base::NumericFormattingState locale(
    const char* localeId,
    const char* decimalSeparator,
    const char* groupingSeparator
)
{
    return {localeId, decimalSeparator, groupingSeparator};
}

std::string localizedMinusSign(const char* localeId)
{
    UErrorCode status = U_ZERO_ERROR;
    const icu::DecimalFormatSymbols symbols(icu::Locale::createFromName(localeId), status);
    std::string result;
    if (U_SUCCESS(status)) {
        symbols.getSymbol(icu::DecimalFormatSymbols::kMinusSignSymbol).toUTF8String(result);
    }
    return result;
}
}  // namespace

TEST(NumericInputTest, canonicalInputIsPreserved)
{
    const auto formatting = locale("de_DE", ",", ".");

    EXPECT_EQ(
        Base::canonicalizeNumericInput("10.00 mm", formatting, Base::NumericInputMode::Quantity),
        "10.00 mm"
    );
}

TEST(NumericInputTest, localizedDecimalIsCanonicalized)
{
    const auto formatting = locale("de_DE", ",", ".");

    EXPECT_EQ(
        Base::canonicalizeNumericInput("1,25 mm", formatting, Base::NumericInputMode::Quantity),
        "1.25 mm"
    );
}

TEST(NumericInputTest, expressionModeKeepsCanonicalAndLocalizedDecimalsDistinctFromGrouping)
{
    const auto formatting = locale("de_DE", ",", ".");

    EXPECT_EQ(
        Base::canonicalizeNumericInput("1.234 mm", formatting, Base::NumericInputMode::Expression),
        "1.234 mm"
    );
    EXPECT_EQ(
        Base::canonicalizeNumericInput("1,234 mm", formatting, Base::NumericInputMode::Expression),
        "1.234 mm"
    );
}

TEST(NumericInputTest, completeLocalizedGroupingIsCanonicalized)
{
    const auto formatting = locale("en_US", ".", ",");

    EXPECT_EQ(
        Base::canonicalizeNumericInput("1,234.5 mm", formatting, Base::NumericInputMode::Quantity),
        "1234.5 mm"
    );
}

TEST(NumericInputTest, negativeGroupedNumberIsCanonicalized)
{
    const auto formatting = locale("en_US", ".", ",");

    EXPECT_EQ(
        Base::canonicalizeNumericInput("-1,234.5 mm", formatting, Base::NumericInputMode::Quantity),
        "-1234.5 mm"
    );
}

TEST(NumericInputTest, localizedNegativeSignIsCanonicalized)
{
    const auto formatting = Base::createNumericFormattingState("fa_IR");
    const auto negativeSign = localizedMinusSign("fa_IR");
    ASSERT_FALSE(negativeSign.empty());
    ASSERT_NE(negativeSign, "-");

    const std::string input = negativeSign + "1" + formatting.decimalSeparator + "25 mm";
    EXPECT_EQ(
        Base::canonicalizeNumericInput(input, formatting, Base::NumericInputMode::Quantity),
        "-1.25 mm"
    );
    EXPECT_EQ(Base::Quantity::parseUserInput(input, formatting), Base::Quantity(-1.25, "mm"));
}

TEST(NumericInputTest, indianGroupingIsCanonicalized)
{
    const auto formatting = locale("en_IN", ".", ",");

    EXPECT_EQ(
        Base::canonicalizeNumericInput("12,34,567 mm", formatting, Base::NumericInputMode::Quantity),
        "1234567 mm"
    );
}

TEST(NumericInputTest, malformedGroupingIsRejectedAsOneLiteral)
{
    const auto formatting = locale("en_US", ".", ",");

    EXPECT_THROW(
        Base::canonicalizeNumericInput("12,34,567 mm", formatting, Base::NumericInputMode::Quantity),
        Base::ParserError
    );
}

TEST(NumericInputTest, groupedScientificNotationIsCanonicalized)
{
    const auto formatting = locale("en_US", ".", ",");

    EXPECT_EQ(
        Base::canonicalizeNumericInput("1,234e5 mm", formatting, Base::NumericInputMode::Quantity),
        "123400000 mm"
    );
    EXPECT_EQ(
        Base::canonicalizeNumericInput("1,234E5 mm", formatting, Base::NumericInputMode::Quantity),
        "123400000 mm"
    );
}

TEST(NumericInputTest, groupingIsDisabledInsideFunctionArguments)
{
    const auto formatting = locale("en_US", ".", ",");

    EXPECT_EQ(
        Base::canonicalizeNumericInput("pow(1,234)", formatting, Base::NumericInputMode::Expression),
        "pow(1;234)"
    );
}

TEST(NumericInputTest, commaDecimalLocaleUsesSemicolonForArguments)
{
    const auto formatting = locale("de_DE", ",", ".");

    EXPECT_EQ(
        Base::canonicalizeNumericInput("pow(1, 234)", formatting, Base::NumericInputMode::Expression),
        "pow(1; 234)"
    );
    EXPECT_EQ(
        Base::canonicalizeNumericInput("pow(1,234)", formatting, Base::NumericInputMode::Expression),
        "pow(1.234)"
    );
}

TEST(NumericInputTest, expressionStringsAreNotCanonicalized)
{
    const auto formatting = locale("en_US", ".", ",");

    EXPECT_EQ(
        Base::canonicalizeNumericInput(
            "parsequant(<<1,234 mm>>)",
            formatting,
            Base::NumericInputMode::Expression
        ),
        "parsequant(<<1,234 mm>>)"
    );
}

TEST(NumericInputTest, unterminatedExpressionStringsArePreservedForParserDiagnostics)
{
    const auto formatting = locale("en_US", ".", ",");
    const std::string input = "parsequant(<<1,234 mm";

    EXPECT_EQ(
        Base::canonicalizeNumericInput(input, formatting, Base::NumericInputMode::Expression),
        input
    );
}

TEST(NumericInputTest, quantityCommentsArePreservedVerbatim)
{
    const auto formatting = locale("en_US", ".", ",");

    EXPECT_EQ(
        Base::canonicalizeNumericInput(
            "1,234 mm [display text: 1,234 and -5.0]",
            formatting,
            Base::NumericInputMode::Quantity
        ),
        "1234 mm [display text: 1,234 and -5.0]"
    );
}

TEST(NumericInputTest, quantityUserInputUsesTheSharedCanonicalizer)
{
    const auto formatting = locale("en_IN", ".", ",");

    EXPECT_EQ(Base::Quantity::parseUserInput("12,34,567 mm", formatting), Base::Quantity(1234567, "mm"));
}
