/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

/// Here the FreeCAD includes sorted by Base,App,Gui......

#include "TaskPanelView.h"
#include "BitmapFactory.h"
#include "iisTaskPanel/include/iisTaskPanel"
#include <Base/Console.h>

using namespace Gui;
using namespace Gui::DockWnd;


/* TRANSLATOR Gui::DockWnd::TaskPanelView */

TaskPanelView::TaskPanelView(Gui::Document* pcDocument, QWidget *parent)
  : DockWindow(pcDocument,parent)
{
#if QT_VERSION <= 0x040104
    // tmp. disable the file logging to suppress some bothering warnings related
    // to Qt 4.1 because it will really pollute the log file with useless stuff
    Base::Console().SetEnabledMsgType("File", ConsoleMsgType::MsgType_Wrn, false);
    Base::Console().SetEnabledMsgType("File", ConsoleMsgType::MsgType_Log, false);
#endif

    setWindowTitle(tr( "Task View"));

    QGridLayout* gridLayout = new QGridLayout(this);
    iisTaskPanel *taskPanel = new iisTaskPanel(this);
    iisTaskBox *tb1 = new iisTaskBox(
        Gui::BitmapFactory().pixmap("document-new"),QLatin1String("Group of Tasks"),true, this);
    taskPanel->addWidget(tb1);
    gridLayout->addWidget(taskPanel, 0, 0, 2, 1);

    iisIconLabel *i1 = new iisIconLabel(
        Gui::BitmapFactory().pixmap("view-zoom-in"), QLatin1String("Do Task 1"), tb1);
    tb1->addIconLabel(i1);
    //connect(i1, SIGNAL(activated()), this, SLOT(task1()));

    iisIconLabel *i2 = new iisIconLabel(
        Gui::BitmapFactory().pixmap("view-zoom-out"), QLatin1String("Do Task 2"), tb1);
    tb1->addIconLabel(i2);

    QHBoxLayout *hbl = new QHBoxLayout();
    tb1->groupLayout()->addLayout(hbl);

    iisIconLabel *i3 = new iisIconLabel(
        Gui::BitmapFactory().pixmap("edit-copy"), QLatin1String("Do Task 3"), tb1);
    tb1->addIconLabel(i3, false);
    hbl->addWidget(i3);

    iisIconLabel *i4 = new iisIconLabel(
        Gui::BitmapFactory().pixmap("edit-cut"), QLatin1String("Do Task 4"), tb1);
    tb1->addIconLabel(i4, false);
    hbl->addWidget(i4);
    i4->setColors(Qt::red, Qt::green, Qt::gray);
    i4->setFocusPen(QPen());

    iisIconLabel *i5 = new iisIconLabel(
        Gui::BitmapFactory().pixmap("edit-paste"), QLatin1String("Do Task 5"), tb1);
    tb1->addIconLabel(i5);

    iisTaskBox *tb2 = new iisTaskBox(
        Gui::BitmapFactory().pixmap("document-print"), QLatin1String("Non-expandable Group"), false, this);
    taskPanel->addWidget(tb2);

    iisIconLabel *i21 = new iisIconLabel(
        Gui::BitmapFactory().pixmap("document-new"), QLatin1String("Do Task 2.1"), tb2);
    tb2->addIconLabel(i21);

    iisIconLabel *i22 = new iisIconLabel(
        Gui::BitmapFactory().pixmap("document-open"), QLatin1String("Do Task 2.2"), tb2);
    tb2->addIconLabel(i22);
    i22->setEnabled(false);

    iisIconLabel *i23 = new iisIconLabel(
        Gui::BitmapFactory().pixmap("document-save"), QLatin1String("Do Task 2.3"), tb2);
    tb2->addIconLabel(i23);

    iisTaskBox *tb3 = new iisTaskBox(QPixmap(), QLatin1String("Group without Icons"), true, this);
    taskPanel->addWidget(tb3);

    iisIconLabel *i31 = new iisIconLabel(QPixmap(), QLatin1String("Do Task 3.1"), tb3);
    tb3->addIconLabel(i31);

    iisIconLabel *i32 = new iisIconLabel(QPixmap(), QLatin1String("Do Task 3.2"), tb3);
    tb3->addIconLabel(i32);

    tb3->groupLayout()->addWidget(new QLabel(QLatin1String("Widgets also allowed:"), this));
    tb3->groupLayout()->addWidget(new QPushButton(QLatin1String("A Button"), this));

    // Other widgets can be also added to the panel
    QLabel *l1 = new QLabel(QLatin1String("A group without header"), this);
    taskPanel->addWidget(l1);


    iisTaskGroup *tb4 = new iisTaskGroup(this);
    taskPanel->addWidget(tb4);

    iisIconLabel *i41 = new iisIconLabel(
        Gui::BitmapFactory().pixmap("system-log-out"), QLatin1String("Do Task 4.1"), tb4);
    tb4->addIconLabel(i41);

    iisIconLabel *i42 = new iisIconLabel(QPixmap(), QLatin1String("Do Task 4.2"), tb4);
    tb4->addIconLabel(i42);

    taskPanel->addStretch();
    taskPanel->setScheme(iisWinXPTaskPanelScheme::defaultScheme());
    //tb1->setScheme(iisWinXPTaskPanelScheme::defaultScheme());
    tb2->setScheme(iisWinXPTaskPanelScheme2::defaultScheme());
    tb3->setScheme(iisWinXPTaskPanelScheme2::defaultScheme());
    //tb4->setScheme(iisWinXPTaskPanelScheme::defaultScheme());

    onUpdate();

    Gui::Selection().Attach(this);

#if QT_VERSION <= 0x040104
    Base::Console().SetEnabledMsgType("File", ConsoleMsgType::MsgType_Wrn, true);
    Base::Console().SetEnabledMsgType("File", ConsoleMsgType::MsgType_Log, true);
#endif
}

TaskPanelView::~TaskPanelView()
{
    Gui::Selection().Detach(this);
}

/// @cond DOXERR
void TaskPanelView::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                            Gui::SelectionSingleton::MessageType Reason)
{
    std::string temp;

    if (Reason.Type == SelectionChanges::AddSelection) {
    }
    else if (Reason.Type == SelectionChanges::ClrSelection) {
    }
    else if (Reason.Type == SelectionChanges::RmvSelection) {
    }
    else if (Reason.Type == SelectionChanges::RmvSelection) {
    }
}

void TaskPanelView::onUpdate(void)
{
}

bool TaskPanelView::onMsg(const char* pMsg)
{
    // no messages yet
    return false;
}
/// @endcond

#include "moc_TaskPanelView.cpp"
