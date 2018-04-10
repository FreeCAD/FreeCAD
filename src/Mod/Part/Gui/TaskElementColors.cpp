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

#include <boost/signals.hpp>
#include <boost/bind.hpp>

#include "ui_TaskElementColors.h"
#include "TaskElementColors.h"
#include "ViewProviderExt.h"

#include <Base/Console.h>

#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Mod/Part/App/PartFeature.h>

FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace PartGui;

namespace PartGui {
    class ElementSelection : public Gui::SelectionFilterGate
    {
        const App::DocumentObject* object;
    public:
        ElementSelection(const App::DocumentObject* obj)
            : Gui::SelectionFilterGate((Gui::SelectionFilter*)0), object(obj)
        {
        }
        bool allow(App::Document* /*pDoc*/, App::DocumentObject*pObj, const char*sSubName)
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            return true;
        }
    };
}

class ElementColors::Private
{
public:
    typedef boost::signals::connection Connection;
    std::unique_ptr<Ui_TaskElementColors> ui;
    ViewProviderPartExt* vp;
    Part::Feature* obj;
    std::set<std::string> elements;
    std::map<std::string,App::Color> oldColors;
    Connection connectDelDoc;
    Connection connectDelObj;
    QPixmap px;
    bool busy;
    long onTopMode;

    std::string editDoc;
    std::string editObj;
    std::string editSub;

    Private(ViewProviderPartExt* vp) : ui(new Ui_TaskElementColors()), vp(vp) {
        auto doc = Gui::Application::Instance->editDocument();
        if(doc) {
            Gui::ViewProviderDocumentObject *vpParent = 0;
            auto editVp = doc->getInEdit(&vpParent,&editSub);
            if(editVp == vp) {
                editDoc = vpParent->getObject()->getDocument()->getName();
                editObj = vpParent->getObject()->getNameInDocument();
            }
        }
        if(editDoc.empty()) {
            editDoc = vp->getObject()->getDocument()->getName();
            editObj = vp->getObject()->getNameInDocument();
            editSub.clear();
        }
        onTopMode = vp->OnTopWhenSelected.getValue();
        vp->OnTopWhenSelected.setValue(3);
        obj = dynamic_cast<Part::Feature*>(vp->getObject());
        busy = false;
        int w = QApplication::style()->standardPixmap(QStyle::SP_DirClosedIcon).width();
        px = QPixmap(w,w);
    }

    ~Private() {
        vp->OnTopWhenSelected.setValue(onTopMode);
    }

    void populate() {
        if(!obj) return;
        oldColors = vp->getElementColors();
        auto subs = obj->ColoredElements.getSubValues(true);
        elements.insert(subs.begin(),subs.end());
        int i=0;
        for(auto &sub : subs)
            addItem(i++,sub.c_str());
    }

    void addItem(int i,const char *sub) {
        if(!obj) return;

        auto res = elements.insert(sub);
        if(i<0 && !res.second)
            return;

        const auto &colors = vp->MappedColors.getValues();
        App::Color color;
        if(i>=0 && i<(int)colors.size())
            color = colors[i];
        else {
            const char *indexed_name = strrchr(sub,'.');
            if(indexed_name)
                ++indexed_name;
            else
                indexed_name = sub;
            color = vp->ShapeColor.getValue();
            try {
                switch(Part::TopoShape::shapeType(indexed_name)) {
                case TopAbs_EDGE:
                    color = vp->LineColor.getValue();
                    break;
                case TopAbs_VERTEX:
                    color = vp->PointColor.getValue();
                    break;
                default:
                    break;
                }
            }catch(...) {}
        }

        std::vector<std::string> subs(1,sub);
        const auto &shape = obj->Shape.getShape();
        for(size_t n=0;n<subs.size();++n) {
            auto &sub = subs[n];
            Part::TopoShape subshape;
            try {
                subshape = shape.getSubTopoShape(sub.c_str());
            }catch(...) {}
            if(subshape.isNull()) {
                if(n) continue;
                FC_WARN("element " << sub << " not found");
                elements.erase(res.first);
                res.first = elements.end();

                for(auto &v : shape.getRelatedElements(sub))
                    subs.push_back(v.first.size()?v.first:v.second);
            }
            if(n) {
                res = elements.insert(sub);
                if(!res.second)
                    continue;
            }
            auto element = shape.getElementName(sub.c_str());
            QColor c;
            c.setRgbF(color.r,color.g,color.b);
            px.fill(c);
            QListWidgetItem* item = new QListWidgetItem(QIcon(px),
                    QString::fromLatin1(element), ui->elementList);
            item->setData(Qt::UserRole,c);
            item->setData(Qt::UserRole+1,QString::fromLatin1(sub.c_str()));
            item->setSelected(true);
        }
    }

    void commit() {
        if(!obj) return;
        std::map<std::string,App::Color> info;
        int count = ui->elementList->count();
        for(int i=0;i<count;++i) {
            auto item = ui->elementList->item(i);
            auto color = item->data(Qt::UserRole).value<QColor>();
            info.emplace(qPrintable(item->text()),
                    App::Color(color.redF(),color.greenF(),color.blueF()));
        }
        vp->setElementColors(info);
    }

    void reset() {
        if(!obj) return;
        vp->setElementColors(oldColors);
    }

    void removeAll() {
        ui->elementList->clear();
        elements.clear();
    }

    void removeItems() {
        for(auto item : ui->elementList->selectedItems()) {
            std::string sub = qPrintable(item->data(Qt::UserRole+1).value<QString>());
            elements.erase(sub);
        }
    }

