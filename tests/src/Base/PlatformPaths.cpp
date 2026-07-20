// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Base/PlatformPaths.h>
#include <Base/PathUtils.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>

#include <FCConfig.h>

namespace
{

namespace fs = std::filesystem;

void setEnvironment(const char* key, const std::string& value)
{
#if defined(FC_OS_WIN32)
    ASSERT_EQ(_putenv_s(key, value.c_str()), 0);
#else
    ASSERT_EQ(setenv(key, value.c_str(), 1), 0);
#endif
}

void unsetEnvironment(const char* key)
{
#if defined(FC_OS_WIN32)
    ASSERT_EQ(_putenv_s(key, ""), 0);
#else
    ASSERT_EQ(unsetenv(key), 0);
#endif
}

class ScopedEnvironment
{
public:
    ScopedEnvironment(const char* key, std::string value)
        : _key(key)
        , _oldValue(Base::environmentVariableUtf8(key))
    {
        setEnvironment(_key, value);
    }

    ~ScopedEnvironment()
    {
        if (_oldValue) {
            setEnvironment(_key, *_oldValue);
        }
        else {
            unsetEnvironment(_key);
        }
    }

private:
    const char* _key;
    std::optional<std::string> _oldValue;
};

class ScopedCurrentPath
{
public:
    explicit ScopedCurrentPath(const fs::path& path)
        : _oldPath(fs::current_path())
    {
        fs::current_path(path);
    }

    ~ScopedCurrentPath()
    {
        std::error_code error;
        fs::current_path(_oldPath, error);
    }

private:
    fs::path _oldPath;
};

class TempDirectory
{
public:
    TempDirectory()
    {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        _path = fs::temp_directory_path() / ("FreeCAD-PlatformPaths-" + std::to_string(suffix));
        fs::create_directories(_path);
    }

    ~TempDirectory()
    {
        std::error_code error;
        fs::remove_all(_path, error);
    }

    const fs::path& path() const
    {
        return _path;
    }

private:
    fs::path _path;
};

std::string pathSeparator()
{
#if defined(FC_OS_WIN32)
    return ";";
#else
    return ":";
#endif
}

}  // namespace

TEST(PlatformPaths, readsUnicodeEnvironmentValueAsUtf8)
{
    constexpr const char* key = "FREECAD_TEST_UTF8_ENV";
    constexpr const char* expected = "FreeCAD-\xE2\x98\x83";

#if defined(FC_OS_WIN32)
    ASSERT_EQ(_wputenv_s(L"FREECAD_TEST_UTF8_ENV", L"FreeCAD-\u2603"), 0);
#else
    ASSERT_EQ(setenv(key, expected, 1), 0);
#endif

    const auto value = Base::environmentVariableUtf8(key);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, expected);

#if defined(FC_OS_WIN32)
    ASSERT_EQ(_wputenv_s(L"FREECAD_TEST_UTF8_ENV", L""), 0);
#else
    ASSERT_EQ(unsetenv(key), 0);
#endif

    EXPECT_FALSE(Base::environmentVariableUtf8(key).has_value());
}

TEST(PlatformPaths, readsLongEnvironmentValue)
{
    constexpr const char* key = "FREECAD_TEST_LONG_ENV";
    const std::string longValue(1024, 'x');
#if defined(FC_OS_WIN32)
    const std::wstring longWideValue(longValue.size(), L'x');
    ASSERT_EQ(_wputenv_s(L"FREECAD_TEST_LONG_ENV", longWideValue.c_str()), 0);
#else
    ASSERT_EQ(setenv(key, longValue.c_str(), 1), 0);
#endif
    const auto readLongValue = Base::environmentVariableUtf8(key);
    ASSERT_TRUE(readLongValue.has_value());
    EXPECT_EQ(*readLongValue, longValue);

#if defined(FC_OS_WIN32)
    ASSERT_EQ(_wputenv_s(L"FREECAD_TEST_LONG_ENV", L""), 0);
#else
    ASSERT_EQ(unsetenv(key), 0);
#endif

    EXPECT_FALSE(Base::environmentVariableUtf8(key).has_value());
}

TEST(PlatformPaths, serializesPortableSeparators)
{
#if defined(FC_OS_WIN32)
    const auto path = std::filesystem::path(L"FreeCAD\\External\\Link.FCStd");
#else
    const auto path = std::filesystem::path("FreeCAD/External/Link.FCStd");
#endif
    EXPECT_EQ(Base::pathToPortableUtf8(path), "FreeCAD/External/Link.FCStd");
}

