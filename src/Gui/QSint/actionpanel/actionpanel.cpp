#include "actionpanel.h"
#include "actionpanelscheme.h"
#include "actiongroup.h"

#include <QtCore/QVariant>


namespace QSint
{


ActionPanel::ActionPanel(QWidget *parent) :
    BaseClass(parent)
{
    setProperty("class", "panel");

    setScheme(ActionPanelScheme::defaultScheme());

    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    QVBoxLayout *vbl = new QVBoxLayout();
    vbl->setMargin(8);
    vbl->setSpacing(8);
    setLayout(vbl);
}

void ActionPanel::setScheme(ActionPanelScheme *scheme)
{
  if (scheme) {
    myScheme = scheme;
    setStyleSheet(myScheme->actionStyle);

    // set scheme for children
    QObjectList list(children());
    foreach(QObject *obj, list) {
      if (dynamic_cast<ActionGroup*>(obj)) {
        ((ActionGroup*)obj)->setScheme(scheme);
        continue;
      }
    }

    update();
  }
}

//void ActionPanel::paintEvent ( QPaintEvent * event )
//{
//  //QPainter p(this);

//  //p.setOpacity(0.5);
//  //p.fillRect(rect(), myScheme->panelBackground);
//}

void ActionPanel::addWidget(QWidget *w)
{
  if (w)
    layout()->addWidget(w);
}

void ActionPanel::addStretch(int s)
{
  ((QVBoxLayout*)layout())->addStretch(s);
}

ActionGroup * ActionPanel::createGroup()
{
    ActionGroup * group = new ActionGroup(this);
    addWidget(group);
    return group;
}

ActionGroup * ActionPanel::createGroup(const QString &title, bool expandable)
{
    ActionGroup * box = new ActionGroup(title, expandable, this);
    addWidget(box);
    return box;
}

ActionGroup * ActionPanel::createGroup(const QPixmap &icon, const QString &title, bool expandable)
{
    ActionGroup * box = new ActionGroup(icon, title, expandable, this);
    addWidget(box);
    return box;
}


QSize ActionPanel::minimumSizeHint() const
{
    return QSize(200,150);
}


} // namespace
