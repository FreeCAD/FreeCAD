// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Gui/FileDialog.h>
#include <Gui/FileDialogInternal.h>


using namespace Gui;
using Filter = FileDialog::Filter;

TEST(FileDialog, testNormalizeSavePath)
{
    const auto filt = FileDialog::Filter::fromFilterString(
        QStringLiteral("Whatever files (what ev.er *.abc *.xyz)")
    );
    const auto test = [filt](QString path, const QString& desired) {
        FileDialogInternal::normalizeSavePath(path, filt);
        const auto firstPassResult = path;
        FileDialogInternal::normalizeSavePath(path, filt);
        EXPECT_EQ(firstPassResult, path) << "not idempotent";
        EXPECT_EQ(path, desired);
    };
    for (const auto& root :
         {QStringLiteral(""),
          QStringLiteral("/root/.trash/"),
          QStringLiteral("C:/$Recycle.bin/"),
          QStringLiteral("//net.worked:123/"),
          QStringLiteral("//?/o:/ntpath/"),
          QStringLiteral("/\x3F?/z:/evil/")}) {
        test(root + "", root + ".abc");
        test(root + "hello", root + "hello.abc");

        // Dots handling
        test(root + "dotty.", root + "dotty.abc");
        test(root + "dotty2..", root + "dotty2..abc");
        test(root + "dotty3...", root + "dotty3...abc");
        test(root + ".hidden", root + ".hidden.abc");
        test(root + ".1dotty1.", root + ".1dotty1.abc");
        test(root + ".1dotty2..", root + ".1dotty2..abc");
        test(root + "..2dotty2..", root + "..2dotty2..abc");

        // Extension not in filter
        test(root + "ascii.txt", root + "ascii.txt.abc");
        test(root + "asciidot.txt.", root + "asciidot.txt.abc");

        // Empty stems but valid extensions
        test(root + ".abc", root + ".abc");
        test(root + ".xyz", root + ".xyz");

        // Regular cases with extension
        test(root + "ext.abc", root + "ext.abc");
        test(root + "ext.parts.abc", root + "ext.parts.abc");
        test(root + ".hidden.ext.abc", root + ".hidden.ext.abc");
        test(root + "ext.xyz", root + "ext.xyz");
        test(root + "ext.parts.xyz", root + "ext.parts.xyz");
        test(root + ".hidden.ext.xyz", root + ".hidden.ext.xyz");

        // Non-wildcard pattern without ext
        test(root + "what", root + "what");
        test(root + "what.", root + "what.abc");
        test(root + ".what", root + ".what.abc");
        test(root + ".what.", root + ".what.abc");

        // Non-wildcard pattern with ext
        test(root + "ev.er", root + "ev.er");
        test(root + "ev.er.", root + "ev.er.abc");
        test(root + ".ev.er", root + ".ev.er.abc");
        test(root + ".ev.er.", root + ".ev.er.abc");
        // Check for mixed up extension matching
        test(root + "notev.er", root + "notev.er.abc");

        // The following two should never be encountered in the wild as they refer to
        // the current or upper directories, but still should be handled gracefully as
        // if they were just user input tacked on after some directory path.
        test(root + ".", root + ".abc");
        test(root + "..", root + "..abc");
    }
}

TEST(FileDialog, testFilterFromFilterString)
{
    const auto eq =
        [](bool positive, const QString& string, const QString& name, const QStringList& patterns) {
            const auto fromString = Filter::fromFilterString(string);
            const Filter expected {name, patterns};
            if (positive) {
                EXPECT_EQ(fromString, expected);
            }
            else {
                EXPECT_NE(fromString, expected);
            }
        };
    eq(true, "A (*.a)", "A", {"*.a"});
    eq(true, "A (*.a *.b)", "A", {"*.a", "*.b"});
    eq(true, "A ( *.a  *.b )", "A", {"*.a", "*.b"});
    eq(false, "A (*.a;*.b)", "A", {"*.a", "*.b"});
    eq(true, "L (*.averylongname)", "L", {"*.averylongname"});
}

TEST(FileDialog, testFilterWildcard)
{
    using F = FileDialog::Filter;
    EXPECT_TRUE(F("Single wildcard", {"*"}).isWildcard());
    EXPECT_TRUE(F("Dotted wildcard", {"*.*"}).isWildcard());
    EXPECT_TRUE(F("Ext & Single wildcard", {"*.abc", "*"}).isWildcard());
    EXPECT_TRUE(F("Ext & Dotted wildcard", {"*.abc", "*.*"}).isWildcard());

    EXPECT_FALSE(F("Empty", {}).isWildcard());
    EXPECT_FALSE(F("Empty string", {""}).isWildcard());
    EXPECT_FALSE(F("Text", {"*.txt"}).isWildcard());
    EXPECT_FALSE(F("Not single wildcard", {"*def"}).isWildcard());
    EXPECT_FALSE(F("Not single wildcard 2", {"def*"}).isWildcard());
    EXPECT_FALSE(F("Not single wildcard 2", {"def*ghi"}).isWildcard());
    EXPECT_FALSE(F("Not dotted wildcard", {"*.*def"}).isWildcard());
    EXPECT_FALSE(F("Not dotted wildcard 2", {"def*.*"}).isWildcard());
    EXPECT_FALSE(F("Not dotted wildcard 3", {"def*.*ghi"}).isWildcard());
    EXPECT_FALSE(F("Ext & Not single wildcard", {"*.abc", "*def"}).isWildcard());
    EXPECT_FALSE(F("Ext & Not single wildcard 2", {"*.abc", "def*"}).isWildcard());
    EXPECT_FALSE(F("Ext & Not single wildcard 3", {"*.abc", "def*ghi"}).isWildcard());
    EXPECT_FALSE(F("Ext & Not dotted wildcard", {"*.abc", "*.*def"}).isWildcard());
    EXPECT_FALSE(F("Ext & Not dotted wildcard 2", {"*.abc", "def*.*"}).isWildcard());
    EXPECT_FALSE(F("Ext & Not dotted wildcard 3", {"*.abc", "def*.*ghi"}).isWildcard());
}
