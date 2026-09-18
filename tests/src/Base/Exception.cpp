// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Base/Exception.h>

TEST(FileException, TestWhatCombinesUntranslatedMessageAndPath)
{
    const Base::FileWritePermissionException exception("/home/user/model.FCStd");

    EXPECT_EQ(exception.getMessage(), "No write permission for the file or the file is read-only");
    EXPECT_STREQ(
        exception.what(),
        "No write permission for the file or the file is read-only: /home/user/model.FCStd"
    );
}

TEST(FileException, TestTranslatedMessageFallsBackToSourceAndKeepsPath)
{
    const Base::FileNotFoundException exception("/home/user/missing.step");

    EXPECT_EQ(exception.getTranslatedMessage(), "File not found: /home/user/missing.step");
}

// The messages double as the "Exceptions" translation keys, so changing one
// silently orphans the translations already made for it.
TEST(FileException, TestMessagesMatchTheTranslationKeys)
{
    EXPECT_EQ(Base::FileNotFoundException("f").getMessage(), "File not found");
    EXPECT_EQ(Base::FileReadPermissionException("f").getMessage(), "No permission to read the file");
    EXPECT_EQ(
        Base::FileWritePermissionException("f").getMessage(),
        "No write permission for the file or the file is read-only"
    );
    EXPECT_EQ(Base::FileFormatException("f").getMessage(), "File format not supported");
    EXPECT_EQ(Base::FileReadException("f").getMessage(), "Error reading from file");
    EXPECT_EQ(Base::FileWriteException("f").getMessage(), "Error writing to file");
    EXPECT_EQ(Base::DirectoryNotFoundException("d").getMessage(), "Directory does not exist");
}
