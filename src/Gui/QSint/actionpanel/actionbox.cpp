/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actionbox.h"

#include <QVariant>


namespace QSint
{


const char* ActionBoxStyle =
    "QSint--ActionBox {"
        "background-color: white;"
        "border: 1px solid white;"
        "border-radius: 3px;"
        "text-align: left;"
    "}"

    "QSint--ActionBox:hover {"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #F9FDFF, stop: 1 #EAF7FF);"
        "border: 1px solid #DAF2FC;"
    "}"


    "QSint--ActionBox QSint--ActionLabel[class='header'] {"
        "text-align: left;"
        "font: 14px;"
        "color: #006600;"
        "background-color: transparent;"
        "border: none;"
    "}"

    "QSint--ActionBox QSint--ActionLabel[class='header']:hover {"
        "color: #00cc00;"
        "text-decoration: underline;"
    "}"


    "QSint--ActionBox QSint--ActionLabel[class='action'] {"
        "background-color: transparent;"
        "border: none;"
        "color: #0033ff;"
        "text-align: left;"
        "font: 11px;"
    "}"

    "QSint--ActionBox QSint--ActionLabel[class='action']:!enabled {"
        "color: #999999;"
    "}"

    "QSint--ActionBox QSint--ActionLabel[class='action']:hover {"
        "color: #0099ff;"
        "text-decoration: underline;"
    "}"

    "QSint--ActionBox QSint--ActionLabel[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
    "}"

;


ActionBox::ActionBox(QWidget *parent) :
    QFrame(parent)
{
    init();
}

ActionBox::ActionBox(const QString & headerText, QWidget *parent) :
    QFrame(parent)
{
    init();
    headerLabel->setText(headerText);
}

ActionBox::ActionBox(const QPixmap & icon, const QString & headerText, QWidget *parent) :
    QFrame(parent)
{
    init();
    headerLabel->setText(headerText);
    setIcon(icon);
}

void ActionBox::init()
{
    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);

    setStyleSheet(QString(ActionBoxStyle));

    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    QVBoxLayout *iconLayout = new QVBoxLayout();
    mainLayout->addLayout(iconLayout);

    iconLabel = new QLabel(this);
    iconLayout->addWidget(iconLabel);
    iconLayout->addStretch();

    dataLayout = new QVBoxLayout();
    mainLayout->addLayout(dataLayout);

    headerLabel = createItem("");
    headerLabel->setProperty("class", "header");
}

void ActionBox::setIcon(const QPixmap & icon)
{
    iconLabel->setPixmap(icon);
    iconLabel->setFixedSize(icon.size());
}

QPixmap ActionBox::icon() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    return iconLabel->pixmap(Qt::ReturnByValue);
#else
    QPixmap p;
    const QPixmap* ptr = iconLabel->pixmap();
    if (ptr)
        p = *ptr;
    return p;
#endif
}

ActionLabel* ActionBox::createItem(QAction * action, QLayout * l)
{
    if (!action)
        return nullptr;

    ActionLabel *act = createItem("", l);
    act->setDefaultAction(action);
    return act;
}

QList<ActionLabel*> ActionBox::createItems(QList<QAction*> actions)
{
    QList<ActionLabel*> list;

    if (actions.isEmpty())
        return list;

    QLayout *l = createHBoxLayout();

    Q_FOREACH (QAction *action, actions) {
        ActionLabel *act = createItem(action, l);
        if (act)
            list.append(act);
    }

    return list;
}

ActionLabel* ActionBox::createItem(const QString & text, QLayout * l)
{
    ActionLabel *act = new ActionLabel(this);
    act->setText(text);
    act->setProperty("class", "action");
    act->setStyleSheet("");

    if (l)
        l->addWidget(act);
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

    if (l) // add horizontal spacer
        l->addItem(spacer = new QSpacerItem(1,0,QSizePolicy::MinimumExpanding,QSizePolicy::Ignored));
    else // add vertical spacer
        dataLayout->addItem(spacer = new QSpacerItem(0,1,QSizePolicy::Ignored,QSizePolicy::MinimumExpanding));

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
    if (!w)
        return;

    w->setParent(this);

    if (l)
        l->addWidget(w);
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
