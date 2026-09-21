// SPDX-License-Identifier: LGPL-2.1-or-later

#include "StartupConfiguration.h"

#include <boost/algorithm/string/join.hpp>

#include <sstream>

namespace App
{

StartupResult applyStartupOptionsToConfig(
    const StartupOptions& options,
    std::map<std::string, std::string>& config
)
{
    if (options.modulePaths) {
        config["AdditionalModulePaths"] = boost::join(*options.modulePaths, ";");
    }

    if (options.macroPaths) {
        config["AdditionalMacroPaths"] = boost::join(*options.macroPaths, ";");
    }

    if (options.disabledAddons) {
        config["DisabledAddons"] = boost::join(*options.disabledAddons, ";");
    }

    if (options.inputFiles) {
        int openFileCount = 0;
        for (const auto& file : *options.inputFiles) {
            config["OpenFile" + std::to_string(openFileCount)] = file;
            ++openFileCount;
        }
        config["OpenFileCount"] = std::to_string(openFileCount);
    }

    if (options.outputFile) {
        config["SaveFile"] = *options.outputFile;
    }

    if (options.startHidden) {
        config["StartHidden"] = "1";
    }

    if (options.writeLog) {
        config["LoggingFile"] = "1";
        config["LoggingFileName"] = config["UserAppData"] + config["ExeName"] + ".log";
    }

    if (options.logFile) {
        config["LoggingFile"] = "1";
        config["LoggingFileName"] = *options.logFile;
    }

    if (options.userConfigFile) {
        config["UserParameter"] = *options.userConfigFile;
    }

    if (options.systemConfigFile) {
        config["SystemParameter"] = *options.systemConfigFile;
    }

    if (options.runTests || options.runOpenTests) {
        std::vector<std::string> testCases;
        bool runAll = false;
        bool printAll = false;
        for (const auto* selected : {&options.runOpenTests, &options.runTests}) {
            if (*selected) {
                for (const auto& test : **selected) {
                    if (test == "0") {
                        runAll = true;
                    }
                    else if (test.empty()) {
                        printAll = true;
                    }
                }
                testCases.insert(testCases.end(), (**selected).begin(), (**selected).end());
            }
        }

        if (printAll) {
            testCases = {"TestApp.PrintAll"};
        }
        else if (runAll) {
            testCases = {"TestApp.All"};
        }

        config["TestCase"] = boost::join(testCases, ",");
        config["RunMode"] = "Internal";
        config["ScriptFileName"] = "FreeCADTest";
        config["ExitTests"] = options.runOpenTests ? "no" : "yes";
    }

    if (options.singleInstance) {
        config["SingleInstance"] = "1";
    }

    // Preserve the historical precedence: informational requests return before --set-config.
    if (options.request == StartupRequest::DumpConfig) {
        std::stringstream message;
        for (const auto& item : config) {
            message << item.first << '=' << item.second << '\n';
        }
        return {StartupRequest::DumpConfig, message.str()};
    }

    if (options.request == StartupRequest::GetConfig) {
        std::stringstream message;
        if (options.getConfig) {
            const auto position = config.find(*options.getConfig);
            if (position != config.end()) {
                message << position->second;
            }
        }
        message << '\n';
        return {StartupRequest::GetConfig, message.str()};
    }

    if (options.setConfig) {
        for (const auto& item : *options.setConfig) {
            const auto separator = item.find('=');
            if (separator != std::string::npos) {
                config[item.substr(0, separator)] = item.substr(separator + 1);
            }
        }
    }

    return {};
}

}  // namespace App
