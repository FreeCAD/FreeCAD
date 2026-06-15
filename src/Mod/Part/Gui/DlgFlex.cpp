// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 F. Foinant-Willig <flachyjoe@gmail.com>            *
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

#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <Precision.hxx>
#include <ShapeExtend_Explorer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <QKeyEvent>
#include <QMessageBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>


#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>
#include <App/Part.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Utilities.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include "ui_DlgFlex.h"
#include "DlgFlex.h"


FC_LOG_LEVEL_INIT("Part", true, true)

using namespace PartGui;

DlgFlex::DlgFlex(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , ui(new Ui_DlgFlex)
{
    ui->setupUi(this);
    setupConnections();

    ui->dsbPitch->setDecimals(Base::UnitsApi::getDecimals());
    findShapes();

    // this will mark as selected all the items in treeWidget that are selected in the document
    Gui::ItemViewSelection sel(ui->treeWidget);
    sel.applyFrom(Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId()));
    sel.applyFrom(Gui::Selection().getObjectsOfType(App::Link::getClassTypeId()));
    sel.applyFrom(Gui::Selection().getObjectsOfType(App::Part::getClassTypeId()));
}

void DlgFlex::setupConnections()
{
    // connect(ui->rbUniform, &QRadioButton::toggled, this, &DlgFlex::onUniformFlexToggled);
}

void DlgFlex::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QDialog::changeEvent(e);
}

//! find all the deformable objects in the active document and load them into the
//! list widget
void DlgFlex::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc) {
        return;
    }
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    m_document = activeDoc->getName();
    m_label = activeDoc->Label.getValue();

    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType<App::DocumentObject>();

    for (auto obj : objs) {
        Part::TopoShape topoShape = Part::Feature::getTopoShape(
            obj,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );
        if (topoShape.isNull()) {
            continue;
        }
        TopoDS_Shape shape = topoShape.getShape();
        if (shape.IsNull()) {
            continue;
        }
        if (canFlex(shape)) {
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
            item->setText(0, QString::fromUtf8(obj->Label.getValue()));
            item->setData(0, Qt::UserRole, QString::fromLatin1(obj->getNameInDocument()));
            Gui::ViewProvider* vp = activeGui->getViewProvider(obj);
            if (vp) {
                item->setIcon(0, vp->getIcon());
            }
        }
    }
}

//! return true if shape can be Flexd.
bool DlgFlex::canFlex(const TopoDS_Shape& shape) const
{
    if (shape.IsNull()) {
        return false;
    }
    // if the shape is a solid or a compound containing shapes, then we can Flex it
    TopAbs_ShapeEnum type = shape.ShapeType();

    if (type == TopAbs_VERTEX || type == TopAbs_COMPSOLID) {
        return false;
    }

    if (type == TopAbs_COMPOUND) {
        TopExp_Explorer xp;
        xp.Init(shape, TopAbs_EDGE);
        for (; xp.More(); xp.Next()) {
            // there is at least 1 edge inside the compound, so as long as it isn't null,
            // we can Flex this shape.  We can stop looking as soon as we find a non-null
            // edge.
            if (!xp.Current().IsNull()) {
                // found a non-null edge
                return true;
            }
        }
        // did not find a non-null shape
        return false;
    }
    // not a Vertex, Compound or CompSolid, must be one of Edge, Wire, Face, Shell or
    // Solid, all of which we can Flex.
    return true;
}

void DlgFlex::accept()
{
    //    Base::Console().message("DS::accept()\n");
    try {
        apply();
        QDialog::accept();
    }
    catch (Base::AbortException&) {
        Base::Console().message("DS::accept - apply failed!\n");
    };
}

