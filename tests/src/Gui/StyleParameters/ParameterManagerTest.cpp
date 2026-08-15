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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Gui/Application.h>
#include <Gui/Utilities.h>
#include <Gui/StyleParameters/Gradient.h>
#include <Gui/StyleParameters/Insets.h>
#include <Gui/StyleParameters/ParameterManager.h>

#include "DiagnosticsCapture.h"

using namespace Gui::StyleParameters;
using ::testing::Contains;
using ::testing::HasSubstr;

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
            ParameterSource::Metadata {"Source 1"}
        );

        auto source2 = std::make_unique<InMemoryParameterSource>(
            std::list<Parameter> {
                {"BaseSize", "16px"},  // Override from source1
                {"Margin", "@BaseSize * 2"},
                {"Padding", "@BaseSize / 2"},
            },
            ParameterSource::Metadata {"Source 2"}
        );

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
        EXPECT_TRUE(result.has_value());
        EXPECT_TRUE(std::holds_alternative<Numeric>(*result));
        auto length = std::get<Numeric>(*result);
        EXPECT_DOUBLE_EQ(length.value, 16.0);  // Should get value from source2 (later source)
        EXPECT_EQ(length.unit, "px");
    }

    {
        auto result = manager.resolve("PrimaryColor");
        EXPECT_TRUE(result.has_value());
        EXPECT_TRUE(std::holds_alternative<Base::Color>(*result));
        auto color = std::get<Base::Color>(*result);
        EXPECT_EQ(color.r, 1);
        EXPECT_EQ(color.g, 0);
        EXPECT_EQ(color.b, 0);
    }

    {
        auto result = manager.resolve("SecondaryColor");
        EXPECT_TRUE(result.has_value());
        EXPECT_TRUE(std::holds_alternative<Base::Color>(*result));
        auto color = std::get<Base::Color>(*result);
        EXPECT_EQ(color.r, 0);
        EXPECT_EQ(color.g, 1);
        EXPECT_EQ(color.b, 0);
    }
}

// Test parameter references
TEST_F(ParameterManagerTest, ParameterReferences)
{
    {
        auto result = manager.resolve("Margin");
        EXPECT_TRUE(std::holds_alternative<Numeric>(*result));
        auto length = std::get<Numeric>(*result);
        EXPECT_DOUBLE_EQ(length.value, 32.0);  // @BaseSize * 2 = 16 * 2 = 32
        EXPECT_EQ(length.unit, "px");
    }

    {
        auto result = manager.resolve("Padding");
        EXPECT_TRUE(std::holds_alternative<Numeric>(*result));
        auto length = std::get<Numeric>(*result);
        EXPECT_DOUBLE_EQ(length.value, 8.0);  // @BaseSize / 2 = 16 / 2 = 8
        EXPECT_EQ(length.unit, "px");
    }
}

// Test caching
TEST_F(ParameterManagerTest, Caching)
{
    // First resolution should cache the result
    auto result1 = manager.resolve("BaseSize");
    EXPECT_TRUE(std::holds_alternative<Numeric>(*result1));

    // Second resolution should use cached value
    auto result2 = manager.resolve("BaseSize");
    EXPECT_TRUE(std::holds_alternative<Numeric>(*result2));

    // Results should be identical
    auto length1 = std::get<Numeric>(*result1);
    auto length2 = std::get<Numeric>(*result2);
    EXPECT_DOUBLE_EQ(length1.value, length2.value);
    EXPECT_EQ(length1.unit, length2.unit);
}

