// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QAbstractItemModel>
#include <QLineEdit>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include "Gui/ExpressionCompleter.h"
#include <src/App/InitApplication.h>

class testExpressionCompleter: public QObject
{
    Q_OBJECT

public:
    testExpressionCompleter()
    {
        tests::initApplication();
    }

private Q_SLOTS:

    void cleanup()
    {
        if (!docName.empty()) {
            App::GetApplication().closeDocument(docName.c_str());
            docName.clear();
        }
    }

    void test_fuzzySearchFindsNestedVarSetPropertiesFromBareInput()
    {
        docName = App::GetApplication().getUniqueDocumentName("test");
        App::Document* doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        App::DocumentObject* host = doc->addObject("App::FeatureTest", "Host");
        QVERIFY(host != nullptr);

        App::DocumentObject* varSet = doc->addObject("App::VarSet", "VarSet");
        QVERIFY(varSet != nullptr);
        App::Property* property = varSet->addDynamicProperty("App::PropertyLength", "hole_spacing");
        QVERIFY(property != nullptr);

        QLineEdit lineEdit;
        Gui::ExpressionCompleter completer(host);
        completer.setWidget(&lineEdit);
        completer.setFilterMode(Qt::MatchContains);
        completer.slotUpdate(QStringLiteral("h"), 1);

        QAbstractItemModel* model = completer.model();
        QVERIFY(model != nullptr);
        QVERIFY(model->rowCount() > 0);

        const QString firstCompletion = model->index(0, 0).data(Qt::DisplayRole).toString();
        QVERIFY(firstCompletion.contains(QStringLiteral("hole_spacing")));

        QStringList completions;
        for (int row = 0; row < model->rowCount(); ++row) {
            completions << model->index(row, 0).data(Qt::DisplayRole).toString();
        }

        QVERIFY(completions.contains(QStringLiteral("VarSet.hole_spacing")));
        QVERIFY(completions.contains(QStringLiteral("<<VarSet>>.hole_spacing")));
    }

private:
    std::string docName;
};

QTEST_MAIN(testExpressionCompleter)

#include "ExpressionCompleter.moc"
