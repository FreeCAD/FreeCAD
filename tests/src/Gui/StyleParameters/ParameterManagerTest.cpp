// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <gtest/gtest.h>

#include <Gui/Application.h>
#include <Gui/StyleParameters/ParameterManager.h>

using namespace Gui::StyleParameters;

class ParameterManagerTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create test sources
        auto source1 = std::make_unique<InMemoryParameterSource>(
            std::list<Parameter> {
                {"BaseSize", "8px"},
                {"PrimaryColor", "#ff0000"},
                {"SecondaryColor", "#00ff00"},
            },
            ParameterSource::Metadata {"Source 1"});

        auto source2 = std::make_unique<InMemoryParameterSource>(
            std::list<Parameter> {
                {"BaseSize", "16px"},  // Override from source1
                {"Margin", "@BaseSize * 2"},
                {"Padding", "@BaseSize / 2"},
            },
            ParameterSource::Metadata {"Source 2"});

        manager.addSource(source1.get());
        manager.addSource(source2.get());
        sources.push_back(std::move(source1));
        sources.push_back(std::move(source2));
    }

    Gui::StyleParameters::ParameterManager manager;
    std::vector<std::unique_ptr<ParameterSource>> sources;
};

// Test basic parameter resolution
TEST_F(ParameterManagerTest, BasicParameterResolution)
{
    {
        auto result = manager.resolve("BaseSize");
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 16.0);  // Should get value from source2 (later source)
        EXPECT_EQ(length.unit, "px");
    }

    {
        auto result = manager.resolve("PrimaryColor");
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        EXPECT_EQ(color.red(), 255);
        EXPECT_EQ(color.green(), 0);
        EXPECT_EQ(color.blue(), 0);
    }

    {
        auto result = manager.resolve("SecondaryColor");
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        EXPECT_EQ(color.red(), 0);
        EXPECT_EQ(color.green(), 255);
        EXPECT_EQ(color.blue(), 0);
    }
}

// Test parameter references
TEST_F(ParameterManagerTest, ParameterReferences)
{
    {
        auto result = manager.resolve("Margin");
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 32.0);  // @BaseSize * 2 = 16 * 2 = 32
        EXPECT_EQ(length.unit, "px");
    }

    {
        auto result = manager.resolve("Padding");
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 8.0);  // @BaseSize / 2 = 16 / 2 = 8
        EXPECT_EQ(length.unit, "px");
    }
}

// Test caching
TEST_F(ParameterManagerTest, Caching)
{
    // First resolution should cache the result
    auto result1 = manager.resolve("BaseSize");
    EXPECT_TRUE(std::holds_alternative<Length>(result1));

    // Second resolution should use cached value
    auto result2 = manager.resolve("BaseSize");
    EXPECT_TRUE(std::holds_alternative<Length>(result2));

    // Results should be identical
    auto length1 = std::get<Length>(result1);
    auto length2 = std::get<Length>(result2);
    EXPECT_DOUBLE_EQ(length1.value, length2.value);
    EXPECT_EQ(length1.unit, length2.unit);
}

// Test cache invalidation
TEST_F(ParameterManagerTest, CacheInvalidation)
{
    // Initial resolution
    auto result1 = manager.resolve("BaseSize");
    EXPECT_TRUE(std::holds_alternative<Length>(result1));
    auto length1 = std::get<Length>(result1);
    EXPECT_DOUBLE_EQ(length1.value, 16.0);

    // Reload should clear cache
    manager.reload();

    // Resolution after reload should work the same
    auto result2 = manager.resolve("BaseSize");
    EXPECT_TRUE(std::holds_alternative<Length>(result2));
    auto length2 = std::get<Length>(result2);
    EXPECT_DOUBLE_EQ(length2.value, 16.0);
    EXPECT_EQ(length1.unit, length2.unit);
}

// Test source priority
TEST_F(ParameterManagerTest, SourcePriority)
{
    // Create a third source with higher priority
    auto source3 = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {
            {"BaseSize", "24px"},  // Should override both previous sources
        },
        ParameterSource::Metadata {"Source 3"});

    manager.addSource(source3.get());
    sources.push_back(std::move(source3));

    // Should get value from the latest source (highest priority)
    auto result = manager.resolve("BaseSize");
    EXPECT_TRUE(std::holds_alternative<Length>(result));
    auto length = std::get<Length>(result);
    EXPECT_DOUBLE_EQ(length.value, 24.0);
    EXPECT_EQ(length.unit, "px");
}

