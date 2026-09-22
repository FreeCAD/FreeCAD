// SPDX-License-Identifier: LGPL-2.1-or-later

#include <App/CommandLine.h>
#include <Base/Exception.h>

#include <QTemporaryDir>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{

App::StartupOptions parse(std::vector<std::string> arguments)
{
    App::CommandLineParseOptions parseOptions;
    parseOptions.configFile = std::nullopt;
    return App::parseCommandLine(App::ProcessArguments(std::move(arguments)), "FreeCAD", parseOptions);
}

}  // namespace

TEST(CommandLine, ProducesTypedStartupOptions)
{
    const auto result = parse(
        {"FreeCAD",
         "--console",
         "--safe-mode",
         "--hidden",
         "--log-file",
         "freecad.log",
         "--set-config",
         "First=one",
         "Second=two",
         "--",
         "model.FCStd"}
    );

    EXPECT_TRUE(result.console);
    EXPECT_TRUE(result.safeMode);
    EXPECT_TRUE(result.startHidden);
    ASSERT_TRUE(result.logFile);
    EXPECT_EQ(*result.logFile, "freecad.log");
    ASSERT_TRUE(result.setConfig);
    EXPECT_EQ(*result.setConfig, (std::vector<std::string> {"First=one", "Second=two"}));
    ASSERT_TRUE(result.inputFiles);
    EXPECT_EQ(*result.inputFiles, (std::vector<std::string> {"model.FCStd"}));
}

TEST(CommandLine, PreservesPresenceAndRepeatedValues)
{
    const auto result = parse(
        {"FreeCAD", "-M", "first", "-M", "second", "-t", "TestOne", "-t", "TestTwo"}
    );

    ASSERT_TRUE(result.modulePaths);
    EXPECT_EQ(*result.modulePaths, (std::vector<std::string> {"first", "second"}));
    ASSERT_TRUE(result.runTests);
    EXPECT_EQ(*result.runTests, (std::vector<std::string> {"TestOne", "TestTwo"}));
    EXPECT_FALSE(result.runOpenTests);
}

TEST(CommandLine, AcceptsQtSingleDashStyleOption)
{
    const auto result = parse({"FreeCAD", "-style", "Fusion", "model.FCStd"});

    ASSERT_TRUE(result.inputFiles);
    EXPECT_EQ(*result.inputFiles, (std::vector<std::string> {"model.FCStd"}));
}

TEST(CommandLine, ThrowsUnknownOptionError)
{
    try {
        parse({"FreeCAD", "--console", "--does-not-exist"});
        FAIL() << "Expected Base::UnknownProgramOption";
    }
    catch (const Base::UnknownProgramOption& error) {
        EXPECT_NE(std::string(error.what()).find("unrecognised option"), std::string::npos);
    }
}

TEST(CommandLine, ThrowsMissingValueError)
{
    try {
        parse({"FreeCAD", "--log-file"});
        FAIL() << "Expected Base::UnknownProgramOption";
    }
    catch (const Base::UnknownProgramOption& error) {
        EXPECT_NE(std::string(error.what()).find("required argument"), std::string::npos);
    }
}

TEST(CommandLine, HelpTakesPriorityOverResponseFileLoading)
{
    const auto result = parse({"FreeCAD", "@file-that-does-not-exist", "--help"});

    EXPECT_EQ(result.request, App::StartupRequest::Help);
    EXPECT_NE(App::formatCommandLineHelp("FreeCAD").find("Usage: FreeCAD"), std::string::npos);
}

TEST(CommandLine, SelectsExplicitInformationalRequestsWithEstablishedPrecedence)
{
    struct Case
    {
        std::vector<std::string> arguments;
        App::StartupRequest expected;
    };
    const std::vector<Case> cases {
        {{"FreeCAD", "--version", "--dump-config", "--get-config", "ExeName"},
         App::StartupRequest::Version},
        {{"FreeCAD", "--verbose", "--version", "--dump-config"}, App::StartupRequest::DumpConfig},
        {{"FreeCAD", "--verbose", "--version"}, App::StartupRequest::VerboseVersion},
        {{"FreeCAD", "--get-config", "ExeName", "--set-config", "ExeName=Changed"},
         App::StartupRequest::GetConfig},
    };

    for (const auto& testCase : cases) {
        SCOPED_TRACE(::testing::PrintToString(testCase.arguments));
        const auto result = parse(testCase.arguments);
        EXPECT_EQ(result.request, testCase.expected);
    }
}

TEST(CommandLine, LoadsStartupConfigurationWithoutCommandLineArguments)
{
    QTemporaryDir temporaryDirectory;
    ASSERT_TRUE(temporaryDirectory.isValid());
    const std::filesystem::path configFile
        = std::filesystem::path(temporaryDirectory.path().toStdString()) / "FreeCAD.cfg";
    {
        std::ofstream stream(configFile);
        stream << "module-path=from-config\n"
               << "output=from-config.FCStd\n";
    }

    App::CommandLineParseOptions parseOptions;
    parseOptions.configFile = configFile.string();
    const auto result = App::loadStartupConfiguration("EmbeddedFreeCAD", parseOptions);

    ASSERT_TRUE(result.modulePaths);
    EXPECT_EQ(*result.modulePaths, (std::vector<std::string> {"from-config"}));
    ASSERT_TRUE(result.outputFile);
    EXPECT_EQ(*result.outputFile, "from-config.FCStd");
    EXPECT_EQ(result.request, App::StartupRequest::Run);
}

TEST(CommandLine, FormatsHelpForResolvedExecutableIdentity)
{
    const std::string help = App::formatCommandLineHelp("BrandedCAD");

    EXPECT_NE(help.find("Usage: BrandedCAD"), std::string::npos);
    EXPECT_NE(help.find("Writes BrandedCAD.log"), std::string::npos);
}