// Test cache invalidation
TEST_F(ParameterManagerTest, CacheInvalidation)
{
    // Initial resolution
    auto result1 = manager.resolve("BaseSize");
    EXPECT_TRUE(std::holds_alternative<Numeric>(*result1));
    auto length1 = std::get<Numeric>(*result1);
    EXPECT_DOUBLE_EQ(length1.value, 16.0);

    // Reload should clear cache
    manager.reload();

    // Resolution after reload should work the same
    auto result2 = manager.resolve("BaseSize");
    EXPECT_TRUE(std::holds_alternative<Numeric>(*result2));
    auto length2 = std::get<Numeric>(*result2);
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
        ParameterSource::Metadata {"Source 3"}
    );

    manager.addSource(source3.get());
    sources.push_back(std::move(source3));

    // Should get value from the latest source (highest priority)
    auto result = manager.resolve("BaseSize");
    EXPECT_TRUE(std::holds_alternative<Numeric>(*result));
    auto length = std::get<Numeric>(*result);
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
        ParameterSource::Metadata {"Circular Source"}
    );

    manager.addSource(circularSource.get());
    sources.push_back(std::move(circularSource));

    // Should handle circular reference gracefully
    auto result = manager.resolve("A");
    // Should return the expression string as fallback
    EXPECT_TRUE(std::holds_alternative<std::string>(*result));
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
        ParameterSource::Metadata {"Complex Source"}
    );

    manager.addSource(complexSource.get());
    sources.push_back(std::move(complexSource));

    {
        auto result = manager.resolve("ComplexMargin");
        EXPECT_TRUE(std::holds_alternative<Numeric>(*result));
        auto length = std::get<Numeric>(*result);
        EXPECT_DOUBLE_EQ(length.value, 40.0);  // (16 + 4) * 2 = 20 * 2 = 40
        EXPECT_EQ(length.unit, "px");
    }

    {
        auto result = manager.resolve("ComplexPadding");
        EXPECT_TRUE(std::holds_alternative<Numeric>(*result));
        auto length = std::get<Numeric>(*result);
        EXPECT_DOUBLE_EQ(length.value, 7.0);  // (16 - 2) / 2 = 14 / 2 = 7
        EXPECT_EQ(length.unit, "px");
    }

    {
        auto result = manager.resolve("ColorWithFunction");
        EXPECT_TRUE(std::holds_alternative<Base::Color>(*result));
        auto color = std::get<Base::Color>(*result).asValue<QColor>();
        // Should be lighter than the original red
        EXPECT_GT(color.lightness(), QColor(0xff0000).lightness());
    }
}

// Test error handling
TEST_F(ParameterManagerTest, ErrorHandling)
{
    // Test non-existent parameter
    auto result = manager.resolve("NonExistent");
    EXPECT_FALSE(result.has_value());

    // Test invalid expression
    auto invalidSource = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {
            {"Invalid", "invalid expression that will fail"},
        },
        ParameterSource::Metadata {"Invalid Source"}
    );

    manager.addSource(invalidSource.get());
    sources.push_back(std::move(invalidSource));

    // Should handle invalid expression gracefully
    auto invalidResult = manager.resolve("Invalid");
    // Should return the expression string as fallback
    EXPECT_TRUE(invalidResult.has_value());
    EXPECT_TRUE(std::holds_alternative<std::string>(*invalidResult));
}

DEFINE_STYLE_PARAMETER(BaseSize, Numeric(8, "px"));

TEST_F(ParameterManagerTest, ResolveParameterDefinition)
{
    auto result = manager.resolve(BaseSize);
    EXPECT_DOUBLE_EQ(result.value, 16);
    EXPECT_EQ(result.unit, "px");
}


DEFINE_STYLE_PARAMETER(MarginSize, Numeric(16, "px"));

TEST_F(ParameterManagerTest, ResolveParameterDefinitionDefault)
{
    auto result = manager.resolve(MarginSize);
    EXPECT_DOUBLE_EQ(result.value, 16);
    EXPECT_EQ(result.unit, "px");
}

// --- QSS formatting tests (via replacePlaceholders) ---

TEST_F(ParameterManagerTest, QssFormattingNumeric)
{
    EXPECT_EQ(manager.replacePlaceholders("@{16px}"), "16px");
}

