// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <src/App/InitApplication.h>

#include "App/Application.h"

class ApplicationTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {}

    void TearDown() override
    {}

    /// A unique integer value, incremented every time this method is called, throughout the entire
    /// test suite. Guaranteed to never give the same value twice as long as the number of calls
    /// never overflows. Used to ensure unique names when testing, even if this part of the test
    /// suite is run many times in a single application instantiation.
    int counter() const
    {
        ++_counter;
        return _counter;
    }

private:
    static int _counter;
};

int ApplicationTest::_counter = 0;

TEST_F(ApplicationTest, testAddTranslatableExportTypeWithCorrectExtension)
{
    auto moduleName = fmt::format("SomeModule{}", counter());
    const char* extension = "notARealType";
    const char* typeString = "Some type (*.notARealType)";
    App::GetApplication().addTranslatableExportType(typeString, {extension}, moduleName.c_str());
    auto types = App::GetApplication().getExportTypes(moduleName.c_str());
    ASSERT_EQ(1, types.size());
    EXPECT_EQ(types[0], extension);
}

TEST_F(ApplicationTest, testAddTranslatableExportTypeWithNonMatchingExtension)
{
    auto moduleName = fmt::format("SomeModule{}", counter());
    const char* extension = "notARealType";
    const char* typeString = "Some type (*.alsoNotARealType)";
    App::GetApplication().addTranslatableExportType(typeString, {extension}, moduleName.c_str());
    auto types = App::GetApplication().getExportTypes(moduleName.c_str());
    ASSERT_EQ(1, types.size());
    EXPECT_EQ(types[0], extension);
}

TEST_F(ApplicationTest, testAddTranslatableExportTypeNoParens)
{
    auto moduleName = fmt::format("SomeModule{}", counter());
    const char* extension = "notARealType";
    const char* typeString = "Some type description without any ending parentheses";
    App::GetApplication().addTranslatableExportType(typeString, {extension}, moduleName.c_str());
    auto types = App::GetApplication().getExportTypes(moduleName.c_str());
    ASSERT_EQ(1, types.size());
    EXPECT_EQ(types[0], extension);
}

TEST_F(ApplicationTest, testAddTranslatableExportTypeMultipleExtensions)
{
    auto moduleName = fmt::format("SomeModule{}", counter());
    std::vector<std::string> extensions = {"notARealType", "alsoNotARealType", "stillNotARealType"};
    const char* typeString = "Some type description without any ending parentheses";
    App::GetApplication().addTranslatableExportType(typeString, extensions, moduleName.c_str());
    auto types = App::GetApplication().getExportTypes(moduleName.c_str());
    ASSERT_EQ(extensions.size(), types.size());
    for (const auto& expectedExtension : extensions) {
        EXPECT_TRUE(std::find(types.begin(), types.end(), expectedExtension) != types.end());
    }
}
