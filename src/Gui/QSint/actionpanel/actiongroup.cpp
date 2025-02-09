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
#include <QApplication>
#include <QStyle>

namespace QSint
{

ActionGroup::ActionGroup(QWidget *parent)
    : QWidget(parent),
      myHeader(std::make_unique<TaskHeader>(QPixmap(), "", false, this))
{
    myHeader->setVisible(false);
    init(false);
}

ActionGroup::ActionGroup(const QString &title, bool expandable, QWidget *parent)
    : QWidget(parent),
      myHeader(std::make_unique<TaskHeader>(QPixmap(), title, expandable, this))
{
    init(true);
}

ActionGroup::ActionGroup(const QPixmap &icon, const QString &title, bool expandable, QWidget *parent)
    : QWidget(parent),
      myHeader(std::make_unique<TaskHeader>(icon, title, expandable, this))
{
    init(true);
}

ActionGroup::~ActionGroup() = default;

void ActionGroup::init(bool hasHeader)
{
    m_foldStep = 0;

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(myHeader.get());

    myGroup = std::make_unique<TaskGroup>(this, hasHeader);
    layout->addWidget(myGroup.get());

    myDummy = std::make_unique<QWidget>(this);
    layout->addWidget(myDummy.get());
    myDummy->hide();

    connect(myHeader.get(), &TaskHeader::activated, this, &ActionGroup::showHide);

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
        m_tempHeight = m_fullHeight = myGroup->height();
        m_foldDelta = m_fullHeight / 20; // foldsteps
        m_foldStep = 20;
        m_foldDirection = -1;

        myGroup->hide();
        myDummy->setFixedSize(myGroup->size());
        QTimer::singleShot(15, this, &ActionGroup::processHide);
    }
    else
    {
        m_foldStep = 20;
        m_foldDirection = 1;
        m_tempHeight = 0;

        QTimer::singleShot(15, this, &ActionGroup::processShow);
    }
    myDummy->show();
}

void ActionGroup::processHide()
{
    if (--m_foldStep == 0)
    {
        myDummy->hide();
        myHeader->setFold(false);
        setFixedHeight(myHeader->height());
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        return;
    }

    m_tempHeight -= m_foldDelta;
    myDummy->setFixedHeight(m_tempHeight);
    setFixedHeight(myDummy->height() + myHeader->height());

    QTimer::singleShot(15, this, &ActionGroup::processHide);
}

void ActionGroup::processShow()
{
    if (--m_foldStep == 0)
    {
        myDummy->hide();
        m_foldPixmap = QPixmap();
        myGroup->show();
        myHeader->setFold(true);
        setFixedHeight(m_fullHeight + myHeader->height());
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        setMaximumHeight(QWIDGETSIZE_MAX);
        return;
    }

    m_tempHeight += m_foldDelta;
    myDummy->setFixedHeight(m_tempHeight);
    setFixedHeight(myDummy->height() + myHeader->height());

    QTimer::singleShot(15, this, &ActionGroup::processShow);
}

void ActionGroup::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);

    int foldsteps = 20;
    bool groupFoldThaw = true; // Change opacity gradually
    FoldEffect groupFoldEffect = NoFolding;

    if (m_foldPixmap.isNull()) return;

    if (myDummy->isVisible())
    {
        if (groupFoldThaw)
        {
            double opacity = (m_foldDirection < 0)
                                 ? static_cast<double>(m_foldStep) / foldsteps
                                 : static_cast<double>(foldsteps - m_foldStep) / foldsteps;
            p.setOpacity(opacity);
        }

        switch (groupFoldEffect)
        {
            case ShrunkFolding:
                p.drawPixmap(myDummy->pos(), m_foldPixmap.scaled(myDummy->size()));
                break;
            case SlideFolding:
                p.drawPixmap(myDummy->pos(), m_foldPixmap,
                             QRect(0, m_foldPixmap.height() - myDummy->height(),
                                   m_foldPixmap.width(), myDummy->height()));
                break;
            default:
                p.drawPixmap(myDummy->pos(), m_foldPixmap);
        }
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