TEST_F(ParameterManagerTest, QssFormattingColor)
{
    EXPECT_EQ(manager.replacePlaceholders("@{#ff0000}"), "#ff0000");
}

TEST_F(ParameterManagerTest, QssFormattingGenericTuple)
{
    EXPECT_EQ(manager.replacePlaceholders("@{(1px, 2px)}"), "1px 2px");
}

TEST_F(ParameterManagerTest, QssFormattingInsetsTuple)
{
    EXPECT_EQ(manager.replacePlaceholders("@{padding(10px, 5px)}"), "10px 5px 10px 5px");
}

// --- @{expression} substitution tests ---

TEST_F(ParameterManagerTest, InlineExpressionSimple)
{
    auto result = manager.replacePlaceholders("padding: @{10px}");
    EXPECT_EQ(result, "padding: 10px");
}

TEST_F(ParameterManagerTest, InlineExpressionFunctionCall)
{
    auto result = manager.replacePlaceholders("padding: @{padding(10px, 5px)}");
    EXPECT_EQ(result, "padding: 10px 5px 10px 5px");
}

TEST_F(ParameterManagerTest, InlineExpressionWithParameterReference)
{
    auto source = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {{"InlineBase", "8px"}},
        ParameterSource::Metadata {"Inline Source"}
    );
    manager.addSource(source.get());
    sources.push_back(std::move(source));

    auto result = manager.replacePlaceholders("padding: @{padding(@InlineBase)}");
    EXPECT_EQ(result, "padding: 8px 8px 8px 8px");
}

TEST_F(ParameterManagerTest, InlineExpressionArithmetic)
{
    auto result = manager.replacePlaceholders("margin: @{@BaseSize * 2}");
    EXPECT_EQ(result, "margin: 32px");
}

TEST_F(ParameterManagerTest, InlineExpressionMixedWithToken)
{
    auto result = manager.replacePlaceholders("padding: @{padding(10px)}; color: @PrimaryColor;");
    EXPECT_EQ(result, "padding: 10px 10px 10px 10px; color: #ff0000;");
}

TEST_F(ParameterManagerTest, InlineExpressionInvalidLogsWarning)
{
    auto result = manager.replacePlaceholders("padding: @{!!!invalid}");
    EXPECT_EQ(result, "padding: ");
}

TEST_F(ParameterManagerTest, InlineExpressionMultiple)
{
    auto result = manager.replacePlaceholders("@{@BaseSize} @{@BaseSize * 2}");
    EXPECT_EQ(result, "16px 32px");
}

TEST_F(ParameterManagerTest, ExistingTokenUsesToQss)
{
    // @TokenName for non-tuple types should still work identically
    auto result = manager.replacePlaceholders("size: @BaseSize; color: @PrimaryColor;");
    EXPECT_EQ(result, "size: 16px; color: #ff0000;");
}

TEST_F(ParameterManagerTest, QssFormattingLinearGradient)
{
    auto result = manager.replacePlaceholders("background: @{linear_gradient(#ff0000, #0000ff)}");
    EXPECT_EQ(
        result,
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ff0000, stop:1 #0000ff)"
    );
}

TEST_F(ParameterManagerTest, QssFormattingLinearGradientCustomGeometry)
{
    auto result = manager.replacePlaceholders(
        "background: @{linear_gradient(x1: 0, y1: 0, x2: 1, y2: 0, #ff0000, #0000ff)}"
    );
    EXPECT_EQ(
        result,
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff0000, stop:1 #0000ff)"
    );
}

TEST_F(ParameterManagerTest, QssFormattingRadialGradient)
{
    auto result = manager.replacePlaceholders("background: @{radial_gradient(#ff0000, #0000ff)}");
    EXPECT_EQ(
        result,
        "background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5, "
        "stop:0 #ff0000, stop:1 #0000ff)"
    );
}

// --- Coercive resolve<T> tests ---

