/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
#endif

#include <QColorDialog>

#include <boost_bind_bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "ui_TaskElementColors.h"
#include "TaskElementColors.h"
#include "ViewProviderLink.h"

#include <Base/Console.h>
#include <App/ComplexGeoData.h>

#include "Application.h"
#include "Control.h"
#include "Document.h"
#include "MainWindow.h"
#include "Selection.h"
#include "BitmapFactory.h"
#include "Command.h"

#include <App/Document.h>
#include <App/DocumentObject.h>

FC_LOG_LEVEL_INIT("Gui",true,true)

using namespace Gui;
namespace bp = boost::placeholders;

class ElementColors::Private: public Gui::SelectionGate
{
public:
    typedef boost::signals2::connection Connection;
    std::unique_ptr<Ui_TaskElementColors> ui;
    ViewProviderDocumentObject *vp;
    ViewProviderDocumentObject *vpParent;
    Document *vpDoc;
    std::map<std::string,QListWidgetItem*> elements;
    std::vector<QListWidgetItem*> items;
    std::string hiddenSub;
    Connection connectDelDoc;
    Connection connectDelObj;
    QPixmap px;
    bool busy;
    long onTopMode;
    bool touched;

    std::string editDoc;
    std::string editObj;
    std::string editSub;
    std::string editElement;

    Private(ViewProviderDocumentObject* vp, const char *element="") 
        : ui(new Ui_TaskElementColors()), vp(vp),editElement(element)
    {
        vpDoc = vp->getDocument();
        vpParent = vp;
        auto doc = Application::Instance->editDocument();
        if(doc) {
            auto editVp = doc->getInEdit(&vpParent,&editSub);
            if(editVp == vp) {
                auto obj = vpParent->getObject();
                editDoc = obj->getDocument()->getName();
                editObj = obj->getNameInDocument();
                editSub = Data::ComplexGeoData::noElementName(editSub.c_str());
            }
        }
        if(editDoc.empty()) {
            vpParent = vp;
            editDoc = vp->getObject()->getDocument()->getName();
            editObj = vp->getObject()->getNameInDocument();
            editSub.clear();
        }
        onTopMode = vpParent->OnTopWhenSelected.getValue();
        busy = false;
        touched = false;
        int w = QApplication::style()->standardPixmap(QStyle::SP_DirClosedIcon).width();
        px = QPixmap(w,w);
    }

    ~Private() {
        try {
            vpParent->OnTopWhenSelected.setValue(onTopMode);
        }
        catch (const Base::Exception& e) {
            e.ReportException();
        }
    }

    bool allow(App::Document *doc, App::DocumentObject *obj, const char *subname) {
        if(editDoc!=doc->getName() ||
           editObj!=obj->getNameInDocument() ||
           !boost::starts_with(subname,editSub))
            return false;
        if(editElement.empty())
            return true;
        const char *dot = strrchr(subname,'.');
        if(!dot)
            dot = subname;
        else
            ++dot;
        return *dot==0 || boost::starts_with(dot,editElement);
    }

    void populate() {
        int i=0;
        for(auto &v : vp->getElementColors())
            addItem(i++,v.first.c_str());
        apply();
    }

