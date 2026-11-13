// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>

#include <QApplication>
#include <QEvent>
#include <QFileDialog>
#include <QKeySequence>
#include <QLocale>
#include <QStringList>
#include <QTest>
#include <QTextDocument>

#include <App/Application.h>
#include <Base/NumericFormatting.h>

#include "Gui/Language/Translator.h"
#include "Gui/NumericLocale.h"
#include <src/LocaleTestHelpers.h>
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

namespace
{
QString translated(const QString& source)
{
    const auto utf8 = source.toUtf8();
    return QCoreApplication::translate("PseudoTest", utf8.constData());
}

class LanguageEvents final: public QObject
{
public:
    int count() const
    {
        return changes;
    }

private:
    bool eventFilter(QObject*, QEvent* event) override
    {
        if (event->type() == QEvent::LanguageChange) {
            ++changes;
        }
        return false;
    }

    int changes {0};
};
}  // namespace

class testTranslatorLocale: public QObject
{
    Q_OBJECT

public:
    testTranslatorLocale()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void init()
    {
        previousLanguage = Gui::Translator::instance()->activeLanguage();
    }

    void cleanup()
    {
        Gui::Translator::instance()->activateLanguage(previousLanguage.c_str());
    }

    void pseudoLifecycle()
    {
        auto* translator = Gui::Translator::instance();
        translator->activateLanguage("English");
        QCOMPARE(translated("Hello"), QStringLiteral("Hello"));

        LanguageEvents events;
        qApp->installEventFilter(&events);
        translator->activateLanguage("English (Pseudo A)");
        QVERIFY(events.count() > 0);
        const auto pseudo = translated("Hello");
        QVERIFY(pseudo != QStringLiteral("Hello"));
        const int changes = events.count();

        // Workbench loading refreshes translations without reinstalling them.
        translator->refresh();
        translator->refresh();
        QCOMPARE(events.count(), changes);
        QCOMPARE(translated("Hello"), pseudo);
        QCOMPARE(QCoreApplication::translate("UncataloguedWorkbench", "Hello"), pseudo);

        translator->activateLanguage("English");
        QVERIFY(events.count() > changes);
        QCOMPARE(translated("Hello"), QStringLiteral("Hello"));
        translator->activateLanguage("English (Pseudo A)");
        QCOMPARE(translated("Hello"), pseudo);

        QCOMPARE(QCoreApplication::translate("QShortcut", "Ctrl"), QStringLiteral("Ctrl"));
        QCOMPARE(QCoreApplication::translate("QKeySequence", "Shift"), QStringLiteral("Shift"));
        QCOMPARE(translated("QT_LAYOUT_DIRECTION"), QStringLiteral("QT_LAYOUT_DIRECTION"));
    }

    void pseudoText_data()
    {
        QTest::addColumn<QString>("source");
        QTest::addColumn<QStringList>("preserved");
        QTest::addColumn<bool>("expanded");

        QTest::newRow("plain") << QStringLiteral("Hello") << QStringList {} << true;
        QTest::newRow("whitespace") << QStringLiteral(" \tHello \n") << QStringList {} << true;
        QTest::newRow("mixed-unicode") << QString::fromUtf8("Hello 日本語 Ω")
                                       << QStringList {QString::fromUtf8("日本語 Ω")} << true;
        QTest::newRow("mnemonic") << QStringLiteral("&Open") << QStringList {"&O"} << true;

        auto unchanged = [](const char* name, const QString& source) {
            QTest::newRow(name) << source << QStringList {} << false;
        };
        unchanged("empty", "");
        unchanged("only-whitespace", " \t\n");
        unchanged("numbers", "123 + 4.5");
        unchanged("punctuation", "!? () []");
        unchanged("unicode", QString::fromUtf8("日本語 Ω"));
        unchanged("shortcut", "Alt+A");
        unchanged("shortcut-chord", "Ctrl+Shift+O");
        unchanged("shortcut-sequence", "Ctrl+K, Ctrl+C");
        unchanged("function-key", "F12");

        for (const auto& token : QStringList {
                 "%1",
                 "%99",
                 "%L1",
                 "%n",
                 "%Ln",
                 "%s",
                 "%d",
                 "%r",
                 "%1.2f",
                 "%08.3lf",
                 "%2$.*f",
                 "%2$*3$.*4$f",
                 "%(name)s",
                 "%%",
                 "{}",
                 "{name}",
                 "{name!r:>10}",
                 "{value:{width}.{precision}f}",
                 "{{name}}",
                 "&amp;",
                 "&#123;",
                 "&#xAB;",
                 "&&",
                 "Alt+A",
                 "Ctrl+Shift+O",
                 "F1",
                 "https://freecad.org/Help",
                 ":/icons/edit.svg",
                 "mailto:info@example.org"
             }) {
            QTest::newRow(qPrintable(token))
                << QStringLiteral("Hello %1 world").arg(token) << QStringList {token} << true;
        }
    }