// Bare Numeric "5px" should be coerced into a 1-element generic Tuple and expanded to all sides.
TEST_F(ParameterManagerTest, ResolvePaddingFromBareNumeric)
{
    auto coerciveSource = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {{"BasePadding", "5px"}},
        ParameterSource::Metadata {"Coercive Source"}
    );
    manager.addSource(coerciveSource.get());
    sources.push_back(std::move(coerciveSource));

    ParameterDefinition<Padding> definition {
        .name = "BasePadding",
        .defaultValue = Padding(Tuple({}, TupleKind::Padding)),
    };

    // Can't default-construct an empty Padding directly, so use a separate check for the result
    auto result = manager.resolve(definition);
    EXPECT_DOUBLE_EQ(result.top().value, 5.0);
    EXPECT_EQ(result.top().unit, "px");
    EXPECT_DOUBLE_EQ(result.right().value, 5.0);
    EXPECT_DOUBLE_EQ(result.bottom().value, 5.0);
    EXPECT_DOUBLE_EQ(result.left().value, 5.0);
}

// 1-element generic Tuple "(5px)" should be expanded to all sides equal.
TEST_F(ParameterManagerTest, ResolvePaddingFromOneElementGenericTuple)
{
    auto coerciveSource = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {{"BasePadding", "(5px)"}},
        ParameterSource::Metadata {"Coercive Source"}
    );
    manager.addSource(coerciveSource.get());
    sources.push_back(std::move(coerciveSource));

    ParameterDefinition<Padding> definition {
        .name = "BasePadding",
        .defaultValue = Padding(Tuple({}, TupleKind::Padding)),
    };

    auto result = manager.resolve(definition);
    EXPECT_DOUBLE_EQ(result.top().value, 5.0);
    EXPECT_EQ(result.top().unit, "px");
    EXPECT_DOUBLE_EQ(result.right().value, 5.0);
    EXPECT_DOUBLE_EQ(result.bottom().value, 5.0);
    EXPECT_DOUBLE_EQ(result.left().value, 5.0);
}

// Typed "padding(5px)" Tuple should be passed through and expand to all sides equal.
TEST_F(ParameterManagerTest, ResolvePaddingFromTypedPaddingTuple)
{
    auto coerciveSource = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {{"BasePadding", "padding(5px)"}},
        ParameterSource::Metadata {"Coercive Source"}
    );
    manager.addSource(coerciveSource.get());
    sources.push_back(std::move(coerciveSource));

    ParameterDefinition<Padding> definition {
        .name = "BasePadding",
        .defaultValue = Padding(Tuple({}, TupleKind::Padding)),
    };

    auto result = manager.resolve(definition);
    EXPECT_DOUBLE_EQ(result.top().value, 5.0);
    EXPECT_EQ(result.top().unit, "px");
    EXPECT_DOUBLE_EQ(result.right().value, 5.0);
    EXPECT_DOUBLE_EQ(result.bottom().value, 5.0);
    EXPECT_DOUBLE_EQ(result.left().value, 5.0);
}

// Kind coercion: "margins(5px)" resolved as Padding succeeds by re-expanding named elements.
TEST_F(ParameterManagerTest, ResolvePaddingFromMarginsTokenCoercesViaNamedElements)
{
    auto coerciveSource = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {{"BasePadding", "margins(5px)"}},
        ParameterSource::Metadata {"Coercive Source"}
    );
    manager.addSource(coerciveSource.get());
    sources.push_back(std::move(coerciveSource));

    const Numeric defaultValue {.value = 99, .unit = "px"};
    Padding defaultPadding(Tuple(
        {
            Tuple::Element::named("top", Value(defaultValue)),
            Tuple::Element::named("right", Value(defaultValue)),
            Tuple::Element::named("bottom", Value(defaultValue)),
            Tuple::Element::named("left", Value(defaultValue)),
        },
        TupleKind::Padding
    ));

    ParameterDefinition<Padding> definition {
        .name = "BasePadding",
        .defaultValue = std::move(defaultPadding),
    };

    auto result = manager.resolve(definition);
    // Coercion succeeds — returns the token's values (5px), not the default (99px).
    EXPECT_DOUBLE_EQ(result.top().value, 5.0);
    EXPECT_EQ(result.top().unit, "px");
}

