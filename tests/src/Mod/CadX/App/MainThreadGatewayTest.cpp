// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Mod/CadX/App/MainThreadGateway.h>

#include <QCoreApplication>

#include <memory>
#include <thread>

TEST(CadXMainThreadGateway, BindsToQtApplicationThreadWhenConstructedElsewhere)
{
    int argc = 1;
    char applicationName[] = "CadXMainThreadGatewayTest";
    char* argv[] = {applicationName, nullptr};
    QCoreApplication application(argc, argv);

    std::unique_ptr<CadX::MainThreadGateway> gateway;
    std::thread creator([&gateway] { gateway = std::make_unique<CadX::MainThreadGateway>(); });
    creator.join();

    ASSERT_NE(gateway, nullptr);
    EXPECT_TRUE(gateway->isMainThread());

    bool ranOnApplicationThread = false;
    gateway->run([&ranOnApplicationThread, &gateway] {
        ranOnApplicationThread = gateway->isMainThread();
    });
    EXPECT_TRUE(ranOnApplicationThread);
}
