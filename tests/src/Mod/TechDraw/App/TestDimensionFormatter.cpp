// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <memory>

#include <QLocale>
#include <QString>

#include <App/Application.h>
#include <Mod/TechDraw/App/DimensionFormatter.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <src/App/InitApplication.h>

namespace
{

class TestDrawViewDimension: public TechDraw::DrawViewDimension
{
public:
    double getDimValue() override
    {
        return 20.2;
    }
};

class TestDimensionFormatter: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (App::Application::GetARGC() == 0) {
            tests::initApplication();
        }
    }

    void SetUp() override
    {
        dimension = std::make_unique<TestDrawViewDimension>();
        dimension->Type.setValue("Distance");
        dimension->EqualTolerance.setValue(false);
        dimension->ShowUnits.setValue(false);
        dimension->FormatSpecOverTolerance.setValue("%+.2f");
        dimension->FormatSpecUnderTolerance.setValue("%+.2f");
    }

    std::unique_ptr<TestDrawViewDimension> dimension;
};

TEST_F(TestDimensionFormatter, TolerancesUseDimensionDisplayUnit)
{
    dimension->OverTolerance.setValue(0.05);
    dimension->UnderTolerance.setValue(-0.55);

    const auto tolerances = dimension->getFormattedToleranceValues(
        TechDraw::DimensionFormatter::Format::FORMATTED);

    bool parsed = false;
    const double underTolerance =
        QLocale().toDouble(QString::fromStdString(tolerances.first), &parsed);
    ASSERT_TRUE(parsed);
    EXPECT_NEAR(underTolerance, -0.55, 1e-6);

    const double overTolerance =
        QLocale().toDouble(QString::fromStdString(tolerances.second), &parsed);
    ASSERT_TRUE(parsed);
    EXPECT_NEAR(overTolerance, 0.05, 1e-6);

    dimension->ShowUnits.setValue(true);
    const auto dimensionUnit = dimension->formatValue(
        dimension->getDimValue(),
        QStringLiteral("%.2f"),
        TechDraw::DimensionFormatter::Format::UNIT,
        true);
    const auto toleranceUnit = dimension->formatValue(
        dimension->OverTolerance.getValue(),
        QStringLiteral("%+.2f"),
        TechDraw::DimensionFormatter::Format::UNIT,
        false);
    EXPECT_EQ(toleranceUnit, dimensionUnit);
}

}  // namespace
