// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <algorithm>
#include <QApplication>
#include <QDir>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QStringList>
#include <QTranslator>
#include <QWidget>

#include <utility>

#include <Base/NumericFormatting.h>

#include <App/Application.h>
#include <Gui/TextEdit.h>
#include "../NumericLocale.h"
#include "Translator.h"

using namespace Gui;

namespace
{
constexpr auto pseudoLanguage = "English (Pseudo A)";
constexpr auto pseudoLocale = "en-XA";

QString expandText(const QString& text)
{
    // Preserve syntax consumed by formatters, rich text, shortcuts and resource loaders.
    static const QRegularExpression tokens(QStringLiteral(
        R"(%(?:\([^)]+\))?(?:\d+\$)?[-+#0 ']*(?:\d+|\*(?:\d+\$)?)?)"
        R"((?:\.(?:\d+|\*(?:\d+\$)?))?(?:hh|ll|[hljztL])?[diuoxXfFeEgGaAcspnrbB])"
        R"(|%(?:L?(?:\d+|n)|%)|\{(?:[^{}]|\{[^{}]*\})*\})"
        R"(|&(?:\#\d+;|\#x[\da-fA-F]+;|[a-zA-Z][a-zA-Z\d]+;|&|[^\s]))"
        R"(|\b(?:(?:Ctrl|Alt|Shift|Meta)\+)+(?:Page (?:Up|Down)|[a-zA-Z\d]+|[^\s]))"
        R"(|\bF(?:[1-9]|[12]\d|3[0-5])\b|(?:https?://|mailto:|:/)[^\s<>"']+)"
    ));
    static const QString letters = QStringLiteral(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
    );
    static const QString accents = QStringLiteral(
        "ÁƁÇÐÉƑĜĤÍĴĶĻḾŃÓṔǪŔŠŢÚṼŴẊÝŽáƀçðéƒĝĥíĵķļḿńóṕǫŕšţúṽŵẋýž"
    );
    static const QString vowels = QStringLiteral("AEIOUaeiou");

    QString result;
    bool changed = false;
    qsizetype offset = 0;
    auto matches = tokens.globalMatch(text);
    auto append = [&](qsizetype end) {
        for (; offset < end; ++offset) {
            const auto ch = text.at(offset);
            const auto index = letters.indexOf(ch);
            if (index < 0) {
                result += ch;
                continue;
            }

            result += accents.at(index);
            if (vowels.contains(ch)) {
                result += accents.at(index);
            }
            changed = true;
        }
    };

    while (matches.hasNext()) {
        const auto match = matches.next();
        append(match.capturedStart());
        result += match.captured();
        offset = match.capturedEnd();
    }
    append(text.size());
    if (!changed) {
        return text;
    }

    // Keep boundary whitespace outside the markers for concatenated labels.
    qsizetype first = 0;
    qsizetype last = result.size();
    while (first < last && result.at(first).isSpace()) {
        ++first;
    }
    while (last > first && result.at(last - 1).isSpace()) {
        --last;
    }
    result.insert(last, QLatin1Char(']'));
    result.insert(first, QLatin1Char('['));
    return result;
}

class PseudoTranslator final: public QTranslator
{
    bool isEmpty() const override
    {
        // Qt needs a nonempty translator to send LanguageChange events.
        return false;
    }

    QString translate(const char* context, const char* sourceText, const char*, int) const override
    {
        const auto text = QString::fromUtf8(sourceText);
        const auto scope = QLatin1String(context);
        // These translations are machine-readable, not labels.
        if (scope == QLatin1String("QShortcut") || scope == QLatin1String("QKeySequence")
            || text == QLatin1String("QT_LAYOUT_DIRECTION")) {
            return text;
        }

        // Leave markup, embedded code and file-filter syntax outside the markers.
        static const QRegularExpression markup(
            QStringLiteral(
                R"(<(?:style|script)\b[^>]*>[\s\S]*?</(?:style|script)\s*>)"
                R"(|<!--[\s\S]*?-->|</?[a-zA-Z](?:[^<>"']|"[^"]*"|'[^']*')*>)"
                R"(|<![^>]*>|<\?[^>]*\?>|\((?:[*?][^\s()]*\s*)+\)|;;)"
            ),
            QRegularExpression::CaseInsensitiveOption
        );

        QString result;
        qsizetype offset = 0;
        auto matches = markup.globalMatch(text);
        while (matches.hasNext()) {
            const auto match = matches.next();
            result += expandText(text.mid(offset, match.capturedStart() - offset));
            result += match.captured();
            offset = match.capturedEnd();
        }
        result += expandText(text.mid(offset));
        return result;
    }
};

