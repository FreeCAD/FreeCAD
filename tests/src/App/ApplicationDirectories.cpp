// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

#include <gtest/gtest.h>
#include <App/ApplicationDirectories.h>

#include <random>
#include <fstream>
#include "Base/Exception.h"

/* NOLINTBEGIN(
    readability-magic-numbers,
    cppcoreguidelines-avoid-magic-numbers,
    readability-function-cognitive-complexity
) */

namespace fs = std::filesystem;

static fs::path MakeUniqueTempDir()
{
    constexpr int maxTries = 128;
    const fs::path base = fs::temp_directory_path();
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<unsigned long long> dist;

    for (int i = 0; i < maxTries; ++i) {
        auto name = "app_directories_test-" + std::to_string(dist(gen));
        fs::path candidate = base / name;
        std::error_code ec;
        if (fs::create_directory(candidate, ec)) {
            return candidate;
        }
        if (ec && ec != std::make_error_code(std::errc::file_exists)) {
            continue;
        }
    }
    throw std::runtime_error("Failed to create unique temp directory");
}

/// A subclass to expose protected members for unit testing
class ApplicationDirectoriesTestClass: public App::ApplicationDirectories
{
    using App::ApplicationDirectories::ApplicationDirectories;

public:
    void wrapAppendVersionIfPossible(const fs::path& basePath, std::vector<std::string>& subdirs) const
    {
        appendVersionIfPossible(basePath, subdirs);
    }

    std::tuple<int, int> wrapExtractVersionFromConfigMap(
        const std::map<std::string, std::string>& config
    )
    {
        return extractVersionFromConfigMap(config);
    }

    static std::filesystem::path wrapSanitizePath(const std::string& pathAsString)
    {
        return sanitizePath(pathAsString);
    }
};

class ApplicationDirectoriesTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        _tempDir = MakeUniqueTempDir();
    }

    std::map<std::string, std::string> generateConfig(
        const std::map<std::string, std::string>& overrides
    ) const
    {
        std::map<std::string, std::string> config {
            {"AppHomePath", _tempDir.string()},
            {"ExeVendor", "Vendor"},
            {"ExeName", "Test"},
            {"BuildVersionMajor", "4"},
            {"BuildVersionMinor", "2"}
        };
        for (const auto& override : overrides) {
            config[override.first] = override.second;
        }
        return config;
    }

    std::unique_ptr<ApplicationDirectoriesTestClass> makeAppDirsForVersion(int major, int minor)
    {
        auto configuration = generateConfig(
            {{"BuildVersionMajor", std::to_string(major)},
             {"BuildVersionMinor", std::to_string(minor)}}
        );
        return std::make_unique<ApplicationDirectoriesTestClass>(configuration);
    }

    fs::path makePathForVersion(const fs::path& base, int major, int minor)
    {
        return base / App::ApplicationDirectories::versionStringForPath(major, minor);
    }

    void TearDown() override
    {
        fs::remove_all(_tempDir);
    }

    fs::path tempDir()
    {
        return _tempDir;
    }

private:
    fs::path _tempDir;
};

namespace fs = std::filesystem;


TEST_F(ApplicationDirectoriesTest, usingCurrentVersionConfigTrueWhenDirMatchesVersion)
{
    // Arrange
    constexpr int major = 3;
    constexpr int minor = 7;
    const fs::path testPath = fs::path("some_kind_of_config")
        / App::ApplicationDirectories::versionStringForPath(major, minor);

    // Act: generate a directory structure with the same version
    auto appDirs = makeAppDirsForVersion(major, minor);

    // Assert
    EXPECT_TRUE(appDirs->usingCurrentVersionConfig(testPath));
}

TEST_F(ApplicationDirectoriesTest, usingCurrentVersionConfigFalseWhenDirDoesntMatchVersion)
{
    // Arrange
    constexpr int major = 3;
    constexpr int minor = 7;
    const fs::path testPath = fs::path("some_kind_of_config")
        / App::ApplicationDirectories::versionStringForPath(major, minor);

    // Act: generate a directory structure with the same version
    auto configuration = generateConfig(
        {{"BuildVersionMajor", std::to_string(major + 1)},
         {"BuildVersionMinor", std::to_string(minor)}}
    );
    auto appDirs = std::make_unique<App::ApplicationDirectories>(configuration);

    // Assert
    EXPECT_FALSE(appDirs->usingCurrentVersionConfig(testPath));
}