// Test parameter listing
TEST_F(ParameterManagerTest, ParameterListing)
{
    auto params = manager.parameters();

    // Should contain all parameters from all sources
    std::set<std::string> paramNames;
    for (const auto& param : params) {
        paramNames.insert(param.name);
    }

    EXPECT_TRUE(paramNames.contains("BaseSize"));
    EXPECT_TRUE(paramNames.contains("PrimaryColor"));
    EXPECT_TRUE(paramNames.contains("SecondaryColor"));
    EXPECT_TRUE(paramNames.contains("Margin"));
    EXPECT_TRUE(paramNames.contains("Padding"));

    // Should not contain duplicates (BaseSize should appear only once)
    EXPECT_EQ(paramNames.count("BaseSize"), 1);
}

// Test expression retrieval
TEST_F(ParameterManagerTest, ExpressionRetrieval)
{
    {
        auto expr = manager.expression("BaseSize");
        EXPECT_TRUE(expr.has_value());
        EXPECT_EQ(*expr, "16px");
    }

    {
        auto expr = manager.expression("Margin");
        EXPECT_TRUE(expr.has_value());
        EXPECT_EQ(*expr, "@BaseSize * 2");
    }

    {
        auto expr = manager.expression("NonExistent");
        EXPECT_FALSE(expr.has_value());
    }
}

// Test parameter retrieval
TEST_F(ParameterManagerTest, ParameterRetrieval)
{
    {
        auto param = manager.parameter("BaseSize");
        EXPECT_TRUE(param.has_value());
        EXPECT_EQ(param->name, "BaseSize");
        EXPECT_EQ(param->value, "16px");
    }

    {
        auto param = manager.parameter("NonExistent");
        EXPECT_FALSE(param.has_value());
    }
}

// Test source management
TEST_F(ParameterManagerTest, SourceManagement)
{
    auto sources = manager.sources();
    EXPECT_EQ(sources.size(), 2);  // We added 2 sources in SetUp

    // Test that we can access the sources
    for (auto source : sources) {
        EXPECT_NE(source, nullptr);
        auto params = source->all();
        EXPECT_FALSE(params.empty());
    }
}

// Test circular reference detection
TEST_F(ParameterManagerTest, CircularReferenceDetection)
{
    // Create a source with circular reference
    auto circularSource = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {
            {"A", "@B"},
            {"B", "@A"},
        },
        ParameterSource::Metadata {"Circular Source"});

    manager.addSource(circularSource.get());
    sources.push_back(std::move(circularSource));

    // Should handle circular reference gracefully
    auto result = manager.resolve("A");
    // Should return the expression string as fallback
    EXPECT_TRUE(std::holds_alternative<std::string>(result));
}

// Test complex expressions
TEST_F(ParameterManagerTest, ComplexExpressions)
{
    // Create a source with complex expressions
    auto complexSource = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {
            {"ComplexMargin", "(@BaseSize + 4px) * 2"},
            {"ComplexPadding", "(@BaseSize - 2px) / 2"},
            {"ColorWithFunction", "lighten(@PrimaryColor, 20)"},
        },
        ParameterSource::Metadata {"Complex Source"});

    manager.addSource(complexSource.get());
    sources.push_back(std::move(complexSource));

    {
        auto result = manager.resolve("ComplexMargin");
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 40.0);  // (16 + 4) * 2 = 20 * 2 = 40
        EXPECT_EQ(length.unit, "px");
    }

    {
        auto result = manager.resolve("ComplexPadding");
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 7.0);  // (16 - 2) / 2 = 14 / 2 = 7
        EXPECT_EQ(length.unit, "px");
    }

    {
        auto result = manager.resolve("ColorWithFunction");
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        // Should be lighter than the original red
        EXPECT_GT(color.lightness(), QColor("#ff0000").lightness());
    }
}

// Test error handling
TEST_F(ParameterManagerTest, ErrorHandling)
{
    // Test non-existent parameter
    auto result = manager.resolve("NonExistent");
    EXPECT_TRUE(std::holds_alternative<std::string>(result));
    EXPECT_EQ(std::get<std::string>(result), "");

    // Test invalid expression
    auto invalidSource = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {
            {"Invalid", "invalid expression that will fail"},
        },
        ParameterSource::Metadata {"Invalid Source"});

    manager.addSource(invalidSource.get());
    sources.push_back(std::move(invalidSource));

    // Should handle invalid expression gracefully
    auto invalidResult = manager.resolve("Invalid");
    // Should return the expression string as fallback
    EXPECT_TRUE(std::holds_alternative<std::string>(invalidResult));
}
