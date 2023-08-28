/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <Precision.hxx>
# include <ShapeExtend_Explorer.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopTools_HSequenceOfShape.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Utilities.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/Part/App/FeatureRevolution.h>

#include "DlgRevolution.h"
#include "ui_DlgRevolution.h"


using namespace PartGui;

class DlgRevolution::EdgeSelection : public Gui::SelectionFilterGate
{
public:
    bool canSelect;

    EdgeSelection()
        : Gui::SelectionFilterGate(nullPointer())
    {
        canSelect = false;
    }
    bool allow(App::Document* /*pDoc*/, App::DocumentObject*pObj, const char*sSubName) override
    {
        this->canSelect = false;

        if (!sSubName || sSubName[0] == '\0')
            return false;
        std::string element(sSubName);
        if (element.substr(0,4) != "Edge")
            return false;
        Part::TopoShape part = Part::Feature::getTopoShape(pObj);
        if (part.isNull()) {
            return false;
        }
        try {
            TopoDS_Shape sub = part.getSubShape(sSubName);
            if (!sub.IsNull() && sub.ShapeType() == TopAbs_EDGE) {
                const TopoDS_Edge& edge = TopoDS::Edge(sub);
                BRepAdaptor_Curve adapt(edge);
                if (adapt.GetType() == GeomAbs_Line || adapt.GetType() == GeomAbs_Circle) {
                    this->canSelect = true;
                    return true;
                }
            }
        }
        catch (...) {
        }

        return false;
    }
};

DlgRevolution::DlgRevolution(QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl)
  , ui(new Ui_DlgRevolution)
  , filter(nullptr)
{
    ui->setupUi(this);
    setupConnections();

    ui->xPos->setRange(-DBL_MAX,DBL_MAX);
    ui->yPos->setRange(-DBL_MAX,DBL_MAX);
    ui->zPos->setRange(-DBL_MAX,DBL_MAX);
    ui->xPos->setUnit(Base::Unit::Length);
    ui->yPos->setUnit(Base::Unit::Length);
    ui->zPos->setUnit(Base::Unit::Length);

    ui->xDir->setRange(-DBL_MAX,DBL_MAX);
    ui->yDir->setRange(-DBL_MAX,DBL_MAX);
    ui->zDir->setRange(-DBL_MAX,DBL_MAX);
    ui->xDir->setUnit(Base::Unit());
    ui->yDir->setUnit(Base::Unit());
    ui->zDir->setUnit(Base::Unit());
    ui->zDir->setValue(1.0);

    ui->angle->setUnit(Base::Unit::Angle);
    ui->angle->setValue(360.0);
    findShapes();

    Gui::ItemViewSelection sel(ui->treeWidget);
    sel.applyFrom(Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId()));
    sel.applyFrom(Gui::Selection().getObjectsOfType(App::Link::getClassTypeId()));
    sel.applyFrom(Gui::Selection().getObjectsOfType(App::Part::getClassTypeId()));

    connect(ui->txtAxisLink, &QLineEdit::textChanged, this, &DlgRevolution::onAxisLinkTextChanged);

    autoSolid();
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgRevolution::~DlgRevolution()
{
    // no need to delete child widgets, Qt does it all for us
    Gui::Selection().rmvSelectionGate();
}

void DlgRevolution::setupConnections()
{
    connect(ui->selectLine, &QPushButton::clicked,
            this, &DlgRevolution::onSelectLineClicked);
    connect(ui->btnX, &QPushButton::clicked,
            this, &DlgRevolution::onButtonXClicked);
    connect(ui->btnY, &QPushButton::clicked,
            this, &DlgRevolution::onButtonYClicked);
    connect(ui->btnZ, &QPushButton::clicked,
            this, &DlgRevolution::onButtonZClicked);
    connect(ui->txtAxisLink, &QLineEdit::textChanged,
            this, &DlgRevolution::onAxisLinkTextChanged);
}

Base::Vector3d DlgRevolution::getDirection() const
{
    return Base::Vector3d(
                ui->xDir->value().getValue(),
                ui->yDir->value().getValue(),
                ui->zDir->value().getValue());
}

Base::Vector3d DlgRevolution::getPosition() const
{
    return Base::Vector3d(
                ui->xPos->value().getValueAs(Base::Quantity::MilliMetre),
                ui->yPos->value().getValueAs(Base::Quantity::MilliMetre),
                ui->zPos->value().getValueAs(Base::Quantity::MilliMetre));
}