// Exact current version (hits: major==currentMajor path; inner if true)
TEST_F(ApplicationDirectoriesTest, isVersionedPathMatchesCurrentMajorAndMinor)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::path p = makePathForVersion(tempDir(), 5, 4);
    EXPECT_TRUE(appDirs->isVersionedPath(p));
}

// Lower minor within the same major (major==currentMajor path; iterates down to a smaller minor)
TEST_F(ApplicationDirectoriesTest, isVersionedPathMatchesLowerMinorSameMajor)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::path p = makePathForVersion(tempDir(), 5, 2);
    EXPECT_TRUE(appDirs->isVersionedPath(p));
}

// Lower major (major!=currentMajor path)
TEST_F(ApplicationDirectoriesTest, isVersionedPathMatchesLowerMajorAnyMinor)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::path p = makePathForVersion(tempDir(), 4, 7);
    EXPECT_TRUE(appDirs->isVersionedPath(p));
}

// Boundary: minor==0 within the same major
TEST_F(ApplicationDirectoriesTest, isVersionedPathMatchesZeroMinorSameMajor)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::path p = makePathForVersion(tempDir(), 5, 0);
    EXPECT_TRUE(appDirs->isVersionedPath(p));
}

// Negative: higher minor than current for the same major is never iterated
TEST_F(ApplicationDirectoriesTest, isVersionedPathDoesNotMatchHigherMinorSameMajor)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::path p = makePathForVersion(tempDir(), 5, 5);
    EXPECT_FALSE(appDirs->isVersionedPath(p));
}

// Negative: higher major than current is never iterated; also covers "non-version" style
TEST_F(ApplicationDirectoriesTest, isVersionedPathDoesNotMatchHigherMajorOrRandomName)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    fs::path higherMajor = makePathForVersion(tempDir(), 6, 1);
    EXPECT_FALSE(appDirs->isVersionedPath(higherMajor));

    fs::path randomName = tempDir() / "not-a-version";
    EXPECT_FALSE(appDirs->isVersionedPath(randomName));
}

// Convenience: path under base for a version folder name.
fs::path versionedPath(const fs::path& base, int major, int minor)
{
    return base / App::ApplicationDirectories::versionStringForPath(major, minor);
}

// Create a regular file (used to prove non-directories are ignored).
void touchFile(const fs::path& p)
{
    fs::create_directories(p.parent_path());
    std::ofstream ofs(p.string());
    ofs << "x";
}

// The exact current version exists -> returned immediately (current-major branch).
TEST_F(ApplicationDirectoriesTest, mostRecentAvailReturnsExactCurrentVersionIfDirectoryExists)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::create_directories(versionedPath(tempDir(), 5, 4));

    EXPECT_EQ(
        appDirs->mostRecentAvailableConfigVersion(tempDir()),
        App::ApplicationDirectories::versionStringForPath(5, 4)
    );
}

// No exact match in the current major: choose the highest available minor <= current
// and prefer the current major over lower majors.
TEST_F(ApplicationDirectoriesTest, mostRecentAvailPrefersSameMajorAndPicksHighestLowerMinor)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::create_directories(versionedPath(tempDir(), 5, 1));
    fs::create_directories(versionedPath(tempDir(), 5, 3));
    fs::create_directories(versionedPath(tempDir(), 4, 99));  // distractor in lower major

    EXPECT_EQ(
        appDirs->mostRecentAvailableConfigVersion(tempDir()),
        App::ApplicationDirectories::versionStringForPath(5, 3)
    );
}

// No directories in current major: scan next lower major from 99 downward,
// returning the highest available minor present (demonstrates descending search).
TEST_F(ApplicationDirectoriesTest, mostRecentAvailForLowerMajorPicksHighestAvailableMinor)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::create_directories(versionedPath(tempDir(), 4, 3));
    fs::create_directories(versionedPath(tempDir(), 4, 42));

    EXPECT_EQ(
        appDirs->mostRecentAvailableConfigVersion(tempDir()),
        App::ApplicationDirectories::versionStringForPath(4, 42)
    );
}