Translator::LocaleFormattingPreference toLocaleFormattingPreference(const int format)
{
    switch (format) {
        case static_cast<int>(Translator::LocaleFormattingPreference::OperatingSystem):
            return Translator::LocaleFormattingPreference::OperatingSystem;
        case static_cast<int>(Translator::LocaleFormattingPreference::SelectedLanguage):
            return Translator::LocaleFormattingPreference::SelectedLanguage;
        case static_cast<int>(Translator::LocaleFormattingPreference::CLocale):
            return Translator::LocaleFormattingPreference::CLocale;
        default:
            throw Base::ValueError(
                "Parameter \"UseLocaleFormatting\" value out of bounds for "
                "Translator::formattingOptions"
            );
    }
}

struct ResolvedNumericLocale
{
    QLocale qtLocale;
    Base::NumericLocaleContext numericContext;
};

ResolvedNumericLocale resolveNumericLocale(const Translator& translator, const std::string& language)
{
    QLocale qtLocale;
    if (Base::isCLocaleName(language)) {
        qtLocale = QLocale::c();
    }
    else {
        qtLocale = QLocale::system();

        if (!language.empty()) {
            const std::string localeName = translator.locale(language);

            if (Base::isCLocaleName(localeName)) {
                qtLocale = QLocale::c();
            }
            else if (!localeName.empty()) {
                const QLocale candidate(
                    QString::fromStdString(localeName == pseudoLocale ? "en_US" : localeName)
                );
                if (candidate.language() != QLocale::C) {
                    qtLocale = candidate;
                }
            }
        }
    }

    return {qtLocale, Gui::numericLocaleContextFor(qtLocale)};
}
}  // namespace

/** \defgroup i18n Internationalization with FreeCAD
 *  \ingroup GUI
 *
 * The internationalization of FreeCAD makes heavy use of the internationalization
 * support of Qt. For more details refer to your Qt documentation.
 *
 * \section stepbystep_language Step by step
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
 * This is an XML file where you can append the .qm file. For the .qrc file you have to define the
 * following curstom build step inside the Visual Studio project file:
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

namespace Gui
{
class TranslatorP
{
public:
    std::string activatedLanguage; /**< Active language */
    std::map<std::string, std::string> mapLanguageTopLevelDomain;
    TStringMap mapSupportedLocales;
    std::vector<QTranslator*> translators; /**< A list of all created translators */
    QStringList paths;
};
}  // namespace Gui

class Translator::ParameterObserver: public ParameterGrp::ObserverType
{
public:
    ParameterObserver(Translator* client)
        : client(client)
    {
        hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/General"
        );
        hGrp->Attach(this);
    }

    void OnChange(Base::Subject<const char*>& caller, const char* creason) override
    {
        (void)caller;

        std::string_view reason = creason;
        if (reason == "UseLocaleFormatting") {
            client->applyLocaleFormattingPreference();
        }
        else if (reason == "SubstituteDecimalSeparator") {
            bool value = hGrp->GetBool("SubstituteDecimalSeparator");
            client->enableDecimalPointConversion(value);
        }
    }

    Translator* client;
    static ParameterGrp::handle hGrp;
};
ParameterGrp::handle Translator::ParameterObserver::hGrp;  // definition for export

Translator* Translator::instance()
{
    if (!_pcSingleton) {
        _pcSingleton = new Translator;
    }
    return _pcSingleton;
}

void Translator::destruct()
{
    if (_pcSingleton) {
        delete _pcSingleton;
    }
    _pcSingleton = nullptr;
}

