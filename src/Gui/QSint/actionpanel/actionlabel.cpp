/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actionlabel.h"

#include <QApplication>
#include <QStyleOptionToolButton>
#include <QAction>
#include <QPainter>
#include <cmath>

namespace QSint
{

ActionLabel::ActionLabel(QWidget *parent)
    : QToolButton(parent)
{
    init();
}

ActionLabel::ActionLabel(QAction *action, QWidget *parent)
    : QToolButton(parent)
{
    init();
    setDefaultAction(action);
}

void ActionLabel::init()
{
    setProperty("class", "action");
    setCursor(Qt::PointingHandCursor);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setFocusPolicy(Qt::StrongFocus);
}

QSize ActionLabel::sizeHint() const
{
    ensurePolished();

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QString s = text().isEmpty() ? QStringLiteral("XXXX") : text();
    QSize textSize = fontMetrics().size(Qt::TextShowMnemonic, s);
    constexpr int padding = 10;

    int width = textSize.width();
    if (!icon().isNull()) {
        width += opt.iconSize.width() + padding;
    }

    if (menu()) {
        width += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);
    }

    int height = qMax(textSize.height(), opt.iconSize.height());

    return style()->sizeFromContents(
        QStyle::CT_PushButton, &opt, QSize(width + 2 * padding, height + padding), this
    );
}

QSize ActionLabel::minimumSizeHint() const
{
    return sizeHint();
}

void ActionLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    constexpr int elementSpacing = 5;
    int iconSize = fontMetrics().height();
    QIcon icon = this->icon();
    QString text = this->text();

    QRect contentRect = rect().adjusted(elementSpacing, 0, -elementSpacing, 0); // Apply margins

    int currentX = contentRect.left();

    if (!icon.isNull()) {
        QPixmap pixmap = icon.pixmap(iconSize, iconSize);
        QRect iconRect(currentX, contentRect.center().y() - iconSize / 2, iconSize, iconSize);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(iconRect, pixmap);
        currentX += iconSize + elementSpacing;
    }

    if (!text.isEmpty()) {
        QRect textRect(currentX, contentRect.top(), contentRect.right() - currentX, contentRect.height());

        QFont font = this->font();
        font.setUnderline(underMouse());
        painter.setFont(font);

        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);
    }
}

} // namespace QSint