void DlgRevolution::getAxisLink(App::PropertyLinkSub &lnk) const
{
    QString text = ui->txtAxisLink->text();

    if (text.length() == 0) {
        lnk.setValue(nullptr);
    } else {
        QStringList parts = text.split(QChar::fromLatin1(':'));
        App::DocumentObject* obj = App::GetApplication().getActiveDocument()->getObject(parts[0].toLatin1());
        if(!obj){
            throw Base::ValueError(tr("Object not found: %1").arg(parts[0]).toUtf8().constData());
        }
        lnk.setValue(obj);
        if (parts.size() == 1) {
            return;
        } else if (parts.size() == 2) {
            std::vector<std::string> subs;
            subs.emplace_back(parts[1].toLatin1().constData());
            lnk.setValue(obj,subs);
        }
    }

}

double DlgRevolution::getAngle() const
{
    return ui->angle->value().getValueAs(Base::Quantity::Degree);
}

void DlgRevolution::setDirection(Base::Vector3d dir)
{
    ui->xDir->setValue(dir.x);
    ui->yDir->setValue(dir.y);
    ui->zDir->setValue(dir.z);
}

void DlgRevolution::setPosition(Base::Vector3d pos)
{
    ui->xPos->setValue(pos.x);
    ui->yPos->setValue(pos.y);
    ui->zPos->setValue(pos.z);
}

void DlgRevolution::setAxisLink(const App::PropertyLinkSub& lnk)
{
    if (!lnk.getValue()){
        ui->txtAxisLink->clear();
        return;
    }
    if (lnk.getSubValues().size() == 1){
        this->setAxisLink(lnk.getValue()->getNameInDocument(), lnk.getSubValues()[0].c_str());
    } else {
        this->setAxisLink(lnk.getValue()->getNameInDocument(), "");
    }
}

void DlgRevolution::setAxisLink(const char* objname, const char* subname)
{
    if(objname && strlen(objname) > 0){
        QString txt = QString::fromLatin1(objname);
        if (subname && strlen(subname) > 0){
            txt = txt + QString::fromLatin1(":") + QString::fromLatin1(subname);
        }
        ui->txtAxisLink->setText(txt);
    } else {
        ui->txtAxisLink->clear();
    }
}

std::vector<App::DocumentObject*> DlgRevolution::getShapesToRevolve() const
{
    QList<QTreeWidgetItem *> items = ui->treeWidget->selectedItems();
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc)
        throw Base::RuntimeError("Document lost");

    std::vector<App::DocumentObject*> objects;
    for (auto item : items) {
        App::DocumentObject* obj = doc->getObject(item->data(0, Qt::UserRole).toString().toLatin1());
        if (!obj)
            throw Base::RuntimeError("Object not found");
        objects.push_back(obj);
    }
    return objects;
}

bool DlgRevolution::validate()
{
    //check source shapes
    if (ui->treeWidget->selectedItems().isEmpty()) {
        QMessageBox::critical(this, windowTitle(),
            tr("Select a shape for revolution, first."));
        return false;
    }

    //check axis link
    bool axisLinkIsValid = false;
    bool axisLinkHasAngle = false;
    try{
        App::PropertyLinkSub lnk;
        this->getAxisLink(lnk);
        double angle_edge = 1e100;
        Base::Vector3d axis, center;
        axisLinkIsValid = Part::Revolution::fetchAxisLink(lnk, center, axis, angle_edge);
        axisLinkHasAngle = angle_edge != 1e100;
    } catch(Base::Exception &err) {
        QMessageBox::critical(this, windowTitle(),
            tr("Revolution axis link is invalid.\n\n%1").arg(QCoreApplication::translate("Exception", err.what())));
        ui->txtAxisLink->setFocus();
        return false;
    } catch(Standard_Failure &err) {
        QMessageBox::critical(this, windowTitle(),
            tr("Revolution axis link is invalid.\n\n%1").arg(QString::fromLocal8Bit(err.GetMessageString())));
        ui->txtAxisLink->setFocus();
        return false;
    } catch(...) {
        QMessageBox::critical(this, windowTitle(),
            tr("Revolution axis link is invalid.\n\n%1").arg(tr("Unknown error")));
        ui->txtAxisLink->setFocus();
        return false;
    }

    //check axis dir
    if (!axisLinkIsValid){
        if(this->getDirection().Length() < Precision::Confusion()){
            QMessageBox::critical(this, windowTitle(),
                tr("Revolution axis direction is zero-length. It must be non-zero."));
            ui->xDir->setFocus();
            return false;
        }
    }

    //check angle
    if (!axisLinkHasAngle){
        if (fabs(this->getAngle() / 180.0 * M_PI) < Precision::Angular()) {
            QMessageBox::critical(this, windowTitle(),
                tr("Revolution angle span is zero. It must be non-zero."));
            ui->angle->setFocus();
            return false;
        }
    }

    return true;
}

