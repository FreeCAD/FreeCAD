#ifndef TEST_APPLICATION_H
#define TEST_APPLICATION_H

#include <FCConfig.h>
#include <App/Application.h>
#include <Base/FileInfo.h>

namespace tests
{

static void initApplication()
{
    if (App::Application::GetARGC() == 0) {
#if defined(FC_OS_LINUX) || defined(FC_OS_BSD)
        std::string tmp = Base::FileInfo::getTempPath();
        tmp += "/FreeCAD";
        Base::FileInfo fi(tmp);
        fi.createDirectory();
        std::string env = "FREECAD_USER_HOME=";
        env += fi.filePath();
        putenv(env.data());
#endif

        constexpr int argc = 1;
        std::array<const char*, argc> argv {"FreeCAD"};
        App::Application::Config()["ExeName"] = "FreeCAD";
        App::Application::init(argc, const_cast<char**>(argv.data()));  // NOLINT
    }
}

}  // namespace tests

#endif  // TEST_APPLICATION_H
