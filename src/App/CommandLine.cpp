// SPDX-License-Identifier: LGPL-2.1-or-later

#include "CommandLine.h"

#include <FCConfig.h>

#include <Base/Exception.h>

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>

#include "ProgramOptionsUtilities.h"

namespace App
{
namespace
{

namespace po = boost::program_options;

template<typename T>
std::optional<T> optionValue(const po::variables_map& values, const char* name)
{
    if (values.contains(name) && !values.at(name).empty()) {
        return values[name].as<T>();
    }
    return std::nullopt;
}

StartupOptions makeStartupOptions(const po::variables_map& values)
{
    StartupOptions options;
    options.console = values.contains("console");
    options.keepDeprecatedPaths = values.contains("keep-deprecated-paths");
    options.writeLog = values.contains("write-log");
    options.singleInstance = values.contains("single-instance");
    options.safeMode = values.contains("safe-mode");
    options.startHidden = values.contains("hidden");

    options.getConfig = optionValue<std::string>(values, "get-config");
    options.setConfig = optionValue<std::vector<std::string>>(values, "set-config");
    options.logFile = optionValue<std::string>(values, "log-file");
    options.userConfigFile = optionValue<std::string>(values, "user-cfg");
    options.systemConfigFile = optionValue<std::string>(values, "system-cfg");
    options.runTests = optionValue<std::vector<std::string>>(values, "run-test");
    options.runOpenTests = optionValue<std::vector<std::string>>(values, "run-open");
    options.modulePaths = optionValue<std::vector<std::string>>(values, "module-path");
    options.macroPaths = optionValue<std::vector<std::string>>(values, "macro-path");
    options.pythonPaths = optionValue<std::vector<std::string>>(values, "python-path");
    options.disabledAddons = optionValue<std::vector<std::string>>(values, "disable-addon");
    options.inputFiles = optionValue<std::vector<std::string>>(values, "input-file");
    options.outputFile = optionValue<std::string>(values, "output");
    return options;
}

std::string parseErrorMessage(const std::exception& error, const po::options_description& visible)
{
    std::stringstream message;
    message << error.what() << '\n' << '\n' << visible << '\n';
    return message.str();
}

std::string unknownParseErrorMessage(const po::options_description& visible)
{
    std::stringstream message;
    message << "Wrong or unknown option, bailing out!" << '\n' << '\n' << visible << '\n';
    return message.str();
}

std::string helpMessage(const std::string& executableName, const po::options_description& visible)
{
    std::stringstream message;
    message << executableName << '\n' << '\n';
    message << "For a detailed description see "
               "https://www.freecad.org/wiki/Start_up_and_Configuration"
            << '\n'
            << '\n';
    message << "Usage: " << executableName << " [options] File1 File2 ..." << '\n' << '\n';
    message << visible << '\n';
    return message.str();
}

void addGenericOptions(po::options_description& generic)
{
    // clang-format off
    generic.add_options()
        ("version,v", "Prints version string")
        ("verbose", "Prints verbose version string")
        ("help,h", "Prints help message")
        ("console,c", "Starts in console mode")
        ("response-file", po::value<std::string>(), "Can be specified with '@name', too")
        ("dump-config", "Dumps configuration")
        ("get-config", po::value<std::string>(), "Prints the value of the requested configuration key")
        ("set-config", po::value<std::vector<std::string>>()->multitoken(), "Sets the value of a configuration key")
        ("keep-deprecated-paths", "If set then config files are kept on the old location");
    // clang-format on
}

void addConfigurationOptions(po::options_description& config, const std::string& executableName)
{
    std::stringstream logDescription;
    logDescription << "Writes " << executableName << ".log to the user directory.";
    // clang-format off
    config.add_options()
        ("write-log,l", logDescription.str().c_str())
        ("log-file", po::value<std::string>(), "Unlike --write-log this allows logging to an arbitrary file")
        ("user-cfg,u", po::value<std::string>(), "User config file to load/save user settings")
        ("system-cfg,s", po::value<std::string>(), "System config file to load/save system settings")
        ("run-test,t", po::value<std::vector<std::string>>()->composing()->implicit_value(std::vector<std::string>{""}, ""),
         "Run one or more test cases (repeat -t for multiple). Use 0 (zero) to run all tests. If no argument is provided then return list of all available tests.")
        ("run-open,r", po::value<std::vector<std::string>>()->composing()->implicit_value(std::vector<std::string>{""}, ""),
         "Run one or more test cases (repeat -r for multiple). Use 0 (zero) to run all tests. If no argument is provided then return list of all available tests.  Keeps UI open after test(s) complete.")
        ("module-path,M", po::value<std::vector<std::string>>()->composing(), "Additional module paths")
        ("macro-path,E", po::value<std::vector<std::string>>()->composing(), "Additional macro paths")
        ("python-path,P", po::value<std::vector<std::string>>()->composing(), "Additional python paths")
        ("disable-addon", po::value<std::vector<std::string>>()->composing(), "Disable a given addon.")
        ("single-instance", "Allow to run a single instance of the application")
        ("safe-mode", "Force enable safe mode")
        ("pass", po::value<std::vector<std::string>>()->multitoken(), "Ignores the following arguments and pass them through to be used by a script");
    // clang-format on
}

void addHiddenOptions(po::options_description& hidden)
{
    // Hidden options are accepted on the command line and in the config file.
    // Qt's window-system options must also be accepted here so Boost does not reject them
    // before QApplication receives the original arguments.
    // clang-format off
    hidden.add_options()
        ("input-file", po::value<std::vector<std::string>>(), "input file")
        ("output", po::value<std::string>(), "output file")
        ("hidden", "don't show the main window")
        ("style", po::value<std::string>(), "set the application GUI style")
        ("stylesheet", po::value<std::string>(), "set the application stylesheet")
        ("session", po::value<std::string>(), "restore the application from an earlier session")
        ("reverse", "set the application's layout direction from right to left")
        ("widgetcount", "print debug messages about widgets")
        ("graphicssystem", po::value<std::string>(), "backend to be used for on-screen widgets and pixmaps")
        ("display", po::value<std::string>(), "set the X-Server")
        ("geometry ", po::value<std::string>(), "set the X-Window geometry")
        ("font", po::value<std::string>(), "set the X-Window font")
        ("fn", po::value<std::string>(), "set the X-Window font")
        ("background", po::value<std::string>(), "set the X-Window background color")
        ("bg", po::value<std::string>(), "set the X-Window background color")
        ("foreground", po::value<std::string>(), "set the X-Window foreground color")
        ("fg", po::value<std::string>(), "set the X-Window foreground color")
        ("button", po::value<std::string>(), "set the X-Window button color")
        ("btn", po::value<std::string>(), "set the X-Window button color")
        ("name", po::value<std::string>(), "set the X-Window name")
        ("title", po::value<std::string>(), "set the X-Window title")
        ("visual", po::value<std::string>(), "set the X-Window to color scheme")
        ("ncols", po::value<int>(), "set the X-Window to color scheme")
        ("cmap", "set the X-Window to color scheme")
#if defined(FC_OS_MACOSX)
        ("psn", po::value<std::string>(), "process serial number")
#endif
        ;
    // clang-format on
}

void loadConfigFile(
    const CommandLineParseOptions& parseOptions,
    const po::options_description& configFileOptions,
    po::variables_map& values
)
{
    if (!parseOptions.configFile) {
        return;
    }

    std::ifstream configStream(*parseOptions.configFile);
    if (configStream) {
        // Boost variables_map keeps the command-line value for scalar options and composes the
        // explicitly composing vector options. Loading the file second preserves that precedence.
        store(parse_config_file(configStream, configFileOptions), values);
    }
}

void selectStartupRequest(StartupOptions& options, const po::variables_map& values)
{
    if (values.contains("version") && !values.contains("verbose")) {
        options.request = StartupRequest::Version;
    }
    else if (values.contains("dump-config")) {
        options.request = StartupRequest::DumpConfig;
    }
    else if (options.getConfig) {
        options.request = StartupRequest::GetConfig;
    }
    else if (values.contains("version") && values.contains("verbose")) {
        options.request = StartupRequest::VerboseVersion;
    }
}

}  // namespace

StartupOptions parseCommandLine(
    const ProcessArguments& processArguments,
    const std::string& executableName,
    const CommandLineParseOptions& parseOptions
)
{
    po::options_description generic("Generic options");
    addGenericOptions(generic);
    po::options_description config("Configuration");
    addConfigurationOptions(config, executableName);

    po::options_description hidden("Hidden options");
    addHiddenOptions(hidden);

    // 0000723: improper handling of Qt-specific command-line arguments.
    // Keep the value attached to these options when passing them through Boost's parser.
    std::vector<std::string> arguments;
    bool mergeNext = false;
    const int argc = processArguments.count();
    for (int i = 1; i < argc; ++i) {
        if (mergeNext) {
            mergeNext = false;
            arguments.back() += "=";
            arguments.back() += processArguments[i];
        }
        else {
            arguments.emplace_back(processArguments[i]);
        }
        mergeNext = processArguments[i] == "-style" || processArguments[i] == "-stylesheet"
            || processArguments[i] == "-session" || processArguments[i] == "-graphicssystem";
    }

    // 0000659: Boost.Program_options 1.49 could abort with an empty description title.
    po::options_description commandLineOptions("Command-line options");
    commandLineOptions.add(generic).add(config).add(hidden);
    po::options_description configFileOptions("Config");
    configFileOptions.add(config).add(hidden);
    po::options_description visible("Allowed options");
    visible.add(generic).add(config);
    po::positional_options_description positional;
    positional.add("input-file", -1);

    po::variables_map values;
    try {
        store(
            po::command_line_parser(arguments)
                .options(commandLineOptions)
                .positional(positional)
                .extra_parser(Util::customSyntax)
                .run(),
            values
        );

        loadConfigFile(parseOptions, configFileOptions, values);
        notify(values);
    }
    catch (const std::exception& error) {
        THROWM(Base::UnknownProgramOption, parseErrorMessage(error, visible));
    }
    catch (...) {
        THROWM(Base::UnknownProgramOption, unknownParseErrorMessage(visible));
    }

    StartupOptions options = makeStartupOptions(values);
    if (values.contains("help")) {
        options.request = StartupRequest::Help;
        return options;
    }

    const auto responseFile = optionValue<std::string>(values, "response-file");
    if (responseFile) {
        std::ifstream responseStream(*responseFile);
        if (!responseStream) {
            std::stringstream message;
            message << "Could no open the response file: '" << *responseFile << "'\n";
            THROWM(Base::UnknownProgramOption, message.str());
        }

        std::stringstream contents;
        contents << responseStream.rdbuf();
        const std::string responseContents = contents.str();
        boost::char_separator<char> separator(" \n\r");
        boost::tokenizer<boost::char_separator<char>> tokens(responseContents, separator);
        std::vector<std::string> responseArguments;
        std::copy(tokens.begin(), tokens.end(), std::back_inserter(responseArguments));
        try {
            store(
                po::command_line_parser(responseArguments)
                    .options(commandLineOptions)
                    .positional(positional)
                    .extra_parser(Util::customSyntax)
                    .run(),
                values
            );
        }
        catch (const std::exception& error) {
            THROWM(Base::UnknownProgramOption, parseErrorMessage(error, visible));
        }
        catch (...) {
            THROWM(Base::UnknownProgramOption, unknownParseErrorMessage(visible));
        }
        options = makeStartupOptions(values);
    }

    selectStartupRequest(options, values);

    return options;
}

StartupOptions loadStartupConfiguration(
    const std::string& executableName,
    const CommandLineParseOptions& parseOptions
)
{
    po::options_description generic("Generic options");
    addGenericOptions(generic);
    po::options_description config("Configuration");
    addConfigurationOptions(config, executableName);
    po::options_description hidden("Hidden options");
    addHiddenOptions(hidden);

    po::options_description configFileOptions("Config");
    configFileOptions.add(config).add(hidden);
    po::options_description visible("Allowed options");
    visible.add(generic).add(config);

    po::variables_map values;
    try {
        loadConfigFile(parseOptions, configFileOptions, values);
        notify(values);
    }
    catch (const std::exception& error) {
        THROWM(Base::UnknownProgramOption, parseErrorMessage(error, visible));
    }
    catch (...) {
        THROWM(Base::UnknownProgramOption, unknownParseErrorMessage(visible));
    }

    return makeStartupOptions(values);
}

std::string formatCommandLineHelp(const std::string& executableName)
{
    po::options_description generic("Generic options");
    addGenericOptions(generic);
    po::options_description config("Configuration");
    addConfigurationOptions(config, executableName);
    po::options_description visible("Allowed options");
    visible.add(generic).add(config);
    return helpMessage(executableName, visible);
}

}  // namespace App

#if defined(_MSC_VER) && BOOST_VERSION < 108200
// Fix a linker error in Boost.Program_options versions before 1.82 on MSVC.
namespace boost::program_options
{
std::string arg = "arg";
const unsigned options_description::m_default_line_length = 80;
}  // namespace boost::program_options
#endif
