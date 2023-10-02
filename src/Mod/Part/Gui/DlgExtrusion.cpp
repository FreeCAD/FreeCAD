/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepAdaptor_Curve.hxx>
# include <BRep_Tool.hxx>
# include <Precision.hxx>
# include <ShapeExtend_Explorer.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <QKeyEvent>
# include <QMessageBox>
#endif

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

#include "ui_DlgExtrusion.h"
#include "DlgExtrusion.h"


FC_LOG_LEVEL_INIT("Part",true,true)

using namespace PartGui;

class DlgExtrusion::EdgeSelection : public Gui::SelectionFilterGate
{
public:
    bool canSelect;

    EdgeSelection()
        : Gui::SelectionFilterGate(nullPointer())
    {
        canSelect = false;
    }
    bool allow(App::Document* /*pDoc*/, App::DocumentObject* pObj, const char* sSubName) override
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
            TopoDS_Shape sub = Part::Feature::getTopoShape(pObj, sSubName, true /*need element*/).getShape();
            if (!sub.IsNull() && sub.ShapeType() == TopAbs_EDGE) {
                const TopoDS_Edge& edge = TopoDS::Edge(sub);
                BRepAdaptor_Curve adapt(edge);
                if (adapt.GetType() == GeomAbs_Line) {
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

DlgExtrusion::DlgExtrusion(QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl), ui(new Ui_DlgExtrusion), filter(nullptr)
{
    ui->setupUi(this);
    setupConnections();

    ui->statusLabel->clear();
    ui->dirX->setDecimals(Base::UnitsApi::getDecimals());
    ui->dirY->setDecimals(Base::UnitsApi::getDecimals());
    ui->dirZ->setDecimals(Base::UnitsApi::getDecimals());
    ui->spinLenFwd->setUnit(Base::Unit::Length);
    ui->spinLenFwd->setValue(10.0);
    ui->spinLenRev->setUnit(Base::Unit::Length);
    ui->spinTaperAngle->setUnit(Base::Unit::Angle);
    ui->spinTaperAngle->setUnit(Base::Unit::Angle);
    findShapes();

    Gui::ItemViewSelection sel(ui->treeWidget);
    sel.applyFrom(Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId()));
    sel.applyFrom(Gui::Selection().getObjectsOfType(App::Link::getClassTypeId()));
    sel.applyFrom(Gui::Selection().getObjectsOfType(App::Part::getClassTypeId()));

    this->onDirModeChanged();
    ui->spinLenFwd->selectAll();
    // Make sure that the spin box has the focus to get key events
    // Calling setFocus() directly doesn't work because the spin box is not
    // yet visible.
    QMetaObject::invokeMethod(ui->spinLenFwd, "setFocus", Qt::QueuedConnection);

    this->autoSolid();
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgExtrusion::~DlgExtrusion()
{
    if (filter){
        Gui::Selection().rmvSelectionGate();
        filter = nullptr;
    }

    // no need to delete child widgets, Qt does it all for us
}

void DlgExtrusion::setupConnections()
{
    connect(ui->rbDirModeCustom, &QRadioButton::toggled,
            this, &DlgExtrusion::onDirModeCustomToggled);
    connect(ui->rbDirModeEdge, &QRadioButton::toggled,
            this, &DlgExtrusion::onDirModeEdgeToggled);
    connect(ui->rbDirModeNormal, &QRadioButton::toggled,
            this, &DlgExtrusion::onDirModeNormalToggled);
    connect(ui->btnSelectEdge, &QPushButton::clicked,
            this, &DlgExtrusion::onSelectEdgeClicked);
    connect(ui->btnX, &QPushButton::clicked,
            this, &DlgExtrusion::onButtnoXClicked);
    connect(ui->btnY, &QPushButton::clicked,
            this, &DlgExtrusion::onButtonYClicked);
    connect(ui->btnZ, &QPushButton::clicked,
            this, &DlgExtrusion::onButtonZClicked);
    connect(ui->chkSymmetric, &QCheckBox::toggled,
            this, &DlgExtrusion::onCheckSymmetricToggled);
    connect(ui->txtLink, &QLineEdit::textChanged,
            this, &DlgExtrusion::onTextLinkTextChanged);
}

void DlgExtrusion::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QDialog::changeEvent(e);
}

void DlgExtrusion::keyPressEvent(QKeyEvent* ke)
{
    // The extrusion dialog is embedded into a task panel
    // which is a parent widget and will handle the event
    ke->ignore();
}

void DlgExtrusion::onDirModeCustomToggled(bool on)
{
    if(on) //this check prevents dual fire of dirmode changed - on radio buttons, one will come on, and other will come off, causing two events.
        this->onDirModeChanged();
}

void DlgExtrusion::onDirModeEdgeToggled(bool on)
{
    if(on)
        this->onDirModeChanged();
}

void DlgExtrusion::onDirModeNormalToggled(bool on)
{
    if(on)
        this->onDirModeChanged();
}

void DlgExtrusion::onSelectEdgeClicked()
{
    if (!filter) {
        filter = new EdgeSelection();
        Gui::Selection().addSelectionGate(filter);
        ui->btnSelectEdge->setText(tr("Selecting..."));

        //visibility automation
        try{
            QString code = QString::fromLatin1(
                        "import Show\n"
                        "tv = Show.TempoVis(App.ActiveDocument, tag= 'PartGui::DlgExtrusion')\n"
                        "tv.hide([%1])"
                        );
            std::vector<App::DocumentObject*>sources = getShapesToExtrude();
            QString features_to_hide;
            for (App::DocumentObject* obj: sources){
                if (!obj)
                    continue;
                features_to_hide.append(QString::fromLatin1("App.ActiveDocument."));
                features_to_hide.append(QString::fromLatin1(obj->getNameInDocument()));
                features_to_hide.append(QString::fromLatin1(", \n"));
            }
            QByteArray code_2 = code.arg(features_to_hide).toLatin1();
            Base::Interpreter().runString(code_2.constData());
        } catch (Base::PyException &e){
            e.ReportException();
        }
    } else {
        Gui::Selection().rmvSelectionGate();
        filter = nullptr;
        ui->btnSelectEdge->setText(tr("Select"));

        //visibility automation
        try{
            Base::Interpreter().runString("del(tv)");
        } catch (Base::PyException &e){
            e.ReportException();
        }
    }
}

void DlgExtrusion::onButtnoXClicked()
{
    Base::Vector3d axis(1.0, 0.0, 0.0);
    if ((getDir() - axis).Length() < 1e-7)
        axis = axis * (-1);
    setDirMode(Part::Extrusion::dmCustom);
    setDir(axis);
}

void DlgExtrusion::onButtonYClicked()
{
    Base::Vector3d axis(0.0, 1.0, 0.0);
    if ((getDir() - axis).Length() < 1e-7)
        axis = axis * (-1);
    setDirMode(Part::Extrusion::dmCustom);
    setDir(axis);
}

void DlgExtrusion::onButtonZClicked()
{
    Base::Vector3d axis(0.0, 0.0, 1.0);
    if ((getDir() - axis).Length() < 1e-7)
        axis = axis * (-1);
    setDirMode(Part::Extrusion::dmCustom);
    setDir(axis);
}

void DlgExtrusion::onCheckSymmetricToggled(bool on)
{
    ui->spinLenRev->setEnabled(!on);
}

void DlgExtrusion::onTextLinkTextChanged(QString)
{
    this->fetchDir();
}

void DlgExtrusion::onDirModeChanged()
{
    Part::Extrusion::eDirMode dirMode = this->getDirMode();
    ui->dirX->setEnabled(dirMode == Part::Extrusion::dmCustom);
    ui->dirY->setEnabled(dirMode == Part::Extrusion::dmCustom);
    ui->dirZ->setEnabled(dirMode == Part::Extrusion::dmCustom);
    ui->txtLink->setEnabled(dirMode == Part::Extrusion::dmEdge);
    this->fetchDir();
}

void DlgExtrusion::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (filter && filter->canSelect) {
            this->setAxisLink(msg.pObjectName, msg.pSubName);
            this->setDirMode(Part::Extrusion::dmEdge);
        }
    }
}

