/* SPDX - License - Identifier: LGPL - 2.1 - or -later
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
#ifndef TECHDRAW_TEMPLATETRANSLATOR_H
#define TECHDRAW_TEMPLATETRANSLATOR_H

#include <QString>
#include <QMap>
#include <QStringList>

#include <Mod/TechDraw/TechDrawGlobal.h>

namespace TechDraw
{

// Note we cannot use Translator::instance()->supportedLocales(); because Translator is in GUI
// Also the language of the template is separated from the system language.
// So we have to implement another translation mechanism.
TechDrawExport extern const char* LanguageEnums[];

class TemplateTranslator
{
public:
    TemplateTranslator();

    QString translate(const QString& key, const QString& languageName) const;
    QStringList getAllKeys() const;
    QStringList getSupportedLanguageNames() const;

private:
    void initializeTranslations();
    QMap<QString, QMap<QString, QString>> m_translations;
    // Example: m_translations["%%TECHDRAW_SCALE%%"]["fr"] = "Échelle";
};

}  // namespace TechDraw

#endif  // TECHDRAW_TEMPLATETRANSLATOR_H