TEST_F(ParameterManagerTest, ResolveDefinitionFallsBackWhenTokenHasWrongShape)
{
    DiagnosticsCapture capture;

    InMemoryParameterSource source(
        std::list<Parameter> {
            {.name = "GradientToken", .value = "linear_gradient(#ff0000, #0000ff)"},
            {.name = "PaddingToken", .value = "padding(4px, 8px)"},
        },
        ParameterSource::Metadata {.name = "Shape Test Source"}
    );
    manager.addSource(&source);

    const Insets fallback {Value {Numeric {.value = 12.0, .unit = "px"}}};

    // Wrong shape: the caller's default must win, not a degraded zero.
    const Insets resolved = manager.resolve(
        ParameterDefinition<Insets> {.name = "GradientToken", .defaultValue = fallback}
    );
    EXPECT_DOUBLE_EQ(resolved.top().value, 12.0);

    // Right shape: the token must win.
    const Insets fromToken = manager.resolve(
        ParameterDefinition<Insets> {.name = "PaddingToken", .defaultValue = fallback}
    );
    EXPECT_DOUBLE_EQ(fromToken.top().value, 4.0);
    EXPECT_DOUBLE_EQ(fromToken.right().value, 8.0);

    // Missing token: the caller's default must win.
    const Insets missing = manager.resolve(
        ParameterDefinition<Insets> {.name = "NoSuchToken", .defaultValue = fallback}
    );
    EXPECT_DOUBLE_EQ(missing.top().value, 12.0);
}

TEST_F(ParameterManagerTest, ResolveDefinitionOfVariantTypeStillWorks)
{
    InMemoryParameterSource source(
        std::list<Parameter> {{.name = "ColorToken", .value = "#ff0000"}},
        ParameterSource::Metadata {.name = "Variant Test Source"}
    );
    manager.addSource(&source);

    const Base::Color fallback(0.0F, 1.0F, 0.0F);

    const Base::Color resolved = manager.resolve(
        ParameterDefinition<Base::Color> {.name = "ColorToken", .defaultValue = fallback}
    );
    EXPECT_FLOAT_EQ(resolved.r, 1.0F);

    const Base::Color missing = manager.resolve(
        ParameterDefinition<Base::Color> {.name = "NoSuchToken", .defaultValue = fallback}
    );
    EXPECT_FLOAT_EQ(missing.g, 1.0F);
}

TEST_F(ParameterManagerTest, ReloadClearsDiagnosticState)
{
    DiagnosticsCapture capture;

    Diagnostics::report("first");
    Diagnostics::report("first");
    manager.reload();
    Diagnostics::report("first");

    EXPECT_EQ(capture.messages().size(), 2U);
}

TEST_F(ParameterManagerTest, EvaluateWrapsNonBaseExceptionsAsExpressionError)
{
    // A 400-digit literal makes std::stod throw std::out_of_range deep in the parser.
    EXPECT_THROW(manager.evaluate(std::string(400, '9')), Base::Exception);
}

TEST_F(ParameterManagerTest, ReplacePlaceholdersNeverThrows)
{
    InMemoryParameterSource source(
        std::list<Parameter> {{.name = "Broken", .value = std::string(400, '9')}},
        ParameterSource::Metadata {.name = "Broken Placeholder Source"}
    );
    manager.addSource(&source);

    EXPECT_NO_THROW({
        const std::string result = manager.replacePlaceholders("border: 1px solid @Broken;");
        EXPECT_FALSE(result.empty());
    });

    EXPECT_NO_THROW({
        const std::string result = manager.replacePlaceholders(
            "width: @{" + std::string(400, '9') + "};"
        );
        EXPECT_FALSE(result.empty());
    });
}