App::DocumentObject& DlgExtrusion::getShapeToExtrude() const
{
    std::vector<App::DocumentObject*> objs = this->getShapesToExtrude();
    if (objs.empty())
        throw Base::ValueError("No shapes selected");
    return *(objs[0]);
}

void DlgExtrusion::fetchDir()
{
    bool lengths_are_at_defaults =
            (fabs(ui->spinLenFwd->value().getValue() - 10.0) < 1e-7)
            && (fabs(ui->spinLenRev->value().getValue() - 0.0) < 1e-7);
    bool lengths_are_zero =
            (fabs(ui->spinLenFwd->value().getValue() - 0.0) < 1e-7)
            && (fabs(ui->spinLenRev->value().getValue() - 0.0) < 1e-7);

    try{
        Base::Vector3d pos, dir;
        bool fetched = false;
        bool dir_has_valid_magnitude = false;
        if(this->getDirMode() == Part::Extrusion::dmEdge){
            App::PropertyLinkSub lnk; this->getAxisLink(lnk);
            fetched = Part::Extrusion::fetchAxisLink(lnk, pos, dir);
            dir_has_valid_magnitude = fetched;
        } else if (this->getDirMode() == Part::Extrusion::dmNormal){
            App::PropertyLink lnk;
            lnk.setValue(&this->getShapeToExtrude());
            dir = Part::Extrusion::calculateShapeNormal(lnk);
            fetched = true;
        }
        if (dir_has_valid_magnitude && lengths_are_at_defaults){
            ui->spinLenFwd->setValue(0);
        } else if (!dir_has_valid_magnitude && lengths_are_zero){
            ui->spinLenFwd->setValue(1.0);
        }
        if (fetched){
            this->setDir(dir);
        }
    } catch (Base::Exception &){

    } catch (...){

    }
}