    void addItem(int i,const char *sub, bool push=false) {
        auto itE = elements.find(sub);
        if(i<0 && itE!=elements.end()) {
            if(push && !ViewProvider::hasHiddenMarker(sub))
                items.push_back(itE->second);
            return;
        }

        const char *marker = ViewProvider::hasHiddenMarker(sub);
        if(marker) {
            auto icon = BitmapFactory().pixmap("Invisible");
            QListWidgetItem* item = new QListWidgetItem(icon,
                    QString::fromLatin1(std::string(sub,marker-sub).c_str()), ui->elementList);
            item->setData(Qt::UserRole,QColor());
            item->setData(Qt::UserRole+1,QString::fromLatin1(sub));
            elements.emplace(sub,item);
            return;
        }

        for(auto &v : vp->getElementColors(sub)) {
            auto it = elements.find(v.first.c_str());
            if(it!=elements.end()) {
                if(push)
                    items.push_back(it->second);
                continue;
            }
            auto color = v.second;
            QColor c;
            c.setRgbF(color.r,color.g,color.b,1.0-color.a);
            px.fill(c);
            QListWidgetItem* item = new QListWidgetItem(QIcon(px),
                    QString::fromLatin1(Data::ComplexGeoData::oldElementName(v.first.c_str()).c_str()), 
                    ui->elementList);
            item->setData(Qt::UserRole,c);
            item->setData(Qt::UserRole+1,QString::fromLatin1(v.first.c_str()));
            if(push)
                items.push_back(item);
            elements.emplace(v.first,item);
        }
    }

    void apply() {
        std::map<std::string,App::Color> info;
        int count = ui->elementList->count();
        for(int i=0;i<count;++i) {
            auto item = ui->elementList->item(i);
            auto color = item->data(Qt::UserRole).value<QColor>();
            std::string sub = qPrintable(item->data(Qt::UserRole+1).value<QString>());
            info.emplace(qPrintable(item->data(Qt::UserRole+1).value<QString>()),
                    App::Color(color.redF(),color.greenF(),color.blueF(),1.0-color.alphaF()));
        }
        if(!App::GetApplication().getActiveTransaction())
            App::GetApplication().setActiveTransaction("Set colors");
        vp->setElementColors(info);
        touched = true;
        Selection().clearSelection();
    }

    void reset() {
        touched = false;
        App::GetApplication().closeActiveTransaction(true);
        Selection().clearSelection();
    }

    void accept() {
        if(touched && ui->recompute->isChecked()) {
            auto obj = vp->getObject();
            obj->touch();
            obj->getDocument()->recompute(obj->getInListRecursive());
            touched = false;
        }
        App::GetApplication().closeActiveTransaction();
    }

    void removeAll() {
        if(elements.size()) {
            hiddenSub.clear();
            ui->elementList->clear();
            elements.clear();
            apply();
        }
    }

    void removeItems() {
        for(auto item : ui->elementList->selectedItems()) {
            std::string sub = qPrintable(item->data(Qt::UserRole+1).value<QString>());
            if(sub == hiddenSub)
                hiddenSub.clear();
            elements.erase(sub);
            delete item;
        }
        apply();
    }

    void editItem(QWidget *parent, QListWidgetItem *item) {
        std::string sub = qPrintable(item->data(Qt::UserRole+1).value<QString>());
        if(ViewProvider::hasHiddenMarker(sub.c_str()))
            return;
        auto color = item->data(Qt::UserRole).value<QColor>();
        QColorDialog cd(color, parent);
        cd.setOption(QColorDialog::ShowAlphaChannel);
        if (cd.exec()!=QDialog::Accepted || color==cd.selectedColor())
            return;
        color = cd.selectedColor();
        item->setData(Qt::UserRole,color);
        px.fill(color);
        item->setIcon(QIcon(px));
        apply();
    }
        
    void onSelectionChanged(const SelectionChanges &msg) {
        // no object selected in the combobox or no sub-element was selected
        if (busy)
            return;
        busy = true;
        switch(msg.Type) {
        case SelectionChanges::ClrSelection:
            ui->elementList->clearSelection();
            break;
        case SelectionChanges::AddSelection:
        case SelectionChanges::RmvSelection:
            if(msg.pDocName && msg.pObjectName && msg.pSubName && msg.pSubName[0]) {
                if(editDoc == msg.pDocName &&
                   editObj == msg.pObjectName &&
                   boost::starts_with(msg.pSubName,editSub))
                {
                    for(auto item : ui->elementList->findItems(
                                QString::fromLatin1(msg.pSubName-editSub.size()), Qt::MatchExactly))
                        item->setSelected(msg.Type==SelectionChanges::AddSelection);
                }
            }
        default:
            break;
        }
        busy = false;
    }

