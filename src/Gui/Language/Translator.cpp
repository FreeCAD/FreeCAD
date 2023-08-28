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
# include <QApplication>
# include <QDir>
# include <QKeyEvent>
# include <QRegularExpression>
# include <QStringList>
# include <QTranslator>
# include <QWidget>
#endif

#include <App/Application.h>
#include <Gui/TextEdit.h>
#include "Translator.h"


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

Translator* Translator::_pcSingleton = nullptr;

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

Translator* Translator::instance()
{
    if (!_pcSingleton)
        _pcSingleton = new Translator;
    return _pcSingleton;
}

void Translator::destruct ()
{
    if (_pcSingleton)
        delete _pcSingleton;
    _pcSingleton=nullptr;
}

Translator::Translator()
{
    // This is needed for Qt's lupdate
    d = new TranslatorP;
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Afrikaans"            )] = "af";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Arabic"               )] = "ar";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Basque"               )] = "eu";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Belarusian"           )] = "be";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Bulgarian"            )] = "bg";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Catalan"              )] = "ca";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Chinese Simplified"   )] = "zh-CN";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Chinese Traditional"  )] = "zh-TW";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Croatian"             )] = "hr";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Czech"                )] = "cs";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Dutch"                )] = "nl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("English"              )] = "en";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Filipino"             )] = "fil";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Finnish"              )] = "fi";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("French"               )] = "fr";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Galician"             )] = "gl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Georgian"             )] = "ka";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("German"               )] = "de";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Greek"                )] = "el";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Hungarian"            )] = "hu";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Indonesian"           )] = "id";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Italian"              )] = "it";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Japanese"             )] = "ja";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Kabyle"               )] = "kab";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Korean"               )] = "ko";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Lithuanian"           )] = "lt";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Norwegian"            )] = "no";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Polish"               )] = "pl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Portuguese, Brazilian")] = "pt-BR";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Portuguese"           )] = "pt-PT";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Romanian"             )] = "ro";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Russian"              )] = "ru";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Serbian"              )] = "sr";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Serbian, Latin"       )] = "sr-CS";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Slovak"               )] = "sk";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Slovenian"            )] = "sl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Spanish"              )] = "es-ES";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Spanish, Argentina"   )] = "es-AR";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Swedish"              )] = "sv-SE";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Turkish"              )] = "tr";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Ukrainian"            )] = "uk";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Valencian"            )] = "val-ES";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Vietnamese"           )] = "vi";

    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General");
    auto entries = hGrp->GetASCII("AdditionalLanguageDomainEntries", "");
    // The format of the entries is "Language Name 1"="code1";"Language Name 2"="code2";...
    // Example: <FCText Name="AdditionalLanguageDomainEntries">"Romanian"="ro";"Polish"="pl";</FCText>
    QRegularExpression matchingRE(QString::fromUtf8("\"(.*[^\\s]+.*)\"\\s*=\\s*\"([^\\s]+)\";?"));
    auto matches = matchingRE.globalMatch(QString::fromStdString(entries));
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString language = match.captured(1);
        QString tld = match.captured(2);
        d->mapLanguageTopLevelDomain[language.toStdString()] = tld.toStdString();
    }

    d->activatedLanguage = "English";

    d->paths = directories();

    enableDecimalPointConversion(hGrp->GetBool("SubstituteDecimalSeparator", false));
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
    for (const auto& it : locales)
        languages.push_back(it.first);

    return languages;
}