// If the candidate path exists but is a regular file, it must be ignored and
// the search must fall back to the next available directory.
TEST_F(ApplicationDirectoriesTest, mostRecentAvailSkipsFilesAndFallsBackToNextDirectory)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    touchFile(versionedPath(tempDir(), 5, 4));               // file at the current version
    fs::create_directories(versionedPath(tempDir(), 5, 3));  // directory at next lower minor

    EXPECT_EQ(
        appDirs->mostRecentAvailableConfigVersion(tempDir()),
        App::ApplicationDirectories::versionStringForPath(5, 3)
    );
}

// Higher minor in the current major is not considered (loop starts at the current minor);
// should fall through to the lower major when nothing <= the current minor exists.
TEST_F(ApplicationDirectoriesTest, mostRecentAvailIgnoresHigherMinorThanCurrentInSameMajor)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::create_directories(versionedPath(tempDir(), 5, 7));  // higher than the current minor; ignored
    fs::create_directories(versionedPath(tempDir(), 4, 1));  // fallback target

    EXPECT_EQ(
        appDirs->mostRecentAvailableConfigVersion(tempDir()),
        App::ApplicationDirectories::versionStringForPath(4, 1)
    );
}

// No candidates anywhere -> empty string returned.
TEST_F(ApplicationDirectoriesTest, mostRecentAvailReturnsEmptyStringWhenNoVersionsPresent)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    EXPECT_EQ(appDirs->mostRecentAvailableConfigVersion(tempDir()), "");
}


// The current version directory exists -> returned immediately
TEST_F(ApplicationDirectoriesTest, mostRecentConfigReturnsCurrentVersionDirectoryIfPresent)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::create_directories(versionedPath(tempDir(), 5, 4));

    fs::path got = appDirs->mostRecentConfigFromBase(tempDir());
    EXPECT_EQ(got, versionedPath(tempDir(), 5, 4));
}

// The current version missing -> falls back to most recent available in same major
TEST_F(ApplicationDirectoriesTest, mostRecentConfigFallsBackToMostRecentInSameMajorWhenCurrentMissing)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    // There is no directory called "5.4"; provide candidates 5.3 and 5.1; also a distractor in a
    // lower major.
    fs::create_directories(versionedPath(tempDir(), 5, 1));
    fs::create_directories(versionedPath(tempDir(), 5, 3));
    fs::create_directories(versionedPath(tempDir(), 4, 99));

    fs::path got = appDirs->mostRecentConfigFromBase(tempDir());
    EXPECT_EQ(got, versionedPath(tempDir(), 5, 3));
}

// The current version path exists as a file (not directory) -> ignored, fallback used
TEST_F(ApplicationDirectoriesTest, mostRecentConfigSkipsFileAtCurrentVersionAndFallsBack)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    touchFile(versionedPath(tempDir(), 5, 4));  // file, not dir
    fs::create_directories(versionedPath(tempDir(), 5, 2));

    fs::path got = appDirs->mostRecentConfigFromBase(tempDir());
    EXPECT_EQ(got, versionedPath(tempDir(), 5, 2));
}

// There are no eligible versions in the current major -> choose the highest available in lower
// majors
TEST_F(ApplicationDirectoriesTest, mostRecentConfigFallsBackToHighestVersionInLowerMajors)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    // No 5.x minor <= 4 exists. (Optionally add a higher minor to prove it's ignored.)
    fs::create_directories(versionedPath(tempDir(), 5, 7));  // ignored (higher than current minor)

    // Provide multiple lower-major candidates; should pick 4.90 over 3.99.
    fs::create_directories(versionedPath(tempDir(), 4, 3));
    fs::create_directories(versionedPath(tempDir(), 4, 90));
    fs::create_directories(versionedPath(tempDir(), 3, 99));

    fs::path got = appDirs->mostRecentConfigFromBase(tempDir());
    EXPECT_EQ(got, versionedPath(tempDir(), 4, 90));
}

// There is nothing available anywhere -> returns startingPath
TEST_F(ApplicationDirectoriesTest, mostRecentConfigReturnsStartingPathWhenNoVersionedConfigExists)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::path got = appDirs->mostRecentConfigFromBase(tempDir());
    EXPECT_EQ(got, tempDir());
}

// True: exact current version directory
TEST_F(ApplicationDirectoriesTest, usingCurrentVersionExactVersionDir)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::create_directories(versionedPath(tempDir(), 5, 4));

    EXPECT_TRUE(appDirs->usingCurrentVersionConfig(versionedPath(tempDir(), 5, 4)));
}

