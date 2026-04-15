// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QStatusBar>
#include <QTest>

#include "Gui/NotificationArea.h"
#include <src/App/InitApplication.h>

class testNotificationArea: public QObject
{
    Q_OBJECT

public:
    testNotificationArea()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void test_InitialUnreadCountIsVisible()  // NOLINT
    {
        QStatusBar statusBar;
        Gui::NotificationArea notificationArea(&statusBar);

        QCOMPARE(notificationArea.text(), QStringLiteral("0"));
    }
};

QTEST_MAIN(testNotificationArea)

#include "NotificationArea.moc"
