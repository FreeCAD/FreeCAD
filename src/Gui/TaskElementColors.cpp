/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
# include <boost/algorithm/string/predicate.hpp>
# include <QColorDialog>
# include <sstream>
#endif

#include <App/ComplexGeoData.h>
#include <App/Document.h>

#include "TaskElementColors.h"
#include "ui_TaskElementColors.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Control.h"
#include "Document.h"
#include "FileDialog.h"
#include "Selection.h"
#include "ViewParams.h"
#include "ViewProviderLink.h"

FC_LOG_LEVEL_INIT("Gui", true, true)

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
        if(touched) {
            touched = false;
            if(ui->recompute->isChecked()) {
                auto obj = vp->getObject();
                auto objs = obj->getInListRecursive();
                objs.push_back(obj);
                for (auto o : objs)
                    o->enforceRecompute();
                obj->getDocument()->recompute(objs);
            } else {
                for (auto obj : App::Document::getDependencyList(
                                {vp->getObject()}, App::Document::DepSort))
                {
                    auto vp = Application::Instance->getViewProvider(obj);
                    if (vp)
                        vp->updateColors();
                }
            }
        }
        App::GetApplication().closeActiveTransaction();
    }

    void removeAll() {
        if(elements.empty())
            return;

        auto itFace = elements.find("Face");
        auto itEdge = elements.find("Edge");
        auto itVertex = elements.find("Vertex");
        bool revert = false;
        for (auto & v : elements) {
            if (itFace != elements.end() && v.first != "Face" && boost::starts_with(v.first, "Face")) {
                revert = true;
                v.second->setData(Qt::UserRole, itFace->second->data(Qt::UserRole));
            }
            if (itEdge != elements.end() && v.first != "Edge" && boost::starts_with(v.first, "Edge")) {
                revert = true;
                v.second->setData(Qt::UserRole, itEdge->second->data(Qt::UserRole));
            }
            if (itVertex != elements.end() && v.first != "Vertex" && boost::starts_with(v.first, "Vertex")) {
                revert = true;
                v.second->setData(Qt::UserRole, itVertex->second->data(Qt::UserRole));
            }
        }
        if (revert)
            apply();
        ui->elementList->clear();
        elements.clear();
        apply();
    }

    void removeItems() {
        std::vector<QListWidgetItem*> faces;
        std::vector<QListWidgetItem*> edges;
        std::vector<QListWidgetItem*> vertexes;
        std::vector<std::string> subs;
        for(auto item : ui->elementList->selectedItems()) {
            subs.push_back(qPrintable(item->data(Qt::UserRole+1).value<QString>()));
            if (subs.back() != "Face" && boost::starts_with(subs.back(), "Face"))
                faces.push_back(item);
            else if (subs.back() != "Edge" && boost::starts_with(subs.back(), "Edge"))
                edges.push_back(item);
            else if (subs.back() != "Vertex" && boost::starts_with(subs.back(), "Vertex"))
                vertexes.push_back(item);
        }

        // In order to better support backward compatibility, Part view provider
        // still allow user to set color through DiffuseColor. So when doing
        // color mapping, it will not touch any element color that are not
        // explicitly set through ColoredElements property. Therefore, when the
        // user removes colored elements here, we must explicitly revert only
        // those element too. Same for the removeAll() above.
        if (faces.size()) {
            auto it = elements.find("Face");
            if (it != elements.end()) {
                for (auto item : faces)
                    item->setData(Qt::UserRole, it->second->data(Qt::UserRole));
            }
        }
        if (edges.size()) {
            auto it = elements.find("Edge");
            if (it != elements.end()) {
                for (auto item : edges)
                    item->setData(Qt::UserRole, it->second->data(Qt::UserRole));
            }
        }
        if (vertexes.size()) {
            auto it = elements.find("Vertex");
            if (it != elements.end()) {
                for (auto item : vertexes)
                    item->setData(Qt::UserRole, it->second->data(Qt::UserRole));
            }
        }
        apply();

        // now remove the items
        for (auto & sub : subs) {
            auto it = elements.find(sub);
            if (it != elements.end()) {
                delete it->second;
                elements.erase(it);
            }
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
        if (DialogOptions::dontUseNativeColorDialog())
            cd.setOptions(QColorDialog::DontUseNativeDialog);
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
        if(busy)
            return;
        busy = true;
        std::map<std::string,int> sels;
        for(auto &sel : Selection().getSelectionEx(
                    editDoc.c_str(),App::DocumentObject::getClassTypeId(), ResolveMode::NoResolve))
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

    d->ui->recompute->setChecked(ViewParams::instance()->getColorRecompute());
    d->ui->onTop->setChecked(ViewParams::instance()->getColorOnTop());

    Selection().addSelectionGate(d, ResolveMode::NoResolve);

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
    ViewParams::instance()->setColorRecompute(checked);
}

void ElementColors::on_onTop_clicked(bool checked) {
    ViewParams::instance()->setColorOnTop(checked);
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
    auto sels = Selection().getSelectionEx(d->editDoc.c_str(), App::DocumentObject::getClassTypeId(), ResolveMode::NoResolve);
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
    auto sels = Selection().getSelectionEx(d->editDoc.c_str(), App::DocumentObject::getClassTypeId(), ResolveMode::NoResolve);
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
                if(!boost::starts_with(sub,d->editSub))
                    continue;
                std::string s(sub.c_str()+d->editSub.size());
                if(s.empty() || s.back() == '.')
                    s += "Face";
                d->addItem(-1,s.c_str(),true);
            }
            break;
        }
    }
    if(d->items.size()) {
        auto color = d->items.front()->data(Qt::UserRole).value<QColor>();
        QColorDialog cd(color, this);
        cd.setOption(QColorDialog::ShowAlphaChannel);
        if (DialogOptions::dontUseNativeColorDialog())
            cd.setOptions(QColorDialog::DontUseNativeDialog);
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
    Application::Instance->setEditDocument(nullptr);
    return true;
}

bool ElementColors::reject()
{
    d->reset();
    Application::Instance->setEditDocument(nullptr);
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
}

void ElementColors::on_elementList_itemEntered(QListWidgetItem *item) {
    std::string name(qPrintable(item->data(Qt::UserRole+1).value<QString>()));
    const char *hidden = ViewProvider::hasHiddenMarker(name.c_str());
    if(hidden)
        name.resize(name.size()-std::strlen(hidden));
    Selection().setPreselect(d->editDoc.c_str(),
            d->editObj.c_str(), (d->editSub+name).c_str(),0,0,0,
            d->ui->onTop->isChecked() ? Gui::SelectionChanges::MsgSource::TreeView
                                      : Gui::SelectionChanges::MsgSource::Internal);
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
        QPixmap(), widget->windowTitle(), true, nullptr);
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
