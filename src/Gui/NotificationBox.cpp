/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
# include <memory>
# include <mutex>
# include <QApplication>
# include <QEvent>
# include <QLabel>
# include <QScreen>
# include <QStyleOption>
# include <QStylePainter>
# include <QTextDocument>
# include <QTimer>
#endif

#include "NotificationBox.h"


using namespace Gui;

namespace Gui {

// https://stackoverflow.com/questions/41402152/stdunique-ptr-and-qobjectdeletelater
struct QObjectDeleteLater {
    void operator()(QObject *o) {
        o->deleteLater();
    }
};

template<typename T>
using qobject_delete_later_unique_ptr = std::unique_ptr<T, QObjectDeleteLater>;

/** Class showing the notification as a label
 * */
class NotificationLabel : public QLabel
{
    Q_OBJECT
public:
    // Windows implementation uses QWidget w to pass the screen (see NotificationBox::showText).
    // This screen is used as parent for QLabel.
    // Linux implementation does not rely on a parent (w = nullptr).
    NotificationLabel(const QString &text, const QPoint &pos, QWidget *w, int displayTime, int minShowTime = 0);
    /// Reuse existing notification to show a new notification (with a new text)
    void reuseNotification(const QString &text, int displayTime, const QPoint &pos);
    /// Hide notification after a hiding timer.
    void hideNotification();
    /// Update the size of the QLabel
    void updateSize(const QPoint &pos);
    /// Event filter
    bool eventFilter(QObject *, QEvent *) override;
    /// Return true if the text provided is the same as the one of an existing notification
    bool notificationLabelChanged(const QString &text);
    /// Place the notification at the given position
    void placeNotificationLabel(const QPoint &pos);

    /// The instance
    static qobject_delete_later_unique_ptr<NotificationLabel> instance;
protected:
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
private:
    /// Re-start the notification expiration timer
    void restartExpireTimer(int displayTime);
    /// Hide notification right away
    void hideNotificationImmediately();

private:
    int minShowTime;
    QTimer hideTimer;
    QTimer expireTimer;
};

qobject_delete_later_unique_ptr<NotificationLabel> NotificationLabel::instance = nullptr;

NotificationLabel::NotificationLabel(const QString &text, const QPoint &pos, QWidget *w, int displayTime, int minShowTime)
: QLabel(w, Qt::ToolTip | Qt::BypassGraphicsProxyWidget), minShowTime(minShowTime)
{
    instance.reset(this);
    setForegroundRole(QPalette::ToolTipText); // defaults to ToolTip QPalette
    setBackgroundRole(QPalette::ToolTipBase); // defaults to ToolTip QPalette
    setPalette(NotificationBox::palette());
    ensurePolished();
    setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, this));
    setFrameStyle(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft);
    setIndent(1);
    qApp->installEventFilter(this);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, nullptr, this) / 255.0);
    setMouseTracking(false);
    hideTimer.setSingleShot(true);
    expireTimer.setSingleShot(true);

    expireTimer.callOnTimeout([this](){
        hideTimer.stop();
        hideNotificationImmediately();
    });

    hideTimer.callOnTimeout([this](){
        expireTimer.stop();
        hideNotificationImmediately();
    });

    reuseNotification(text, displayTime, pos);
}

void NotificationLabel::restartExpireTimer(int displayTime)
{
    int time;

    if (displayTime > 0) {
        time = displayTime;
    }
    else {
        time = 10000 + 40 * qMax(0, text().length()-100);
    }

    expireTimer.start(time);
    hideTimer.stop();
}

void NotificationLabel::reuseNotification(const QString &text, int displayTime, const QPoint &pos)
{
    setText(text);
    updateSize(pos);
    restartExpireTimer(displayTime);
}

void NotificationLabel::updateSize(const QPoint &pos)
{
    // Ensure that we get correct sizeHints by placing this window on the right screen.
    QFontMetrics fm(font());
    QSize extra(1, 0);
    // Make it look good with the default ToolTip font on Mac, which has a small descent.
    if (fm.descent() == 2 && fm.ascent() >= 11) {
        ++extra.rheight();
    }

    setWordWrap(Qt::mightBeRichText(text()));

    QSize sh = sizeHint();

    // ### When the above WinRT code is fixed, windowhandle should be used to find the screen.
    QScreen *screen = QGuiApplication::screenAt(pos);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    else {
        const qreal screenWidth = screen->geometry().width();
        if (!wordWrap() && sh.width() > screenWidth) {
            setWordWrap(true);
            sh = sizeHint();
        }
    }

    resize(sh + extra);
}

void NotificationLabel::paintEvent(QPaintEvent *ev)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.initFrom(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();
    QLabel::paintEvent(ev);
}

void NotificationLabel::resizeEvent(QResizeEvent *e)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;

    option.initFrom(this);

    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask)) {
        setMask(frameMask.region);
    }

    QLabel::resizeEvent(e);
}

