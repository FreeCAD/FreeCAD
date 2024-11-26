// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <memory>

#include <Mod/Spreadsheet/App/Sheet.h>
#include <Mod/Spreadsheet/App/PropertySheet.h>

class PropertySheetTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
    void SetUp() override
    {
        _sheet = std::make_unique<Spreadsheet::Sheet>();
        _propertySheet = std::make_unique<Spreadsheet::PropertySheet>(_sheet.get());
    }
    void TearDown() override
    {
        _sheet.reset();
        _propertySheet.reset();
    }

    /// Get a non-owning pointer to the internal PropertySheet for this test
    Spreadsheet::PropertySheet* propertySheet()
    {
        return _propertySheet.get();
    }

private:
    std::unique_ptr<Spreadsheet::Sheet> _sheet;
    std::unique_ptr<Spreadsheet::PropertySheet> _propertySheet;
};

TEST_F(PropertySheetTest, isValidCellAddressName)  // NOLINT
{
    // Test some things
}
