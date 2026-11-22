// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QRegularExpression>

#include "PseudoTranslator.h"

namespace
{
QString expandText(const QString& text)
{
    // Preserve syntax consumed by formatters, rich text, shortcuts and resource loaders.
    static const QRegularExpression tokens(QStringLiteral(
        R"(%(?:\([^)]+\))?)"              // Named argument: %(name)s
        R"((?:\d+\$)?)"                   // Argument position: %2$s
        R"([-+#0 ']*)"                    // Flags: %+d
        R"((?:\d+|\*(?:\d+\$)?)?)"        // Width: %*3$d
        R"((?:\.(?:\d*|\*(?:\d+\$)?))?)"  // Precision: %.2f
        R"((?:hh|ll|[hljztL])?)"          // Length modifier: %lld
        R"([diuoxXfFeEgGaAcspnrbB])"      // Conversion: %s
        R"(|%L?\d+)"                      // Qt argument: %L1
        R"(|%L?n)"                        // Qt plural count: %n
        R"(|%%)"                          // Escaped percent: %%
        R"(|\{(?:[^{}]|\{[^{}]*\})*\})"   // Brace field: {value:{width}}
        R"(|&\#\d+;)"                     // Decimal entity: &#123;
        R"(|&\#x[\da-fA-F]+;)"            // Hexadecimal entity: &#xAB;
        R"(|&[a-zA-Z][a-zA-Z\d]+;)"       // Named entity: &amp;
        R"(|&&)"                          // Escaped ampersand: &&
        R"(|&[^\s])"                      // Mnemonic: &O
        R"(|\b(?:(?:Ctrl|Alt|Shift|Meta)\+)+(?:Page (?:Up|Down)|[a-zA-Z\d]+|[^\s]))"  // Shortcut:
                                                                                      // Ctrl+Shift+O
        R"(|\bF(?:[1-9]|[12]\d|3[0-5])\b)"              // Function key: F12
        R"(|(?:https?://|mailto:|qrc:/|:/)[^\s<>"']+)"  // URL: https://freecad.org/help
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

}  // namespace

bool Gui::PseudoTranslator::isEmpty() const
{
    // Qt needs a nonempty translator to send LanguageChange events.
    return false;
}

QString Gui::PseudoTranslator::translate(
    const char* context,
    const char* sourceText,
    [[maybe_unused]] const char* disambiguation,
    [[maybe_unused]] int n
) const
{
    auto text = QString::fromUtf8(sourceText);
    const auto scope = QLatin1String(context);
    // These translations are machine-readable, not labels.
    if (scope == QLatin1String("QShortcut") || scope == QLatin1String("QKeySequence")
        || text == QLatin1String("QT_LAYOUT_DIRECTION")) {
        return text;
    }

    // Leave markup, embedded code and file-filter syntax outside the markers.
    static const QRegularExpression markup(
        QStringLiteral(
            // Complete style/script elements, including their contents.
            R"(<(?:style|script)\b[^>]*>[\s\S]*?</(?:style|script)\s*>)"
            // HTML comments and tags, allowing quoted angle brackets in attributes.
            R"(|<!--[\s\S]*?-->|</?[a-zA-Z](?:[^<>"']|"[^"]*"|'[^']*')*>)"
            // Declarations, processing instructions, file patterns and filter separators.
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
