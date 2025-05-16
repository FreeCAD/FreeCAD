// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2025 Pierre-Louis Boyer

#include <algorithm>
#include <QComboBox>
#include <QDesktopServices>
#include <QDirIterator>
#include <QSignalBlocker>
#include <QUrl>

#include <App/Application.h>
#include <Base/Console.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/HatchLine.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "HatchPatterns.h"

namespace TechDrawGui
{
namespace
{
PatternEntry patternEntry(const QString& filePath, const QString& patName = {})
{
    const auto type = patName.isEmpty() ? PatternUIType::SVG : PatternUIType::PAT;
    QString label = QFileInfo(filePath).fileName();
    if (type == PatternUIType::PAT) {
        label += QStringLiteral(" - ") + patName;
    }
    return {label, type, QDir::fromNativeSeparators(filePath), patName};
}

PatternEntry readPattern(const char* fileKey, const char* nameKey)
{
    const auto prefs = TechDraw::Preferences::getPreferenceGroup("Hatch");
    return patternEntry(
        QString::fromStdString(prefs->GetASCII(fileKey)),
        QString::fromStdString(prefs->GetASCII(nameKey))
    );
}

void writePattern(const PatternEntry& entry, const char* fileKey, const char* nameKey)
{
    const auto prefs = TechDraw::Preferences::getPreferenceGroup("Hatch");
    prefs->SetASCII(fileKey, entry.filePath.toUtf8().constData());
    prefs->SetASCII(nameKey, entry.patNameInternal.toUtf8().constData());
}
}  // namespace

void HatchPatterns::populate(QComboBox* box)
{
    const QSignalBlocker blocker(box);
    box->clear();
    QStringList files;
    const QString resourceDir = QString::fromStdString(
        App::Application::getResourceDir() + "Mod/TechDraw/"
    );
    const QStringList directories {
        resourceDir + QStringLiteral("Patterns/"),
        resourceDir + QStringLiteral("PAT/"),
        userDirectory(PatternUIType::SVG),
        userDirectory(PatternUIType::PAT)
    };
    for (const auto& directory : directories) {
        QDirIterator it(
            directory,
            {QStringLiteral("*.svg"),
             QStringLiteral("*.SVG"),
             QStringLiteral("*.pat"),
             QStringLiteral("*.PAT")},
            QDir::Files,
            QDirIterator::Subdirectories
        );
        while (it.hasNext()) {
            files.push_back(it.next());
        }
    }

    // Include paths configured before the unified picker, including external custom files.
    files.push_back(QString::fromStdString(TechDraw::DrawHatch::prefSvgHatch()));
    files.push_back(QString::fromStdString(TechDraw::DrawGeomHatch::prefGeomHatchFile()));
    std::ranges::transform(files, files.begin(), [](const auto& file) {
        return QDir::fromNativeSeparators(file);
    });
    files.removeDuplicates();
    files.sort(Qt::CaseInsensitive);
    for (const auto& file : files) {
        if (!QFileInfo::exists(file)) {
            continue;
        }
        if (QFileInfo(file).suffix().compare(QLatin1String("pat"), Qt::CaseInsensitive) == 0) {
            auto filePath = file.toStdString();
            for (const auto& name : TechDraw::PATLineSpec::getPatternList(filePath)) {
                const auto entry = patternEntry(file, QString::fromStdString(name));
                box->addItem(entry.displayName, QVariant::fromValue(entry));
            }
        }
        else {
            const auto entry = patternEntry(file);
            box->addItem(entry.displayName, QVariant::fromValue(entry));
        }
    }
}

void HatchPatterns::select(QComboBox* box, const PatternEntry& entry)
{
    for (int i = 0; i < box->count(); ++i) {
        if (box->itemData(i).value<PatternEntry>() == entry) {
            box->setCurrentIndex(i);
            return;
        }
    }
    // Existing documents may carry an embedded pattern whose external file is unavailable.
    box->addItem(entry.displayName, QVariant::fromValue(entry));
    box->setCurrentIndex(box->count() - 1);
}

PatternEntry HatchPatterns::defaultPattern()
{
    return readPattern("HatchDefaultFile", "HatchDefaultName");
}

PatternEntry HatchPatterns::lastUsedPattern()
{
    auto entry = readPattern("HatchLastUsedFile", "HatchLastUsedName");
    if (entry.filePath.isEmpty()) {
        entry = patternEntry(QString::fromStdString(TechDraw::DrawHatch::prefSvgHatch()));
    }
    return entry;
}

void HatchPatterns::saveDefaultPattern(const PatternEntry& entry)
{
    writePattern(entry, "HatchDefaultFile", "HatchDefaultName");
}

void HatchPatterns::saveLastUsedPattern(const PatternEntry& entry)
{
    writePattern(entry, "HatchLastUsedFile", "HatchLastUsedName");
}

QString HatchPatterns::userDirectory(PatternUIType type)
{
    return QString::fromStdString(App::Application::getUserAppDataDir() + "Mod/TechDraw/")
        + (type == PatternUIType::SVG ? QStringLiteral("Patterns") : QStringLiteral("PAT"));
}

void HatchPatterns::openUserDirectory()
{
    const QString directory = userDirectory(PatternUIType::SVG);
    if (!QDir().mkpath(directory) || !QDesktopServices::openUrl(QUrl::fromLocalFile(directory))) {
        Base::Console().error(
            "Could not open hatch patterns directory '%s'.\n",
            directory.toUtf8().constData()
        );
    }
}
}  // namespace TechDrawGui
