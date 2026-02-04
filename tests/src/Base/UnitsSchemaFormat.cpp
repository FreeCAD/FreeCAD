#include <gtest/gtest.h>

#include <Base/Quantity.h>
#include <Base/UnitsSchema.h>

#include <unicode/locid.h>
#include <unicode/utypes.h>

#include <string>

namespace
{
class ScopedIcuLocale
{
public:
    explicit ScopedIcuLocale(const char* name)
        : previous {icu::Locale::getDefault()}
    {
        UErrorCode status = U_ZERO_ERROR;
        icu::Locale::setDefault(icu::Locale(name), status);
    }

    ~ScopedIcuLocale()
    {
        UErrorCode status = U_ZERO_ERROR;
        icu::Locale::setDefault(previous, status);
    }

    ScopedIcuLocale(const ScopedIcuLocale&) = delete;
    ScopedIcuLocale(ScopedIcuLocale&&) = delete;
    ScopedIcuLocale& operator=(const ScopedIcuLocale&) = delete;
    ScopedIcuLocale& operator=(ScopedIcuLocale&&) = delete;

private:
    icu::Locale previous;
};

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
}  // namespace

TEST(UnitsSchemaFormatTest, fixed_uses_precision_and_trailing_zeroes)
{
    ScopedIcuLocale locale("en_US");

    Base::QuantityFormat fmt(Base::QuantityFormat::Fixed, 2);
    fmt.option = Base::QuantityFormat::None;

    EXPECT_EQ(translateValue(123.0, fmt), "123.00");
}

TEST(UnitsSchemaFormatTest, scientific_uses_exponent_and_precision)
{
    ScopedIcuLocale locale("en_US");

    Base::QuantityFormat fmt(Base::QuantityFormat::Scientific, 2);
    fmt.option = Base::QuantityFormat::None;

    const std::string s = translateValue(123.0, fmt);

    const auto ePos = s.find_first_of("eE");
    ASSERT_NE(ePos, std::string::npos);

    const auto dotPos = s.find('.');
    ASSERT_NE(dotPos, std::string::npos);
    ASSERT_LT(dotPos, ePos);
    EXPECT_EQ(ePos - dotPos - 1, 2U);
}

TEST(UnitsSchemaFormatTest, omit_group_separator_option_removes_grouping)
{
    ScopedIcuLocale locale("en_US");

    Base::QuantityFormat fmtGroup(Base::QuantityFormat::Default, 0);
    fmtGroup.option = Base::QuantityFormat::None;

    Base::QuantityFormat fmtNoGroup(Base::QuantityFormat::Default, 0);
    fmtNoGroup.option = Base::QuantityFormat::OmitGroupSeparator;

    const std::string withGrouping = translateValue(12345.0, fmtGroup);
    const std::string noGrouping = translateValue(12345.0, fmtNoGroup);

    EXPECT_NE(withGrouping.find(','), std::string::npos);
    EXPECT_EQ(noGrouping.find(','), std::string::npos);
}
