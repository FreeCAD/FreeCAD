// SPDX - License - Identifier: LGPL - 2.1 - or -later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"

#include "TemplateTranslator.h"
#include <Base/ServiceProvider.h>

namespace TechDraw
{
TechDrawExport const char* LanguageEnums[] = {
    "English",
    "Afrikaans",
    "Català",
    "Dansk",
    "Deutsch",
    "Español",
    "Euskara",
    "Filipino",
    "Français",
    "Galego",
    "Hrvatski",
    "Indonesia",
    "Italiano",
    "Lietuvių",
    "Magyar",
    "Nederlands",
    "Norsk bokmål",
    "Polski",
    "Português",
    "Română",
    "Slovenčina",
    "Slovenščina",
    "Srpski",
    "Suomi",
    "Svenska",
    "Taqbaylit",
    "Tiếng Việt",
    "Türkçe",
    "Valencian",
    "Čeština",
    "Ελληνικά",    // Greek
    "Беларуская",  // Belarusian
    "Български",   // Bulgarian
    "Русский",     // Russian
    "Српски",      // Serbian
    "Українська",  // Ukrainian
    "العربية",     // Arabic
    "ქართული",     // Georgian
    "日本語",      // Japanese
    "简体中文",    // Chinese Simplified
    "繁體中文",    // Chinese Traditional
    "한국어",      // Korean
    nullptr};

QString TemplateTranslator::translate(const QString& key, const QString& languageName) const
{
    if (auto* service = Base::provideService<TemplateTranslationService>()) {
        return service->translate(key, languageName);
    }
    return key;
}

QStringList TemplateTranslator::getAllKeys() const
{
    static const char* const keys[] = {
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Scale:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Sheet:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Title, supplementary title:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Issue date:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Created by:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Approved by:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Drawing number:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Part Material:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Revision:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "General tolerances:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Owner:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Document type:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Responsible department:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Document status:"),
        QT_TRANSLATE_NOOP("TechDraw::TemplateTranslator", "Language:"),
    };
    QStringList result;
    for (const char* key : keys) {
        result.append(QString::fromUtf8(key));
    }
    return result;
}

QStringList TemplateTranslator::getSupportedLanguageNames() const
{
    QStringList names;
    for (const char* const* language = LanguageEnums; *language; ++language) {
        names.append(QString::fromUtf8(*language));
    }
    names.sort();
    return names;
}

}  // namespace TechDraw
