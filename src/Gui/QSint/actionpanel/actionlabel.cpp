/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actionlabel.h"

#include <QStyleOptionToolButton>
#include <QApplication>


namespace QSint
{


const char* ActionLabelStyle =
    "QSint--ActionLabel[class='action'] {"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "color: #0033ff;"
        "text-align: left;"
        "font: 11px;"
    "}"

    "QSint--ActionLabel[class='action']:!enabled {"
        "color: #999999;"
    "}"

    "QSint--ActionLabel[class='action']:hover {"
        "color: #0099ff;"
        "text-decoration: underline;"
    "}"

    "QSint--ActionLabel[class='action']:focus {"
        "border: 1px dotted black;"
    "}"

    "QSint--ActionLabel[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
    "}"
;


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

    setStyleSheet(QString(ActionLabelStyle));

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

    QSize sizeHint = (style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this).
                  expandedTo(QApplication::globalStrut()));

    return sizeHint;
}

QSize ActionLabel::minimumSizeHint() const
{
    return sizeHint();
}


} // namespace