void DlgExtrusion::autoSolid()
{
    try{
        App::DocumentObject* dobj = &this->getShapeToExtrude();
        Part::TopoShape shape = Part::Feature::getTopoShape(dobj);
        if (shape.isNull()) {
            return;
        }
        TopoDS_Shape sh = shape.getShape();
        if (sh.IsNull())
            return;
        ShapeExtend_Explorer xp;
        Handle(TopTools_HSequenceOfShape) leaves = xp.SeqFromCompound(sh, /*recursive= */Standard_True);
        int cntClosedWires = 0;
        for(int i = 0; i < leaves->Length(); i++){
            const TopoDS_Shape &leaf = leaves->Value(i+1);
            if (leaf.IsNull())
                return;
            if (leaf.ShapeType() == TopAbs_WIRE || leaf.ShapeType() == TopAbs_EDGE){
                if (BRep_Tool::IsClosed(leaf)){
                    cntClosedWires++;
                }
            }
        }
        ui->chkSolid->setChecked( cntClosedWires == leaves->Length() );
    } catch(...) {

    }
}

void DlgExtrusion::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc)
        return;
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    this->document = activeDoc->getName();
    this->label = activeDoc->Label.getValue();

    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType<App::DocumentObject>();

    for (auto obj : objs) {
        Part::TopoShape topoShape = Part::Feature::getTopoShape(obj);
        if (topoShape.isNull()) {
            continue;
        }
        TopoDS_Shape shape = topoShape.getShape();
        if (shape.IsNull()) continue;
        if (canExtrude(shape)) {
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
            item->setText(0, QString::fromUtf8(obj->Label.getValue()));
            item->setData(0, Qt::UserRole, QString::fromLatin1(obj->getNameInDocument()));
            Gui::ViewProvider* vp = activeGui->getViewProvider(obj);
            if (vp)
                item->setIcon(0, vp->getIcon());
        }
    }
}

bool DlgExtrusion::canExtrude(const TopoDS_Shape& shape) const
{
    if (shape.IsNull())
        return false;
    TopAbs_ShapeEnum type = shape.ShapeType();
    if (type == TopAbs_VERTEX || type == TopAbs_EDGE ||
        type == TopAbs_WIRE || type == TopAbs_FACE ||
        type == TopAbs_SHELL)
        return true;
    if (type == TopAbs_COMPOUND) {
        TopExp_Explorer xp;
        xp.Init(shape,TopAbs_SOLID);
        while (xp.More()) {
            return false;
        }
        xp.Init(shape,TopAbs_COMPSOLID);
        while (xp.More()) {
            return false;
        }

        return true;
    }

    return false;
}

