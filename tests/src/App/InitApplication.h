#ifndef TEST_APPLICATION_H
#define TEST_APPLICATION_H

#include <App/Application.h>

namespace tests
{

static void initApplication()
{
    if (App::Application::GetARGC() == 0) {
        constexpr int argc = 1;
        std::array<const char*, argc> argv {"FreeCAD"};
        App::Application::Config()["ExeName"] = "FreeCAD";
        App::Application::init(argc, const_cast<char**>(argv.data()));  // NOLINT
    }
}

}  // namespace tests

#endif  // TEST_APPLICATION_H
