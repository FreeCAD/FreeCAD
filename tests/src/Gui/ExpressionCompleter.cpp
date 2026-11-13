// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QLineEdit>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/ExpressionCompleter.h>
#include <src/App/InitApplication.h>

class testExpressionCompleter: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
    }

    void unitNamedProperties_data()
    {
        QTest::addColumn<QString>("prefix");
        QTest::addColumn<QString>("expected");
        QTest::addColumn<bool>("fuzzy");
        for (bool fuzzy : {false, true}) {
            const QByteArray mode = fuzzy ? "fuzzy-" : "prefix-";
            QTest::newRow((mode + "local").constData())
                << QStringLiteral("N") << QStringLiteral(".N") << fuzzy;
            QTest::newRow((mode + "explicit-local").constData())
                << QStringLiteral(".N") << QStringLiteral(".N") << fuzzy;
            QTest::newRow((mode + "qualified").constData())
                << QStringLiteral("VarSet.N") << QStringLiteral("VarSet.N") << fuzzy;
            QTest::newRow((mode + "us-unit").constData())
                << QStringLiteral("ft") << QStringLiteral(".ft") << fuzzy;
            QTest::newRow((mode + "ordinary").constData())
                << QStringLiteral("Count") << QStringLiteral("Count") << fuzzy;
        }
    }

    void unitNamedProperties()
    {
        QFETCH(QString, prefix);
        QFETCH(QString, expected);
        QFETCH(bool, fuzzy);

        auto& app = App::GetApplication();
        const auto name = app.getUniqueDocumentName("completion_test");
        auto* doc = app.newDocument(name.c_str(), "testUser");
        auto* obj = doc->addObject("App::VarSet", "VarSet");
        for (const auto* propertyName : {"N", "ft", "Count"}) {
            obj->addDynamicProperty("App::PropertyFloat", propertyName);
        }

        QStringList completions;
        {
            QLineEdit editor;
            Gui::ExpressionCompleter completer(obj);
            completer.setWidget(&editor);
            completer.setFilterMode(fuzzy ? Qt::MatchContains : Qt::MatchStartsWith);
            completer.slotUpdate(prefix, prefix.size());
            for (int row = 0; completer.setCurrentRow(row); ++row) {
                completions.append(completer.currentCompletion());
            }
        }
        app.closeDocument(name.c_str());
        QVERIFY2(completions.contains(expected), qPrintable(completions.join(QStringLiteral(", "))));
    }
};

QTEST_MAIN(testExpressionCompleter)

#include "ExpressionCompleter.moc"