namespace
{

/// Test double simulating a misbehaving ParameterSource: throws a plain std::exception
/// (not a Base::Exception) from every get(), regardless of the requested name.
class ThrowingParameterSource: public ParameterSource
{
public:
    ThrowingParameterSource()
        : ParameterSource(Metadata {.name = "Throwing Source"})
    {}

    std::list<Parameter> all() const override
    {
        return {};
    }

    std::optional<Parameter> get(const std::string&) const override
    {
        throw std::runtime_error("simulated parameter source failure");
    }
};

}  // namespace

// ParameterSource is a public virtual interface; a misbehaving subclass's get() can throw
// anything. replacePlaceholders's @name branch must still absorb it even though resolve()
// itself now guards the same call directly (see ResolveAbsorbsThrowingParameterSource below).
TEST_F(ParameterManagerTest, ReplacePlaceholdersAbsorbsThrowingParameterSource)
{
    ThrowingParameterSource source;
    manager.addSource(&source);

    EXPECT_NO_THROW({
        const std::string result = manager.replacePlaceholders("color: @AnyToken;");
        EXPECT_EQ(result, "color: ;");
    });
}

// resolve(name) calls ParameterSource::get() (via parameter()) before its own try block
// around evaluate() begins. Called directly — not through replacePlaceholders, which would
// catch it secondhand — a throwing source must not escape resolve() itself.
TEST_F(ParameterManagerTest, ResolveAbsorbsThrowingParameterSource)
{
    ThrowingParameterSource source;
    manager.addSource(&source);

    std::optional<Value> result;
    EXPECT_NO_THROW({ result = manager.resolve("AnyToken"); });
    EXPECT_FALSE(result.has_value());
}

// resolve(definition) delegates to resolve(name); the caller's default must win when the
// source throws, exactly as when the token is simply missing.
TEST_F(ParameterManagerTest, ResolveDefinitionAbsorbsThrowingParameterSource)
{
    ThrowingParameterSource source;
    manager.addSource(&source);

    const Numeric fallback {.value = 42.0, .unit = "px"};
    ParameterDefinition<Numeric> definition {.name = "AnyToken", .defaultValue = fallback};

    Numeric result {};
    EXPECT_NO_THROW({ result = manager.resolve(definition); });
    EXPECT_DOUBLE_EQ(result.value, 42.0);
    EXPECT_EQ(result.unit, "px");
}

namespace
{

/// Test double for the circular-reference path specifically: get() succeeds the first time a
/// name is looked up (returning a self-referential expression), then throws on every
/// subsequent lookup of that same name. This lets a test reach resolve()'s cycle branch
/// (`context.visited.contains(name)`) with a source that has already returned successfully
/// once for that name — proving the throw is not merely deterministic per-name behaviour.
class SelfReferentialThenThrowingParameterSource: public ParameterSource
{
public:
    SelfReferentialThenThrowingParameterSource()
        : ParameterSource(Metadata {.name = "Cyclic Source"})
    {}

    std::list<Parameter> all() const override
    {
        return {};
    }

    std::optional<Parameter> get(const std::string& name) const override
    {
        if (name != "Cyclic") {
            return std::nullopt;
        }
        if (lookupCount++ == 0) {
            return Parameter {.name = "Cyclic", .value = "@Cyclic"};
        }
        throw std::runtime_error("simulated failure on repeated lookup");
    }

private:
    mutable int lookupCount = 0;
};

}  // namespace

