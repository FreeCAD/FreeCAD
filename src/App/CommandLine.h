// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "FCGlobal.h"
#include "ProcessArguments.h"

#include <optional>
#include <string>
#include <vector>

namespace App
{

enum class StartupRequest
{
    Run,
    Help,
    Version,
    DumpConfig,
    GetConfig,
    VerboseVersion,
};

struct AppExport StartupResult
{
    StartupRequest request {StartupRequest::Run};
    std::string message;

    [[nodiscard]] bool shouldExit() const
    {
        return request != StartupRequest::Run;
    }
};

/** Parsed command-line and FreeCAD.cfg options.
 *
 * This type deliberately does not expose Boost.Program_options. An engaged optional means that
 * the corresponding value was explicitly provided, including when its contained vector is empty.
 */
struct AppExport StartupOptions
{
    StartupRequest request {StartupRequest::Run};

    bool console {false};
    bool keepDeprecatedPaths {false};
    bool writeLog {false};
    bool singleInstance {false};
    bool safeMode {false};
    bool startHidden {false};

    std::optional<std::string> getConfig;
    std::optional<std::vector<std::string>> setConfig;
    std::optional<std::string> logFile;
    std::optional<std::string> userConfigFile;
    std::optional<std::string> systemConfigFile;
    std::optional<std::vector<std::string>> runTests;
    std::optional<std::vector<std::string>> runOpenTests;
    std::optional<std::vector<std::string>> modulePaths;
    std::optional<std::vector<std::string>> macroPaths;
    std::optional<std::vector<std::string>> pythonPaths;
    std::optional<std::vector<std::string>> disabledAddons;
    std::optional<std::vector<std::string>> inputFiles;
    std::optional<std::string> outputFile;
};

struct AppExport CommandLineParseOptions
{
    // nullopt disables implicit configuration-file loading (useful for callers and unit tests).
    std::optional<std::string> configFile {"FreeCAD.cfg"};
};

/** Parse startup options.
 * @throws Base::UnknownProgramOption if the command line or configuration is invalid.
 */
AppExport StartupOptions parseCommandLine(
    const ProcessArguments& arguments,
    const std::string& executableName,
    const CommandLineParseOptions& parseOptions = {}
);

/** Load startup options from FreeCAD.cfg without manufacturing a command line.
 *
 * This is the initialization entry point for embedded users which have an executable identity but
 * no FreeCAD command line of their own. It uses the same option schema as parseCommandLine().
 * @throws Base::UnknownProgramOption if the configuration is invalid.
 */
AppExport StartupOptions loadStartupConfiguration(
    const std::string& executableName,
    const CommandLineParseOptions& parseOptions = {}
);

/** Format help from the shared Boost option schema for the resolved executable identity. */
AppExport std::string formatCommandLineHelp(const std::string& executableName);

}  // namespace App
