// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "WindowDecorationButton.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>

WindowDecorationButton::WindowDecorationButton(Role role, QWidget *parent)
    : QAbstractButton(parent)
    , m_role(role)
{
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover);
    updateObjectName();
    updateText();
}

void WindowDecorationButton::setRole(Role role)
{
    if (m_role == role) return;
    m_role = role;
    updateObjectName();
    updateText();
    update();
}

void WindowDecorationButton::updateObjectName()
{
    switch (m_role) {
    case Minimize: setObjectName(QStringLiteral("minimizeButton")); break;
    case Maximize: setObjectName(QStringLiteral("maximizeButton")); break;
    case Restore:  setObjectName(QStringLiteral("maximizeButton")); break;
    case Close:    setObjectName(QStringLiteral("closeButton")); break;
    }
}

void WindowDecorationButton::updateText()
{
    switch (m_role) {
    case Minimize: setText(QStringLiteral("\u2500")); break; // ─
    case Maximize: setText(QStringLiteral("\u25A1")); break; // □
    case Restore:  setText(QStringLiteral("\u29C9")); break; // ⧉
    case Close:    setText(QStringLiteral("\u2715")); break; // ✕
    }
}

QSize WindowDecorationButton::sizeHint() const
{
    int h = parentWidget() ? parentWidget()->height() : 30;
    return {qMax(h + 8, 46), h};
}

void WindowDecorationButton::initStyleOption(QStyleOptionButton *option) const
{
    option->initFrom(this);
    option->text = text();
    if (isDown())
        option->state |= QStyle::State_Sunken;
}

void WindowDecorationButton::paintEvent(QPaintEvent *)
{
    QStyleOptionButton opt;
    initStyleOption(&opt);

    QPainter p(this);

    // Draw background (resolves :hover/:pressed from QSS)
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    // Draw text icon (resolves color from QSS including :hover)
    // Shift up slightly — Unicode symbols often sit below visual center
    if (m_role == Close) {
        opt.rect.adjust(0, -1, 0, -1);
    }
    style()->drawControl(QStyle::CE_PushButtonLabel, &opt, &p, this);
}