void DlgExtrusion::accept()
{
    try{
        apply();
        QDialog::accept();
    } catch (Base::AbortException&){

    };
}

void DlgExtrusion::apply()
{
    try{
        if (!validate())
            throw Base::AbortException();

        if (filter) //if still selecting edge - stop. This is important for visibility automation.
            this->onSelectEdgeClicked();

        Gui::WaitCursor wc;
        App::Document* activeDoc = App::GetApplication().getDocument(this->document.c_str());
        if (!activeDoc) {
            QMessageBox::critical(this, windowTitle(),
                tr("The document '%1' doesn't exist.").arg(QString::fromUtf8(this->label.c_str())));
            return;
        }
        activeDoc->openTransaction("Extrude");

        Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
            .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part");
        bool addBaseName = hGrp->GetBool("AddBaseObjectName", false);

        std::vector<App::DocumentObject*> objects = this->getShapesToExtrude();
        for (App::DocumentObject* sourceObj: objects) {
            assert(sourceObj);

            if (Part::Feature::getTopoShape(sourceObj).isNull()){
                FC_ERR("Object " << sourceObj->getFullName()
                        << " is not Part object (has no OCC shape). Can't extrude it.");
                continue;
            }

            std::string name;
            name = sourceObj->getDocument()->getUniqueObjectName("Extrude").c_str();
            if (addBaseName) {
                //FIXME: implement
                //QString baseName = QString::fromLatin1("Extrude_%1").arg(sourceObjectName);
                //label = QString::fromLatin1("%1_Extrude").arg((*it)->text(0));
            }

            FCMD_OBJ_DOC_CMD(sourceObj,"addObject('Part::Extrusion','" << name << "')");
            auto newObj = sourceObj->getDocument()->getObject(name.c_str());

            this->writeParametersToFeature(*newObj, sourceObj);

            Gui::Command::copyVisual(newObj, "ShapeColor", sourceObj);
            Gui::Command::copyVisual(newObj, "LineColor", sourceObj);
            Gui::Command::copyVisual(newObj, "PointColor", sourceObj);

            FCMD_OBJ_HIDE(sourceObj);
        }

        activeDoc->commitTransaction();
        Gui::Command::updateActive();
    }
    catch (Base::AbortException&){
        throw;
    }
    catch (Base::Exception &err){
        QMessageBox::critical(this,
                              windowTitle(),
                              tr("Creating Extrusion failed.\n%1")
                                  .arg(QCoreApplication::translate("Exception", err.what())));
        return;
    }
    catch(...) {
        QMessageBox::critical(this, windowTitle(),
            tr("Creating Extrusion failed.\n%1").arg(QString::fromUtf8("Unknown error")));
        return;
    }
}

void DlgExtrusion::reject()
{
    if (filter) //if still selecting edge - stop.
        this->onSelectEdgeClicked();

    QDialog::reject();
}

Base::Vector3d DlgExtrusion::getDir() const
{
    return Base::Vector3d(
                ui->dirX->value(),
                ui->dirY->value(),
                ui->dirZ->value());
}

void DlgExtrusion::setDir(Base::Vector3d newDir)
{
    ui->dirX->setValue(newDir.x);
    ui->dirY->setValue(newDir.y);
    ui->dirZ->setValue(newDir.z);
}

Part::Extrusion::eDirMode DlgExtrusion::getDirMode() const
{
    if(ui->rbDirModeCustom->isChecked())
        return Part::Extrusion::dmCustom;
    if(ui->rbDirModeEdge->isChecked())
        return Part::Extrusion::dmEdge;
    if(ui->rbDirModeNormal->isChecked())
        return Part::Extrusion::dmNormal;

    //we shouldn't get here...
    return Part::Extrusion::dmCustom;
}