// True: current version directory with trailing separator (exercises filename().empty() branch)
TEST_F(ApplicationDirectoriesTest, usingCurrentVersionVersionDirWithTrailingSlash)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::create_directories(versionedPath(tempDir(), 5, 4));
    fs::path p = versionedPath(tempDir(), 5, 4) / "";  // ensure trailing separator

    EXPECT_TRUE(appDirs->usingCurrentVersionConfig(p));
}

// False: a file inside the current version directory (filename != version string)
TEST_F(ApplicationDirectoriesTest, usingCurrentVersionFileInsideVersionDirIsFalse)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::path filePath = versionedPath(tempDir(), 5, 4) / "config.yaml";
    touchFile(filePath);

    EXPECT_FALSE(appDirs->usingCurrentVersionConfig(filePath));
}

// False: lower version directory
TEST_F(ApplicationDirectoriesTest, usingCurrentVersionLowerVersionDirIsFalse)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::create_directories(versionedPath(tempDir(), 5, 3));

    EXPECT_FALSE(appDirs->usingCurrentVersionConfig(versionedPath(tempDir(), 5, 3)));
}

// False: higher version directory
TEST_F(ApplicationDirectoriesTest, usingCurrentVersionHigherVersionDirIsFalse)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::create_directories(versionedPath(tempDir(), 6, 0));

    EXPECT_FALSE(appDirs->usingCurrentVersionConfig(versionedPath(tempDir(), 6, 0)));
}

// False: non-version directory (e.g., base dir)
TEST_F(ApplicationDirectoriesTest, usingCurrentVersionNonVersionDirIsFalse)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    fs::create_directories(tempDir());

    EXPECT_FALSE(appDirs->usingCurrentVersionConfig(tempDir()));
}

void writeFile(const fs::path& p, std::string_view contents)
{
    fs::create_directories(p.parent_path());
    std::ofstream ofs(p.string(), std::ios::binary);
    ofs << contents;
}

std::string readFile(const fs::path& p)
{
    std::ifstream ifs(p.string(), std::ios::binary);
    return {std::istreambuf_iterator<char>(ifs), {}};
}

// Creates destination and copies flat files over
TEST_F(ApplicationDirectoriesTest, migrateConfigCreatesDestinationAndCopiesFiles)
{
    // Arrange
    fs::path oldPath = tempDir() / "old";
    fs::path newPath = tempDir() / "new";
    writeFile(oldPath / "a.txt", "alpha");
    writeFile(oldPath / "b.ini", "bravo");

    // Act
    App::ApplicationDirectories::migrateConfig(oldPath, newPath);

    // Assert
    ASSERT_TRUE(fs::exists(newPath));
    ASSERT_TRUE(fs::is_directory(newPath));

    EXPECT_TRUE(fs::exists(newPath / "a.txt"));
    EXPECT_TRUE(fs::exists(newPath / "b.ini"));
    EXPECT_EQ(readFile(newPath / "a.txt"), "alpha");
    EXPECT_EQ(readFile(newPath / "b.ini"), "bravo");

    EXPECT_TRUE(fs::exists(oldPath / "a.txt"));
    EXPECT_TRUE(fs::exists(oldPath / "b.ini"));
}

// newPath is a subdirectory of oldPath -> skip copying newPath into itself
TEST_F(ApplicationDirectoriesTest, migrateConfigSkipsSelfWhenNewIsSubdirectoryOfOld)
{
    // Arrange
    fs::path oldPath = tempDir() / "container";
    fs::path newPath = oldPath / "migrated";
    writeFile(oldPath / "c.yaml", "charlie");
    writeFile(oldPath / "d.cfg", "delta");

    // Act
    App::ApplicationDirectories::migrateConfig(oldPath, newPath);

    // Assert
    ASSERT_TRUE(fs::exists(newPath));
    ASSERT_TRUE(fs::is_directory(newPath));
    EXPECT_TRUE(fs::exists(newPath / "c.yaml"));
    EXPECT_TRUE(fs::exists(newPath / "d.cfg"));

    // Do not copy the destination back into itself (no nested 'migrated/migrated')
    EXPECT_FALSE(fs::exists(newPath / newPath.filename()));
}

