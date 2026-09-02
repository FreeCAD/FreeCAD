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

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QMutex>
#include <QPainter>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QThread>
#include <QWaitCondition>
#include <QWindow>

#include <App/Application.h>
#include <App/Metadata.h>
#include <Base/Console.h>

#include "BitmapFactory.h"
#include "SplashScreen.h"
#include "Tools.h"

using namespace Gui;

namespace Gui
{

/** Displays all messages at startup inside the splash screen.
 * \author Werner Mayer
 */
class SplashObserver: public Base::ILogger
{
public:
    SplashObserver(const SplashObserver&) = delete;
    SplashObserver(SplashObserver&&) = delete;
    SplashObserver& operator=(const SplashObserver&) = delete;
    SplashObserver& operator=(SplashObserver&&) = delete;

    explicit SplashObserver(QSplashScreen* splasher = nullptr)
        : splash(splasher)
        , alignment(Qt::AlignBottom | Qt::AlignLeft)
        , textColor(Qt::black)
    {
        Base::Console().attachObserver(this);

        // allow to customize text position and color
        const std::map<std::string, std::string>& cfg = App::Application::Config();
        auto al = cfg.find("SplashAlignment");
        if (al != cfg.end()) {
            QString alt = QString::fromLatin1(al->second.c_str());
            int align = 0;
            if (alt.startsWith(QLatin1String("VCenter"))) {
                align = Qt::AlignVCenter;
            }
            else if (alt.startsWith(QLatin1String("Top"))) {
                align = Qt::AlignTop;
            }
            else {
                align = Qt::AlignBottom;
            }

            if (alt.endsWith(QLatin1String("HCenter"))) {
                align += Qt::AlignHCenter;
            }
            else if (alt.endsWith(QLatin1String("Right"))) {
                align += Qt::AlignRight;
            }
            else {
                align += Qt::AlignLeft;
            }

            alignment = align;
        }

        // choose text color
        auto tc = cfg.find("SplashTextColor");
        if (tc != cfg.end()) {
            QColor col(QString::fromStdString(tc->second));
            if (col.isValid()) {
                textColor = col;
            }
        }
    }
    ~SplashObserver() override
    {
        Base::Console().detachObserver(this);
    }
    const char* name() override
    {
        return "SplashObserver";
    }
    void sendLog(
        const std::string& notifiername,
        const std::string& msg,
        Base::LogStyle level,
        Base::IntendedRecipient recipient,
        Base::ContentType content
    ) override
    {
        Q_UNUSED(notifiername)
        Q_UNUSED(recipient)
        Q_UNUSED(content)

#ifdef FC_DEBUG
        Q_UNUSED(level)
        Log(msg);
#else
        if (level == Base::LogStyle::Log) {
            Log(msg);
        }
#endif
    }
    void Log(const std::string& text)
    {
        QString msg(QString::fromStdString(text));
        QRegularExpression rx;
        // ignore 'Init:' and 'Mod:' prefixes
        rx.setPattern(QLatin1String("^\\s*(Init:|Mod:)\\s*"));
        auto match = rx.match(msg);
        if (match.hasMatch()) {
            msg = msg.mid(match.capturedLength());
        }
        else {
            // ignore activation of commands
            rx.setPattern(QLatin1String(R"(^\s*(\+App::|Create|CmdC:|CmdG:|Act:)\s*)"));
            match = rx.match(msg);
            if (match.hasMatch() && match.capturedStart() == 0) {
                return;
            }
        }

        splash->showMessage(msg.replace(QLatin1String("\n"), QString()), alignment, textColor);
        QMutex mutex;
        QMutexLocker ml(&mutex);
        QWaitCondition().wait(&mutex, 50);
    }

private:
    QSplashScreen* splash;
    int alignment;
    QColor textColor;
};

/**
 * Displays a warning about this being a developer build. Designed for display in the Splashscreen.
 * \param painter The painter to draw the warning into
 * \param startPosition The painter-space coordinates to start the warning box at.
 * \param maxSize The maximum extents for the box that is drawn. If the text exceeds this size it
 * will be scaled down to fit.
 * \note The text string is translatable, so its length is somewhat unpredictable. It is always
 * displayed as two lines, regardless of the length of the text (e.g. no wrapping is done). Only the
 * width is considered, the height simply follows from the font size.
 */
static void renderDevBuildWarning(
    QPainter& painter,
    const QPoint startPosition,
    const QSize maxSize,
    QColor color
)
{
    // Create a background box that fades out the artwork for better legibility
    QColor fader(Qt::white);
    constexpr float halfDensity(0.65F);
    fader.setAlphaF(halfDensity);
    QBrush fillBrush(fader, Qt::BrushStyle::SolidPattern);
    painter.setBrush(fillBrush);

    // Construct the lines of text and figure out how much space they need
    const auto devWarningLine1 = QObject::tr("WARNING: This is a development version.");
    const auto devWarningLine2 = QObject::tr("Do not use it in a production environment.");
    const QString devWarning = QStringLiteral("%1\n%2").arg(devWarningLine1, devWarningLine2);
    QFontMetrics fontMetrics(painter.font());  // Try to use the existing font
    int padding = QtTools::horizontalAdvance(fontMetrics, QLatin1String("M"));  // Arbitrary
    int line1Width = QtTools::horizontalAdvance(fontMetrics, devWarningLine1);
    int line2Width = QtTools::horizontalAdvance(fontMetrics, devWarningLine2);
    int boxWidth = std::max(line1Width, line2Width) + 2 * padding;
    int lineHeight = fontMetrics.lineSpacing();
    if (boxWidth > maxSize.width()) {
        // Especially if the text was translated, there is a chance that using the existing font
        // will exceed the width of the Splashscreen graphic. Resize down so that it fits, no matter
        // how long the text strings are.
        float reductionFactor = static_cast<float>(maxSize.width()) / static_cast<float>(boxWidth);
        QFont newFont = painter.font();
        if (newFont.pixelSize() > 0) {
            const int newFontSize = static_cast<int>(newFont.pixelSize() * reductionFactor);
            newFont.setPixelSize(std::max(1, newFontSize));
        }
        else {
            newFont.setPointSizeF(newFont.pointSizeF() * reductionFactor);
        }
        padding = static_cast<int>(padding * reductionFactor);
        painter.setFont(newFont);
        lineHeight = painter.fontMetrics().lineSpacing();
        boxWidth = maxSize.width();
    }
    constexpr float lineExpansionFactor(2.3F);
    int boxHeight = static_cast<int>(lineHeight * lineExpansionFactor);

    // Draw the background rectangle and center the text within it
    painter.setPen(color);
    const QRect box(startPosition, QSize(boxWidth, boxHeight));
    painter.drawRect(box);
    painter.drawText(box, Qt::AlignCenter, devWarning);
}

static QPixmap defaultSplashImage()
{
    constexpr QSizeF splashSize(480.0, 220.0);
    return BitmapFactory().pixmapFromSvg(":/icons/freecadsplash.svg", splashSize);
}

static QFont defaultSplashFont(const QFont& fallback)
{
    static const QString fontFamily = []() {
        const int fontId = QFontDatabase::addApplicationFont(
            QStringLiteral(":/fonts/URWGothic-Demi.ttf")
        );
        if (fontId < 0) {
            Base::Console().warning("Failed to load the splash screen font\n");
            return QString();
        }

        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        return families.isEmpty() ? QString() : families.front();
    }();

    QFont font(fallback);
    if (!fontFamily.isEmpty()) {
        font.setFamily(fontFamily);
        font.setStyleName(QStringLiteral("Demi"));
    }
    return font;
}

}  // namespace Gui

