// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2026 The FreeCAD Project Association AISBL                             *
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

#include <QDebug>
#include <QTest>
#include <fstream>
#include <iostream>

#include <App/Application.h>

#include "Gui/Dialogs/DlgVersionMigrator.h"

#include "App/ApplicationDirectories.h"

#include <src/App/InitApplication.h>
#include "../../Base/RandomUtils.h"


class TestablePathMigrationWorker: public Gui::Dialog::PathMigrationWorker
{
    using Gui::Dialog::PathMigrationWorker::PathMigrationWorker;

public:
    TestablePathMigrationWorker(std::string userConfigDir, std::string userAppDir, int major, int minor)
        : PathMigrationWorker(userConfigDir, userAppDir, major, minor)
        , testConfigDir(userConfigDir)
        , testUserAppDir(userAppDir) {};

    void testableReplaceOccurrencesInPreferences()
    {
        replaceOccurrencesInPreferences();
    }

    std::filesystem::path testableLocateNewPreferences()
    {
        return locateNewPreferences();
    }

    std::string testableGenerateNewUserAppPathString(const std::string& oldPath)
    {
        return generateNewUserAppPathString(oldPath);
    }

    static void testableReplaceInContents(
        std::string& contents,
        const std::string& oldString,
        const std::string& newString
    )
    {
        replaceInContents(contents, oldString, newString);
    }

    std::string getConfigDir()
    {
        return testConfigDir;
    }

    std::string getUserAppDir()
    {
        return testUserAppDir;
    }

private:
    std::string testConfigDir;
    std::string testUserAppDir;
};

class testPathMigrationWorker final: public QObject
{
    Q_OBJECT

public:
    testPathMigrationWorker()
    {
        tests::initApplication();
    }

private Q_SLOTS:

    void init()  // NOLINT
    {}

    void cleanup()  // NOLINT
    {}

    void replaceInContents_data()  // NOLINT
    {
        QTest::addColumn<QString>("contents");
        QTest::addColumn<QString>("oldString");
        QTest::addColumn<QString>("newString");
        QTest::addColumn<QString>("expected");

        // No-op / no matches
        QTest::newRow("empty-contents") << "" << "a" << "b" << "";
        QTest::newRow("no-match") << "abcdef" << "x" << "y" << "abcdef";

        // Single match
        QTest::newRow("single-match-middle") << "abcXYZdef" << "XYZ" << "Q" << "abcQdef";
        QTest::newRow("single-match-begin") << "XYZdef" << "XYZ" << "Q" << "Qdef";
        QTest::newRow("single-match-end") << "abcXYZ" << "XYZ" << "Q" << "abcQ";

        // Multiple matches
        QTest::newRow("multiple-separated") << "a1a2a3" << "a" << "b" << "b1b2b3";
        QTest::newRow("multiple-words") << "foo bar foo" << "foo" << "x" << "x bar x";

        // Adjacent / overlapping-looking cases (should replace non-overlapping occurrences)
        QTest::newRow("adjacent") << "aaaa" << "aa" << "b" << "bb";  // "aa" + "aa"
        QTest::newRow("pattern-repeats") << "ababab" << "ab" << "x" << "xxx";

        // oldString longer/shorter than newString
        QTest::newRow("shorten") << "abc123abc" << "abc" << "a" << "a123a";
        QTest::newRow("expand") << "a-b-a-b" << "a" << "LONG" << "LONG-b-LONG-b";

        // oldString == newString
        QTest::newRow("old-equals-new") << "same same" << "same" << "same" << "same same";

        // newString contains oldString (avoid infinite loop)
        // Expected behavior for a correct implementation: only original matches are replaced.
        QTest::newRow("new-contains-old") << "aa" << "a" << "aa" << "aaaa";

        // oldString empty: treat empty oldString as no-op.
        QTest::newRow("empty-oldString-noop") << "abc" << "" << "X" << "abc";

        // newString empty: delete occurrences
        QTest::newRow("delete-occurrences") << "bananas" << "na" << "" << "bas";
    }

    void replaceInContents()  // NOLINT
    {
        QFETCH(QString, contents);
        QFETCH(QString, oldString);
        QFETCH(QString, newString);
        QFETCH(QString, expected);

        std::string c = contents.toStdString();
        const std::string oldS = oldString.toStdString();
        const std::string newS = newString.toStdString();

        TestablePathMigrationWorker::testableReplaceInContents(c, oldS, newS);

        QCOMPARE(QString::fromStdString(c), expected);
    }

