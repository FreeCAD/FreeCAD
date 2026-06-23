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
#include <random>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include <App/ProgramInformation.h>

#include "InitApplication.h"

namespace fs = std::filesystem;

namespace
{

fs::path makeUniqueTempDir()
{
    constexpr int maxTries = 128;
    const fs::path base = fs::temp_directory_path();
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<unsigned long long> dist;

    for (int i = 0; i < maxTries; ++i) {
        auto name = "program_information_test-" + std::to_string(dist(gen));
        fs::path candidate = base / name;
        std::error_code error;
        if (fs::create_directory(candidate, error)) {
            return candidate;
        }
    }

    throw std::runtime_error("Failed to create unique temp directory");
}

class ProgramInformationTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        tests::initApplication();
        tempDir = makeUniqueTempDir();
    }

    void TearDown() override
    {
        std::error_code error;
        fs::remove_all(tempDir, error);
    }

    fs::path tempDir;
};

}  // namespace

TEST_F(ProgramInformationTest, addOnPathWithTrailingSeparatorUsesDirectoryName)
{
    const fs::path addonDir = tempDir / "TrailingSeparatorAddon";
    ASSERT_TRUE(fs::create_directory(addonDir));

    auto addonPath = addonDir.string();
    addonPath += fs::path::preferred_separator;

    std::stringstream output;
    App::ProgramInformation::getVerboseAddOnsInfo(output, {{"AdditionalModulePaths", addonPath}});

    EXPECT_THAT(output.str(), testing::HasSubstr("Installed mods:\n"));
    EXPECT_THAT(output.str(), testing::HasSubstr("  * TrailingSeparatorAddon\n"));
}
