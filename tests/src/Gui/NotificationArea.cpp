// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QCoreApplication>
#include <QImage>
#include <QMainWindow>
#include <QMenu>
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

private:
    struct PresentationState
    {
        QString text;
        QSize size;
        QSize sizeHint;
        QSize minimumSizeHint;
        int renderedTextPixels;
    };

    static auto countDifferingPixels(const QImage& left, const QImage& right) -> int
    {
        if (left.isNull() || right.isNull() || left.size() != right.size()) {
            return 0;
        }

        int differingPixels = 0;

        for (int y = 0; y < left.height(); ++y) {
            for (int x = 0; x < left.width(); ++x) {
                if (left.pixel(x, y) != right.pixel(x, y)) {
                    ++differingPixels;
                }
            }
        }

        return differingPixels;
    }

    static auto renderedTextRegion(const Gui::NotificationArea& notificationArea, const QImage& image)
        -> QRect
    {
        const int glyphWidth = qMax(notificationArea.fontMetrics().horizontalAdvance(QStringLiteral("0")), 1);
        const int rightInset = 4;
        const int topInset = qMin(4, qMax(image.height() - 1, 0));
        const int bottomInset = qMin(4, qMax(image.height() - topInset - 1, 0));
        const int cropWidth = qMax(glyphWidth + 12, 16);
        const int left = qMax(qMin(image.width() / 2, image.width() - cropWidth - rightInset), 0);
        const int width = qMax(image.width() - left - rightInset, 1);
        const int height = qMax(image.height() - topInset - bottomInset, 1);
        return QRect(left, topInset, width, height);
    }

    static auto countRenderedTextPixels(Gui::NotificationArea& notificationArea) -> int
    {
        const auto originalText = notificationArea.text();
        const auto originalSize = notificationArea.size();
        const auto originalMinimumSize = notificationArea.minimumSize();
        const auto originalMaximumSize = notificationArea.maximumSize();

        notificationArea.setFixedSize(originalSize);
        QCoreApplication::processEvents();

        const auto withText = notificationArea.grab().toImage().convertToFormat(QImage::Format_ARGB32);

        notificationArea.setText(QString());
        QCoreApplication::processEvents();
        const auto withoutText = notificationArea.grab().toImage().convertToFormat(QImage::Format_ARGB32);

        notificationArea.setText(originalText);
        notificationArea.setMinimumSize(originalMinimumSize);
        notificationArea.setMaximumSize(originalMaximumSize);
        notificationArea.resize(originalSize);
        QCoreApplication::processEvents();

        const auto region = renderedTextRegion(notificationArea, withText);
        return countDifferingPixels(withText.copy(region), withoutText.copy(region));
    }

    static auto capturePresentationState(Gui::NotificationArea& notificationArea)
        -> PresentationState
    {
        return {
            notificationArea.text(),
            notificationArea.size(),
            notificationArea.sizeHint(),
            notificationArea.minimumSizeHint(),
            countRenderedTextPixels(notificationArea),
        };
    }

private Q_SLOTS:
    void test_InitialUnreadCountIsVisible()  // NOLINT
    {
        QStatusBar statusBar;
        Gui::NotificationArea notificationArea(&statusBar);

        QCOMPARE(notificationArea.text(), QStringLiteral("0"));
    }

    void test_StartupPresentationMatchesPostClickPresentation()  // NOLINT
    {
        QMainWindow mainWindow;
        auto* statusBar = new QStatusBar(&mainWindow);
        mainWindow.setStatusBar(statusBar);

        auto* notificationArea = new Gui::NotificationArea(statusBar);
        notificationArea->setObjectName(QStringLiteral("notificationArea"));
        statusBar->addPermanentWidget(notificationArea);

        mainWindow.resize(900, 180);
        mainWindow.show();

        QTRY_VERIFY(notificationArea->isVisible());
        QTRY_VERIFY(notificationArea->width() > 0);

        auto startup = capturePresentationState(*notificationArea);
        QCOMPARE(startup.text, QStringLiteral("0"));
        QVERIFY(startup.renderedTextPixels > 0);

        auto* menu = notificationArea->menu();
        QVERIFY(menu != nullptr);

        QTest::mouseClick(notificationArea,
                          Qt::LeftButton,
                          Qt::NoModifier,
                          notificationArea->rect().center());
        QTRY_VERIFY(menu->isVisible());
        auto menuOpen = capturePresentationState(*notificationArea);
        QCOMPARE(startup.text, menuOpen.text);
        QCOMPARE(startup.sizeHint, menuOpen.sizeHint);
        QCOMPARE(startup.minimumSizeHint, menuOpen.minimumSizeHint);
        QVERIFY(menuOpen.renderedTextPixels > 0);
        QCOMPARE(startup.renderedTextPixels, menuOpen.renderedTextPixels);

        menu->hide();
        QTRY_VERIFY(!menu->isVisible());
        QTRY_VERIFY(!notificationArea->isDown());

        QTest::mouseMove(&mainWindow, QPoint(8, 8));
        QTRY_COMPARE(notificationArea->text(), QStringLiteral("0"));

        auto afterClick = capturePresentationState(*notificationArea);

        QCOMPARE(startup.text, afterClick.text);
        QCOMPARE(startup.size, afterClick.size);
        QCOMPARE(startup.sizeHint, afterClick.sizeHint);
        QCOMPARE(startup.minimumSizeHint, afterClick.minimumSizeHint);
        QCOMPARE(startup.renderedTextPixels, afterClick.renderedTextPixels);
    }
};

QTEST_MAIN(testNotificationArea)

#include "NotificationArea.moc"
