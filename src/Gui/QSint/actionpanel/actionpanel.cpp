// SPDX-License-Identifier: LGPL-3.0-only
/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actionpanel.h"
#include "actionpanelscheme.h"
#include "actiongroup.h"

#include <QVariant>

namespace QSint
{

ActionPanel::ActionPanel(QWidget *parent) :
    BaseClass(parent), mySpacer(nullptr)
{
    setProperty("class", "panel");
    setScheme(ActionPanelScheme::defaultScheme());
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    auto *vbl = new QVBoxLayout();
    vbl->setContentsMargins(4, 8, 4, 8);
    vbl->setSpacing(8);
    setLayout(vbl);
}

void ActionPanel::setScheme(ActionPanelScheme *scheme)
{
    if (!scheme) {
        return;
    }
    myScheme = scheme;
    // set scheme for children
    QObjectList list(children());
    Q_FOREACH(QObject *obj, list) {
      if (dynamic_cast<ActionGroup*>(obj)) {
        ((ActionGroup*)obj)->setScheme(scheme);
        continue;
      }
    }
    update();
}

void ActionPanel::addWidget(QWidget *w)
{
    if (w) layout()->addWidget(w);
}

void ActionPanel::removeWidget(QWidget *w)
{
    if (w) layout()->removeWidget(w);
}

void ActionPanel::addStretch()
{
    if (!mySpacer) {
        mySpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
        layout()->addItem(mySpacer);
    }
}

void ActionPanel::removeStretch()
{
    if (mySpacer) {
        layout()->removeItem(mySpacer);
        delete mySpacer;
        mySpacer = nullptr;
    }
}

QSize ActionPanel::minimumSizeHint() const
{
    QSize s = QWidget::minimumSizeHint();
    return {qMax(s.width(), 200), qMax(s.height(), 150)};
}

} // namespace QSint