    void pseudoText()
    {
        QFETCH(QString, source);
        QFETCH(QStringList, preserved);
        QFETCH(bool, expanded);
        Gui::Translator::instance()->activateLanguage("English (Pseudo A)");
        const auto result = translated(source);

        for (const auto& token : preserved) {
            QCOMPARE(result.count(token), source.count(token));
        }
        if (!expanded) {
            QCOMPARE(result, source);
            return;
        }

        QVERIFY(result != source);
        QVERIFY(result.size() > source.size() + 2);
        QVERIFY(std::any_of(result.cbegin(), result.cend(), [](QChar ch) {
            return ch.unicode() > 127;
        }));
        QVERIFY(result.trimmed().startsWith('['));
        QVERIFY(result.trimmed().endsWith(']'));
        const auto first = source.indexOf(source.trimmed());
        const auto last = first + source.trimmed().size();
        QCOMPARE(result.left(first), source.left(first));
        QCOMPARE(result.right(source.size() - last), source.mid(last));
        QCOMPARE(QKeySequence::mnemonic(result), QKeySequence::mnemonic(source));
        if (preserved.contains(QStringLiteral("%1"))) {
            QVERIFY(result.arg(QStringLiteral("model.FCStd")).contains(QStringLiteral("model.FCStd")));
        }
    }

    void pseudoNumbers_data()
    {
        QTest::addColumn<QString>("placeholder");
        QTest::addColumn<int>("number");
        for (const auto& placeholder : {QStringLiteral("%n"), QStringLiteral("%Ln")}) {
            for (const int number : {0, 1, 12, 12345}) {
                QTest::newRow(qPrintable(placeholder + QString::number(number)))
                    << placeholder << number;
            }
        }
    }

    void pseudoNumbers()
    {
        QFETCH(QString, placeholder);
        QFETCH(int, number);
        tests::ScopedLocaleEnvironment localeEnvironment {{.qtLocale = "en_US"}};
        Gui::Translator::instance()->activateLanguage("English (Pseudo A)");
        const auto source = QStringLiteral("Hello %1 world").arg(placeholder).toUtf8();
        const auto result
            = QCoreApplication::translate("PseudoTest", source.constData(), nullptr, number);
        const auto digits = placeholder == QStringLiteral("%Ln") ? QLocale().toString(number)
                                                                 : QString::number(number);

        QVERIFY(result.contains(digits));
        QVERIFY(!result.contains(placeholder));
        QVERIFY(result.startsWith('['));
        QVERIFY(result.endsWith(']'));
    }