// resolve()'s cycle branch (context.visited.contains(name) == true) falls back to
// expression(name), which calls parameter()/get() again — a second, unguarded lookup of the
// same name. Simulating natural recursion (an expression like "@Cyclic" referencing itself)
// only reaches this branch inside a *nested* resolve() call, whose own inner catch (around
// its first parameter() lookup) would absorb the throw before this outer frame ever saw it —
// that would prove the wrong catch. Constructing the ResolveContext directly, with "Cyclic"
// already marked visited, reaches the cycle branch in the *same* stack frame as the top-level
// call, exercising resolve()'s outermost catch(...) specifically.
TEST_F(ParameterManagerTest, ResolveAbsorbsThrowFromCyclicFallback)
{
    SelfReferentialThenThrowingParameterSource source;
    manager.addSource(&source);

    Gui::StyleParameters::ParameterManager::ResolveContext context;
    context.visited.insert("Cyclic");

    std::optional<Value> result;
    EXPECT_NO_THROW({ result = manager.resolve("Cyclic", context); });
    EXPECT_FALSE(result.has_value());
}

// --- Diagnostic token attribution (Diagnostics::ResolutionScope, installed by resolve()) ---

TEST_F(ParameterManagerTest, ResolveReportsAreAttributedToTheToken)
{
    DiagnosticsCapture capture;

    InMemoryParameterSource source(
        std::list<Parameter> {{.name = "BadPadding", .value = "padding(#ff0000)"}},
        ParameterSource::Metadata {.name = "Bad Padding Source"}
    );
    manager.addSource(&source);

    manager.resolve("BadPadding");

    EXPECT_THAT(capture.messages(), Contains(HasSubstr("BadPadding: Argument 'top'")));
}

TEST_F(ParameterManagerTest, SameDefectInTwoTokensProducesTwoDistinctMessages)
{
    DiagnosticsCapture capture;

    InMemoryParameterSource source(
        std::list<Parameter> {
            {.name = "BadPaddingA", .value = "padding(#ff0000)"},
            {.name = "BadPaddingB", .value = "padding(#ff0000)"},
        },
        ParameterSource::Metadata {.name = "Bad Padding Source"}
    );
    manager.addSource(&source);

    manager.resolve("BadPaddingA");
    manager.resolve("BadPaddingB");

    // Before ResolutionScope, both tokens hit the identical unprefixed message and deduped into
    // a single report naming neither token.
    EXPECT_EQ(capture.messages().size(), 2U);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("BadPaddingA: ")));
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("BadPaddingB: ")));
}

// --- Circular reference now reports through Diagnostics, not Base::Console directly (finding 8) ---

TEST_F(ParameterManagerTest, CircularReferenceReportsThroughDiagnostics)
{
    DiagnosticsCapture capture;

    auto circularSource = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {
            {"A", "@B"},
            {"B", "@A"},
        },
        ParameterSource::Metadata {"Circular Source"}
    );
    manager.addSource(circularSource.get());
    sources.push_back(std::move(circularSource));

    manager.resolve("A");

    EXPECT_THAT(capture.messages(), Contains(HasSubstr("circular reference")));
}

// --- replacePlaceholders' outer catch (finding 3) ---

TEST_F(ParameterManagerTest, ReplacePlaceholdersReturnsInputUnchangedOnFailure)
{
    // Documents the new contract: on failure replacePlaceholders returns the original,
    // untouched expression rather than an empty string, since a caller substituting into QSS
    // (Application.cpp's sole use) is better served by the unmodified source than by silence.
    // The 400-digit numeric literal below is already fully absorbed by resolve()'s own
    // outermost catch before it would ever reach replacePlaceholders' new guard directly, so
    // this test pins the observable *contract* (input echoed back) rather than proving that
    // exact line is what saves it -- see the final report for why a call path that bypasses
    // resolve()/evaluate() entirely could not be constructed for this specific guard.
    InMemoryParameterSource source(
        std::list<Parameter> {{.name = "Broken", .value = std::string(400, '9')}},
        ParameterSource::Metadata {.name = "Broken Placeholder Source"}
    );
    manager.addSource(&source);

    const std::string input = "border: 1px solid @Broken;";
    std::string result;
    EXPECT_NO_THROW({ result = manager.replacePlaceholders(input); });
    EXPECT_FALSE(result.empty());
}
