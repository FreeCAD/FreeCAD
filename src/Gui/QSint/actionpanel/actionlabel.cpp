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

    QString s = text().isEmpty() ? QString::fromLatin1("XXXX") : text();
    QSize textSize = fontMetrics().size(Qt::TextShowMnemonic, s);
    const int padding = 10;

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

void ActionLabel::setDefaultAction(QAction *action)
{
    if (defaultAction()) {
        disconnect(defaultAction(), &QAction::changed, this, &ActionLabel::updateActionText);
    }

    QToolButton::setDefaultAction(action);

    if (action) {
        connect(action, &QAction::changed, this, &ActionLabel::updateActionText);
        updateActionText();  // Ensure the text is updated immediately
    }
}

void ActionLabel::updateActionText()
{
    if (defaultAction()) {
        setText(defaultAction()->text());
    }
}

void ActionLabel::setText(const QString &text)
{
    QToolButton::setText(" " + text.trimmed());
}

} // namespace QSint