// oldPath empty -> still creates the destination and does nothing else
TEST_F(ApplicationDirectoriesTest, migrateConfigEmptyOldPathJustCreatesDestination)
{
    fs::path oldPath = tempDir() / "empty_old";
    fs::path newPath = tempDir() / "dest_only";

    fs::create_directories(oldPath);
    App::ApplicationDirectories::migrateConfig(oldPath, newPath);

    ASSERT_TRUE(fs::exists(newPath));
    ASSERT_TRUE(fs::is_directory(newPath));

    // No files expected
    bool hasEntries = (fs::directory_iterator(newPath) != fs::directory_iterator {});
    EXPECT_FALSE(hasEntries);
}

// Case: recursively copy nested directories and files
TEST_F(ApplicationDirectoriesTest, migrateConfigRecursivelyCopiesDirectoriesAndFiles)
{
    fs::path oldPath = tempDir() / "old_tree";
    fs::path newPath = tempDir() / "new_tree";

    // old_tree/
    //   config/
    //     env/
    //       a.txt
    //   empty_dir/
    writeFile(oldPath / "config" / "env" / "a.txt", "alpha");
    fs::create_directories(oldPath / "empty_dir");

    App::ApplicationDirectories::migrateConfig(oldPath, newPath);

    // Expect structure replicated under newPath
    ASSERT_TRUE(fs::exists(newPath));
    EXPECT_TRUE(fs::exists(newPath / "config"));
    EXPECT_TRUE(fs::exists(newPath / "config" / "env"));
    EXPECT_TRUE(fs::exists(newPath / "config" / "env" / "a.txt"));
    EXPECT_EQ(readFile(newPath / "config" / "env" / "a.txt"), "alpha");

    // Empty directories should be created as well
    EXPECT_TRUE(fs::exists(newPath / "empty_dir"));
    EXPECT_TRUE(fs::is_directory(newPath / "empty_dir"));
}

// Case: newPath is subdir of oldPath -> recursively copy, but do NOT copy newPath into itself
TEST_F(ApplicationDirectoriesTest, migrateConfigNewPathSubdirRecursivelyCopiesAndSkipsSelf)
{
    fs::path oldPath = tempDir() / "src_tree";
    fs::path newPath = oldPath / "migrated";  // destination under source

    // src_tree/
    //   folderA/
    //     child/
    //       f.txt
    writeFile(oldPath / "folderA" / "child" / "f.txt", "filedata");

    App::ApplicationDirectories::migrateConfig(oldPath, newPath);

    // Copied recursively into destination
    EXPECT_TRUE(fs::exists(newPath / "folderA" / "child" / "f.txt"));
    EXPECT_EQ(readFile(newPath / "folderA" / "child" / "f.txt"), "filedata");

    // Do not copy the destination back into itself (no migrated/migrated)
    EXPECT_FALSE(fs::exists(newPath / newPath.filename()));
}

// Versioned input: `path` == base/<some older version>` -> newPath == base/<current>
TEST_F(ApplicationDirectoriesTest, migrateAllPathsVersionedInputChoosesParentPlusCurrent)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    fs::path base = tempDir() / "v_branch";
    fs::path older = versionedPath(base, 5, 1);  // versioned input (isVersionedPath == true)
    fs::create_directories(older);
    writeFile(older / "sentinel.txt", "s");

    std::vector<fs::path> inputs {older};
    appDirs->migrateAllPaths(inputs);

    fs::path expectedDest = versionedPath(base, 5, 4);
    EXPECT_TRUE(fs::exists(expectedDest));
    EXPECT_TRUE(fs::is_directory(expectedDest));
}

// Non-versioned input: `path` == base -> newPath == base/<current>
TEST_F(ApplicationDirectoriesTest, migrateAllPathsNonVersionedInputAppendsCurrent)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    fs::path base = tempDir() / "plain_base";
    fs::create_directories(base);
    writeFile(base / "config.yaml", "x");

    std::vector<fs::path> inputs {base};
    appDirs->migrateAllPaths(inputs);

    fs::path expectedDest = versionedPath(base, 5, 4);
    EXPECT_TRUE(fs::exists(expectedDest));
    EXPECT_TRUE(fs::is_directory(expectedDest));
}

// Pre-existing destination -> throws Base::RuntimeError
TEST_F(ApplicationDirectoriesTest, migrateAllPathsIgnoresIfDestinationAlreadyExists_NonVersioned)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    fs::path base = tempDir() / "exists_case";
    fs::create_directories(base);
    fs::path dest = versionedPath(base, 5, 4);
    fs::create_directories(dest);  // destination already exists

    std::vector<fs::path> inputs {base};

    ASSERT_NO_THROW(appDirs->migrateAllPaths(inputs));
}