void NotificationLabel::hideNotification()
{
    if (!hideTimer.isActive()) {
        hideTimer.start(300);
    }
}

void NotificationLabel::hideNotificationImmediately()
{
    close(); // to trigger QEvent::Close which stops the animation
    instance = nullptr;
}

bool NotificationLabel::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o)

    switch (e->type()) {
        case QEvent::MouseButtonPress:
        {
             // If minimum on screen time has already lapsed - hide the notification no matter where the click was done
            auto total = expireTimer.interval();
            auto remaining = expireTimer.remainingTime();
            auto lapsed = total - remaining;
            // ... or if the click is inside the notification, hide it no matter if the minimum onscreen time has lapsed or not
            auto insideclick = this->underMouse();
            if( lapsed > minShowTime || insideclick) {
                hideNotification();

                return insideclick;
            }
        }
        default:
            break;
    }
    return false;
}

void NotificationLabel::placeNotificationLabel(const QPoint &pos)
{
    QPoint p = pos;
    const QScreen *screen = QGuiApplication::screenAt(pos);
    // a QScreen's handle *should* never be null, so this is a bit paranoid
    if (screen && screen->handle()) {
        const QSize cursorSize = QSize(16, 16);

        QPoint offset(2, cursorSize.height());
        // assuming an arrow shape, we can just move to the side for very large cursors
        if (cursorSize.height() > 2 * this->height())
            offset = QPoint(cursorSize.width() / 2, 0);

        p += offset;

        QRect screenRect = screen->geometry();

        if (p.x() + this->width() > screenRect.x() + screenRect.width())
            p.rx() -= 4 + this->width();
        if (p.y() + this->height() > screenRect.y() + screenRect.height())
            p.ry() -= 24 + this->height();
        if (p.y() < screenRect.y())
            p.setY(screenRect.y());
        if (p.x() + this->width() > screenRect.x() + screenRect.width())
            p.setX(screenRect.x() + screenRect.width() - this->width());
        if (p.x() < screenRect.x())
            p.setX(screenRect.x());
        if (p.y() + this->height() > screenRect.y() + screenRect.height())
            p.setY(screenRect.y() + screenRect.height() - this->height());
    }

    this->move(p);
}

bool NotificationLabel::notificationLabelChanged(const QString &text)
{
    return NotificationLabel::instance->text() != text;
}

/***************************** NotificationBox **********************************/

void NotificationBox::showText(const QPoint &pos, const QString &text, int displayTime, unsigned int minShowTime)
{
    // a label does already exist
    if (NotificationLabel::instance && NotificationLabel::instance->isVisible()){
        if (text.isEmpty()){ // empty text means hide current label
            NotificationLabel::instance->hideNotification();
            return;
        }
        else {
            // If the label has changed, reuse the one that is showing (removes flickering)
            if (NotificationLabel::instance->notificationLabelChanged(text)){
                NotificationLabel::instance->reuseNotification(text, displayTime, pos);
                NotificationLabel::instance->placeNotificationLabel(pos);
            }
            return;
        }
    }

    // no label can be reused, create new label:
    if (!text.isEmpty()) {
#ifdef Q_OS_WIN32
        // On windows, we can't use the widget as parent otherwise the window will be
        // raised when the toollabel will be shown
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_DEPRECATED
        new NotificationLabel(text,
                              pos,
                              nullptr,
                              displayTime,
                              minShowTime);// NotificationLabel manages its own lifetime.
        QT_WARNING_POP
#else
        new NotificationLabel(text,
                              pos,
                              nullptr,
                              displayTime,
                              minShowTime);// sets NotificationLabel::instance to itself
#endif
        NotificationLabel::instance->placeNotificationLabel(pos);
        NotificationLabel::instance->setObjectName(QLatin1String("NotificationBox_label"));

        NotificationLabel::instance->showNormal();
    }
}

bool NotificationBox::isVisible()
{
    return (NotificationLabel::instance != nullptr && NotificationLabel::instance->isVisible());
}

QString NotificationBox::text()
{
    if (NotificationLabel::instance)
        return NotificationLabel::instance->text();
    return QString();
}

Q_GLOBAL_STATIC(QPalette, notificationbox_palette)

QPalette NotificationBox::palette()
{
    return *notificationbox_palette();
}

QFont NotificationBox::font()
{
    return QApplication::font("NotificationLabel");
}

void NotificationBox::setPalette(const QPalette &palette)
{
    *notificationbox_palette() = palette;
    if (NotificationLabel::instance)
        NotificationLabel::instance->setPalette(palette);
}

void NotificationBox::setFont(const QFont &font)
{
    QApplication::setFont(font, "NotificationLabel");
}

} // namespace Gui

#include "NotificationBox.moc"
