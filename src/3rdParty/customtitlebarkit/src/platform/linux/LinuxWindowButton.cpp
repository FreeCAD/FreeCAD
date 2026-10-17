// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "LinuxWindowButton.h"

#include <QApplication>
#include <QEnterEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QPainter>
#include <QPalette>
#include <QWidget>

// --- LinuxButtonGroup ---

LinuxButtonGroup::LinuxButtonGroup(QWidget *groupWidget)
    : QObject(groupWidget)
{
    groupWidget->setAttribute(Qt::WA_Hover);
    groupWidget->installEventFilter(this);
}

bool LinuxButtonGroup::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::HoverEnter) {
        hovered = true;
        updateButtons();
    } else if (event->type() == QEvent::HoverLeave) {
        hovered = false;
        updateButtons();
    }
    return false;
}

void LinuxButtonGroup::updateButtons()
{
    for (auto *btn : parent()->findChildren<QAbstractButton *>())
        btn->update();
}

// --- LinuxSymmetricMarginFilter ---

LinuxSymmetricMarginFilter::LinuxSymmetricMarginFilter(QHBoxLayout *layout, int buttonSize,
                                                       QObject *parent)
    : QObject(parent), m_layout(layout), m_buttonSize(buttonSize)
{
}

bool LinuxSymmetricMarginFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        int h = static_cast<QWidget *>(obj)->height();
        int margin = (h - m_buttonSize) / 2;
        m_layout->setContentsMargins(margin, 0, margin, 0);
    }
    return false;
}

// --- LinuxWindowButton ---

LinuxWindowButton::LinuxWindowButton(Role role, LinuxButtonGroup *group, QWidget *parent)
    : QAbstractButton(parent), m_role(role), m_group(group)
{
    setFixedSize(ButtonSize, ButtonSize);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover);
    setCursor(Qt::ArrowCursor);
}

QSize LinuxWindowButton::sizeHint() const
{
    return {ButtonSize, ButtonSize};
}

QIcon LinuxWindowButton::themeIcon() const
{
    switch (m_role) {
    case Close:    return QIcon::fromTheme(QStringLiteral("window-close-symbolic"));
    case Minimize: return QIcon::fromTheme(QStringLiteral("window-minimize-symbolic"));
    case Maximize: return QIcon::fromTheme(QStringLiteral("window-maximize-symbolic"));
    }
    return {};
}


void LinuxWindowButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QPalette &pal = palette();
    const bool hovered = underMouse();
    const bool pressed = isDown();

    // Fill with an opaque base so semi-transparent overlays don't accumulate
    // when the backing store isn't cleared (WA_TranslucentBackground windows).
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(rect(), pal.window());
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // GNOME/Adwaita style: subtle circle background, uniform for all buttons
    {
        QColor base = pal.color(QPalette::WindowText);
        QColor bg;
        if (pressed)
            bg = QColor(base.red(), base.green(), base.blue(), 60);
        else if (hovered)
            bg = QColor(base.red(), base.green(), base.blue(), 40);
        else
            bg = QColor(base.red(), base.green(), base.blue(), 20);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(QRectF(rect()), 6, 6);
    }

    // Icon — always visible (GNOME style)
    // Paint the system theme icon directly, centered in the button
    QIcon icon = themeIcon();
    if (!icon.isNull()) {
        const int iconSize = ButtonSize * 2 / 3;  // e.g. 16px icon in 24px button
        QRect iconRect(0, 0, iconSize, iconSize);
        iconRect.moveCenter(rect().center());
        icon.paint(&p, iconRect);
        return;
    }

    // Fallback: thin vector icons matching -symbolic weight
    QColor fg = pal.color(QPalette::WindowText);
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0;
    QPen pen(fg, 1.0, Qt::SolidLine, Qt::RoundCap);
    p.setPen(pen);

    switch (m_role) {
    case Close: {
        const qreal s = 4.0;
        p.drawLine(QPointF(cx - s, cy - s), QPointF(cx + s, cy + s));
        p.drawLine(QPointF(cx + s, cy - s), QPointF(cx - s, cy + s));
        break;
    }
    case Minimize: {
        const qreal s = 4.0;
        p.drawLine(QPointF(cx - s, cy), QPointF(cx + s, cy));
        break;
    }
    case Maximize: {
        p.setBrush(Qt::NoBrush);
        const qreal s = 4.0;
        p.drawRect(QRectF(cx - s, cy - s, 2 * s, 2 * s));
        break;
    }
    }
}