void DlgExtrusion::setDirMode(Part::Extrusion::eDirMode newMode)
{
    ui->rbDirModeCustom->blockSignals(true);
    ui->rbDirModeEdge->blockSignals(true);
    ui->rbDirModeNormal->blockSignals(true);

    ui->rbDirModeCustom->setChecked(newMode == Part::Extrusion::dmCustom);
    ui->rbDirModeEdge->setChecked(newMode == Part::Extrusion::dmEdge);
    ui->rbDirModeNormal->setChecked(newMode == Part::Extrusion::dmNormal);

    ui->rbDirModeCustom->blockSignals(false);
    ui->rbDirModeEdge->blockSignals(false);
    ui->rbDirModeNormal->blockSignals(false);
    this->onDirModeChanged();
}

void DlgExtrusion::getAxisLink(App::PropertyLinkSub& lnk) const
{
    QString text = ui->txtLink->text();

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

void DlgExtrusion::setAxisLink(const App::PropertyLinkSub& lnk)
{
    if (!lnk.getValue()){
        ui->txtLink->clear();
        return;
    }
    if (lnk.getSubValues().size() == 1){
        this->setAxisLink(lnk.getValue()->getNameInDocument(), lnk.getSubValues()[0].c_str());
    } else {
        this->setAxisLink(lnk.getValue()->getNameInDocument(), "");
    }
}

void DlgExtrusion::setAxisLink(const char* objname, const char* subname)
{
    if(objname && strlen(objname) > 0){
        QString txt = QString::fromLatin1(objname);
        if (subname && strlen(subname) > 0){
            txt = txt + QString::fromLatin1(":") + QString::fromLatin1(subname);
        }
        ui->txtLink->setText(txt);
    } else {
        ui->txtLink->clear();
    }
}

std::vector<App::DocumentObject*> DlgExtrusion::getShapesToExtrude() const
{
    QList<QTreeWidgetItem *> items = ui->treeWidget->selectedItems();
    App::Document* doc = App::GetApplication().getDocument(this->document.c_str());
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

bool DlgExtrusion::validate()
{
    //check source shapes
    if (ui->treeWidget->selectedItems().isEmpty()) {
        QMessageBox::critical(this, windowTitle(),
            tr("No shapes selected for extrusion. Select some, first."));
        return false;
    }

    //check axis link
    QString errmsg;
    bool hasValidAxisLink = false;
    try{
        App::PropertyLinkSub lnk;
        this->getAxisLink(lnk);
        Base::Vector3d dir, base;
        hasValidAxisLink = Part::Extrusion::fetchAxisLink(lnk, base, dir);
    } catch(Base::Exception &err) {
        errmsg = QCoreApplication::translate("Exception", err.what());
    } catch(Standard_Failure &err) {
        errmsg = QString::fromLocal8Bit(err.GetMessageString());
    } catch(...) {
        errmsg = tr("Unknown error");
    }
    if (this->getDirMode() == Part::Extrusion::dmEdge && !hasValidAxisLink){
        if (errmsg.length() > 0)
            QMessageBox::critical(this, windowTitle(), tr("Extrusion direction link is invalid.\n\n%1").arg(errmsg));
        else
            QMessageBox::critical(this, windowTitle(), tr("Direction mode is to use an edge, but no edge is linked."));
        ui->txtLink->setFocus();
        return false;
    } else if (this->getDirMode() != Part::Extrusion::dmEdge && !hasValidAxisLink){
        //axis link is invalid, but it is not required by the mode. We shouldn't complain it's invalid then...
        ui->txtLink->clear();
    }

    //check normal
    if (this->getDirMode() == Part::Extrusion::dmNormal){
        errmsg.clear();
        try {
            App::PropertyLink lnk;
            lnk.setValue(&this->getShapeToExtrude()); //simplified - check only for the first shape.
            Part::Extrusion::calculateShapeNormal(lnk);
        } catch(Base::Exception &err) {
            errmsg = QCoreApplication::translate("Exception", err.what());
        } catch(Standard_Failure &err) {
            errmsg = QString::fromLocal8Bit(err.GetMessageString());
        } catch(...) {
            errmsg = QString::fromUtf8("Unknown error");
        }
        if (errmsg.length() > 0){
            QMessageBox::critical(this, windowTitle(), tr("Can't determine normal vector of shape to be extruded. Please use other mode. \n\n(%1)").arg(errmsg));
            ui->rbDirModeNormal->setFocus();
            return false;
        }
    }

    //check axis dir
    if (this->getDirMode() == Part::Extrusion::dmCustom){
        if(this->getDir().Length() < Precision::Confusion()){
            QMessageBox::critical(this, windowTitle(),
                tr("Extrusion direction vector is zero-length. It must be non-zero."));
            ui->dirX->setFocus();
            return false;
        }
    }

    //check lengths
    if (!ui->chkSymmetric->isChecked()
            && fabs(ui->spinLenFwd->value().getValue() + ui->spinLenRev->value().getValue()) < Precision::Confusion()
            && ! (fabs(ui->spinLenFwd->value().getValue() - ui->spinLenRev->value().getValue()) < Precision::Confusion())){
        QMessageBox::critical(this, windowTitle(),
            tr("Total extrusion length is zero (length1 == -length2). It must be nonzero."));
        ui->spinLenFwd->setFocus();
        return false;
    }

    return true;
}

void DlgExtrusion::writeParametersToFeature(App::DocumentObject &feature, App::DocumentObject* base) const
{
    Gui::Command::doCommand(Gui::Command::Doc,"f = App.getDocument('%s').getObject('%s')", feature.getDocument()->getName(), feature.getNameInDocument());

    if (base)
        Gui::Command::doCommand(Gui::Command::Doc,"f.Base = App.getDocument('%s').getObject('%s')", base->getDocument()->getName(), base->getNameInDocument());

    Part::Extrusion::eDirMode dirMode = this->getDirMode();
    const char* modestr = Part::Extrusion::eDirModeStrings[dirMode];
    Gui::Command::doCommand(Gui::Command::Doc,"f.DirMode = \"%s\"", modestr);

    if (dirMode == Part::Extrusion::dmCustom){
        Base::Vector3d dir = this->getDir();
        Gui::Command::doCommand(Gui::Command::Doc, "f.Dir = App.Vector(%.15f, %.15f, %.15f)", dir.x, dir.y, dir.z);
    }

    App::PropertyLinkSub lnk;
    this->getAxisLink(lnk);
    std::stringstream linkstr;
    if (!lnk.getValue()) {
        linkstr << "None";
    } else {
        linkstr << "(App.getDocument(\"" << lnk.getValue()->getDocument()->getName() <<"\")." << lnk.getValue()->getNameInDocument();
        linkstr << ", [";
        for (const std::string &str: lnk.getSubValues()){
            linkstr << "\"" << str << "\"";
        }
        linkstr << "])";
    }
    Gui::Command::doCommand(Gui::Command::Doc,"f.DirLink = %s", linkstr.str().c_str());

    Gui::Command::doCommand(Gui::Command::Doc,"f.LengthFwd = %.15f", ui->spinLenFwd->value().getValue());
    Gui::Command::doCommand(Gui::Command::Doc,"f.LengthRev = %.15f", ui->spinLenRev->value().getValue());

    Gui::Command::doCommand(Gui::Command::Doc,"f.Solid = %s", ui->chkSolid->isChecked() ? "True" : "False");
    Gui::Command::doCommand(Gui::Command::Doc,"f.Reversed = %s", ui->chkReversed->isChecked() ? "True" : "False");
    Gui::Command::doCommand(Gui::Command::Doc,"f.Symmetric = %s", ui->chkSymmetric->isChecked() ? "True" : "False");
    Gui::Command::doCommand(Gui::Command::Doc,"f.TaperAngle = %.15f", ui->spinTaperAngle->value().getValue());
    Gui::Command::doCommand(Gui::Command::Doc,"f.TaperAngleRev = %.15f", ui->spinTaperAngleRev->value().getValue());
}


// ---------------------------------------

TaskExtrusion::TaskExtrusion()
{
    widget = new DlgExtrusion();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Extrude"),
        widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

bool TaskExtrusion::accept()
{
    widget->accept();
    return (widget->result() == QDialog::Accepted);
}

bool TaskExtrusion::reject()
{
    widget->reject();
    return true;
}

void TaskExtrusion::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        try{
            widget->apply();
        } catch (Base::AbortException&){

        };
    }
}

#include "moc_DlgExtrusion.cpp"
