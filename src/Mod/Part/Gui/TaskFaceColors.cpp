/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <TopExp_Explorer.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <QMessageBox>
# include <QSet>
#endif

#include <boost/signals.hpp>
#include <boost/bind.hpp>

#include "ui_TaskFaceColors.h"
#include "TaskFaceColors.h"
#include "ViewProviderExt.h"

#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Mod/Part/App/PartFeature.h>


using namespace PartGui;

namespace PartGui {
    class FaceSelection : public Gui::SelectionFilterGate
    {
        const App::DocumentObject* object;
    public:
        FaceSelection(const App::DocumentObject* obj)
            : Gui::SelectionFilterGate((Gui::SelectionFilter*)0), object(obj)
        {
        }
        bool allow(App::Document*pDoc, App::DocumentObject*pObj, const char*sSubName)
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            std::string element(sSubName);
            return element.substr(0,4) == "Face";
        }
    };
}

class FaceColors::Private
{
public:
    typedef boost::signals::connection Connection;
    Ui_TaskFaceColors* ui;
    ViewProviderPartExt* vp;
    App::DocumentObject* obj;
    Gui::Document* doc;
    std::vector<App::Color> current,perface;
    QSet<int> index;
    Connection connectDelDoc;
    Connection connectDelObj;

    Private(ViewProviderPartExt* vp) : ui(new Ui_TaskFaceColors()), vp(vp)
    {
        obj = vp->getObject();
        doc = Gui::Application::Instance->getDocument(obj->getDocument());

        // build up map edge->face
        TopTools_IndexedMapOfShape mapOfShape;
        TopExp_Explorer xp(static_cast<Part::Feature*>(obj)->Shape.getValue(), TopAbs_FACE);
        while (xp.More()) {
            mapOfShape.Add(xp.Current());
            xp.Next();
        }

        current = vp->DiffuseColor.getValues();
        if (current.empty())
            current.push_back(vp->ShapeColor.getValue());
        perface = current;
        perface.resize(mapOfShape.Extent(), perface.front());
    }
    ~Private()
    {
        delete ui;
    }
};

/* TRANSLATOR PartGui::TaskFaceColors */

FaceColors::FaceColors(ViewProviderPartExt* vp, QWidget* parent)
  : d(new Private(vp))
{
    d->ui->setupUi(this);
    d->ui->groupBox->setTitle(QString::fromUtf8(vp->getObject()->Label.getValue()));
    d->ui->colorButton->setDisabled(true);

    FaceSelection* gate = new FaceSelection(d->vp->getObject());
    Gui::Selection().addSelectionGate(gate);

    d->connectDelDoc = Gui::Application::Instance->signalDeleteDocument.connect(boost::bind
        (&FaceColors::slotDeleteDocument, this, _1));
    d->connectDelObj = Gui::Application::Instance->signalDeletedObject.connect(boost::bind
        (&FaceColors::slotDeleteObject, this, _1));
}

FaceColors::~FaceColors()
{
    Gui::Selection().rmvSelectionGate();
    d->connectDelDoc.disconnect();
    d->connectDelObj.disconnect();
    delete d;
}

void FaceColors::slotDeleteDocument(const Gui::Document& Doc)
{
    if (d->doc == &Doc)
        Gui::Control().closeDialog();
}

void FaceColors::slotDeleteObject(const Gui::ViewProvider& obj)
{
    if (d->vp == &obj)
        Gui::Control().closeDialog();
}

void FaceColors::on_defaultButton_clicked()
{
    std::fill(d->perface.begin(), d->perface.end(), d->vp->ShapeColor.getValue());
    d->vp->DiffuseColor.setValues(d->perface);
}

void FaceColors::on_colorButton_changed()
{
    if (!d->index.isEmpty()) {
        QColor c = d->ui->colorButton->color();
        for (QSet<int>::iterator it = d->index.begin(); it != d->index.end(); ++it) {
            d->perface[*it].set(c.redF(), c.greenF(), c.blueF());
        }
        d->vp->DiffuseColor.setValues(d->perface);
    }
}

void FaceColors::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // no object selected in the combobox or no sub-element was selected
    if (!msg.pSubName)
        return;
    bool selection_changed = false;
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        // when adding a sub-element to the selection check
        // whether this is the currently handled object
        App::Document* doc = d->obj->getDocument();
        std::string docname = doc->getName();
        std::string objname = d->obj->getNameInDocument();
        if (docname==msg.pDocName && objname==msg.pObjectName) {
            int index = std::atoi(msg.pSubName+4)-1;
            d->index.insert(index);
            const App::Color& c = d->perface[index];
            QColor color;
            color.setRgbF(c.r,c.g,c.b);
            d->ui->colorButton->setColor(color);
            selection_changed = true;
        }
    }
    else if (msg.Type == Gui::SelectionChanges::RmvSelection) {
        App::Document* doc = d->obj->getDocument();
        std::string docname = doc->getName();
        std::string objname = d->obj->getNameInDocument();
        if (docname==msg.pDocName && objname==msg.pObjectName) {
            int index = std::atoi(msg.pSubName+4)-1;
            d->index.remove(index);
            selection_changed = true;
        }
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        d->index.clear();
        selection_changed = true;
    }

    if (selection_changed) {
        QString faces = QString::fromAscii("[");
        int size = d->index.size();
        for (QSet<int>::iterator it = d->index.begin(); it != d->index.end(); ++it) {
            faces += QString::number(*it + 1);
            if (--size > 0)
                faces += QString::fromAscii(",");
        }
        faces += QString::fromAscii("]");
        d->ui->labelElement->setText(faces);
        d->ui->colorButton->setDisabled(d->index.isEmpty());
    }
}

bool FaceColors::accept()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(d->vp->getObject()->getDocument());
    doc->resetEdit();
    return true;
}

bool FaceColors::reject()
{
    int ret = QMessageBox::question(this, tr("Face colors"), tr("Do you really want to cancel?"),
        QMessageBox::Yes, QMessageBox::No|QMessageBox::Default|QMessageBox::Escape);
    if (ret == QMessageBox::Yes) {
        Gui::Document* doc = Gui::Application::Instance->getDocument(d->vp->getObject()->getDocument());
        doc->resetEdit();
        d->vp->DiffuseColor.setValues(d->current);
        return true;
    }
    else {
        return false;
    }
}

void FaceColors::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui->retranslateUi(this);
    }
}


/* TRANSLATOR PartGui::TaskFaceColors */

TaskFaceColors::TaskFaceColors(ViewProviderPartExt* vp)
{
    widget = new FaceColors(vp);
    taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskFaceColors::~TaskFaceColors()
{
}

void TaskFaceColors::open()
{
}

void TaskFaceColors::clicked(int)
{
}

bool TaskFaceColors::accept()
{
    return widget->accept();
}

bool TaskFaceColors::reject()
{
    return widget->reject();
}

#include "moc_TaskFaceColors.cpp"
