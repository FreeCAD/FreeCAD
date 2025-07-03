/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actionbox.h"

#include <QVariant>
#include <QStyleOptionFrame>

namespace QSint
{

ActionBox::ActionBox(QWidget *parent)
    : QFrame(parent)
{
    init();
}

ActionBox::ActionBox(const QString & headerText, QWidget *parent)
    : QFrame(parent)
{
    init(headerText);
}

ActionBox::ActionBox(const QPixmap & icon, const QString & headerText, QWidget *parent)
    : QFrame(parent)
{
    init(headerText);
    setIcon(icon);
}

void ActionBox::init(const QString &headerText)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    setFrameStyle(QFrame::NoFrame);

    auto *mainLayout = new QHBoxLayout(this);

    auto *iconLayout = new QVBoxLayout();
    mainLayout->addLayout(iconLayout);

    iconLabel = new QLabel(this);
    iconLayout->addWidget(iconLabel);
    iconLayout->addStretch();

    dataLayout = new QVBoxLayout();
    mainLayout->addLayout(dataLayout);

    headerLabel = createItem(headerText);
    headerLabel->setProperty("class", "header");
}

void ActionBox::setIcon(const QPixmap & icon)
{
    iconLabel->setPixmap(icon);
    iconLabel->setFixedSize(icon.size());
}

QPixmap ActionBox::icon() const
{
    return iconLabel->pixmap(Qt::ReturnByValue);
}

ActionLabel* ActionBox::createItem(QAction * action, QLayout * l)
{
    if (!action) {
        return nullptr;
    }
    ActionLabel *act = createItem("", l);
    act->setDefaultAction(action);
    return act;
}

QList<ActionLabel*> ActionBox::createItems(QList<QAction*> actions)
{
    QList<ActionLabel*> list;

    if (actions.isEmpty()) {
        return list;
    }
    QLayout *l = createHBoxLayout();

    for (QAction *action : actions) {
        if (auto *act = createItem(action, l)) {
            list.append(act);
        }
    }

    return list;
}

ActionLabel* ActionBox::createItem(const QString & text, QLayout * l)
{
    ActionLabel *act = new ActionLabel(this);
    act->setText(text);
    act->setProperty("class", "action");

    if (l) {
        l->addWidget(act);
    }
    else {
        QHBoxLayout *hbl = new QHBoxLayout();
        hbl->addWidget(act);
        createSpacer(hbl);
        dataLayout->addLayout(hbl);
    }

    return act;
}

ActionLabel* ActionBox::createItem(const QPixmap & icon, const QString & text, QLayout * l)
{
    ActionLabel *act = createItem(text, l);
    act->setIcon(QIcon(icon));
    return act;
}

QSpacerItem* ActionBox::createSpacer(QLayout * l)
{
    QSpacerItem * spacer;

    if (l) {
        // add horizontal spacer
        l->addItem(spacer = new QSpacerItem(1,0,QSizePolicy::MinimumExpanding,QSizePolicy::Ignored));
    }
    else {
        // add vertical spacer
        dataLayout->addItem(spacer = new QSpacerItem(0,1,QSizePolicy::Ignored,QSizePolicy::MinimumExpanding));
    }
    return spacer;
}

QLayout* ActionBox::createHBoxLayout()
{
    QHBoxLayout *hbl = new QHBoxLayout();
    dataLayout->addLayout(hbl);
    QHBoxLayout *hbl1 = new QHBoxLayout();
    hbl->addLayout(hbl1);
    createSpacer(hbl);
    return hbl1;
}

void ActionBox::addLayout(QLayout * l)
{
    if (l) {
        dataLayout->addLayout(l);
        l->setParent(this);
    }
}

void ActionBox::addWidget(QWidget * w, QLayout * l)
{
    if (!w) {
        return;
    }
    w->setParent(this);

    if (l) {
        l->addWidget(w);
    }
    else {
        QHBoxLayout *hbl = new QHBoxLayout();
        hbl->addWidget(w);
        createSpacer(hbl);
        dataLayout->addLayout(hbl);
    }
}

QSize ActionBox::minimumSizeHint() const
{
    return {150,65};
}

} // namespace