    void editItem(QWidget *parent, QListWidgetItem *item) {
        auto color = item->data(Qt::UserRole).value<QColor>();
        QColorDialog cd(color, parent);
        if (cd.exec()!=QDialog::Accepted || color==cd.selectedColor())
            return;
        color = cd.selectedColor();
        item->setData(Qt::UserRole,color);
        px.fill(color);
        item->setIcon(QIcon(px));
        commit();
    }
        
    void onSelectionChanged(const Gui::SelectionChanges &msg) {
        // no object selected in the combobox or no sub-element was selected
        if (busy || !obj)
            return;
        busy = true;
        switch(msg.Type) {
        case Gui::SelectionChanges::ClrSelection:
            ui->elementList->clearSelection();
            break;
        case Gui::SelectionChanges::AddSelection:
        case Gui::SelectionChanges::RmvSelection:
            if(msg.pDocName && msg.pObjectName && msg.pSubName && msg.pSubName[0]) {
                if(strcmp(obj->getNameInDocument(),msg.pObjectName)!=0 ||
                   strcmp(obj->getDocument()->getName(),msg.pDocName)!=0)
                    break;
                for(auto item : ui->elementList->findItems(QString::fromLatin1(msg.pSubName),0))
                    item->setSelected(msg.Type==Gui::SelectionChanges::AddSelection);
            }
        default:
            break;
        }
        busy = false;
    }

    void onSelectionChanged() {
        if(!obj || busy) return;
        busy = true;
        std::map<std::string,int> sels;
        for(auto &sel : Gui::Selection().getSelectionEx()) {
            if(sel.getObject() == obj) {
                for(auto &sub : sel.getSubNames())
                    sels[sub] = 1;
                break;
            }
        }
        for(auto item : ui->elementList->selectedItems()) {
            std::string name(qPrintable(item->text()));
            auto &v = sels[name];
            if(!v)
                Gui::Selection().addSelection(editDoc.c_str(),
                        editObj.c_str(), (editSub+name).c_str());
            v = 2;
        }
        for(auto &v : sels) {
            if(v.second!=2) {
                Gui::Selection().rmvSelection(editDoc.c_str(),
                        editObj.c_str(), (editSub+v.first).c_str());
            }
        }
        busy = false;
    }
};

/* TRANSLATOR PartGui::TaskElementColors */

ElementColors::ElementColors(ViewProviderPartExt* vp, QWidget* parent)
    :d(new Private(vp))
{
    Q_UNUSED(parent);
    d->ui->setupUi(this);
    d->ui->objectLabel->setText(QString::fromUtf8(vp->getObject()->Label.getValue()));
    d->ui->elementList->setMouseTracking(true); // needed for itemEntered() to work

    ElementSelection* gate = new ElementSelection(d->vp->getObject());
    Gui::Selection().addSelectionGate(gate);

    d->connectDelDoc = Gui::Application::Instance->signalDeleteDocument.connect(boost::bind
        (&ElementColors::slotDeleteDocument, this, _1));
    d->connectDelObj = Gui::Application::Instance->signalDeletedObject.connect(boost::bind
        (&ElementColors::slotDeleteObject, this, _1));

    d->populate();
    d->commit();
}

ElementColors::~ElementColors()
{
    Gui::Selection().rmvSelectionGate();
    d->connectDelDoc.disconnect();
    d->connectDelObj.disconnect();
}

void ElementColors::slotDeleteDocument(const Gui::Document& Doc)
{
    if(!d->obj) return;
    if (d->vp->getDocument()==&Doc || d->editDoc==Doc.getDocument()->getName())
        Gui::Control().closeDialog();
}

void ElementColors::slotDeleteObject(const Gui::ViewProvider& obj)
{
    if (d->vp==&obj)
        Gui::Control().closeDialog();
}

void ElementColors::on_removeSelection_clicked()
{
    d->removeItems();
}

void ElementColors::on_addSelection_clicked()
{
    auto sels = Gui::Selection().getSelectionEx("",App::DocumentObject::getClassTypeId(),2);
    for(auto &sel : sels) {
        if(sel.getObject() != d->obj)
            continue;
        const auto &subs = sel.getSubNames();
        if(subs.size()) {
            for(auto &sub : subs) 
                d->addItem(-1,sub.c_str());
            d->commit();
        }
        break;
    }
}

void ElementColors::on_removeAll_clicked()
{
    d->removeAll();
    d->commit();
}

bool ElementColors::accept()
{
    Gui::Application::Instance->setEditDocument(0);
    return true;
}

bool ElementColors::reject()
{
    d->reset();
    Gui::Application::Instance->setEditDocument(0);
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
    Gui::Selection().rmvPreselect();
}

void ElementColors::on_elementList_itemEntered(QListWidgetItem *item) {
    if(!d->obj) return;
    Gui::Selection().setPreselect(d->editDoc.c_str(),
            d->editObj.c_str(), (d->editSub+qPrintable(item->text())).c_str(),0,0,0,true);
}

void ElementColors::on_elementList_itemSelectionChanged() {
    d->onSelectionChanged();
}

void ElementColors::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    d->onSelectionChanged(msg);
}

void ElementColors::on_elementList_itemDoubleClicked(QListWidgetItem *item) {
    d->editItem(this,item);
}

/* TRANSLATOR PartGui::TaskElementColors */

TaskElementColors::TaskElementColors(ViewProviderPartExt* vp)
{
    widget = new ElementColors(vp);
    taskbox = new Gui::TaskView::TaskBox(
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