// Multiple inputs: one versioned, one non-versioned -> both destinations created
TEST_F(ApplicationDirectoriesTest, migrateAllPathsProcessesMultipleInputs)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    // Versioned input A: baseA/5.2
    fs::path baseA = tempDir() / "multiA";
    fs::path olderA = versionedPath(baseA, 5, 2);
    fs::create_directories(olderA);
    writeFile(olderA / "a.txt", "a");

    // Non-versioned input B: baseB
    fs::path baseB = tempDir() / "multiB";
    fs::create_directories(baseB);
    writeFile(baseB / "b.txt", "b");

    std::vector<fs::path> inputs {olderA, baseB};
    appDirs->migrateAllPaths(inputs);

    EXPECT_TRUE(fs::exists(versionedPath(baseA, 5, 4)));  // parent_path() / current
    EXPECT_TRUE(fs::exists(versionedPath(baseB, 5, 4)));  // base / current
}

// Already versioned (final component is a version) -> no change
TEST_F(ApplicationDirectoriesTest, appendVecAlreadyVersionedBails)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    fs::path base = tempDir() / "bail_vec";
    std::vector<std::string> sub {
        "configs",
        App::ApplicationDirectories::versionStringForPath(5, 2)
    };  // versioned tail
    fs::create_directories(base / sub[0] / sub[1]);

    auto before = sub;
    appDirs->wrapAppendVersionIfPossible(base, sub);
    EXPECT_EQ(sub, before);  // unchanged
}

// Base exists & current version dir present -> append current
TEST_F(ApplicationDirectoriesTest, appendVecBaseExistsAppendsCurrentWhenPresent)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    fs::path base = tempDir() / "vec_current";
    std::vector<std::string> sub {"configs"};
    fs::create_directories(base / "configs");
    fs::create_directories(versionedPath(base / "configs", 5, 4));

    appDirs->wrapAppendVersionIfPossible(base, sub);

    ASSERT_EQ(sub.size(), 2u);
    EXPECT_EQ(sub.back(), App::ApplicationDirectories::versionStringForPath(5, 4));
}

// Base exists, no current; lower minors exist -> append highest ≤ current in same major
TEST_F(ApplicationDirectoriesTest, appendVecPicksHighestLowerMinorInSameMajor)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    fs::path base = tempDir() / "vec_lower_minor";
    std::vector<std::string> sub {"configs"};
    fs::create_directories(versionedPath(base / "configs", 5, 1));
    fs::create_directories(versionedPath(base / "configs", 5, 3));
    // distractor in lower major
    fs::create_directories(versionedPath(base / "configs", 4, 99));

    appDirs->wrapAppendVersionIfPossible(base, sub);

    ASSERT_EQ(sub.size(), 2u);
    EXPECT_EQ(sub.back(), App::ApplicationDirectories::versionStringForPath(5, 3));
}

// Base exists, nothing in current major; lower major exists -> append highest available lower major
TEST_F(ApplicationDirectoriesTest, appendVecFallsBackToLowerMajor)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    fs::path base = tempDir() / "vec_lower_major";
    std::vector<std::string> sub {"configs"};
    fs::create_directories(versionedPath(base / "configs", 4, 90));
    fs::create_directories(versionedPath(base / "configs", 3, 99));

    appDirs->wrapAppendVersionIfPossible(base, sub);

    ASSERT_EQ(sub.size(), 2u);
    EXPECT_EQ(sub.back(), App::ApplicationDirectories::versionStringForPath(4, 90));
}

// Base exists but contains no versioned subdirs -> append nothing (vector unchanged)
TEST_F(ApplicationDirectoriesTest, appendVecNoVersionedChildrenLeavesVectorUnchanged)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    fs::path base = tempDir() / "vec_noversions";
    std::vector<std::string> sub {"configs"};
    fs::create_directories(base / "configs");  // but no versioned children

    auto before = sub;
    appDirs->wrapAppendVersionIfPossible(base, sub);
    EXPECT_EQ(sub, before);
}

// Base does not exist -> append current version string
TEST_F(ApplicationDirectoriesTest, appendVecBaseMissingAppendsCurrentSuffix)
{
    auto appDirs = makeAppDirsForVersion(5, 4);

    fs::path base = tempDir() / "vec_missing";
    std::vector<std::string> sub {"configs"};  // base/configs doesn't exist

    appDirs->wrapAppendVersionIfPossible(base, sub);

    ASSERT_EQ(sub.size(), 2u);
    EXPECT_EQ(sub.back(), App::ApplicationDirectories::versionStringForPath(5, 4));
}