void DlgRevolution::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QDialog::changeEvent(e);
    }
}

void DlgRevolution::keyPressEvent(QKeyEvent* ke)
{
    // The revolution dialog is embedded into a task panel
    // which is a parent widget and will handle the event
    ke->ignore();
}

void DlgRevolution::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc)
        return;
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);

    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType<App::DocumentObject>();

    for (auto obj : objs) {
        Part::TopoShape topoShape = Part::Feature::getTopoShape(obj);
        if (topoShape.isNull()) {
            continue;
        }
        TopoDS_Shape shape = topoShape.getShape();
        if (shape.IsNull()) continue;

        TopExp_Explorer xp;
        xp.Init(shape,TopAbs_SOLID);
        if (xp.More()) continue; // solids not allowed
        xp.Init(shape,TopAbs_COMPSOLID);
        if (xp.More()) continue; // compound solids not allowed
        // So allowed are: vertex, edge, wire, face, shell and compound
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, QString::fromUtf8(obj->Label.getValue()));
        item->setData(0, Qt::UserRole, QString::fromLatin1(obj->getNameInDocument()));
        Gui::ViewProvider* vp = activeGui->getViewProvider(obj);
        if (vp) item->setIcon(0, vp->getIcon());
    }
}

void DlgRevolution::accept()
{
    if (!this->validate())
        return;
    Gui::WaitCursor wc;
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    activeDoc->openTransaction("Revolve");

    try{
        QString shape, type, name, solid;
        QList<QTreeWidgetItem *> items = ui->treeWidget->selectedItems();
        if (ui->checkSolid->isChecked()) {
            solid = QString::fromLatin1("True");}
        else {
            solid = QString::fromLatin1("False");}

        App::PropertyLinkSub axisLink;
        this->getAxisLink(axisLink);
        QString strAxisLink;
        if (axisLink.getValue()){
            strAxisLink = QString::fromLatin1("(App.ActiveDocument.%1, %2)")
                    .arg(QString::fromLatin1(axisLink.getValue()->getNameInDocument()),
                         axisLink.getSubValues().size() ==  1 ?
                             QString::fromLatin1("\"%1\"").arg(QString::fromLatin1(axisLink.getSubValues()[0].c_str()))
                             : QString() );
        } else {
            strAxisLink = QString::fromLatin1("None");
        }

        QString symmetric;
        if (ui->checkSymmetric->isChecked()) {
            symmetric = QString::fromLatin1("True");}
        else {
            symmetric = QString::fromLatin1("False");}

        for (auto item : items) {
            shape = item->data(0, Qt::UserRole).toString();
            type = QString::fromLatin1("Part::Revolution");
            name = QString::fromLatin1(activeDoc->getUniqueObjectName("Revolve").c_str());
            Base::Vector3d axis = this->getDirection();
            Base::Vector3d pos = this->getPosition();


            QString code = QString::fromLatin1(
                "FreeCAD.ActiveDocument.addObject(\"%1\",\"%2\")\n"
                "FreeCAD.ActiveDocument.%2.Source = FreeCAD.ActiveDocument.%3\n"
                "FreeCAD.ActiveDocument.%2.Axis = (%4,%5,%6)\n"
                "FreeCAD.ActiveDocument.%2.Base = (%7,%8,%9)\n"
                "FreeCAD.ActiveDocument.%2.Angle = %10\n"
                "FreeCAD.ActiveDocument.%2.Solid = %11\n"
                "FreeCAD.ActiveDocument.%2.AxisLink = %12\n"
                "FreeCAD.ActiveDocument.%2.Symmetric = %13\n"
                "FreeCADGui.ActiveDocument.%3.Visibility = False\n")
                .arg(type, name, shape) //%1, 2, 3
                .arg(axis.x,0,'f',15) //%4
                .arg(axis.y,0,'f',15) //%5
                .arg(axis.z,0,'f',15) //%6
                .arg(pos.x, 0,'f',15) //%7
                .arg(pos.y, 0,'f',15) //%8
                .arg(pos.z, 0,'f',15) //%9
                .arg(getAngle(),0,'f',15) //%10
                .arg(solid, //%11
                     strAxisLink, //%12
                     symmetric) //%13
                ;
            Gui::Command::runCommand(Gui::Command::App, code.toLatin1());
            QByteArray to = name.toLatin1();
            QByteArray from = shape.toLatin1();
            Gui::Command::copyVisual(to, "ShapeColor", from);
            Gui::Command::copyVisual(to, "LineColor", from);
            Gui::Command::copyVisual(to, "PointColor", from);
        }

        activeDoc->commitTransaction();
        activeDoc->recompute();
    } catch (Base::Exception &err) {
        QMessageBox::critical(this, windowTitle(),
            tr("Creating Revolve failed.\n\n%1").arg(QCoreApplication::translate("Exception", err.what())));
        return;
    } catch (...){
        QMessageBox::critical(this, windowTitle(),
            tr("Creating Revolve failed.\n\n%1").arg(QString::fromUtf8("Unknown error")));
        return;
    }

    QDialog::accept();
}

