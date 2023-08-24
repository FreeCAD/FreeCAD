// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QDebug>
#include <QLayout>
#include <QMainWindow>
#include <QTest>
#include <QPushButton>

#include <App/Application.h>

#include "Gui/QuantitySpinBox.h"
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)


class testQuantitySpinBox: public QObject
{
    Q_OBJECT

public:
    testQuantitySpinBox()
        : qsb(nullptr)
        , mainWindow(nullptr)
    {
        tests::initApplication();
        auto topLevel = qApp->topLevelWindows();

        for (const auto window : topLevel) {
            if (auto mw = qobject_cast<QMainWindow*>(window)) {
                mainWindow = mw;
                break;
            }
        }
        if (!mainWindow) {
            mainWindow = new QMainWindow();
        }
        but = new QPushButton;
        delete mainWindow->layout();
        mainWindow->setLayout(new QVBoxLayout);
        mainWindow->layout()->addWidget(but);
        mainWindow->resize(500, 500);

        mainWindow->show();
    }

private Q_SLOTS:

    void init()
    {
        qsb = new Gui::QuantitySpinBox();
        mainWindow->layout()->addWidget(qsb);
    }

    void cleanup()
    {
        mainWindow->layout()->removeWidget(qsb);
        delete qsb;
    }

    void KeepUnits()
    {
        QFETCH(QString, input);
        QFETCH(Base::Quantity, result);

        Base::Quantity quant = qsb->valueFromText(input);
        QCOMPARE(quant, result);
    }

    void KeepUnits_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<Base::Quantity>("result");

