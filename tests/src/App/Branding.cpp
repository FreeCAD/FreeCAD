// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <fstream>

#include "App/Branding.h"
#include "Base/PathUtils.h"
#include <src/TempDirectory.h>

TEST(Branding, readsUnicodePath)
{
    tests::TempDirectory tempDir {"branding"};
    const auto path = Base::pathFromUtf8("FreeCAD-branding-\xE2\x98\x83.xml");
    const auto filePath = tempDir.path() / path;

    {
        std::ofstream file(filePath);
        ASSERT_TRUE(file);
        file << "<?xml version=\"1.0\"?><Branding><Application>Test</Application></Branding>";
    }

    App::Branding branding;
    const bool read = branding.readFile(filePath);

    ASSERT_TRUE(read);
    const auto application = branding.getUserDefines().find("Application");
    ASSERT_NE(application, branding.getUserDefines().end());
    EXPECT_EQ(application->second, "Test");
}