// Happy path: exact integers
TEST_F(ApplicationDirectoriesTest, extractVersionSucceedsWithPlainIntegers)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    std::map<std::string, std::string> m {{"BuildVersionMajor", "7"}, {"BuildVersionMinor", "2"}};
    auto [maj, min] = appDirs->wrapExtractVersionFromConfigMap(m);
    EXPECT_EQ(maj, 7);
    EXPECT_EQ(min, 2);
}

// Whitespace tolerated by std::stoi
TEST_F(ApplicationDirectoriesTest, extractVersionSucceedsWithWhitespace)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    std::map<std::string, std::string> m {
        {"BuildVersionMajor", "  10  "},
        {"BuildVersionMinor", "\t3\n"}
    };
    auto [maj, min] = appDirs->wrapExtractVersionFromConfigMap(m);
    EXPECT_EQ(maj, 10);
    EXPECT_EQ(min, 3);
}

// Missing major key -> rethrows as Base::RuntimeError
TEST_F(ApplicationDirectoriesTest, extractVersionMissingMajorThrowsRuntimeError)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    std::map<std::string, std::string> m {{"BuildVersionMinor", "1"}};
    EXPECT_THROW(appDirs->wrapExtractVersionFromConfigMap(m), Base::RuntimeError);
}

// Missing minor key -> rethrows as Base::RuntimeError
TEST_F(ApplicationDirectoriesTest, extractVersionMissingMinorThrowsRuntimeError)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    std::map<std::string, std::string> m {{"BuildVersionMajor", "1"}};
    EXPECT_THROW(appDirs->wrapExtractVersionFromConfigMap(m), Base::RuntimeError);
}

// Non-numeric -> std::stoi throws invalid_argument, rethrown as Base::RuntimeError
TEST_F(ApplicationDirectoriesTest, extractVersionNonNumericThrowsRuntimeError)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    std::map<std::string, std::string> m {{"BuildVersionMajor", "abc"}, {"BuildVersionMinor", "2"}};
    EXPECT_THROW(appDirs->wrapExtractVersionFromConfigMap(m), Base::RuntimeError);
}

// Overflow -> std::stoi throws out_of_range, rethrown as Base::RuntimeError
TEST_F(ApplicationDirectoriesTest, extractVersionOverflowThrowsRuntimeError)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    std::map<std::string, std::string> m {
        {"BuildVersionMajor", "9999999999999999999999999"},
        {"BuildVersionMinor", "1"}
    };
    EXPECT_THROW(appDirs->wrapExtractVersionFromConfigMap(m), Base::RuntimeError);
}

// Document current behavior: negative numbers are accepted and returned as-is
TEST_F(ApplicationDirectoriesTest, extractVersionNegativeNumbersPassThrough)
{
    auto appDirs = makeAppDirsForVersion(5, 4);
    std::map<std::string, std::string> m {{"BuildVersionMajor", "-2"}, {"BuildVersionMinor", "-7"}};
    auto [maj, min] = appDirs->wrapExtractVersionFromConfigMap(m);
    EXPECT_EQ(maj, -2);
    EXPECT_EQ(min, -7);
}


TEST_F(ApplicationDirectoriesTest, sanitizeRemovesNullCharacterAtEnd)
{
    std::string input = std::string("valid_path") + '\0' + "junk_after";
    std::filesystem::path result = ApplicationDirectoriesTestClass::wrapSanitizePath(input);

    EXPECT_EQ(result.string(), "valid_path");
    EXPECT_EQ(result.string().find('\0'), std::string::npos);
}

TEST_F(ApplicationDirectoriesTest, sanitizeReturnsUnchangedIfNoNullCharacter)
{
    std::string input = "clean_path/without_nulls";
    std::filesystem::path result = ApplicationDirectoriesTestClass::wrapSanitizePath(input);

    EXPECT_EQ(result.string(), input);
    EXPECT_EQ(result.string().find('\0'), std::string::npos);
}

/* NOLINTEND(
    readability-magic-numbers,
    cppcoreguidelines-avoid-magic-numbers,
    readability-function-cognitive-complexity
) */
