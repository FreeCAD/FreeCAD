// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <Mod/Material/App/PreCompiled.h>
#ifndef _PreComp_
#endif

#include <QString>

#include <App/Application.h>

#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>

// clang-format off

TEST(Material, TestDummy)
{
    FAIL() << "Checking that this even works\n";
}

TEST(Material, TestApplication)
{
    App::Application& application = App::GetApplication();
    if (&application == nullptr)
        ADD_FAILURE() << "Application failure\n";

    if (App::Application::GetARGC() == 0) {
        constexpr int argc = 1;
        std::array<char*, argc> argv {"FreeCAD"};
        App::Application::Config()["ExeName"] = "FreeCAD";
        App::Application::init(argc, argv.data());
    }
    App::Application& application2 = App::GetApplication();
    if (&application2 == nullptr)
        ADD_FAILURE() << "Application 2 failure\n";

    SUCCEED();
}

// TEST(Material, TestResources)
// {
//     try {
//         auto param = App::GetApplication().GetParameterGroupByPath(
//             "User parameter:BaseApp/Preferences/Mod/Material/Resources");
//         EXPECT_NE(param, nullptr);
//     }
//     catch (const std::exception &e)
//     {
//         FAIL() << "Exception: " << e.what() << "\n";
//     }
// }

// TEST(Material, TestModelLoad)
// {
//     auto manager = Materials::ModelManager::getManager();
//     EXPECT_NE(manager, nullptr);
//     auto density = manager->getModel(QString::fromStdString("454661e5-265b-4320-8e6f-fcf6223ac3af"));
//     // EXPECT_NE(density, nullptr);
//     EXPECT_NE(density.getName(), QString::fromStdString("density"));
//     EXPECT_EQ(density.getUUID(), QString::fromStdString("454661e5-265b-4320-8e6f-fcf6223ac3af"));
// }

// clang-format on