// create a FeatureFlex for each scalable object
void DlgFlex::apply()
{
    //    Base::Console().message("DS::apply()\n");
    try {
        if (!validate()) {
            QMessageBox::critical(this, windowTitle(), tr("No scalable shapes selected"));
            return;
        }

        Gui::WaitCursor wc;
        App::Document* activeDoc = App::GetApplication().getDocument(m_document.c_str());
        if (!activeDoc) {
            QMessageBox::critical(
                this,
                windowTitle(),
                tr("The document '%1' doesn't exist.").arg(QString::fromUtf8(m_label.c_str()))
            );
            return;
        }
        activeDoc->openTransaction("Flex");

        Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                                 .GetUserParameter()
                                                 .GetGroup("BaseApp")
                                                 ->GetGroup("Preferences")
                                                 ->GetGroup("Mod/Part");
        bool addBaseName = hGrp->GetBool("AddBaseObjectName", false);

        std::vector<App::DocumentObject*> objects = this->getShapesToFlex();
        for (App::DocumentObject* sourceObj : objects) {
            assert(sourceObj);

            if (Part::Feature::getTopoShape(
                    sourceObj,
                    Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
                )
                    .isNull()) {
                FC_ERR(
                    "Object " << sourceObj->getFullName()
                              << " is not a shape object. Scaling is not possible."
                );
                continue;
            }

            std::string name;
            name = sourceObj->getDocument()->getUniqueObjectName("Flex").c_str();
            if (addBaseName) {
                // FIXME: implement
                // QString baseName = QStringLiteral("Flex_%1").arg(sourceObjectName);
                // label = QStringLiteral("%1_Flex").arg((*it)->text(0));
            }

            FCMD_OBJ_DOC_CMD(sourceObj, "addObject('Part::Flex','" << name << "')");
            auto newObj = sourceObj->getDocument()->getObject(name.c_str());

            this->writeParametersToFeature(*newObj, sourceObj);

            Gui::Command::copyVisual(newObj, "ShapeAppearance", sourceObj);
            Gui::Command::copyVisual(newObj, "LineColor", sourceObj);
            Gui::Command::copyVisual(newObj, "PointColor", sourceObj);

            FCMD_OBJ_HIDE(sourceObj);
        }

        activeDoc->commitTransaction();
        Gui::Command::updateActive();
    }
    catch (Base::AbortException&) {
        throw;
    }
    catch (Base::Exception& err) {
        QMessageBox::critical(
            this,
            windowTitle(),
            tr("Creating Flex failed.\n%1").arg(QCoreApplication::translate("Exception", err.what()))
        );
        return;
    }
    catch (...) {
        QMessageBox::critical(
            this,
            windowTitle(),
            tr("Creating Flex failed.\n%1").arg(QStringLiteral("Unknown error"))
        );
        return;
    }
}

void DlgFlex::reject()
{
    QDialog::reject();
}

//! retrieve the document objects associated with the selected items in the list
//! widget
std::vector<App::DocumentObject*> DlgFlex::getShapesToFlex() const
{
    //    Base::Console().message("DS::getShapesToFlex()\n");
    QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
    App::Document* doc = App::GetApplication().getDocument(m_document.c_str());
    if (!doc) {
        throw Base::RuntimeError("Document lost");
    }

    std::vector<App::DocumentObject*> objects;
    for (auto item : items) {
        App::DocumentObject* obj = doc->getObject(item->data(0, Qt::UserRole).toString().toLatin1());
        if (!obj) {
            throw Base::RuntimeError("Object not found");
        }
        objects.push_back(obj);
    }
    return objects;
}

//! return true if at least one item in the list widget corresponds to an
//! available document object in the document
bool DlgFlex::validate()
{
    QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
    App::Document* doc = App::GetApplication().getDocument(m_document.c_str());
    if (!doc) {
        throw Base::RuntimeError("Document lost");
    }

    std::vector<App::DocumentObject*> objects;
    for (auto item : items) {
        App::DocumentObject* obj = doc->getObject(item->data(0, Qt::UserRole).toString().toLatin1());
        if (!obj) {
            throw Base::RuntimeError("Object not found");
        }
        objects.push_back(obj);
    }
    return !objects.empty();
}

//! update a FeatureFlex with the parameters from the UI
void DlgFlex::writeParametersToFeature(App::DocumentObject& feature, App::DocumentObject* base) const
{
    //    Base::Console().message("DS::writeParametersToFeature()\n");
    Gui::Command::doCommand(
        Gui::Command::Doc,
        "f = App.getDocument('%s').getObject('%s')",
        feature.getDocument()->getName(),
        feature.getNameInDocument()
    );

    if (!base) {
        return;
    }

    Gui::Command::doCommand(
        Gui::Command::Doc,
        "f.Base = App.getDocument('%s').getObject('%s')",
        base->getDocument()->getName(),
        base->getNameInDocument()
    );
    Gui::Command::doCommand(Gui::Command::Doc, "f.Samples = %i", ui->dsbSamples->value());
    Gui::Command::doCommand(Gui::Command::Doc, "f.Pitch = %.7f", ui->dsbPitch->value());
}

// ---------------------------------------

TaskFlex::TaskFlex()
{
    widget = new DlgFlex();
    addTaskBox(Gui::BitmapFactory().pixmap("Part_Flex"), widget);
}

bool TaskFlex::accept()
{
    widget->accept();
    return (widget->result() == QDialog::Accepted);
}

bool TaskFlex::reject()
{
    widget->reject();
    return true;
}

void TaskFlex::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        try {
            widget->apply();
        }
        catch (Base::AbortException&) {
        };
    }
}

#include "moc_DlgFlex.cpp"