// ------------------------------------------------------------------------------

/**
 * Constructs a splash screen that will display the pixmap.
 */
SplashScreen::SplashScreen(const QPixmap& pixmap, Qt::WindowFlags f)
    : QSplashScreen(pixmap, f)
{
    // write the messages to splasher
    messages = new SplashObserver(this);
}

/** Destruction. */
SplashScreen::~SplashScreen()
{
    delete messages;
}

void SplashScreen::finish(QWidget* w)
{
    Q_UNUSED(w);
    close();
}

bool SplashScreen::event(QEvent* e)
{
    if (e->type() == QEvent::Show) {
        // Bypass QSplashScreen::event for Show events specifically
        return QWidget::event(e);  // NOLINT(bugprone-parent-virtual-call)
    }
    return QSplashScreen::event(e);
}

void SplashScreen::show()
{
    QSplashScreen::show();

    // Our repaint will call processEvents later on, no need to waste time here
    if (messages->bErr) {
        return;
    }

    // Show events are bypassed and splash window is already created by the above show()
    // call. Now process pending events from underlying OS which actually brings splash
    // into existence (such as X' MapNotify/Expose events or Wayland's xdg_surface
    // configuration acknowledgement)
    constexpr int TimeOut = 250;
    constexpr int TimeSlice = 20;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < TimeOut) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, TimeSlice);
        QCoreApplication::sendPostedEvents();
        if (windowHandle()->isExposed()) {
            break;
        }
        QThread::msleep(TimeSlice);
    }
}

