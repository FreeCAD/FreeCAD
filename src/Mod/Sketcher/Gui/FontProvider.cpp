// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2 or         *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "FontProvider.h"

#include <set>

#include <FCConfig.h>

#if defined(FC_OS_LINUX) || defined(FC_OS_BSD)
# include <memory>
#endif

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

#include <App/Application.h>

#if defined(FC_OS_LINUX) || defined(FC_OS_BSD)
# include <fontconfig/fontconfig.h>
#endif

namespace
{

QStringList filesInDirectories(const QStringList& directories)
{
    QStringList files;
    for (const QString& directory : directories) {
        QDirIterator it(directory, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            files << it.next();
        }
    }
    return files;
}

#if defined(FC_OS_LINUX) || defined(FC_OS_BSD)
template<typename T, void (*destroy)(T*)>
using FcPtr = std::unique_ptr<T, decltype([](T* object) { destroy(object); })>;

QStringList platformFontFiles()
{
    if (!FcInit()) {
        return {};
    }

    FcPtr<FcPattern, FcPatternDestroy> pattern {FcPatternCreate()};
    FcPtr<FcObjectSet, FcObjectSetDestroy> objects {FcObjectSetBuild(FC_FILE, nullptr)};
    if (!pattern || !objects) {
        return {};
    }

    FcPtr<FcFontSet, FcFontSetDestroy> fonts {FcFontList(nullptr, pattern.get(), objects.get())};
    if (!fonts) {
        return {};
    }

    QStringList files;
    for (int i = 0; i < fonts->nfont; ++i) {
        FcChar8* file = nullptr;
        if (FcPatternGetString(fonts->fonts[i], FC_FILE, 0, &file) == FcResultMatch) {
            files << QString::fromUtf8(reinterpret_cast<const char*>(file));
        }
    }
    return files;
}
#elif defined(FC_OS_WIN32)
QStringList platformFontFiles()
{
    // Includes the system font directory, normally C:/Windows/Fonts.
    QStringList directories = QStandardPaths::standardLocations(QStandardPaths::FontsLocation);
    const QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    if (!localAppData.isEmpty()) {
        directories << localAppData + QStringLiteral("/Microsoft/Windows/Fonts");
    }
    return filesInDirectories(directories);
}
#elif defined(FC_OS_MACOSX)
QStringList platformFontFiles()
{
    // Includes ~/Library/Fonts, /Library/Fonts, and /System/Library/Fonts.
    QStringList directories = QStandardPaths::standardLocations(QStandardPaths::FontsLocation);
    directories << QStringLiteral("/Network/Library/Fonts");
    return filesInDirectories(directories);
}
#else
QStringList platformFontFiles()
{
    // Includes ~/.local/share/fonts, ~/.fonts, and font directories below XDG_DATA_DIRS.
    return filesInDirectories(QStandardPaths::standardLocations(QStandardPaths::FontsLocation));
}
#endif

}  // namespace

namespace SketcherGui
{

QMap<QString, QString> findAvailableFontFiles()
{
    QStringList files = filesInDirectories(
        {QString::fromStdString(App::Application::getResourceDir() + "Mod/TechDraw/Resources/fonts/")}
    );
    files += platformFontFiles();

    QMap<QString, QString> fontMap;
    std::set<QString> names;
    const std::set<QString> suffixes {
        QStringLiteral("ttf"),
        QStringLiteral("otf"),
        QStringLiteral("ttc")
    };
    for (const QString& file : files) {
        const QFileInfo info(file);
        if (info.isFile() && suffixes.contains(info.suffix().toCaseFolded())
            && names.insert(info.baseName().toCaseFolded()).second) {
            fontMap.insert(info.baseName(), info.absoluteFilePath());
        }
    }
    return fontMap;
}

}  // namespace SketcherGui
