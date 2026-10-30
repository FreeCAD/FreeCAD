// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2025 Pierre-Louis Boyer

#ifndef TECHDRAWGUI_HATCHPATTERNS_H
#define TECHDRAWGUI_HATCHPATTERNS_H

#include <QMetaType>
#include <QString>

class QComboBox;

namespace TechDrawGui
{

enum class PatternUIType
{
    SVG,
    PAT
};

struct PatternEntry
{
    QString displayName;
    PatternUIType type = PatternUIType::SVG;
    QString filePath;
    QString patNameInternal;

    bool operator==(const PatternEntry& other) const
    {
        return type == other.type && filePath == other.filePath
            && (type == PatternUIType::SVG || patNameInternal == other.patNameInternal);
    }
};

namespace HatchPatterns
{
void populate(QComboBox* box);
void select(QComboBox* box, const PatternEntry& entry);
PatternEntry defaultPattern();
PatternEntry lastUsedPattern();
void saveDefaultPattern(const PatternEntry& entry);
void saveLastUsedPattern(const PatternEntry& entry);
QString userDirectory(PatternUIType type);
void openUserDirectory();
}  // namespace HatchPatterns
}  // namespace TechDrawGui

Q_DECLARE_METATYPE(TechDrawGui::PatternEntry)

#endif
