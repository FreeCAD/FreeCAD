// SPDX-License-Identifier: LGPL-2.1-or-later

#include <App/StartupConfiguration.h>

#include <gtest/gtest.h>

TEST(StartupConfiguration, AppliesTypedOptionsToApplicationConfiguration)
{
    App::StartupOptions options;
    options.modulePaths = std::vector<std::string> {"module-a", "module-b"};
    options.macroPaths = std::vector<std::string> {"macro-a", "macro-b"};
    options.disabledAddons = std::vector<std::string> {"AddonA", "AddonB"};
    options.inputFiles = std::vector<std::string> {"first.FCStd", "second.FCStd"};
    options.outputFile = "out.FCStd";
    options.startHidden = true;
    options.logFile = "freecad.log";
    options.userConfigFile = "user.cfg";
    options.systemConfigFile = "system.cfg";
    options.singleInstance = true;

    std::map<std::string, std::string> config;
    const auto result = App::applyStartupOptionsToConfig(options, config);

    EXPECT_FALSE(result.shouldExit());
    EXPECT_EQ(config["AdditionalModulePaths"], "module-a;module-b");
    EXPECT_EQ(config["AdditionalMacroPaths"], "macro-a;macro-b");
    EXPECT_EQ(config["DisabledAddons"], "AddonA;AddonB");
    EXPECT_EQ(config["OpenFileCount"], "2");
    EXPECT_EQ(config["OpenFile0"], "first.FCStd");
    EXPECT_EQ(config["OpenFile1"], "second.FCStd");
    EXPECT_EQ(config["SaveFile"], "out.FCStd");
    EXPECT_EQ(config["StartHidden"], "1");
    EXPECT_EQ(config["LoggingFileName"], "freecad.log");
    EXPECT_EQ(config["UserParameter"], "user.cfg");
    EXPECT_EQ(config["SystemParameter"], "system.cfg");
    EXPECT_EQ(config["SingleInstance"], "1");
}

TEST(StartupConfiguration, PreservesTestSelectionRules)
{
    App::StartupOptions options;
    options.runOpenTests = std::vector<std::string> {"TestGui.One", "0"};
    options.runTests = std::vector<std::string> {"TestApp.One"};

    std::map<std::string, std::string> config;
    (void)App::applyStartupOptionsToConfig(options, config);

    EXPECT_EQ(config["TestCase"], "TestApp.All");
    EXPECT_EQ(config["RunMode"], "Internal");
    EXPECT_EQ(config["ScriptFileName"], "FreeCADTest");
    EXPECT_EQ(config["ExitTests"], "no");

    options.runTests = std::vector<std::string> {""};
    (void)App::applyStartupOptionsToConfig(options, config);
    EXPECT_EQ(config["TestCase"], "TestApp.PrintAll");
}

TEST(StartupConfiguration, GetConfigObservesValuesBeforeSetConfig)
{
    App::StartupOptions options;
    options.request = App::StartupRequest::GetConfig;
    options.getConfig = "Key";
    options.setConfig = std::vector<std::string> {"Key=after"};
    std::map<std::string, std::string> config {{"Key", "before"}};

    const auto result = App::applyStartupOptionsToConfig(options, config);

    EXPECT_EQ(result.request, App::StartupRequest::GetConfig);
    EXPECT_EQ(result.message, "before\n");
    EXPECT_EQ(config["Key"], "before");
}

TEST(StartupConfiguration, AppliesSetConfigDuringNormalStartup)
{
    App::StartupOptions options;
    options.setConfig = std::vector<std::string> {"Key=after", "Ignored", "Empty="};
    std::map<std::string, std::string> config {{"Key", "before"}};

    const auto result = App::applyStartupOptionsToConfig(options, config);

    EXPECT_FALSE(result.shouldExit());
    EXPECT_EQ(config["Key"], "after");
    EXPECT_EQ(config["Empty"], "");
    EXPECT_FALSE(config.contains("Ignored"));
}
