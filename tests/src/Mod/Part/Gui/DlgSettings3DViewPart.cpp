// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QApplication>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QTest>
#include <QTimer>

#include <Mod/Part/Gui/DlgSettings3DViewPartImp.h>
#include <src/App/InitApplication.h>

class TestDlgSettings3DViewPart: public QObject
{
    Q_OBJECT

public:
    TestDlgSettings3DViewPart()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void angularDeflectionWaitsForEditingFinished()
    {
        PartGui::DlgSettings3DViewPart page;
        page.setAttribute(Qt::WA_DontShowOnScreen);
        page.show();

        auto* spinBox = page.findChild<QDoubleSpinBox*>("maxAngularDeflection");
        QVERIFY(spinBox);
        auto* lineEdit = spinBox->findChild<QLineEdit*>();
        QVERIFY(lineEdit);

        bool warningShown = false;
        QTimer closeWarning;
        connect(&closeWarning, &QTimer::timeout, [&warningShown]() {
            auto* messageBox = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (messageBox) {
                warningShown = true;
                messageBox->accept();
            }
        });
        closeWarning.start(1);

        lineEdit->selectAll();
        QTest::keyClicks(lineEdit, "1");
        QVERIFY(!warningShown);

        QTest::keyClicks(lineEdit, "5");
        QTest::keyClick(lineEdit, Qt::Key_Return);
        QVERIFY(!warningShown);

        lineEdit->selectAll();
        QTest::keyClicks(lineEdit, "1");
        QVERIFY(!warningShown);

        QTest::keyClick(lineEdit, Qt::Key_Return);
        QVERIFY(warningShown);
        page.hide();
    }
};

QTEST_MAIN(TestDlgSettings3DViewPart)

#include "DlgSettings3DViewPart.moc"