    void replaceInContents_idempotent_when_old_not_present()  // NOLINT
    {
        std::string c = "no matches here";
        const std::string oldS = "ZZZ";
        const std::string newS = "YYY";

        TestablePathMigrationWorker::testableReplaceInContents(c, oldS, newS);
        const std::string once = c;

        // run twice: should stay identical
        TestablePathMigrationWorker::testableReplaceInContents(c, oldS, newS);
        QCOMPARE(QString::fromStdString(c), QString::fromStdString(once));
    }

    std::unique_ptr<TestablePathMigrationWorker> makeWorker(int major, int minor)
    {
        std::string userConfigDir = Base::FileInfo::pathToString(
            std::filesystem::temp_directory_path() / Base::generateRandomName("Config_")
        );
        std::string userAppDataDir = Base::FileInfo::pathToString(
            std::filesystem::temp_directory_path() / Base::generateRandomName("AppData_")
        );
        return std::make_unique<TestablePathMigrationWorker>(userConfigDir, userAppDataDir, major, minor);
    }

    void generateNewUserAppPathString_no_version_now()
    {
        auto worker = makeWorker(1, 1);
        std::filesystem::path testPath = std::filesystem::temp_directory_path() / Base::generateRandomName("foo") / "bar";
        std::string oldPath = Base::FileInfo::pathToString(testPath);
        std::string newPath = worker->testableGenerateNewUserAppPathString(oldPath);


        std::string expectedAddition = App::ApplicationDirectories::versionStringForPath(1, 1);

        std::string expectedPath = Base::FileInfo::pathToString(testPath / expectedAddition);
        QCOMPARE(newPath, expectedPath);
    }

    void generateNewUserAppPathString_version_in_current()
    {
        auto worker = makeWorker(1, 1);
        std::filesystem::path testPath = std::filesystem::temp_directory_path() / Base::generateRandomName("foo") / "bar"
            / "v1-0";
        std::string oldPath = Base::FileInfo::pathToString(testPath);
        std::string newPath = worker->testableGenerateNewUserAppPathString(oldPath);

        std::string expectedAddition = App::ApplicationDirectories::versionStringForPath(1, 1);

        std::string expectedPath = Base::FileInfo::pathToString(
            testPath.parent_path() / expectedAddition
        );
        QCOMPARE(newPath, expectedPath);
    }

    void locateNewPreferences()
    {
        auto worker = makeWorker(1, 1);
        std::string configDir = worker->getConfigDir();
        std::filesystem::path expectedNewPreferences = Base::FileInfo::stringToPath(configDir);
        expectedNewPreferences = expectedNewPreferences / "v1-1" / "user.cfg";
        std::string expectedNewPreferencesString = Base::FileInfo::pathToString(expectedNewPreferences);
        std::string actualNewPreferencesString = Base::FileInfo::pathToString(
            worker->testableLocateNewPreferences()
        );
        QCOMPARE(actualNewPreferencesString, expectedNewPreferencesString);
    }

    static void _writePreferencesTestData(std::ostream& stream, const std::string& pathToInject)
    {
        stream << "<FCParamGroup Name=\"Macro\">\n"
               << "  <FCBool Name=\"LocalEnvironment\" Value=\"1\"/>\n"
               << "  <FCText Name=\"MacroPath\">" << pathToInject << "</FCText>";
    }

    void replaceOccurrencesInPreferences()
    {
        auto worker = makeWorker(1, 1);
        std::string configDir = worker->getConfigDir();
        std::string userAppDir = worker->getUserAppDir();
        std::filesystem::path newPrefs = Base::FileInfo::stringToPath(configDir);
        newPrefs = newPrefs / "v1-1" / "user.cfg";
        std::filesystem::create_directories(newPrefs.parent_path());
        std::ofstream prefs(newPrefs);
        _writePreferencesTestData(prefs, userAppDir);
        prefs.close();
        worker->testableReplaceOccurrencesInPreferences();
        std::ifstream loadedPrefs(newPrefs, std::ios::in | std::ios::binary);
        std::string modifiedPrefsData {
            std::istreambuf_iterator<char>(loadedPrefs),
            std::istreambuf_iterator<char>()
        };
        loadedPrefs.close();
        auto expectedNewDir = Base::FileInfo::pathToString(
            Base::FileInfo::stringToPath(userAppDir) / "v1-1"
        );
        Q_ASSERT(modifiedPrefsData.find(expectedNewDir) != std::string::npos);
    }
};


QTEST_MAIN(testPathMigrationWorker)

#include "DlgVersionMigrator.moc"