/**
 * Draws the contents of the splash screen using painter \a painter. The default
 * implementation draws the message passed by message().
 */
void SplashScreen::drawContents(QPainter* painter)
{
    QSplashScreen::drawContents(painter);
}

void SplashScreen::setShowMessages(bool on)
{
    messages->bErr = on;
    messages->bMsg = on;
    messages->bLog = on;
    messages->bWrn = on;
}

QPixmap SplashScreen::splashImage()
{
    // search in the UserAppData dir as very first
    QPixmap splash_image;
    bool usingDefaultSplash = false;
    QFileInfo fi(QStringLiteral("images:splash_image.png"));
    if (fi.isFile() && fi.exists()) {
        splash_image.load(fi.filePath(), "PNG");
    }

    // if no image was found try the config
    std::string splash_path = App::Application::Config()["SplashScreen"];
    if (splash_image.isNull()) {
        QString path = QString::fromStdString(splash_path);
        if (QDir(path).isRelative()) {
            QString home = QString::fromStdString(App::Application::getHomePath());
            path = QFileInfo(QDir(home), path).absoluteFilePath();
        }

        splash_image.load(path);
    }

    // now try the icon paths
    if (splash_image.isNull()) {
        if (splash_path == "freecadsplash") {
            splash_image = defaultSplashImage();
            usingDefaultSplash = !splash_image.isNull();
        }
        else {
            splash_image = Gui::BitmapFactory().pixmap(splash_path.c_str());
        }
    }

    // include application name and version number
    std::map<std::string, std::string>::const_iterator tc = App::Application::Config().find(
        "SplashInfoColor"
    );
    std::map<std::string, std::string>::const_iterator wc = App::Application::Config().find(
        "SplashWarningColor"
    );
    if (tc != App::Application::Config().end() && wc != App::Application::Config().end()) {
        QString title = qApp->applicationName();
        QString major = QString::fromStdString(App::Application::Config()["BuildVersionMajor"]);
        QString minor = QString::fromStdString(App::Application::Config()["BuildVersionMinor"]);
        QString point = QString::fromStdString(App::Application::Config()["BuildVersionPoint"]);
        QString suffix = QString::fromStdString(App::Application::Config()["BuildVersionSuffix"]);
        QString version = QStringLiteral("%1.%2.%3%4").arg(major, minor, point, suffix);
        QString position, fontFamily;

        std::map<std::string, std::string>::const_iterator te = App::Application::Config().find(
            "SplashInfoExeName"
        );
        std::map<std::string, std::string>::const_iterator tv = App::Application::Config().find(
            "SplashInfoVersion"
        );
        std::map<std::string, std::string>::const_iterator tp = App::Application::Config().find(
            "SplashInfoPosition"
        );
        std::map<std::string, std::string>::const_iterator tf = App::Application::Config().find(
            "SplashInfoFont"
        );
        if (te != App::Application::Config().end()) {
            title = QString::fromStdString(te->second);
        }
        if (tv != App::Application::Config().end()) {
            version = QString::fromStdString(tv->second);
        }
        if (tp != App::Application::Config().end()) {
            position = QString::fromStdString(tp->second);
        }
        if (tf != App::Application::Config().end()) {
            fontFamily = QString::fromStdString(tf->second);
        }

        QPainter painter;
        painter.begin(&splash_image);
        bool customFontApplied = false;
        if (!fontFamily.isEmpty()) {
            QFont font = painter.font();
            if (font.fromString(fontFamily)) {
                painter.setFont(font);
                customFontApplied = true;
            }
        }

        QColor color(QString::fromStdString(tc->second));
        QColor warningColor(QString::fromStdString(wc->second));
        if (usingDefaultSplash && title == QStringLiteral("FreeCAD") && color.isValid()) {
            QFont numberFont = painter.font();
            if (!customFontApplied) {
                numberFont = defaultSplashFont(numberFont);
            }
            numberFont.setPixelSize(20);
            QFont labelFont(numberFont);
            labelFont.setPixelSize(16);
            painter.setPen(color);

            const QString versionLabel = QStringLiteral("version");
            const QFontMetricsF numberMetrics(numberFont);
            const QFontMetricsF labelMetrics(labelFont);
            const QRectF numberBounds = numberMetrics.boundingRect(version);
            const QRectF labelBounds = labelMetrics.boundingRect(versionLabel);
            // Match the version line to the wordmark geometry in freecadsplash.svg.
            constexpr qreal versionRight = 418;
            constexpr qreal versionBaseline = 158;
            constexpr qreal versionSpacing = 8.0;
            const qreal numberPosition = versionRight - numberBounds.right();
            painter.setFont(numberFont);
            painter.drawText(QPointF(numberPosition, versionBaseline), version);
            painter.setFont(labelFont);
            painter.drawText(
                QPointF(
                    numberPosition + numberBounds.left() - versionSpacing - labelBounds.right(),
                    versionBaseline
                ),
                versionLabel
            );

            if (suffix == QStringLiteral("dev") && warningColor.isValid()) {
                numberFont.setPixelSize(10);
                numberFont.setHintingPreference(QFont::PreferVerticalHinting);
                painter.setFont(numberFont);
                const QFontMetrics warningMetrics(numberFont);
                const int lineHeight = warningMetrics.lineSpacing();
                constexpr int padding = 12;
                const qreal pixelRatio = splash_image.devicePixelRatio();
                const int width = qRound(splash_image.width() / pixelRatio);
                const int height = qRound(splash_image.height() / pixelRatio);
                const int boxHeight = static_cast<int>(lineHeight * 2.3F);
                const QPoint startPosition(padding, height - boxHeight - padding);
                const QSize maxSize(width - 2 * padding, boxHeight);
                renderDevBuildWarning(painter, startPosition, maxSize, warningColor);
            }

            painter.end();
            return splash_image;
        }

        QFont fontExe = painter.font();
        fontExe.setPointSizeF(20.0);
        QFontMetrics metricExe(fontExe);
        int l = QtTools::horizontalAdvance(metricExe, title);
        if (title == QLatin1String("FreeCAD")) {
            l = 0.0;  // "FreeCAD" text is already part of the splashscreen, version goes below it
        }
        int w = splash_image.width();
        int h = splash_image.height();

        QFont fontVer = painter.font();
        fontVer.setPointSizeF(11.0);
        QFontMetrics metricVer(fontVer);
        int v = QtTools::horizontalAdvance(metricVer, version);

        int x = -1, y = -1;
        QRegularExpression rx(QLatin1String("(\\d+).(\\d+)"));
        auto match = rx.match(position);
        if (match.hasMatch()) {
            x = match.captured(1).toInt();
            y = match.captured(2).toInt();
        }
        else {
            x = w - (l + v + 10);
            y = h - 20;
        }

        if (color.isValid()) {
            painter.setPen(color);
            painter.setFont(fontExe);
            if (title != QLatin1String("FreeCAD")) {
                // FreeCAD's Splashscreen already contains the EXE name, no need to draw it
                painter.drawText(x, y, title);
            }
            painter.setFont(fontVer);
            painter.drawText(x + (l + 235), y - 7, version);
            if (suffix == QStringLiteral("dev") && warningColor.isValid()) {
                fontVer.setPointSizeF(14.0);
                fontVer.setHintingPreference(QFont::PreferVerticalHinting);
                painter.setFont(fontVer);
                const QFontMetrics warningMetrics(fontVer);
                const int lineHeight = warningMetrics.lineSpacing();
                const int padding {45};  // Distance from the edge of the graphic's bounding box
                QPoint startPosition(padding, y + lineHeight * 2);
                QSize maxSize(w / splash_image.devicePixelRatio() - 2 * padding, lineHeight * 3);
                renderDevBuildWarning(painter, startPosition, maxSize, warningColor);
            }
            painter.end();
        }
    }

    return splash_image;
}
