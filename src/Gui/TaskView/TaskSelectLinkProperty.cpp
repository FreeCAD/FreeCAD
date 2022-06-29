/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Gui/BitmapFactory.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SelectionObject.h>

#include "ui_TaskSelectLinkProperty.h"
#include "TaskSelectLinkProperty.h"


using namespace Gui::TaskView;

/* TRANSLATOR Gui::TaskView::TaskSelectLinkProperty */

TaskSelectLinkProperty::TaskSelectLinkProperty(const char *sFilter,App::Property *prop,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("mouse_pointer"),tr("edit selection"),true, parent),Filter(nullptr),LinkSub(nullptr),LinkList(nullptr)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskSelectLinkProperty();
    ui->setupUi(proxy);

    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);
    Gui::Selection().Attach(this);

    ui->Remove->setIcon(Gui::BitmapFactory().iconFromTheme("delete"));
    ui->Add->setIcon(Gui::BitmapFactory().iconFromTheme("list-add"));
    ui->Invert->setIcon(Gui::BitmapFactory().iconFromTheme("list-remove"));
    ui->Help->setIcon(Gui::BitmapFactory().iconFromTheme("help-browser"));

    // deactivate not implemented stuff
    ui->Remove->setDisabled(true);
    ui->Add->setDisabled(true);
    ui->Invert->setDisabled(true);
    ui->Help->setDisabled(true);

    // property have to be set! 
    assert(prop);
    StartObject = nullptr;
    if (prop->getTypeId().isDerivedFrom(App::PropertyLinkSub::getClassTypeId())) {
        LinkSub = dynamic_cast<App::PropertyLinkSub *>(prop);
    }
    else if (prop->getTypeId().isDerivedFrom(App::PropertyLinkList::getClassTypeId())) {
        LinkList = dynamic_cast<App::PropertyLinkList *>(prop);
    }
    else {
        Base::Console().Warning("Unknown Link property type in "
            "Gui::TaskView::TaskSelectLinkProperty::TaskSelectLinkProperty()");
    }

    setFilter(sFilter);
}

TaskSelectLinkProperty::~TaskSelectLinkProperty()
{
    delete ui;
    Gui::Selection().Detach(this);
}

void TaskSelectLinkProperty::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

/// @cond DOXERR


bool TaskSelectLinkProperty::setFilter(const char * sFilter)
{
    Filter = new SelectionFilter(sFilter);
    return Filter->isValid();
}


void TaskSelectLinkProperty::activate(void)
{
    // first clear the selection
    Gui::Selection().clearSelection();
    // set the gate for the filter 
    Gui::Selection().addSelectionGate(new SelectionFilterGate(Filter));

    // In case of LinkSub property 
    if (LinkSub) {
        // save the start values for a cnacel operation (reject())
        StartValueBuffer = LinkSub->getSubValues();
        StartObject      = LinkSub->getValue();
        if(StartObject) {
            std::string ObjName = StartObject->getNameInDocument();
            std::string DocName = StartObject->getDocument()->getName();

            for (std::vector<std::string>::const_iterator it = StartValueBuffer.begin();it!=StartValueBuffer.end();++it)
            {
                Gui::Selection().addSelection(DocName.c_str(),ObjName.c_str(),it->c_str());
            }
        }
        
    }
    // In case of LinkList property 
    else if (LinkList) {
        // save the start values for a cnacel operation (reject())
        const std::vector<App::DocumentObject*> &Values = LinkList->getValues();
        for(std::vector<App::DocumentObject*>::const_iterator it = Values.begin();it!=Values.end();++it)
        {
            std::string ObjName = (*it)->getNameInDocument();
            std::string DocName = (*it)->getDocument()->getName();
            Gui::Selection().addSelection(DocName.c_str(),ObjName.c_str());
        }
    }

    checkSelectionStatus();
}

bool TaskSelectLinkProperty::accept(void)
{
    // set the proptery with the selection
    sendSelection2Property();

    // clear selection and remove gate (return to normal operation)
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
    return true;
}

bool TaskSelectLinkProperty::reject(void)
{
    if(LinkSub){
        // restore the old values
        LinkSub->setValue(StartObject,StartValueBuffer);
    }

    // clear selection and remove gate (return to normal operation)
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
    return true;
}

void TaskSelectLinkProperty::sendSelection2Property(void)
{
    if (LinkSub) {
        std::vector<Gui::SelectionObject> temp = Gui::Selection().getSelectionEx();
        assert(temp.size() >= 1);
        LinkSub->setValue(temp[0].getObject(),temp[0].getSubNames());
    }
    else if (LinkList) {
        std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx();
        std::vector<App::DocumentObject*> temp;
        for (std::vector<Gui::SelectionObject>::iterator it=sel.begin();it!=sel.end();++it)
            temp.push_back(it->getObject());
        
        LinkList->setValues(temp);
    }

}

void TaskSelectLinkProperty::checkSelectionStatus(void)
{
    QPalette palette(QApplication::palette());

    if (Filter->match()) {
        palette.setBrush(QPalette::Base,QColor(200,250,200));
        Q_EMIT emitSelectionFit();
    }
    else {
        palette.setBrush(QPalette::Base,QColor(250,200,200));
        Q_EMIT emitSelectionMisfit();
    }
    //ui->listWidget->setAutoFillBackground(true);
    ui->listWidget->setPalette(palette);
}

void TaskSelectLinkProperty::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                                      Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller); 
    if (Reason.Type == SelectionChanges::AddSelection ||
        Reason.Type == SelectionChanges::RmvSelection ||
        Reason.Type == SelectionChanges::SetSelection ||
        Reason.Type == SelectionChanges::ClrSelection) {
            ui->listWidget->clear();
            std::vector<Gui::SelectionSingleton::SelObj> sel = Gui::Selection().getSelection();
            for (std::vector<Gui::SelectionSingleton::SelObj>::const_iterator it=sel.begin();it!=sel.end();++it){
                std::string temp;
                temp += it->FeatName;
                if (strcmp(it->SubName, "") != 0){
                    temp += "::";
                    temp += it->SubName;
                }
                new QListWidgetItem(QString::fromLatin1(temp.c_str()), ui->listWidget);
            }
            checkSelectionStatus();
    }
}
/// @endcond

void TaskSelectLinkProperty::on_Remove_clicked(bool)
{
}

void TaskSelectLinkProperty::on_Add_clicked(bool)
{
}

void TaskSelectLinkProperty::on_Invert_clicked(bool)
{
}

void TaskSelectLinkProperty::on_Help_clicked(bool)
{
}


#include "moc_TaskSelectLinkProperty.cpp"
