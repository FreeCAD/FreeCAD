// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <utility>

#include <Base/Exception.h>
#include <Base/NumericFormatting.h>
#include <Base/NumericInput.h>
#include <Base/Quantity.h>

TEST(NumericFormattingTest, createsLocaleSpecificSeparators)
{
    const auto enUs = Base::createNumericLocaleContext("en_US");
    const auto deDe = Base::createNumericLocaleContext("de_DE");
    const auto enIn = Base::createNumericLocaleContext("en_IN");

    EXPECT_EQ(enUs.localeId, "en_US");
    EXPECT_EQ(enUs.decimalSeparator, ".");
    EXPECT_EQ(enUs.groupingSeparator, ",");
    EXPECT_EQ(enUs.positiveSign, "+");
    EXPECT_EQ(enUs.negativeSign, "-");
    EXPECT_EQ(enUs.primaryGroupingSize, 3);
    EXPECT_EQ(enUs.secondaryGroupingSize, 3);
    EXPECT_EQ(enUs.zeroDigit, "0");

    EXPECT_EQ(deDe.localeId, "de_DE");
    EXPECT_EQ(deDe.decimalSeparator, ",");
    EXPECT_EQ(deDe.groupingSeparator, ".");

    EXPECT_EQ(enIn.localeId, "en_IN");
    EXPECT_EQ(enIn.decimalSeparator, ".");
    EXPECT_EQ(enIn.groupingSeparator, ",");
    EXPECT_EQ(enIn.primaryGroupingSize, 3);
    EXPECT_EQ(enIn.secondaryGroupingSize, 2);
}

TEST(NumericFormattingTest, formattedNativeDigitsRoundTripThroughScannerAndQuantity)
{
    Base::QuantityFormat fixed(Base::QuantityFormat::Fixed, 1);
    fixed.option = Base::QuantityFormat::None;
    Base::QuantityFormat scientific(Base::QuantityFormat::Scientific, 2);
    scientific.option = Base::QuantityFormat::None;

    for (const auto localeId : {"fa_IR", "ar_EG"}) {
        const auto locale = Base::createNumericLocaleContext(localeId);
        for (const auto [value, format] :
             {std::pair {1234.5, fixed}, std::pair {-1234.5, fixed}, std::pair {1.25e6, scientific}}) {
            const auto formatted = Base::formatNumericValue(value, format, locale);
            if (format.format == Base::QuantityFormat::Scientific) {
                EXPECT_NE(formatted.find('e'), std::string::npos) << localeId << ": " << formatted;
            }
            const auto scanned
                = Base::scanLocalizedNumber(formatted, locale, Base::NumericSyntaxContext::Standalone);
            ASSERT_EQ(scanned.status, Base::LocalizedNumberResult::Status::Complete)
                << localeId << ": " << formatted;
            EXPECT_DOUBLE_EQ(scanned.value, value);
            if (format.format == Base::QuantityFormat::Scientific) {
                EXPECT_EQ(scanned.canonicalText, "1.25e6");
            }

            const auto quantity = Base::Quantity::parseUserInput(formatted + " mm", locale);
            EXPECT_DOUBLE_EQ(quantity.getValue(), value);
            EXPECT_EQ(quantity.getUnit(), Base::Unit::Length);
        }
    }
}

TEST(NumericFormattingTest, formatsValuesWithEffectiveSeparators)
{
    const Base::NumericLocaleContext formatting {"en_US", ",", "\xC2\xA0", "+", "-", 3, 3};

    Base::QuantityFormat fixed(Base::QuantityFormat::Fixed, 2);
    fixed.option = Base::QuantityFormat::None;
    EXPECT_EQ(Base::formatNumericValue(1.5, fixed, formatting), "1,50");

    Base::QuantityFormat grouped(Base::QuantityFormat::Default, 0);
    grouped.option = Base::QuantityFormat::None;
    EXPECT_EQ(
        Base::formatNumericValue(12345.0, grouped, formatting),
        std::string {"12\xC2\xA0"
                     "345"}
    );
}

TEST(NumericFormattingTest, cLocaleUsesDeterministicFallback)
{
    const Base::NumericLocaleContext expected {"en_US_POSIX", ".", ",", "+", "-", 3, 3, "0"};
    EXPECT_EQ(Base::createNumericLocaleContext("C"), expected);
}

TEST(NumericFormattingTest, normalizesIcuLocaleIdentifiers)
{
    EXPECT_EQ(Base::normalizeIcuLocaleId(""), "en_US_POSIX");
    EXPECT_EQ(Base::normalizeIcuLocaleId("POSIX"), "en_US_POSIX");
    EXPECT_EQ(Base::normalizeIcuLocaleId("de_DE"), "de_DE");
}

TEST(NumericFormattingTest, rejectsBogusIcuLocaleIdentifiers)
{
    constexpr auto bogusLocale = "not a locale";

    EXPECT_THROW(Base::normalizeIcuLocaleId(bogusLocale), Base::ValueError);
    EXPECT_THROW(Base::createNumericLocaleContext(bogusLocale), Base::ValueError);
    EXPECT_THROW(Base::setIcuDefaultLocale(bogusLocale), Base::ValueError);
}

TEST(NumericFormattingTest, publishedStateIsACompleteSnapshot)
{
    const auto previous = Base::currentNumericLocaleContext();
    const Base::NumericLocaleContext expected {"en_US", ".", ",", "+", "-", 3, 3, "0"};

    Base::publishNumericLocaleContext(expected);

    EXPECT_EQ(Base::currentNumericLocaleContext(), expected);
    Base::publishNumericLocaleContext(previous);
}

TEST(NumericFormattingTest, failedLocaleResolutionDoesNotPublishPartialState)
{
    const auto previous = Base::currentNumericLocaleContext();
    const Base::NumericLocaleContext expected {"en_IN", ".", ",", "+", "-", 3, 2, "0"};
    Base::publishNumericLocaleContext(expected);

    EXPECT_THROW(Base::createNumericLocaleContext("not a locale"), Base::ValueError);
    EXPECT_EQ(Base::currentNumericLocaleContext(), expected);

    Base::publishNumericLocaleContext(previous);
}
