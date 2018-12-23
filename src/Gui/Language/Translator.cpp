/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <algorithm>
# include <QTranslator>
# include <QStringList>
# include <QDir>
# include <QApplication>
#endif

#include "Translator.h"
#include <App/Application.h>

using namespace Gui;

/** \defgroup i18n Internationalization with FreeCAD
 *  \ingroup GUI
 *
 * The internationalization of FreeCAD makes heavy use of the internationalization
 * support of Qt. For more details refer to your Qt documentation.
 * 
 * \section stepbystep Step by step
 * To integrate a new language into FreeCAD or one of its application modules
 * you have to perform the following steps:
 *
 * \subsection tsfile Creation of a .ts file
 * First you have to generate a .ts file for the language to be translated. You can do this
 * by running the \a lupdate tool in the \a bin path of your Qt installation. As argument
 * you can specify either all related source files and the .ts output file or a Qt project 
 * file (.pro) which contains all relevant source files.
 *
 * \subsection translate Translation into your language
 * To translate the english string literals into the language you want to support you can open your
 * .ts file with \a QtLinguist and translate all literals by hand. Another way
 * for translation is to use the tool \a tsauto from Sebastien Fricker.This tool uses the 
 * engine from Google web page (www.google.com). tsauto supports the languages
 * \li english
 * \li french
 * \li german
 * \li italian
 * \li portuguese and
 * \li spanish
 *
 * \remark To get most of the literals translated you should have removed all
 * special characters (like &, !, ?, ...). Otherwise the translation could fail.
 * After having translated all literals you can load the .ts file into QtLinguist and 
 * invoke the menu item \a Release which generates the binary .qm file.
 *
 * \subsection usets Integration of the .qm file
 * The .qm file should now be integrated into the GUI library (either of FreeCAD
 * itself or its application module). The .qm file will be embedded into the
 * resulting binary file. So, at runtime you don't need any .qm files any
 * more. Indeed you will have a bigger binary file but you haven't any troubles
 * concerning missing .qm files.
 *
 * To integrate the .qm file into the executable you have to create a resource file (.qrc), first.
 * This is an XML file where you can append the .qm file. For the .qrc file you have to define the following
 * curstom build step inside the Visual Studio project file:
 *
 * Command Line: rcc.exe -name $(InputName) $(InputPath) -o "$(InputDir)qrc_$(InputName).cpp"
 * Outputs:      $(InputDir)qrc_$(InputName).cpp
 * 
 * For the gcc build system you just have to add the line \<resourcefile\>.qrc to the BUILT_SOURCES
 * sources section of the Makefile.am, run automake and configure (or ./confog.status) afterwards.
 *
 * Finally, you have to add a the line
 * \code
 * 
 * Q_INIT_RESOURCE(resource);
 *
 * \endcode
 * 
 * where \a resource is the name of the .qrc file. That's all!
 */

/* TRANSLATOR Gui::Translator */

Translator* Translator::_pcSingleton = 0;

namespace Gui {
class TranslatorP
{
public:
    std::string activatedLanguage; /**< Active language */
    std::map<std::string, std::string> mapLanguageTopLevelDomain;
    TStringMap mapSupportedLocales;
    std::list<QTranslator*> translators; /**< A list of all created translators */
    QStringList paths;
};
}

Translator* Translator::instance(void)
{
    if (!_pcSingleton)
        _pcSingleton = new Translator;
    return _pcSingleton;
}

void Translator::destruct (void)
{
    if (_pcSingleton)
        delete _pcSingleton;
    _pcSingleton=0;
}

Translator::Translator()
{
    // This is needed for Qt's lupdate
    d = new TranslatorP;
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("English"              )] = "en";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("German"               )] = "de";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Spanish"              )] = "es-ES";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("French"               )] = "fr";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Italian"              )] = "it";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Japanese"             )] = "ja";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Chinese Simplified"   )] = "zh-CN";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Chinese Traditional"  )] = "zh-TW";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Korean"               )] = "ko";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Russian"              )] = "ru";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Swedish"              )] = "sv-SE";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Afrikaans"            )] = "af";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Norwegian"            )] = "no";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Portuguese, Brazilian")] = "pt-BR";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Portuguese"           )] = "pt-PT";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Dutch"                )] = "nl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Ukrainian"            )] = "uk";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Finnish"              )] = "fi";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Croatian"             )] = "hr";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Polish"               )] = "pl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Czech"                )] = "cs";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Hungarian"            )] = "hu";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Romanian"             )] = "ro";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Slovak"               )] = "sk";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Turkish"              )] = "tr";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Slovenian"            )] = "sl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Basque"               )] = "eu";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Catalan"              )] = "ca";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Galician"             )] = "gl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Kabyle"               )] = "kab";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Korean"               )] = "ko";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Filipino"             )] = "fil";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Indonesian"           )] = "id";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Lithuanian"           )] = "lt";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Valencian"            )] = "val-ES";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Arabic"               )] = "ar";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Vietnamese"           )] = "vi";
    
    d->activatedLanguage = "English";

    d->paths = directories();
}