        QTest::newRow("simple") << QStringLiteral("1mm") << Base::Quantity(1, "mm");
        QTest::newRow("numerator") << QStringLiteral("1mm/10") << Base::Quantity(0.1, "mm");
        QTest::newRow("denominator") << QStringLiteral("1/10mm") << Base::Quantity(0.1, "mm");
    }

    void KeepFormat()
    {
        auto quant = qsb->value();
        auto format = quant.getFormat();
        format.precision = 7;
        quant.setFormat(format);

        qsb->setValue(quant);

        auto val1 = qsb->value();
        QCOMPARE(val1.getFormat().precision, 7);

        // format shouldn't change after setting a double
        qsb->setValue(3.5);
        auto val2 = qsb->value();
        QCOMPARE(val2.getFormat().precision, 7);
    }

    // void test_DefaultUnit()
    void bug_DefaultUnit()
    {
        QTest::keyClicks(qsb, "1um");
        QTest::keyClick(qsb, Qt::Key_Enter);

        QCOMPARE(qsb->value(), Base::Quantity(1, "um"));

        // Delete content
        QTest::mouseClick(qsb, Qt::MouseButton::LeftButton);
        QTest::keySequence(qsb, {"Ctrl+A"});
        QTest::keyClick(qsb, Qt::Key_Delete);

        QTest::keyClicks(qsb, "3");
        QTest::keyClick(qsb, Qt::Key_Enter);

        // QCOMPARE(qsb->value(), Base::Quantity(3, "um"));
        QVERIFY2(qsb->value() != Base::Quantity(3, "um"),
                 "You fixed a bug!! Please rewrite this test");
    }

    void hasValidInput()
    {
        QFETCH(QString, input);
        QFETCH(bool, result);

        QTest::keyClicks(qsb, input);
        QTest::keyClick(qsb, Qt::Key_Enter);
        QCOMPARE(qsb->hasValidInput(), result);
    }

    void hasValidInput_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("result");  // Whether valid or not

        // Valids
        QTest::newRow("+") << QStringLiteral("3+5 mm") << true;
        QTest::newRow("-") << QStringLiteral("4-2 mm") << true;
        QTest::newRow("*") << QStringLiteral("5 mm * 4") << true;
        QTest::newRow("/") << QStringLiteral("24 / 2 km") << true;
        QTest::newRow("--") << QStringLiteral("3 -- 1 mm") << true;
        QTest::newRow("no operator") << QStringLiteral("94 mm") << true;
        QTest::newRow("decimal separator")
            << QStringLiteral("2....2 mm") << true;  // This works, but 2,,,,2 mm doesn't????

        // Bug: the below input is not valid if focusOutEvent but is valid on normalize()
        // even though focusOutEvent triggers normalize()?
        // QTest::newRow("") << QStringLiteral("mm 12") << true;
        // QTest::newRow("") << QStringLiteral("2,,,,2 mm") << true;
        // QTest::newRow("") << QStringLiteral("422,32.23,23 mm") << true;

        // Invalids
        QTest::newRow("**") << QStringLiteral("3**2") << false;
        QTest::newRow("mm") << QStringLiteral("mm") << false;
        QTest::newRow("km") << QStringLiteral("km") << false;
        // Bug: whether this is valid depends on environment?
        // QTest::newRow("empty") << QStringLiteral("") << false;
        QTest::newRow("random letters") << QStringLiteral("asdaskd") << false;
    }

    void normalize()
    {
        QFETCH(QString, input);
        QFETCH(bool, result);

        QTest::keyClicks(qsb, input);
        QCOMPARE(qsb->isNormalized(), result);
        qsb->normalize();
        QVERIFY(qsb->isNormalized());
    }

    void normalize_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("result");  // Whether isNormalized

        QTest::newRow("none") << QStringLiteral("3 mm") << true;
        QTest::newRow("+") << QStringLiteral("3+5 mm") << false;
        QTest::newRow("-") << QStringLiteral("4-2 mm") << false;
        QTest::newRow("*") << QStringLiteral("5 mm * 4") << false;
        QTest::newRow("/") << QStringLiteral("24 / 2 km") << false;
        QTest::newRow("++") << QStringLiteral("3 ++ 4mm") << false;

        // Bug: not covered by isNormalized()
        // QTest::newRow("") << QStringLiteral("3^2 mm") << false;
        // QTest::newRow("") << QStringLiteral("42 % 2 mm") << false;
    }

    // void test_setUnit()
    void bug_setUnit()
    {
        QTest::keyClicks(qsb, "2 km");
        QTest::keyClick(qsb, Qt::Key_Enter);
        QTest::mouseClick(but, Qt::MouseButton::LeftButton);

        // Bug: it visually km, but not in API
        // QCOMPARE(qsb->unitText(), "km");
        QVERIFY2(qsb->unitText() != "km", "You fixed a bug!! Please rewrite this test");

        qsb->setUnitText("cm");

        // QCOMPARE(qsb->unitText(), "cm");
        QVERIFY2(qsb->unitText() != "cm", "You fixed a bug!! Please rewrite this test");
    }

    void bug_setUnitText()
    {
        qsb->setUnitText(QStringLiteral("cm"));
        QTest::keyClicks(qsb, "5 cm");
        QTest::keyClick(qsb, Qt::Key_Enter);
        QTest::mouseClick(but, Qt::MouseButton::LeftButton);

        // QCOMPARE(qsb->unitText(), "cm");
        QVERIFY2(qsb->unitText() != "cm", "You fixed a bug!! Please rewrite this test");

        qsb->clear();
        QTest::keyClicks(qsb, "5 cm");
        qsb->normalize();

        // QCOMPARE(qsb->unitText(), "cm");
        QVERIFY2(qsb->unitText() != "cm", "You fixed a bug!! Please rewrite this test");
    }

    void bug_EvaluateInput()
    {
        auto quant = qsb->value();
        auto format = quant.getFormat();
        format.precision = 7;
        quant.setFormat(format);
        qsb->setValue(quant);  // This sets lineedit
        qsb->clear();          // So we need to clear again. Bug?

        QTest::keyClicks(qsb, "3um+5um");
        QTest::keyClick(qsb, Qt::Key_Enter);
        QCOMPARE(qsb->text(), "8,0000000 µm");  // No issue when containing `+`

        qsb->clear();

        QTest::mouseClick(qsb, Qt::MouseButton::LeftButton);  // focusIn
        QTest::keyClicks(qsb, "1um");
        // Bug: enter doesn't trigger evaluation/normalization, it only happens on focusOutEvent.
        // In FreeCAD GUI environment enter key works somehow, probably by indirect focusOutEvent
        QTest::keyClick(qsb, Qt::Key_Enter);
        // Here we test if bug has spontaneously been fixed by not focusOut:
        // QTest::mouseClick(but, Qt::MouseButton::LeftButton);

        // QCOMPARE(qsb->text(), "1,0000000 µm");
        //  Issue when not containing `+`
        QVERIFY2(qsb->text() != "1,0000000 µm", "You fixed a bug!! Please rewrite this test");
    }

    // void test_ImplicitUnits()
    void bug_ImplicitUnits()
    {
        QTest::keyClicks(qsb, "5 mm * 4");  // This input specifically causes the bug
        QTest::keyClick(qsb, Qt::Key_Enter);
        qsb->clear();

        QTest::keyClicks(qsb, "3+5um");
        QTest::keyClick(qsb, Qt::Key_Enter);

        // Bug: it evalueates to 3,0050000 mm (3 mm + 5 um)
        // QCOMPARE(qsb->text(), "8,0000000 µm");
        QVERIFY2(qsb->text() != "8,0000000 µm", "You fixed a bug!! Please rewrite this test");
    }

private:
    Gui::QuantitySpinBox* qsb;
    QPushButton* but;
    QMainWindow* mainWindow;
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testQuantitySpinBox)

#include "QuantitySpinBox.moc"