    void pseudoHtml_data()
    {
        QTest::addColumn<QString>("source");
        QTest::addColumn<QStringList>("preserved");
        QTest::addColumn<QString>("tail");
        QTest::newRow("inline") << QStringLiteral("<p><b>Hello</b></p>")
                                << QStringList {"<p>", "<b>", "</b>", "</p>"} << QString {};
        QTest::newRow("attributes")
            << QStringLiteral("<a href=\"https://freecad.org/help\" title=\"a > b\">Hello</a>")
            << QStringList {"<a href=\"https://freecad.org/help\" title=\"a > b\">", "</a>"}
            << QString {};
        QTest::newRow("entities") << QStringLiteral("<p>Hello</p><p>&amp; &#169; &#x3A9;</p>")
                                  << QStringList {"<p>", "</p>", "&amp;", "&#169;", "&#x3A9;"}
                                  << QString::fromUtf8("\n& © Ω");
        QTest::newRow("css") << QStringLiteral(
            "<html><head><style>p { color: red; }</style></head>"
            "<body><p>Hello</p></body></html>"
        ) << QStringList {"<html><head><style>p { color: red; }</style></head><body><p>", "</p></body></html>"}
                             << QString {};
        QTest::newRow("comments") << QStringLiteral(
            "<!DOCTYPE html><!--Keep--><html><body><p>Hello</p></body></html>"
        ) << QStringList {"<!DOCTYPE html><!--Keep--><html><body><p>", "</p></body></html>"}
                                  << QString {};
    }

    void pseudoHtml()
    {
        QFETCH(QString, source);
        QFETCH(QStringList, preserved);
        QFETCH(QString, tail);
        Gui::Translator::instance()->activateLanguage("English (Pseudo A)");
        const auto result = translated(source);
        for (const auto& part : preserved) {
            QCOMPARE(result.count(part), source.count(part));
        }

        QTextDocument document;
        document.setHtml(result);
        QCOMPARE(document.toPlainText(), translated("Hello") + tail);
    }

    void pseudoFilters_data()
    {
        QTest::addColumn<QString>("source");
        QTest::addColumn<QStringList>("patterns");
        QTest::newRow("document") << QStringLiteral("FreeCAD (*.FCStd)") << QStringList {"*.FCStd"};
        QTest::newRow("several-patterns")
            << QStringLiteral("Images (*.svg *.png *.jpg)") << QStringList {"*.svg *.png *.jpg"};
        QTest::newRow("several-filters")
            << QStringLiteral("FreeCAD (*.FCStd *.FCStd?);;All files (*)")
            << QStringList {"*.FCStd *.FCStd?", "*"};
        QTest::newRow("all-files") << QStringLiteral("All files (*.*)") << QStringList {"*.*"};
        QTest::newRow("single-character")
            << QStringLiteral("Images (?.png)") << QStringList {"?.png"};
    }

    void pseudoFilters()
    {
        QFETCH(QString, source);
        QFETCH(QStringList, patterns);
        Gui::Translator::instance()->activateLanguage("English (Pseudo A)");
        QFileDialog dialog;
        dialog.setOption(QFileDialog::DontUseNativeDialog);
        dialog.setNameFilter(translated(source));
        const auto filters = dialog.nameFilters();
        QCOMPARE(filters.size(), patterns.size());
        for (qsizetype i = 0; i < patterns.size(); ++i) {
            QVERIFY(filters.at(i).startsWith('['));
            QCOMPARE(
                filters.at(i).mid(filters.at(i).lastIndexOf(" (")),
                QStringLiteral(" (%1)").arg(patterns.at(i))
            );
        }
    }

    void pseudoLocale()
    {
        auto* translator = Gui::Translator::instance();
        const auto locales = translator->supportedLocales();
        const auto entry = locales.find("English (Pseudo A)");
        QVERIFY(entry != locales.end());
        QCOMPARE(entry->second, std::string("en-XA"));
        QCOMPARE(translator->locale("English (Pseudo A)"), std::string("en-XA"));

        tests::ScopedLocaleEnvironment localeEnvironment {
            {.qtLocale = "de_DE", .formattingLocale = "de_DE", .icuLocale = "de_DE"}
        };
        translator->setLocale("English (Pseudo A)");
        const auto context = Base::currentNumericLocaleContext();
        QCOMPARE(QLocale().name(), QStringLiteral("en_US"));
        QCOMPARE(context.localeId, std::string("en_US"));
        QCOMPARE(context.decimalSeparator, std::string("."));
        QCOMPARE(context.groupingSeparator, std::string(","));
        QCOMPARE(QString::fromLatin1(icu::Locale::getDefault().getName()), QStringLiteral("en_US"));
    }