Translator::~Translator()
{
    removeTranslators();
    delete d;
}

TStringList Translator::supportedLanguages() const
{
    TStringList languages;
    TStringMap locales = supportedLocales();
    for (auto it : locales)
        languages.push_back(it.first);

    return languages;
}

TStringMap Translator::supportedLocales() const
{
    if (!d->mapSupportedLocales.empty())
        return d->mapSupportedLocales;

    // List all .qm files
    QDir dir(QLatin1String(":/translations"));
    for (std::map<std::string,std::string>::const_iterator it = d->mapLanguageTopLevelDomain.begin();
        it != d->mapLanguageTopLevelDomain.end(); ++it) {
        QString filter = QString::fromLatin1("*_%1.qm").arg(QLatin1String(it->second.c_str()));
        QStringList fileNames = dir.entryList(QStringList(filter), QDir::Files, QDir::Name);
        if (!fileNames.isEmpty())
            d->mapSupportedLocales[it->first] = it->second;
    }

    return d->mapSupportedLocales;
}

void Translator::activateLanguage (const char* lang)
{
    removeTranslators(); // remove the currently installed translators
    d->activatedLanguage = lang;
    TStringList languages = supportedLanguages();
    if (std::find(languages.begin(), languages.end(), lang) != languages.end()) {
        refresh();
    }
}

std::string Translator::activeLanguage() const
{
    return d->activatedLanguage;
}

std::string Translator::locale(const std::string& lang) const
{
    std::string loc;
    std::map<std::string, std::string>::const_iterator tld = d->mapLanguageTopLevelDomain.find(lang);
    if (tld != d->mapLanguageTopLevelDomain.end())
        loc = tld->second;

    return loc;
}

QStringList Translator::directories() const
{
    QStringList list;
    QDir home(QString::fromUtf8(App::Application::getUserAppDataDir().c_str()));
    list.push_back(home.absoluteFilePath(QLatin1String("translations")));
    QDir resc(QString::fromUtf8(App::Application::getResourceDir().c_str()));
    list.push_back(resc.absoluteFilePath(QLatin1String("translations")));
    list.push_back(QLatin1String(":/translations"));
    return list;
}

void Translator::addPath(const QString& path)
{
    d->paths.push_back(path);
}

void Translator::installQMFiles(const QDir& dir, const char* locale)
{
    QString filter = QString::fromLatin1("*_%1.qm").arg(QLatin1String(locale));
    QStringList fileNames = dir.entryList(QStringList(filter), QDir::Files, QDir::Name);
    for (QStringList::Iterator it = fileNames.begin(); it != fileNames.end(); ++it){
        bool ok=false;
        for (std::list<QTranslator*>::const_iterator tt = d->translators.begin();
            tt != d->translators.end(); ++tt) {
            if ((*tt)->objectName() == *it) {
                ok = true; // this file is already installed
                break;
            }
        }

        // okay, we need to install this file
        if (!ok) {
            QTranslator* translator = new QTranslator;
            translator->setObjectName(*it);
            if (translator->load(dir.filePath(*it))) {
                qApp->installTranslator(translator);
                d->translators.push_back(translator);
            }
            else {
                delete translator;
            }
        }
    }
}

/**
 * This method checks for newly added (internal) .qm files which might be added at runtime. This e.g. happens if a plugin
 * gets loaded at runtime. For each newly added files that supports the currently set language a new translator object is created 
 * to load the file.
 */
void Translator::refresh()
{
    std::map<std::string, std::string>::iterator tld = d->mapLanguageTopLevelDomain.find(d->activatedLanguage);
    if (tld == d->mapLanguageTopLevelDomain.end())
        return; // no language activated
    for (QStringList::iterator it = d->paths.begin(); it != d->paths.end(); ++it) {
        QDir dir(*it);
        installQMFiles(dir, tld->second.c_str());
    }
}

/**
 * Uninstalls all translators.
 */
void Translator::removeTranslators()
{
    for (std::list<QTranslator*>::iterator it = d->translators.begin(); it != d->translators.end(); ++it) {
        qApp->removeTranslator(*it);
        delete *it;
    }

    d->translators.clear();
}

#include "moc_Translator.cpp"
