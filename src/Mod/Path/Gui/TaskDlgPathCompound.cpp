/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#ifndef _PreComp_
# include <QRegularExpression>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>

#include "TaskDlgPathCompound.h"
#include "ui_TaskDlgPathCompound.h"


using namespace PathGui;
using namespace Gui;

/* TRANSLATOR PathGui::TaskWidgetPathCompound */


//**************************************************************************
// TaskWidget
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskWidgetPathCompound::TaskWidgetPathCompound(ViewProviderPathCompound *CompoundView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Path_Compound"),tr("Compound paths"),true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskDlgPathCompound();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    Path::FeatureCompound* pcCompound = static_cast<Path::FeatureCompound*>(CompoundView->getObject());
    const std::vector<App::DocumentObject*> &Paths = pcCompound->Group.getValues();
    for (std::vector<App::DocumentObject*>::const_iterator it= Paths.begin();it!=Paths.end();++it) {
        QString name = QString::fromLatin1((*it)->getNameInDocument());
        name += QString::fromLatin1(" (");
        name += QString::fromUtf8((*it)->Label.getValue());
        name += QString::fromLatin1(")");
        ui->PathsList->addItem(name);
    }
}

TaskWidgetPathCompound::~TaskWidgetPathCompound()
{
    delete ui;
}

std::vector<std::string> TaskWidgetPathCompound::getList() const {
    std::vector<std::string> names;
    for(int i = 0; i < ui->PathsList->count(); i++)
    {
        QListWidgetItem* item = ui->PathsList->item(i);
        QString name = item->text();
        QStringList result;
        result = name.split(QRegularExpression(QString::fromLatin1("\\s+")));
        std::cout << result[0].toStdString() << std::endl;
        names.push_back(result[0].toStdString());
    }
    return names;
}

void TaskWidgetPathCompound::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPathCompound::TaskDlgPathCompound(PathGui::ViewProviderPathCompound *obj)
    : TaskDialog(),CompoundView(obj)
{
    assert(CompoundView);
    (void)CompoundView; // fix clang warning
    parameter  = new TaskWidgetPathCompound(CompoundView);
    Content.push_back(parameter);
}

TaskDlgPathCompound::~TaskDlgPathCompound()
{
}

//==== calls from the TaskView ===============================================================


void TaskDlgPathCompound::open()
{
}

void TaskDlgPathCompound::clicked(int button)
{
    Q_UNUSED(button);
}

bool TaskDlgPathCompound::accept()
{
    std::vector<App::DocumentObject*> paths;
    Path::FeatureCompound* pcCompound = static_cast<Path::FeatureCompound*>(CompoundView->getObject());
    App::Document* pcDoc = static_cast<App::Document*>(pcCompound->getDocument());
    std::vector<std::string> names = parameter->getList();
    for(std::size_t i = 0; i < names.size(); i++)
    {
        App::DocumentObject* pcPath = static_cast<App::DocumentObject*>(pcDoc->getObject(names[i].c_str()));
        paths.push_back(pcPath);
    }
    pcCompound->Group.setValues(paths);
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    return true;
}

bool TaskDlgPathCompound::reject()
{
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    return true;
}

void TaskDlgPathCompound::helpRequested()
{
}


#include "moc_TaskDlgPathCompound.cpp"
