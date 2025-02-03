/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actionlabel.h"

#include <QApplication>
#include <QStyleOptionToolButton>


namespace QSint
{


ActionLabel::ActionLabel(QWidget *parent) :
    QToolButton(parent)
{
    init();
}

ActionLabel::ActionLabel(QAction *action, QWidget *parent) :
    QToolButton(parent)
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

    int w = 0, h = 0;

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QString s(text());
    bool empty = s.isEmpty();
    if (empty)
        s = QString::fromLatin1("XXXX");
    QFontMetrics fm = fontMetrics();
    QSize sz = fm.size(Qt::TextShowMnemonic, s);
    w += sz.width();
    h = qMax(h, sz.height());
    opt.rect.setSize(QSize(w, h)); // PM_MenuButtonIndicator depends on the height

    if (!icon().isNull()) {
        int ih = opt.iconSize.height();
        int iw = opt.iconSize.width() + 4;
        w += iw;
        h = qMax(h, ih);
    }

    if (menu())
        w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);

    h += 4;
    w += 8;

    QSize sizeHint = style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this);

    return sizeHint;
}

QSize ActionLabel::minimumSizeHint() const
{
    return sizeHint();
}


} // namespace