Translator::Translator()
{
    observer = std::make_unique<Translator::ParameterObserver>(this);

    // This is needed for Qt's lupdate
    // clang-format off
    d = new TranslatorP;
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Afrikaans"             )] = "af";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Arabic"                )] = "ar";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Basque"                )] = "eu";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Belarusian"            )] = "be";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Bulgarian"             )] = "bg";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Catalan"               )] = "ca";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Chinese (Simplified)"  )] = "zh-CN";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Chinese (Traditional)" )] = "zh-TW";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Croatian"              )] = "hr";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Czech"                 )] = "cs";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Danish"                )] = "da";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Dutch"                 )] = "nl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("English"               )] = "en";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("English (Pseudo A)"    )] = pseudoLocale;
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Filipino"              )] = "fil";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Finnish"               )] = "fi";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("French"                )] = "fr";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Galician"              )] = "gl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Georgian"              )] = "ka";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("German"                )] = "de";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Greek"                 )] = "el";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Hungarian"             )] = "hu";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Indonesian"            )] = "id";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Italian"               )] = "it";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Japanese"              )] = "ja";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Kabyle"                )] = "kab";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Korean"                )] = "ko";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Lithuanian"            )] = "lt";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Norwegian"             )] = "no";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Polish"                )] = "pl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Portuguese (Brazilian)")] = "pt-BR";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Portuguese"            )] = "pt-PT";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Romanian"              )] = "ro";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Russian"               )] = "ru";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Serbian"               )] = "sr";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Serbian (Latin)"       )] = "sr-CS";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Slovak"                )] = "sk";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Slovenian"             )] = "sl";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Spanish"               )] = "es-ES";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Spanish (Argentina)"   )] = "es-AR";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Swedish"               )] = "sv";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Turkish"               )] = "tr";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Ukrainian"             )] = "uk";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Valencian"             )] = "val-ES";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Vietnamese"            )] = "vi";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Malay")] = "ms";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Tamil")] = "ta";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Irish")] = "ga-IE";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Lao")] = "lo";
    d->mapLanguageTopLevelDomain[QT_TR_NOOP("Hebrew")] = "he";

    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General");
    auto entries = hGrp->GetASCII("AdditionalLanguageDomainEntries", "");
    // The format of the entries is "Language Name 1"="code1";"Language Name 2"="code2";...
    // Example: <FCText Name="AdditionalLanguageDomainEntries">"Romanian"="ro";"Polish"="pl";</FCText>
    QRegularExpression matchingRE(QStringLiteral("\"(.*[^\\s]+.*)\"\\s*=\\s*\"([^\\s]+)\";?"));
    auto matches = matchingRE.globalMatch(QString::fromStdString(entries));
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString language = match.captured(1);
        QString tld = match.captured(2);
        d->mapLanguageTopLevelDomain[language.toStdString()] = tld.toStdString();
    }
    // clang-format on

    d->activatedLanguage = "English";

    d->paths = directories();

    enableDecimalPointConversion(hGrp->GetBool("SubstituteDecimalSeparator", false));
}

Translator::~Translator()
{
    removeTranslators();
    delete d;
}

TLanguageList Translator::supportedLanguages() const
{
    TLanguageList languages;
    const TStringMap locales = supportedLocales();
    languages.reserve(locales.size());
    for (const auto& it : locales) {
        languages.emplace_back(it.first);
    }
    return languages;
}

TStringMap Translator::supportedLocales() const
{
    if (!d->mapSupportedLocales.empty()) {
        return d->mapSupportedLocales;
    }

    d->mapSupportedLocales.emplace(pseudoLanguage, pseudoLocale);

    // List all *_*.qm files, and if any match a known locale,
    // report that locale as supported.
    const QStringList qmFilter(QStringLiteral("*_*.qm"));
    for (const auto& directoryName : std::as_const(d->paths)) {
        const QDir dir(directoryName);
        const QStringList fileNames = dir.entryList(qmFilter, QDir::Files);
        for (const auto& file : fileNames) {
            const auto lang
                = file.mid(file.lastIndexOf('_') + 1).chopped(sizeof(".qm") - 1).toStdString();
            for (const auto& domainMap : d->mapLanguageTopLevelDomain) {
                if (lang == domainMap.second) {
                    // Emplace only inserts if no element exists at the key yet,
                    // avoiding string copies here.
                    d->mapSupportedLocales.emplace(domainMap.first, domainMap.second);
                }
            }
        }
    }

    return d->mapSupportedLocales;
}

void Translator::activateLanguage(const char* lang)
{
    removeTranslators();  // remove the currently installed translators
    d->activatedLanguage = lang;
    const TLanguageList languages = supportedLanguages();
    if (std::ranges::find(languages, lang) != languages.end()) {
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
    if (tld != d->mapLanguageTopLevelDomain.end()) {
        loc = tld->second;
    }

    return loc;
}

void Translator::applyLocaleFormattingPreference() const
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/General"
    );
    const auto format = toLocaleFormattingPreference(hGrp->GetInt("UseLocaleFormatting", 0));
    switch (format) {
        case LocaleFormattingPreference::OperatingSystem:
            setLocale();  // Defaults to system locale.
            break;
        case LocaleFormattingPreference::SelectedLanguage:
            // Language must be activated before locale changes can follow it.
            setLocale(hGrp->GetASCII("Language", activeLanguage().c_str()));
            break;
        case LocaleFormattingPreference::CLocale:
            setLocale("C");
            break;
    }
}

