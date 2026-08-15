// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include <Gui/DocumentRecovery.h>
#include <zipios++/zipoutputstream.h>

#include <fstream>
#include <memory>

namespace
{

int openFileCount()
{
    return QDir(QStringLiteral("/proc/self/fd")).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).size();
}

void createArchive(const QString& path)
{
    std::ofstream output(path.toStdString(), std::ios::binary);
    zipios::ZipOutputStream archive(output);
    for (int i = 0; i < 32; ++i) {
        archive.putNextEntry("entry" + std::to_string(i));
        archive << "data";
    }
    archive.close();
}

}  // namespace

TEST(DocumentRecovery, ReleasesValidationStreams)
{
#ifndef __linux__
    GTEST_SKIP() << "The descriptor check requires /proc/self/fd";
#else
    int argc = 1;
    char arg0[] = "Gui_tests_run";
    char* argv[] = {arg0, nullptr};
    std::unique_ptr<QApplication> application;
    if (!QCoreApplication::instance()) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        application = std::make_unique<QApplication>(argc, argv);
    }

    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());

    const QString original = temp.filePath(QStringLiteral("project.FCStd"));
    const QString recoveryDir = temp.filePath(QStringLiteral("recovery"));
    ASSERT_TRUE(QDir().mkpath(recoveryDir));
    createArchive(original);
    ASSERT_TRUE(QFile::copy(original, QDir(recoveryDir).filePath(QStringLiteral("fc_recovery_file.fcstd"))));

    QFile metadata(QDir(recoveryDir).filePath(QStringLiteral("fc_recovery_file.xml")));
    ASSERT_TRUE(metadata.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&metadata);
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
           << "<AutoRecovery SchemaVersion=\"1\">\n"
           << "  <Status>Created</Status>\n"
           << "  <Label>Recovery test</Label>\n"
           << "  <FileName>" << original << "</FileName>\n"
           << "</AutoRecovery>\n";
    metadata.close();

    const int before = openFileCount();
    {
        Gui::Dialog::DocumentRecovery recovery({QFileInfo(recoveryDir)});
    }
    const int after = openFileCount();

    EXPECT_LE(after - before, 2);
#endif
}