void DlgRevolution::onSelectLineClicked()
{
    if (!filter) {
        filter = new EdgeSelection();
        Gui::Selection().addSelectionGate(filter);
        ui->selectLine->setText(tr("Selecting... (line or arc)"));
    } else {
        Gui::Selection().rmvSelectionGate();
        filter = nullptr;
        ui->selectLine->setText(tr("Select reference"));
    }
}

void DlgRevolution::onButtonXClicked()
{
    setDirection(Base::Vector3d(1,0,0));
    if (!ui->xDir->isEnabled())
        ui->txtAxisLink->clear();
}

void DlgRevolution::onButtonYClicked()
{
    setDirection(Base::Vector3d(0,1,0));
    if (!ui->xDir->isEnabled())
        ui->txtAxisLink->clear();
}

void DlgRevolution::onButtonZClicked()
{
    setDirection(Base::Vector3d(0,0,1));
    if (!ui->xDir->isEnabled())
        ui->txtAxisLink->clear();
}

void DlgRevolution::onAxisLinkTextChanged(QString)
{
    bool en = true;
    try{
        Base::Vector3d pos, dir;
        double angle_edge = 1e100;
        App::PropertyLinkSub lnk; this->getAxisLink(lnk);
        bool fetched = Part::Revolution::fetchAxisLink(lnk, pos, dir, angle_edge);
        if (fetched){
            this->setDirection(dir);
            this->setPosition(pos);
            if (angle_edge != 1e100){
                ui->angle->setValue(0.0);
            } else if (fabs(ui->angle->value().getValue()) < 1e-12) {
                ui->angle->setValue(360.0);
            }
            en = false;
        }
    } catch (Base::Exception &){

    } catch (...){

    }
    ui->xDir->setEnabled(en);
    ui->yDir->setEnabled(en);
    ui->zDir->setEnabled(en);
    ui->xPos->setEnabled(en);
    ui->yPos->setEnabled(en);
    ui->zPos->setEnabled(en);
}

void DlgRevolution::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (filter && filter->canSelect) {
            this->setAxisLink(msg.pObjectName, msg.pSubName);
        }
    }
}

App::DocumentObject&DlgRevolution::getShapeToRevolve() const
{
    std::vector<App::DocumentObject*> objs = this->getShapesToRevolve();
    if (objs.empty())
        throw Base::ValueError("No shapes selected");
    return *(objs[0]);
}

void DlgRevolution::autoSolid()
{
    try{
        App::DocumentObject &dobj = this->getShapeToRevolve();
        Part::TopoShape topoShape = Part::Feature::getTopoShape(&dobj);
        if (topoShape.isNull()) {
            return;
        } else {
            TopoDS_Shape sh = topoShape.getShape();
            if (sh.IsNull())
                return;
            ShapeExtend_Explorer xp;
            Handle(TopTools_HSequenceOfShape) leaves = xp.SeqFromCompound(sh, /*recursive= */Standard_True);
            int cntClosedWires = 0;
            for (int i = 0; i < leaves->Length(); i++) {
                const TopoDS_Shape &leaf = leaves->Value(i+1);
                if (leaf.IsNull())
                    return;
                if (leaf.ShapeType() == TopAbs_WIRE || leaf.ShapeType() == TopAbs_EDGE){
                    if (BRep_Tool::IsClosed(leaf)){
                        cntClosedWires++;
                    }
                }
            }
            ui->checkSolid->setChecked( cntClosedWires == leaves->Length() );
        }
    } catch(...){

    }

}

// ---------------------------------------

TaskRevolution::TaskRevolution()
{
    widget = new DlgRevolution();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Revolve"),
        widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

bool TaskRevolution::accept()
{
    widget->accept();
    return (widget->result() == QDialog::Accepted);
}

#include "moc_DlgRevolution.cpp"