void Translator::setLocale(const std::string& language) const
{
    // Resolve Qt and the complete numeric-locale context from the same source
    // so quantity formatting can match what Qt widgets display.
    const auto resolved = resolveNumericLocale(*this, language);
    auto nextContext = resolved.numericContext;
#ifdef FC_DEBUG
    const auto previousState = Base::currentNumericLocaleContext();
    const bool localeChanged = previousState != nextContext;
#endif
    // Complete the fallible ICU operation before publishing Qt and FreeCAD's context. A failure
    // leaves all three locale consumers unchanged.
    Base::setIcuDefaultLocale(nextContext.localeId);
    QLocale::setDefault(resolved.qtLocale);
    Base::publishNumericLocaleContext(std::move(nextContext));
    updateLocaleChange();

#ifdef FC_DEBUG
    if (localeChanged) {
        const QByteArray bcp47Name = resolved.qtLocale.bcp47Name().toUtf8();
        const QByteArray localeName = resolved.qtLocale.name().toUtf8();
        Base::Console()
            .log("Locale changed to {} => {}\n", bcp47Name.constData(), localeName.constData());
    }
#endif
}

void Translator::updateLocaleChange() const
{
    for (auto& topLevelWidget : qApp->topLevelWidgets()) {
        topLevelWidget->setLocale(QLocale());
    }
}

QStringList Translator::directories() const
{
    QStringList list;
    auto dir = App::GetApplication()
                   .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
                   ->GetASCII("AdditionalTranslationsDirectory", "");
    if (!dir.empty()) {
        list.push_back(QString::fromStdString(dir));
    }
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
    const QString filter = QStringLiteral("*_%1.qm").arg(QLatin1String(locale));
    const QStringList fileNames = dir.entryList(QStringList(filter), QDir::Files, QDir::Name);
    d->translators.reserve(fileNames.size());
    for (const auto& it : fileNames) {
        bool ok = false;
        for (const auto translator : d->translators) {
            if (translator->objectName() == it) {
                ok = true;  // this file is already installed
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
 * This method checks for newly added (internal) .qm files which might be added at runtime. This
 * e.g. happens if a plugin gets loaded at runtime. For each newly added files that supports the
 * currently set language a new translator object is created to load the file.
 */
void Translator::refresh()
{
    if (d->activatedLanguage == pseudoLanguage) {
        if (d->translators.empty()) {
            auto translator = new PseudoTranslator;
            d->translators.push_back(translator);
            qApp->installTranslator(translator);
        }
        return;
    }

    std::map<std::string, std::string>::iterator tld = d->mapLanguageTopLevelDomain.find(
        d->activatedLanguage
    );
    if (tld == d->mapLanguageTopLevelDomain.end()) {
        return;  // no language activated
    }
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
        QKeyEvent* kev = static_cast<QKeyEvent*>(ev);
        Qt::KeyboardModifiers mod = kev->modifiers();
        int key = kev->key();
        if ((mod & Qt::KeypadModifier) && (key == Qt::Key_Period || key == Qt::Key_Comma)) {
            if (ev->spontaneous()) {
                auto dp = QString(QLocale().decimalPoint());
                int dpcode = QKeySequence(dp)[0].key();
                if (kev->text() != dp) {
                    QKeyEvent modifiedKeyEvent(
                        kev->type(),
                        dpcode,
                        mod,
                        dp,
                        kev->isAutoRepeat(),
                        kev->count()
                    );
                    qApp->sendEvent(obj, &modifiedKeyEvent);
                    return true;
                }
            }
            if (dynamic_cast<Gui::TextEdit*>(obj) && key != Qt::Key_Period) {
                QKeyEvent modifiedKeyEvent(
                    kev->type(),
                    Qt::Key_Period,
                    mod,
                    QChar::fromLatin1('.'),
                    kev->isAutoRepeat(),
                    kev->count()
                );
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
        Base::Console().instance().warning(
            "Translator: decimal point converter is already installed\n"
        );
    }
#endif
    if (on && !decimalPointConverter) {
        decimalPointConverter = std::unique_ptr<Translator, std::function<void(Translator*)>>(
            this,
            [](Translator* evFilter) { qApp->removeEventFilter(evFilter); }
        );
        qApp->installEventFilter(decimalPointConverter.get());
    }
}

bool Translator::isEnabledDecimalPointConversion() const
{
    return static_cast<bool>(decimalPointConverter);
}

#include "moc_Translator.cpp"