    void onSelectionChanged() {
        if(busy) return;
        busy = true;
        std::map<std::string,int> sels;
        for(auto &sel : Selection().getSelectionEx(
                    editDoc.c_str(),App::DocumentObject::getClassTypeId(),0)) 
        {
            if(sel.getFeatName()!=editObj) continue;
            for(auto &sub : sel.getSubNames()) {
                if(boost::starts_with(sub,editSub))
                    sels[sub.c_str()+editSub.size()] = 1;
            }
            break;
        }
        for(auto item : ui->elementList->selectedItems()) {
            std::string name(qPrintable(item->data(Qt::UserRole+1).value<QString>()));
            if(ViewProvider::hasHiddenMarker(name.c_str()))
                continue;
            auto &v = sels[name];
            if(!v)
                Selection().addSelection(editDoc.c_str(),
                        editObj.c_str(), (editSub+name).c_str());
            v = 2;
        }
        for(auto &v : sels) {
            if(v.second!=2) {
                Selection().rmvSelection(editDoc.c_str(),
                        editObj.c_str(), (editSub+v.first).c_str());
            }
        }
        busy = false;
    }
};

/* TRANSLATOR Gui::TaskElementColors */

ElementColors::ElementColors(ViewProviderDocumentObject* vp, bool noHide)
    :d(new Private(vp))
{
    d->ui->setupUi(this);
    d->ui->objectLabel->setText(QString::fromUtf8(vp->getObject()->Label.getValue()));
    d->ui->elementList->setMouseTracking(true); // needed for itemEntered() to work

    if(noHide)
        d->ui->hideSelection->setVisible(false);

    ParameterGrp::handle hPart = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    d->ui->recompute->setChecked(hPart->GetBool("ColorRecompute",true));
    d->ui->onTop->setChecked(hPart->GetBool("ColorOnTop",true));
    if(d->ui->onTop->isChecked()) 
        d->vpParent->OnTopWhenSelected.setValue(3);

    Selection().addSelectionGate(d,0);

    d->connectDelDoc = Application::Instance->signalDeleteDocument.connect(boost::bind
        (&ElementColors::slotDeleteDocument, this, bp::_1));
    d->connectDelObj = Application::Instance->signalDeletedObject.connect(boost::bind
        (&ElementColors::slotDeleteObject, this, bp::_1));

    d->populate();
}

ElementColors::~ElementColors()
{
    d->connectDelDoc.disconnect();
    d->connectDelObj.disconnect();
    Selection().rmvSelectionGate();
}

void ElementColors::on_recompute_clicked(bool checked) {
    ParameterGrp::handle hPart = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    hPart->SetBool("ColorRecompute",checked);
}

void ElementColors::on_onTop_clicked(bool checked) {
    ParameterGrp::handle hPart = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    hPart->SetBool("ColorOnTop",checked);
    d->vpParent->OnTopWhenSelected.setValue(checked?3:d->onTopMode);
}

void ElementColors::slotDeleteDocument(const Document& Doc)
{
    if (d->vpDoc==&Doc || d->editDoc==Doc.getDocument()->getName())
        Control().closeDialog();
}

void ElementColors::slotDeleteObject(const ViewProvider& obj)
{
    if (d->vp==&obj)
        Control().closeDialog();
}

void ElementColors::on_removeSelection_clicked()
{
    d->removeItems();
}

void ElementColors::on_boxSelect_clicked()
{
    auto cmd = Application::Instance->commandManager().getCommandByName("Std_BoxElementSelection");
    if(cmd)
        cmd->invoke(0);
}

