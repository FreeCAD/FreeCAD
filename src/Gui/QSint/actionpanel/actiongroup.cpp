/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/
#include "actiongroup.h"
#include "taskheader_p.h"
#include "taskgroup_p.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QTimer>

namespace QSint
{

ActionGroup::ActionGroup(QWidget *parent)
    : QWidget(parent),
      myHeader(new TaskHeader(QPixmap(), "", false, this))
{
    myHeader->setVisible(false);
    init(false);
}

ActionGroup::ActionGroup(const QString &title, bool expandable, QWidget *parent)
    : QWidget(parent),
      myHeader(new TaskHeader(QPixmap(), title, expandable, this))
{
    init(true);
}

ActionGroup::ActionGroup(const QPixmap &icon, const QString &title, bool expandable, QWidget *parent)
    : QWidget(parent),
      myHeader(new TaskHeader(icon, title, expandable, this))
{
    init(true);
}

ActionGroup::~ActionGroup() = default;

void ActionGroup::init(bool hasHeader)
{
    m_foldStep = 0;
    myScheme = ActionPanelScheme::defaultScheme();
    setBackgroundRole(QPalette::Button);
    setAutoFillBackground(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(myHeader);

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Raised);
    separator->setFixedHeight(separatorHeight);
    separator->setContentsMargins(8, 0, 8, 0);
    separator->setProperty("class", "separator");
    layout->addWidget(separator);

    myGroup = new TaskGroup(this, hasHeader);
    layout->addWidget(myGroup);

    myDummy = new QWidget(this);
    layout->addWidget(myDummy);
    myDummy->hide();

    connect(myHeader, &TaskHeader::activated, this, &ActionGroup::showHide);
}

void ActionGroup::setScheme(ActionPanelScheme *scheme)
{
    myScheme = scheme;
    myHeader->setScheme(scheme);
    myGroup->setScheme(scheme);
    update();
}

QBoxLayout* ActionGroup::groupLayout()
{
    return myGroup->groupLayout();
}

ActionLabel* ActionGroup::addAction(QAction *action, bool addToLayout, bool addStretch)
{
    if (!action) return nullptr;

    auto *label = new ActionLabel(action, this);
    myGroup->addActionLabel(label, addToLayout, addStretch);

    return label;
}

ActionLabel* ActionGroup::addActionLabel(ActionLabel *label, bool addToLayout, bool addStretch)
{
    if (!label) return nullptr;

    myGroup->addActionLabel(label, addToLayout, addStretch);
    return label;
}

bool ActionGroup::addWidget(QWidget *widget, bool addToLayout, bool addStretch)
{
    return myGroup->addWidget(widget, addToLayout, addStretch);
}

void ActionGroup::showHide()
{
    if (m_foldStep || !myHeader->expandable()) return;

    if (myGroup->isVisible())
    {
        m_foldPixmap = myGroup->transparentRender();
        m_tempHeight = m_fullHeight = myGroup->height();
        m_foldDelta = m_fullHeight / myScheme->groupFoldSteps;
        m_foldStep = myScheme->groupFoldSteps;
        m_foldDirection = -1;

        myGroup->hide();
        myDummy->setFixedSize(myGroup->size());

        QTimer::singleShot(myScheme->groupFoldDelay, this, &ActionGroup::processHide);
    }
    else
    {
        m_foldStep = myScheme->groupFoldSteps;
        m_foldDirection = 1;
        m_tempHeight = 0;

        QTimer::singleShot(myScheme->groupFoldDelay, this, &ActionGroup::processShow);
    }
    myDummy->show();
}

void ActionGroup::processHide()
{
    if (--m_foldStep == 0)
    {
        myDummy->hide();
        myHeader->setFold(false);
        setFixedHeight(myHeader->height() + separatorHeight);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        return;
    }

    m_tempHeight -= m_foldDelta;
    myDummy->setFixedHeight(m_tempHeight);
    setFixedHeight(myDummy->height() + myHeader->height() + separatorHeight);

    QTimer::singleShot(myScheme->groupFoldDelay, this, &ActionGroup::processHide);
}

void ActionGroup::processShow()
{
    if (--m_foldStep == 0)
    {
        myDummy->hide();
        m_foldPixmap = QPixmap();
        myGroup->show();
        myHeader->setFold(true);
        setFixedHeight(m_fullHeight + myHeader->height() + separatorHeight);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        setMaximumHeight(QWIDGETSIZE_MAX);
        return;
    }

    m_tempHeight += m_foldDelta;
    myDummy->setFixedHeight(m_tempHeight);
    setFixedHeight(myDummy->height() + myHeader->height() + separatorHeight);

    QTimer::singleShot(myScheme->groupFoldDelay, this, &ActionGroup::processShow);
}

void ActionGroup::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);

    if (!myDummy->isVisible()) {
        return;
    }
    if (myScheme->groupFoldThaw)
    {
        double opacity = (m_foldDirection < 0)
                                ? static_cast<double>(m_foldStep) / myScheme->groupFoldSteps
                                : static_cast<double>(myScheme->groupFoldSteps - m_foldStep) / myScheme->groupFoldSteps;
        p.setOpacity(opacity);
    }

    switch (myScheme->groupFoldEffect)
    {
        case ActionPanelScheme::ShrunkFolding:
            p.drawPixmap(myDummy->pos(), m_foldPixmap.scaled(myDummy->size()));
            break;
        case ActionPanelScheme::SlideFolding:
            p.drawPixmap(myDummy->pos(), m_foldPixmap,
                            QRect(0, m_foldPixmap.height() - myDummy->height(),
                                m_foldPixmap.width(), myDummy->width()));
            break;
        default:
            p.drawPixmap(myDummy->pos(), m_foldPixmap);
    }
}

bool ActionGroup::isExpandable() const
{
    return myHeader->expandable();
}

void ActionGroup::setExpandable(bool expandable)
{
    myHeader->setExpandable(expandable);
}

bool ActionGroup::hasHeader() const
{
    return myHeader->isVisible();
}

void ActionGroup::setHeader(bool enable)
{
    myHeader->setVisible(enable);
}

QString ActionGroup::headerText() const
{
    return myHeader->myTitle->text();
}

void ActionGroup::setHeaderText(const QString &headerText)
{
    myHeader->myTitle->setText(headerText);
}

void ActionGroup::setHeaderIcon(const QPixmap &icon)
{
    myHeader->myTitle->setIcon(icon);
}

QSize ActionGroup::minimumSizeHint() const
{
    return {200, 65};
}

} // namespace