#if !defined(FC_OS_WIN32) && !defined(FC_OS_MACOSX)
TEST(PlatformPaths, ignoresRelativeXdgHomeWhenPathExists)
{
    TempDirectory temp;
    ScopedCurrentPath currentPath(temp.path());
    fs::create_directories("relative-config");
    fs::create_directories("relative-data");
    fs::create_directories("relative-cache");
    ScopedEnvironment home("HOME", temp.path().string());
    ScopedEnvironment config("XDG_CONFIG_HOME", "relative-config");
    ScopedEnvironment data("XDG_DATA_HOME", "relative-data");
    ScopedEnvironment cache("XDG_CACHE_HOME", "relative-cache");

    const auto paths = Base::standardPaths();

    EXPECT_EQ(paths.config, temp.path() / ".config");
    EXPECT_EQ(paths.data, temp.path() / ".local" / "share");
    EXPECT_EQ(paths.cache, temp.path() / ".cache");
}

TEST(PlatformPaths, ignoresRelativeXdgHomeWhenPathDoesNotExist)
{
    TempDirectory temp;
    ScopedCurrentPath currentPath(temp.path());
    ScopedEnvironment home("HOME", temp.path().string());
    ScopedEnvironment config("XDG_CONFIG_HOME", "missing-config");
    ScopedEnvironment data("XDG_DATA_HOME", "missing-data");
    ScopedEnvironment cache("XDG_CACHE_HOME", "missing-cache");

    const auto paths = Base::standardPaths();

    EXPECT_TRUE(paths.config.is_absolute());
    EXPECT_TRUE(paths.data.is_absolute());
    EXPECT_TRUE(paths.cache.is_absolute());
    EXPECT_EQ(paths.config, temp.path() / ".config");
    EXPECT_EQ(paths.data, temp.path() / ".local" / "share");
    EXPECT_EQ(paths.cache, temp.path() / ".cache");
}
#endif

TEST(PlatformPaths, resolveExecutablePathUsesCurrentDirectoryForEmptyPathComponent)
{
    TempDirectory temp;
    ScopedCurrentPath currentPath(temp.path());
    const fs::path executable = temp.path() / "freecad-test-executable";
    std::ofstream(executable) << "#!/bin/sh\n";
#if !defined(FC_OS_WIN32)
    fs::permissions(
        executable,
        fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec,
        fs::perm_options::replace
    );
#endif
    ScopedEnvironment path("PATH", pathSeparator() + "missing");

    EXPECT_EQ(
        Base::resolveExecutablePath(executable.filename().string().c_str()),
        Base::canonicalIfExists(executable)
    );
}

TEST(PlatformPaths, resolveExecutablePathSkipsDirectories)
{
    TempDirectory temp;
    const fs::path first = temp.path() / "first";
    const fs::path second = temp.path() / "second";
    const std::string name = "freecad-test-executable";
    fs::create_directories(first / name);
    const fs::path executable = second / name;
    fs::create_directories(second);
    std::ofstream(executable) << "#!/bin/sh\n";
#if !defined(FC_OS_WIN32)
    fs::permissions(
        executable,
        fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec,
        fs::perm_options::replace
    );
#endif
    ScopedEnvironment path("PATH", first.string() + pathSeparator() + second.string());

    EXPECT_EQ(Base::resolveExecutablePath(name.c_str()), Base::canonicalIfExists(executable));
}

#if !defined(FC_OS_WIN32)
TEST(PlatformPaths, resolveExecutablePathSkipsNonExecutableFiles)
{
    TempDirectory temp;
    const fs::path first = temp.path() / "first";
    const fs::path second = temp.path() / "second";
    const std::string name = "freecad-test-executable";
    fs::create_directories(first);
    const fs::path nonExecutable = first / name;
    std::ofstream(nonExecutable) << "not executable\n";
    fs::permissions(
        nonExecutable,
        fs::perms::owner_read | fs::perms::owner_write,
        fs::perm_options::replace
    );

    fs::create_directories(second);
    const fs::path executable = second / name;
    std::ofstream(executable) << "#!/bin/sh\n";
    fs::permissions(
        executable,
        fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec,
        fs::perm_options::replace
    );
    ScopedEnvironment path("PATH", first.string() + pathSeparator() + second.string());

    EXPECT_EQ(Base::resolveExecutablePath(name.c_str()), Base::canonicalIfExists(executable));
}
#endif
