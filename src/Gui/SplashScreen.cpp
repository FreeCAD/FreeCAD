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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <cstdlib>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QPainter>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QWaitCondition>
#endif

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
        Base::Console().AttachObserver(this);

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
        Base::Console().DetachObserver(this);
    }
    const char* Name() override
    {
        return "SplashObserver";
    }
    void SendLog(const std::string& notifiername,
                 const std::string& msg,
                 Base::LogStyle level,
                 Base::IntendedRecipient recipient,
                 Base::ContentType content) override
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
static void renderDevBuildWarning(QPainter& painter,
                                  const QPoint startPosition,
                                  const QSize maxSize,
                                  QColor color)
{
    // Create a background box that fades out the artwork for better legibility
    QColor fader(Qt::white);
    constexpr float halfDensity(0.65F);
    fader.setAlphaF(halfDensity);
    QBrush fillBrush(fader, Qt::BrushStyle::SolidPattern);
    painter.setBrush(fillBrush);

    // Construct the lines of text and figure out how much space they need
    const auto devWarningLine1 = QObject::tr("WARNING: This is a development version.");
    const auto devWarningLine2 = QObject::tr("Please do not use it in a production environment.");
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
        int newFontSize = static_cast<int>(painter.font().pointSize() * reductionFactor);
        padding *= reductionFactor;
        QFont newFont = painter.font();
        newFont.setPointSize(newFontSize);
        painter.setFont(newFont);
        lineHeight = painter.fontMetrics().lineSpacing();
        boxWidth = maxSize.width();
    }
    constexpr float lineExpansionFactor(2.3F);
    int boxHeight = static_cast<int>(lineHeight * lineExpansionFactor);

    // Draw the background rectangle and the text
    painter.setPen(color);
    painter.drawRect(startPosition.x(), startPosition.y(), boxWidth, boxHeight);
    painter.drawText(startPosition.x() + padding, startPosition.y() + lineHeight, devWarningLine1);
    painter.drawText(startPosition.x() + padding,
                     startPosition.y() + 2 * lineHeight,
                     devWarningLine2);
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
    QFileInfo fi(QString::fromLatin1("images:splash_image.png"));
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
    float pixelRatio(1.0);
    if (splash_image.isNull()) {
        // determine the count of splashes
        QStringList pixmaps =
            Gui::BitmapFactory().findIconFiles().filter(QString::fromStdString(splash_path));
        // divide by 2 since there's two sets (normal and 2x)
        // minus 1 to ignore the default splash that isn't numbered
        int splash_count = pixmaps.count() / 2 - 1;

        // set a random splash path
        if (splash_count > 0) {
            int random = rand() % splash_count;
            splash_path += std::to_string(random);
        }

        if (qApp->devicePixelRatio() > 1.0) {
            // For HiDPI screens, we have a double-resolution version of the splash image
            splash_path += "_2x";
            splash_image = Gui::BitmapFactory().pixmap(splash_path.c_str());
            splash_image.setDevicePixelRatio(2.0);
            pixelRatio = 2.0;
        }
        else {
            splash_image = Gui::BitmapFactory().pixmap(splash_path.c_str());
        }
    }

    // include application name and version number
    std::map<std::string, std::string>::const_iterator tc =
        App::Application::Config().find("SplashInfoColor");
    std::map<std::string, std::string>::const_iterator wc =
        App::Application::Config().find("SplashWarningColor");
    if (tc != App::Application::Config().end() && wc != App::Application::Config().end()) {
        QString title = qApp->applicationName();
        QString major = QString::fromStdString(App::Application::Config()["BuildVersionMajor"]);
        QString minor = QString::fromStdString(App::Application::Config()["BuildVersionMinor"]);
        QString point = QString::fromStdString(App::Application::Config()["BuildVersionPoint"]);
        QString suffix = QString::fromStdString(App::Application::Config()["BuildVersionSuffix"]);
        QString version = QString::fromLatin1("%1.%2.%3%4").arg(major, minor, point, suffix);
        QString position, fontFamily;

        std::map<std::string, std::string>::const_iterator te =
            App::Application::Config().find("SplashInfoExeName");
        std::map<std::string, std::string>::const_iterator tv =
            App::Application::Config().find("SplashInfoVersion");
        std::map<std::string, std::string>::const_iterator tp =
            App::Application::Config().find("SplashInfoPosition");
        std::map<std::string, std::string>::const_iterator tf =
            App::Application::Config().find("SplashInfoFont");
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
        if (!fontFamily.isEmpty()) {
            QFont font = painter.font();
            if (font.fromString(fontFamily)) {
                painter.setFont(font);
            }
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

        QColor color(QString::fromStdString(tc->second));
        if (color.isValid()) {
            painter.setPen(color);
            painter.setFont(fontExe);
            if (title != QLatin1String("FreeCAD")) {
                // FreeCAD's Splashscreen already contains the EXE name, no need to draw it
                painter.drawText(x, y, title);
            }
            painter.setFont(fontVer);
            painter.drawText(x + (l + 235), y - 7, version);
            QColor warningColor(QString::fromStdString(wc->second));
            if (suffix == QLatin1String("dev") && warningColor.isValid()) {
                fontVer.setPointSizeF(14.0);
                painter.setFont(fontVer);
                const int lineHeight = metricVer.lineSpacing();
                const int padding {45};  // Distance from the edge of the graphic's bounding box
                QPoint startPosition(padding, y + lineHeight * 2);
                QSize maxSize(w / pixelRatio - 2 * padding, lineHeight * 3);
                renderDevBuildWarning(painter, startPosition, maxSize, warningColor);
            }
            painter.end();
        }
    }

    return splash_image;
}