    void test_NumericLocaleContextRealignsMismatchedQtAndIcuDefaults()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeEnvironment {
            {.qtLocale = "da_DK", .formattingLocale = "en_US", .icuLocale = "fr_FR"}
        };

        QCOMPARE(QLocale().name(), QStringLiteral("da_DK"));
        QCOMPARE(QString::fromLatin1(icu::Locale::getDefault().getName()), QStringLiteral("fr_FR"));
        QCOMPARE(
            QString::fromStdString(Base::currentNumericLocaleContext().localeId),
            QStringLiteral("en_US")
        );

        Gui::Translator::instance()->setLocale("German");
        const auto formattingState = Base::currentNumericLocaleContext();

        QCOMPARE(QLocale().name(), QStringLiteral("de_DE"));
        QCOMPARE(QLocale().decimalPoint(), QChar(','));
        QCOMPARE(QLocale().groupSeparator(), QChar('.'));
        QCOMPARE(QString::fromStdString(formattingState.localeId), QStringLiteral("de_DE"));
        QCOMPARE(QString::fromStdString(formattingState.decimalSeparator), QStringLiteral(","));
        QCOMPARE(QString::fromStdString(formattingState.groupingSeparator), QStringLiteral("."));
        QCOMPARE(QString::fromStdString(formattingState.positiveSign), QStringLiteral("+"));
        QCOMPARE(QString::fromStdString(formattingState.negativeSign), QStringLiteral("-"));
        QCOMPARE(formattingState.primaryGroupingSize, 3);
        QCOMPARE(formattingState.secondaryGroupingSize, 3);
        QCOMPARE(QString::fromLatin1(icu::Locale::getDefault().getName()), QStringLiteral("de_DE"));
    }

    void test_WidgetLocaleContextIsCompleteAndIndependentOfPublishedState()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeEnvironment {
            {.qtLocale = "da_DK", .formattingLocale = "fr_FR", .icuLocale = "fr_FR"}
        };
        const QLocale widgetLocale(QStringLiteral("en_IN"));

        const auto context = Gui::numericLocaleContextFor(widgetLocale);

        QCOMPARE(QString::fromStdString(context.localeId), QStringLiteral("en_IN"));
        QCOMPARE(QString::fromStdString(context.decimalSeparator), QString(widgetLocale.decimalPoint()));
        QCOMPARE(
            QString::fromStdString(context.groupingSeparator),
            QString(widgetLocale.groupSeparator())
        );
        QCOMPARE(QString::fromStdString(context.positiveSign), QString(widgetLocale.positiveSign()));
        QCOMPARE(QString::fromStdString(context.negativeSign), QString(widgetLocale.negativeSign()));
        QCOMPARE(context.primaryGroupingSize, 3);
        QCOMPARE(context.secondaryGroupingSize, 2);
        QVERIFY(context != Base::currentNumericLocaleContext());
    }

    void test_OperatingSystemFormattingUsesSystemLocaleSeparators()  // NOLINT
    {
        auto general = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/General"
        );
        const int previousPreference = general->GetInt("UseLocaleFormatting", 0);

        general->SetInt(
            "UseLocaleFormatting",
            static_cast<int>(Gui::Translator::LocaleFormattingPreference::OperatingSystem)
        );
        Gui::Translator::instance()->applyLocaleFormattingPreference();

        const auto systemLocale = QLocale::system();
        const auto formatting = Base::currentNumericLocaleContext();
        QCOMPARE(
            QString::fromStdString(formatting.decimalSeparator),
            QString(systemLocale.decimalPoint())
        );
        QCOMPARE(
            QString::fromStdString(formatting.groupingSeparator),
            QString(systemLocale.groupSeparator())
        );

        general->SetInt("UseLocaleFormatting", previousPreference);
    }

private:
    std::string previousLanguage;
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testTranslatorLocale)

#include "TranslatorLocale.moc"
