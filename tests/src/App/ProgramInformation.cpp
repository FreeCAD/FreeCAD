// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.
/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <sstream>

#include <App/ProgramInformation.h>

#include "InitApplication.h"
#include <src/TempDirectory.h>

namespace fs = std::filesystem;

namespace
{

class ProgramInformationTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        tests::initApplication();
    }

    tests::TempDirectory tempDir {"program_information_test"};
};

}  // namespace

TEST_F(ProgramInformationTest, addOnPathWithTrailingSeparatorUsesDirectoryName)
{
    const fs::path addonDir = tempDir.path() / "TrailingSeparatorAddon";
    ASSERT_TRUE(fs::create_directory(addonDir));

    auto addonPath = addonDir.string();
    addonPath += fs::path::preferred_separator;

    std::stringstream output;
    App::ProgramInformation::getVerboseAddOnsInfo(output, {{"AdditionalModulePaths", addonPath}});

    EXPECT_THAT(output.str(), testing::HasSubstr("Installed mods:\n"));
    EXPECT_THAT(output.str(), testing::HasSubstr("  * TrailingSeparatorAddon\n"));
}
