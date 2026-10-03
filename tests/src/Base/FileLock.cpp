#include <gtest/gtest.h>

#include <Base/FileInfo.h>
#include <Base/FileLock.h>

#include <chrono>
#include <filesystem>
#include <string>

TEST(FileLockTest, TryLockZeroSucceedsWhenAvailable)
{
#if defined(__EMSCRIPTEN__)
    GTEST_SKIP() << "File locking is a no-op in Emscripten/WASM (single-process).";
#endif

    std::string fn = Base::FileInfo::getTempFileName();
    fn.append(".lock");

    Base::FileLock lockFile1(fn);
    EXPECT_TRUE(lockFile1.tryLock(std::chrono::milliseconds::zero()));
    EXPECT_TRUE(lockFile1.isLocked());
    lockFile1.unlock();

    Base::FileLock lockFile2(fn);
    EXPECT_TRUE(lockFile2.tryLock(std::chrono::milliseconds::zero()));
    lockFile2.unlock();

    std::error_code ec;
    (void)std::filesystem::remove(std::filesystem::path(fn), ec);
}

TEST(FileLockTest, UnlockRemovesLockFile)  // NOLINT
{
#if defined(__EMSCRIPTEN__)
    GTEST_SKIP() << "File locking is a no-op in Emscripten/WASM (single-process).";
#endif

    std::string fn = Base::FileInfo::getTempFileName();
    fn.append(".lock");

    Base::FileLock lockFile(fn);
    ASSERT_TRUE(lockFile.tryLock(std::chrono::milliseconds::zero()));
    EXPECT_TRUE(std::filesystem::exists(fn));
    lockFile.unlock();
    EXPECT_FALSE(std::filesystem::exists(fn));
}

TEST(FileLockTest, TryLockReportsUnavailableWhenDirectoryMissing)  // NOLINT
{
#if defined(__EMSCRIPTEN__)
    GTEST_SKIP() << "File locking is a no-op in Emscripten/WASM (single-process).";
#endif

    const std::filesystem::path missing = std::filesystem::path(Base::FileInfo::getTempFileName())
        / "missing" / "file.lock";

    Base::FileLock lockFile(Base::FileInfo::pathToString(missing));
    EXPECT_FALSE(lockFile.tryLock(std::chrono::milliseconds::zero()));
    EXPECT_FALSE(lockFile.isLocked());
    EXPECT_EQ(lockFile.lastFailure(), Base::FileLock::Failure::Unavailable);
}
