/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QMessageBox>
# include <QRegExp>
# include <QTreeWidget>
# include <TopoDS_Shape.hxx>
# include <TopExp_Explorer.hxx>
# include <boost/bind.hpp>
# include <cfloat>
#endif

#include "Mirroring.h"
#include "ui_Mirroring.h"
#include "../App/PartFeature.h"
#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/Utilities.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

using namespace PartGui;

/* TRANSLATOR PartGui::Mirroring */

Mirroring::Mirroring(QWidget* parent)
  : QWidget(parent), ui(new Ui_Mirroring)
{
    ui->setupUi(this);
    ui->baseX->setRange(-DBL_MAX, DBL_MAX);
    ui->baseY->setRange(-DBL_MAX, DBL_MAX);
    ui->baseZ->setRange(-DBL_MAX, DBL_MAX);
    findShapes();

    Gui::ItemViewSelection sel(ui->shapes);
    sel.applyFrom(Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId()));
}

/*  
 *  Destroys the object and frees any allocated resources
 */
Mirroring::~Mirroring()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void Mirroring::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

void Mirroring::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc) return;
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    if (!activeGui) return;

    this->document = QString::fromAscii(activeDoc->getName());
    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType
        (Part::Feature::getClassTypeId());

    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it!=objs.end(); ++it) {
        const TopoDS_Shape& shape = static_cast<Part::Feature*>(*it)->Shape.getValue();
        if (!shape.IsNull()) {
            QString label = QString::fromUtf8((*it)->Label.getValue());
            QString name = QString::fromAscii((*it)->getNameInDocument());
            
            QTreeWidgetItem* child = new QTreeWidgetItem();
            child->setText(0, label);
            child->setToolTip(0, label);
            child->setData(0, Qt::UserRole, name);
            Gui::ViewProvider* vp = activeGui->getViewProvider(*it);
            if (vp) child->setIcon(0, vp->getIcon());
            ui->shapes->addTopLevelItem(child);
        }
    }
}

bool Mirroring::accept()
{
    if (ui->shapes->selectedItems().isEmpty()) {
        QMessageBox::critical(this, windowTitle(),
            tr("Select a shape for mirroring, first."));
        return false;
    }

    App::Document* activeDoc = App::GetApplication().getDocument((const char*)this->document.toAscii());
    if (!activeDoc) {
        QMessageBox::critical(this, windowTitle(),
            tr("No such document '%1'.").arg(this->document));
        return false;
    }

    Gui::WaitCursor wc;
    unsigned int count = activeDoc->countObjectsOfType(Base::Type::fromName("Part::Mirroring"));
    activeDoc->openTransaction("Mirroring");

    QString shape, label;
    QRegExp rx(QString::fromAscii(" \\(Mirror #\\d+\\)$"));
    QList<QTreeWidgetItem *> items = ui->shapes->selectedItems();
    float normx=0, normy=0, normz=0;
    int index = ui->comboBox->currentIndex();
    if (index == 0)
        normz = 1.0f;
    else if (index == 1)
        normy = 1.0f;
    else
        normx = 1.0f;
    double basex = ui->baseX->value();
    double basey = ui->baseY->value();
    double basez = ui->baseZ->value();
    for (QList<QTreeWidgetItem *>::iterator it = items.begin(); it != items.end(); ++it) {
        shape = (*it)->data(0, Qt::UserRole).toString();
        label = (*it)->text(0);

        // if we already have the suffix " (Mirror #<number>)" remove it
        int pos = label.indexOf(rx);
        if (pos > -1)
            label = label.left(pos);
        label.append(QString::fromAscii(" (Mirror #%1)").arg(++count));

        QString code = QString::fromAscii(
            "__doc__=FreeCAD.getDocument(\"%1\")\n"
            "__doc__.addObject(\"Part::Mirroring\")\n"
            "__doc__.ActiveObject.Source=__doc__.getObject(\"%2\")\n"
            "__doc__.ActiveObject.Label=\"%3\"\n"
            "__doc__.ActiveObject.Normal=(%4,%5,%6)\n"
            "__doc__.ActiveObject.Base=(%7,%8,%9)\n"
            "del __doc__")
            .arg(this->document).arg(shape).arg(label)
            .arg(normx).arg(normy).arg(normz)
            .arg(basex).arg(basey).arg(basez);
        Gui::Application::Instance->runPythonCode((const char*)code.toAscii());
        QByteArray from = shape.toAscii();
        Gui::Command::copyVisual("ActiveObject", "ShapeColor", from);
        Gui::Command::copyVisual("ActiveObject", "LineColor", from);
        Gui::Command::copyVisual("ActiveObject", "PointColor", from);
    }

    activeDoc->commitTransaction();
    activeDoc->recompute();
    return true;
}

// ---------------------------------------

TaskMirroring::TaskMirroring()
{
    widget = new Mirroring();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Mirror.svg"),
        widget->windowTitle(), false, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskMirroring::~TaskMirroring()
{
    // automatically deleted in the sub-class
}

bool TaskMirroring::accept()
{
    return widget->accept();
}

#include "moc_Mirroring.cpp"
