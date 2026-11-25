// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QTest>
#include <QTextBlock>
#include <QTextLayout>

#include <Gui/Application.h>
#include <Gui/ExpressionCompleter.h>
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

namespace
{
// Returns the highlighter format applied at the given position of the first block.
QTextCharFormat formatAt(const Gui::ExpressionTextEdit& edit, int position)
{
    const QTextBlock block = edit.document()->firstBlock();
    for (const auto& range : block.layout()->formats()) {
        if (position >= range.start && position < range.start + range.length) {
            return range.format;
        }
    }
    return {};
}

// Returns the foreground color applied at the given position, if any.
QColor colorAt(const Gui::ExpressionTextEdit& edit, int position)
{
    const QTextCharFormat format = formatAt(edit, position);
    return format.hasProperty(QTextFormat::ForegroundBrush) ? format.foreground().color() : QColor();
}

bool isBoldAt(const Gui::ExpressionTextEdit& edit, int position)
{
    return formatAt(edit, position).fontWeight() == QFont::Bold;
}
}  // namespace

class testExpressionTextEdit: public QObject
{
    Q_OBJECT

public:
    testExpressionTextEdit()
    {
        tests::initApplication();
        if (!Gui::Application::Instance) {
            new Gui::Application(/*GUIenabled=*/false);
        }
    }

private Q_SLOTS:
    void test_TokensAreColoredByKind()  // NOLINT
    {
        Gui::ExpressionTextEdit edit;
        edit.setPlainText(QStringLiteral("sin(2 mm) + Box.Length"));

        const QColor function = colorAt(edit, 0);
        const QColor number = colorAt(edit, 4);
        const QColor unit = colorAt(edit, 6);
        const QColor plus = colorAt(edit, 10);

        QVERIFY(function.isValid());
        QVERIFY(number.isValid());
        QVERIFY(unit.isValid());
        QVERIFY(plus.isValid());
        QVERIFY(function != number);
        QVERIFY(number != unit);
        QCOMPARE(colorAt(edit, 3), colorAt(edit, 8));  // '(' of a function call matches ')'
        QVERIFY(colorAt(edit, 3) != function);
        QVERIFY(!colorAt(edit, 12).isValid());  // identifiers keep the default text color
        QVERIFY(isBoldAt(edit, 12));            // 'Box'
        QVERIFY(isBoldAt(edit, 16));            // 'Length'
        QVERIFY(!isBoldAt(edit, 15));           // '.'
        QVERIFY(!isBoldAt(edit, 4));            // numbers are not names
    }

    void test_NonAsciiTextKeepsPositions()  // NOLINT
    {
        Gui::ExpressionTextEdit edit;
        edit.setPlainText(QStringLiteral(u"Länge + 2"));

        QVERIFY(!colorAt(edit, 1).isValid());
        QVERIFY(isBoldAt(edit, 4));            // last letter of 'Länge'
        QVERIFY(!isBoldAt(edit, 5));           // space after it
        QVERIFY(colorAt(edit, 6).isValid());   // '+'
        QVERIFY(colorAt(edit, 8).isValid());   // '2'
        QVERIFY(!colorAt(edit, 7).isValid());  // space between them
    }
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testExpressionTextEdit)

#include "ExpressionTextEdit.moc"