void ElementColors::on_hideSelection_clicked() {
    auto sels = Selection().getSelectionEx(d->editDoc.c_str(),App::DocumentObject::getClassTypeId(),0);
    for(auto &sel : sels) {
        if(d->editObj!=sel.getFeatName())
            continue;
        const auto &subs = sel.getSubNames();
        if(subs.size()) {
            for(auto &sub : subs) {
                if(boost::starts_with(sub,d->editSub)) {
                    auto name = Data::ComplexGeoData::noElementName(sub.c_str()+d->editSub.size());
                    name += ViewProvider::hiddenMarker();
                    d->addItem(-1,name.c_str());
                }
            }
            d->apply();
        }
        return;
    }
}

void ElementColors::on_addSelection_clicked()
{
    auto sels = Selection().getSelectionEx(d->editDoc.c_str(),App::DocumentObject::getClassTypeId(),0);
    d->items.clear();
    if(sels.empty())
        d->addItem(-1,"Face",true);
    else {
        for(auto &sel : sels) {
            if(d->editObj!=sel.getFeatName())
                continue;
            const auto &subs = sel.getSubNames();
            if(subs.empty()) {
                d->addItem(-1,"Face",true);
                break;
            }
            for(auto &sub : subs) {
                if(boost::starts_with(sub,d->editSub))
                    d->addItem(-1,sub.c_str()+d->editSub.size(),true);
            }
            break;
        }
    }
    if(d->items.size()) {
        auto color = d->items.front()->data(Qt::UserRole).value<QColor>();
        QColorDialog cd(color, this);
        cd.setOption(QColorDialog::ShowAlphaChannel);
        if (cd.exec()!=QDialog::Accepted)
            return;
        color = cd.selectedColor();
        for(auto item : d->items) {
            item->setData(Qt::UserRole,color);
            d->px.fill(color);
            item->setIcon(QIcon(d->px));
        }
        d->apply();
    }
}

void ElementColors::on_removeAll_clicked()
{
    d->removeAll();
}

bool ElementColors::accept()
{
    d->accept();
    Application::Instance->setEditDocument(0);
    return true;
}

bool ElementColors::reject()
{
    d->reset();
    Application::Instance->setEditDocument(0);
    return true;
}

void ElementColors::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui->retranslateUi(this);
    }
}

void ElementColors::leaveEvent(QEvent *e) {
    QWidget::leaveEvent(e);
    Selection().rmvPreselect();
    if(d->hiddenSub.size()) {
        d->vp->partialRender({d->hiddenSub},false);
        d->hiddenSub.clear();
    }
}

void ElementColors::on_elementList_itemEntered(QListWidgetItem *item) {
    std::string name(qPrintable(item->data(Qt::UserRole+1).value<QString>()));
    if(d->hiddenSub.size()) {
        d->vp->partialRender({d->hiddenSub},false);
        d->hiddenSub.clear();
    }
    if(ViewProvider::hasHiddenMarker(name.c_str())) {
        d->hiddenSub = name;
        d->vp->partialRender({name},true);
        name.resize(name.size()-ViewProvider::hiddenMarker().size());
    }
    Selection().setPreselect(d->editDoc.c_str(),
            d->editObj.c_str(), (d->editSub+name).c_str(),0,0,0,
            d->ui->onTop->isChecked()?2:1);
}

void ElementColors::on_elementList_itemSelectionChanged() {
    d->onSelectionChanged();
}

void ElementColors::onSelectionChanged(const SelectionChanges& msg)
{
    d->onSelectionChanged(msg);
}

void ElementColors::on_elementList_itemDoubleClicked(QListWidgetItem *item) {
    d->editItem(this,item);
}

/* TRANSLATOR Gui::TaskElementColors */

TaskElementColors::TaskElementColors(ViewProviderDocumentObject* vp, bool noHide)
{
    widget = new ElementColors(vp,noHide);
    taskbox = new TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskElementColors::~TaskElementColors()
{
}

void TaskElementColors::open()
{
}

void TaskElementColors::clicked(int)
{
}

bool TaskElementColors::accept()
{
    return widget->accept();
}

bool TaskElementColors::reject()
{
    return widget->reject();
}

#include "moc_TaskElementColors.cpp"
