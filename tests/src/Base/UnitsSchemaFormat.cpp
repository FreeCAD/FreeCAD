#include <gtest/gtest.h>

#include <Base/Quantity.h>
#include <Base/UnitsSchema.h>

#include <src/LocaleTestHelpers.h>

#include <string>

namespace
{
Base::UnitsSchema makeNoTranslationSchema()
{
    Base::UnitsSchemaSpec spec {};
    spec.num = 0;
    spec.name = "TestSchema";
    spec.basicLengthUnitStr = "mm";
    spec.description = "Test schema";
    spec.isDefault = true;
    return Base::UnitsSchema(std::move(spec));
}

std::string translateValue(const double value, const Base::QuantityFormat& fmt)
{
    Base::Quantity q(value);
    q.setFormat(fmt);

    auto schema = makeNoTranslationSchema();
    double factor = 0.0;
    std::string unitString;
    return schema.translate(q, factor, unitString);
}

std::string translateValue(const double value, const Base::QuantityFormat& fmt, const char* localeId)
{
    Base::Quantity q(value);
    q.setFormat(fmt);

    auto schema = makeNoTranslationSchema();
    double factor = 0.0;
    std::string unitString;
    return schema.translate(q, localeId, factor, unitString);
}
}  // namespace

TEST(UnitsSchemaFormatTest, fixed_uses_precision_and_trailing_zeroes)
{
    Base::QuantityFormat fmt(Base::QuantityFormat::Fixed, 2);
    fmt.option = Base::QuantityFormat::None;

    EXPECT_EQ(translateValue(123.0, fmt, "en_US"), "123.00");
}

TEST(UnitsSchemaFormatTest, scientific_uses_exponent_and_precision)
{
    Base::QuantityFormat fmt(Base::QuantityFormat::Scientific, 2);
    fmt.option = Base::QuantityFormat::None;

    const std::string s = translateValue(123.0, fmt, "en_US");

    const auto ePos = s.find_first_of("eE");
    ASSERT_NE(ePos, std::string::npos);

    const auto dotPos = s.find('.');
    ASSERT_NE(dotPos, std::string::npos);
    ASSERT_LT(dotPos, ePos);
    EXPECT_EQ(ePos - dotPos - 1, 2U);
}

TEST(UnitsSchemaFormatTest, default_matches_qt_general_scientific_thresholds)
{
    Base::QuantityFormat fmt(Base::QuantityFormat::Default, 6);
    fmt.option = Base::QuantityFormat::None;

    EXPECT_EQ(translateValue(1e9, fmt, "en_US"), "1e+09");
    EXPECT_EQ(translateValue(1e6, fmt, "en_US"), "1e+06");
    EXPECT_EQ(translateValue(1e-6, fmt, "en_US"), "1e-06");
}

TEST(UnitsSchemaFormatTest, omit_group_separator_option_removes_grouping)
{
    Base::QuantityFormat fmtGroup(Base::QuantityFormat::Default, 0);
    fmtGroup.option = Base::QuantityFormat::None;

    Base::QuantityFormat fmtNoGroup(Base::QuantityFormat::Default, 0);
    fmtNoGroup.option = Base::QuantityFormat::OmitGroupSeparator;

    const std::string withGrouping = translateValue(12345.0, fmtGroup, "en_US");
    const std::string noGrouping = translateValue(12345.0, fmtNoGroup, "en_US");

    EXPECT_NE(withGrouping.find(','), std::string::npos);
    EXPECT_EQ(noGrouping.find(','), std::string::npos);
}

TEST(UnitsSchemaFormatTest, explicit_locale_overrides_current_and_icu_defaults)
{
    tests::ScopedFormattingLocaleState localeState {"de_DE", "fr_FR"};

    Base::QuantityFormat fmt(Base::QuantityFormat::Fixed, 2);
    fmt.option = Base::QuantityFormat::None;

    EXPECT_EQ(translateValue(1.5, fmt, "en_US"), "1.50");
}

TEST(UnitsSchemaFormatTest, default_translation_uses_current_numeric_formatting_locale)
{
    tests::ScopedFormattingLocaleState localeState {"de_DE", "en_US"};

    Base::QuantityFormat fmt(Base::QuantityFormat::Fixed, 2);
    fmt.option = Base::QuantityFormat::None;

    EXPECT_EQ(translateValue(1.5, fmt), "1,50");
}

TEST(UnitsSchemaFormatTest, effective_numeric_separators_override_locale_symbols)
{
    tests::ScopedCurrentNumericFormattingSeparators separators {",", "\xC2\xA0"};

    Base::QuantityFormat fmtFixed(Base::QuantityFormat::Fixed, 2);
    fmtFixed.option = Base::QuantityFormat::None;
    EXPECT_EQ(translateValue(1.5, fmtFixed, "en_US"), "1,50");

    Base::QuantityFormat fmtGrouped(Base::QuantityFormat::Default, 0);
    fmtGrouped.option = Base::QuantityFormat::None;
    EXPECT_EQ(
        translateValue(12345.0, fmtGrouped, "en_US"),
        std::string {"12\xC2\xA0"
                     "345"}
    );
}
