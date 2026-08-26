// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Mod/CadX/App/ToolRegistry.h>

namespace
{
CadX::ToolDefinition definition(CadX::ToolExecutor executor)
{
    return {"cadx.test", "closed test tool",
            CadX::ToolClassification::Read,
            R"json({"type":"object","properties":{"mode":{"type":"string","enum":["ok"]},"items":{"type":"array","items":{"type":"object","properties":{"id":{"type":"string"}},"required":["id"],"additionalProperties":false}}},"required":["mode"],"additionalProperties":false})json",
            "cadx.test-result.v1", std::move(executor), CadX::ThreadRequirement::Any, 4096};
}
}  // namespace

TEST(CadXToolRegistry, RejectsMalformedRootBeforeExecutor)
{
    CadX::ToolRegistry registry;
    int calls = 0;
    std::string diagnostic;
    ASSERT_TRUE(registry.registerDefinition(
        definition([&](const std::string&) {
            ++calls;
            return CadX::ToolResult::success("cadx.test-result.v1", "{}");
        }),
        diagnostic))
        << diagnostic;

    const auto unknown = registry.execute("cadx.test", R"({"mode":"ok","extra":true})");
    EXPECT_FALSE(unknown.ok);
    EXPECT_EQ(unknown.errorCode, "CADX_TOOL_ARGUMENTS_INVALID");

    const auto nested = registry.execute(
        "cadx.test", R"({"mode":"ok","items":[{"id":"x","extra":1}]})");
    EXPECT_FALSE(nested.ok);
    EXPECT_EQ(nested.errorCode, "CADX_TOOL_ARGUMENTS_INVALID");

    const auto valid = registry.execute("cadx.test", R"({"mode":"ok"})");
    EXPECT_TRUE(valid.ok);
    EXPECT_EQ(calls, 1);
}

TEST(CadXToolRegistry, RejectsNonObjectAndMissingRequiredArguments)
{
    CadX::ToolRegistry registry;
    std::string diagnostic;
    ASSERT_TRUE(registry.registerDefinition(
        definition([](const std::string&) {
            return CadX::ToolResult::success("cadx.test-result.v1", "{}");
        }),
        diagnostic));

    EXPECT_EQ(registry.execute("cadx.test", "[]").errorCode,
              "CADX_TOOL_ARGUMENTS_INVALID");
    EXPECT_EQ(registry.execute("cadx.test", R"({})").errorCode,
              "CADX_TOOL_ARGUMENTS_INVALID");
    EXPECT_EQ(registry.execute("cadx.test", R"({"mode":3})").errorCode,
              "CADX_TOOL_ARGUMENTS_INVALID");
}