TStringMap Translator::supportedLocales() const
{
    if (!d->mapSupportedLocales.empty())
        return d->mapSupportedLocales;

    // List all .qm files
    for (const auto& domainMap : d->mapLanguageTopLevelDomain) {
        for (const auto& directoryName : qAsConst(d->paths)) {
            QDir dir(directoryName);
            QString filter = QString::fromLatin1("*_%1.qm").arg(QString::fromStdString(domainMap.second));
            QStringList fileNames = dir.entryList(QStringList(filter), QDir::Files, QDir::Name);
            if (!fileNames.isEmpty()) {
                d->mapSupportedLocales[domainMap.first] = domainMap.second;
                break;
            }
        }
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

void Translator::setLocale(const std::string& language) const
{
    auto loc = QLocale::system(); //Defaulting to OS locale
    if (language == "C" || language == "c") {
        loc = QLocale::c();
    }
    else {
        auto bcp47 = locale(language);
        if (!bcp47.empty())
            loc  = QLocale(QString::fromStdString(bcp47));
    }
    QLocale::setDefault(loc);
    updateLocaleChange();

#ifdef FC_DEBUG
    Base::Console().Log("Locale changed to %s => %s\n", qPrintable(loc.bcp47Name()), qPrintable(loc.name()));
#endif
}

void Translator::updateLocaleChange() const
{
    for (auto &topLevelWidget : qApp->topLevelWidgets()) {
        topLevelWidget->setLocale(QLocale());
    }
}

QStringList Translator::directories() const
{
    QStringList list;
    auto dir = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
        GetASCII("AdditionalTranslationsDirectory", "");
    if (!dir.empty())
        list.push_back(QString::fromStdString(dir));
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
    for (const auto &it : fileNames){
        bool ok=false;
        for (std::list<QTranslator*>::const_iterator tt = d->translators.begin();
            tt != d->translators.end(); ++tt) {
            if ((*tt)->objectName() == it) {
                ok = true; // this file is already installed
                break;
            }
        }

        // okay, we need to install this file
        if (!ok) {
            auto translator = new QTranslator;
            translator->setObjectName(it);
            if (translator->load(dir.filePath(it))) {
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
    for (const QString& it : d->paths) {
        QDir dir(it);
        installQMFiles(dir, tld->second.c_str());
    }
}

/**
 * Uninstalls all translators.
 */
void Translator::removeTranslators()
{
    for (QTranslator* it : d->translators) {
        qApp->removeTranslator(it);
        delete it;
    }

    d->translators.clear();
}

bool Translator::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() == QEvent::KeyPress || ev->type() == QEvent::KeyRelease) {
        QKeyEvent *kev = static_cast<QKeyEvent *>(ev);
        Qt::KeyboardModifiers mod = kev->modifiers();
        int key = kev->key();
        if ((mod & Qt::KeypadModifier) && (key == Qt::Key_Period || key == Qt::Key_Comma)) {
            if (ev->spontaneous()) {
                auto dp = QString(QLocale().decimalPoint());
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
                int dpcode = QKeySequence(dp)[0];
#else
                int dpcode = QKeySequence(dp)[0].key();
#endif
                if (kev->text() != dp) {
                    QKeyEvent modifiedKeyEvent(kev->type(), dpcode, mod, dp, kev->isAutoRepeat(), kev->count());
                    qApp->sendEvent(obj, &modifiedKeyEvent);
                    return true;
                }
            }
            if (dynamic_cast<Gui::TextEdit*>(obj) && key != Qt::Key_Period) {
                QKeyEvent modifiedKeyEvent(kev->type(), Qt::Key_Period, mod, QChar::fromLatin1('.'), kev->isAutoRepeat(), kev->count());
                qApp->sendEvent(obj, &modifiedKeyEvent);
                return true;
            }
        }
    }
    return false;
}

void Translator::enableDecimalPointConversion(bool on)
{
    if (!qApp) {
        return;
    }

    if (!on) {
        decimalPointConverter.reset();
        return;
    }
#if FC_DEBUG
    if (on && decimalPointConverter) {
        Base::Console().Instance().Warning("Translator: decimal point converter is already installed\n");
    }
#endif
    if (on && !decimalPointConverter) {
        decimalPointConverter = std::unique_ptr<Translator, std::function<void(Translator*)>>(this,
            [](Translator* evFilter) {
                qApp->removeEventFilter(evFilter);
            }
        );
        qApp->installEventFilter(decimalPointConverter.get());
    }
}

bool Translator::isEnabledDecimalPointConversion() const
{
    return static_cast<bool>(decimalPointConverter);
}

#include "moc_Translator.cpp"
