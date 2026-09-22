// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/Application.h>
#include <App/CommandLine.h>
#include <App/ProcessArguments.h>

namespace tests
{

static void initApplication()
{
    if (!App::Application::isInitialized()) {
        App::Application::Config()["ExeName"] = "FreeCAD";
        App::ProcessArguments arguments(std::vector<std::string> {"FreeCAD"});
        const auto options = App::loadStartupConfiguration(App::Application::Config()["ExeName"]);
        App::Application::init(options, arguments);
    }
}

}  // namespace tests
