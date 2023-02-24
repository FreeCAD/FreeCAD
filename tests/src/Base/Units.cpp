#include "gtest/gtest.h"
#include <QLocale>
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>
#include <Base/UnitsSchemaImperial1.h>

class Quantity : public ::testing::Test {
protected:
    void SetUp() override {
        QLocale loc(QLocale::C);
        QLocale::setDefault(loc);
    }
    void TearDown() override {
    }
};

TEST_F(Quantity, TestSchemeImperialTwo)
{
    Base::UnitsSchemaImperialDecimal scheme;
    Base::Quantity quantity{1.0, Base::Unit::Length};

    double factor;
    QString unitString;
    QString result = scheme.schemaTranslate(quantity, factor, unitString);
    EXPECT_EQ(result.toStdString(), "0.04 in");
}

TEST_F(Quantity, TestSchemeImperialOne)
{
    Base::UnitsSchemaImperialDecimal scheme;
    Base::Quantity quantity{1.0, Base::Unit::Length};

    Base::QuantityFormat format = quantity.getFormat();
    format.precision = 1;
    quantity.setFormat(format);

    double factor;
    QString unitString;
    QString result = scheme.schemaTranslate(quantity, factor, unitString);

    EXPECT_EQ(result.toStdString(), "0.0 in");
}

TEST_F(Quantity, TestSafeUserString)
{
    Base::UnitsApi::setSchema(Base::UnitSystem::ImperialDecimal);

    Base::Quantity quantity{1.0, Base::Unit::Length};
    Base::QuantityFormat format = quantity.getFormat();
    format.precision = 1;
    quantity.setFormat(format);

    QString result = quantity.getSafeUserString();

    EXPECT_EQ(result.toStdString(), "1 mm");
}
