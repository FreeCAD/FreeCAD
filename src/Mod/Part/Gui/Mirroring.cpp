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

# include <cmath>
# include <limits>

# include <gp_Ax2.hxx>
# include <gp_Circ.hxx>
# include <gp_Dir.hxx>
# include <gp_Pnt.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Geom_Plane.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopExp_Explorer.hxx>

# include <QMessageBox>
# include <QRegularExpression>
# include <QTreeWidget>
# include <QComboBox>
#endif

#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Utilities.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PrimitiveFeature.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/FeatureMirroring.h>
#include <App/Datums.h>

#include "Mirroring.h"

#include "ui_Mirroring.h"


using namespace PartGui;

namespace PartGui {
class MirrorPlaneSelection : public Gui::SelectionFilterGate
{
public:
    explicit MirrorPlaneSelection()
        : Gui::SelectionFilterGate()
    {
    }
    /**
     * We can't simply check if the selection is a face or an edge because only certain faces
     * and edges can work.  Bspline faces won't work, and only circle edges are supported.  But we
     * also allow document object selections for part::plane, partdesign::plane, and origin planes,
     * as well as any part::feature with only a single face or a single circle edge.  App::Links are
     * supported, provided the object they are linking to meets the above criteria.
     */

    bool allow(App::Document* /*pDoc*/, App::DocumentObject* pObj, const char* sSubName) override
    {
        std::string subString(sSubName);

        if (pObj->isDerivedFrom<Part::Plane>() || pObj->isDerivedFrom<App::Plane>()
                || (strstr(pObj->getNameInDocument(), "Plane") && pObj->isDerivedFrom<Part::Datum>())) {
            return true;
            // reference is an app::link or a part::feature or some subobject
        } else if (pObj->isDerivedFrom<Part::Feature>() || pObj->isDerivedFrom<App::Link>()) {
            bool isFace = false; //will be true if user selected face subobject or if object only has 1 face
            bool isEdge = false; //will be true if user selected edge subobject or if object only has 1 edge
            TopoDS_Shape shape;
            if (subString.length() > 0){
                shape = Part::Feature::getTopoShape(pObj,
                                                      Part::ShapeOption::NeedSubElement
                                                    | Part::ShapeOption::ResolveLink
                                                    | Part::ShapeOption::Transform,
                                                    sSubName).getShape();                            

                if (strstr(subString.c_str(), "Face")){
                    isFace = true; //was face subobject, e.g. Face3
                } else {
                    if (strstr(subString.c_str(), "Edge")){
                        isEdge = true; //was edge subobject, e.g. Edge7
                    }
                }
            } else {
                //no subobjects were selected, so this is entire shape of feature
                shape = Part::Feature::getShape(pObj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
            }

            // if there is only 1 face or 1 edge, then we don't need to force the user to select that face or edge
            // instead we can infer what was intended
            int faceCount = Part::TopoShape(shape).countSubShapes(TopAbs_FACE);
            int edgeCount = Part::TopoShape(shape).countSubShapes(TopAbs_EDGE);

            TopoDS_Face face;
            TopoDS_Edge edge;

            if (isFace) { //user selected a face, so use shape to get the TopoDS::Face
                face = TopoDS::Face(shape);
            } else {
                if (faceCount == 1) { //entire feature selected, but it only has 1 face, so get that face
                    TopoDS_Shape tdface = Part::TopoShape(shape).getSubShape(std::string("Face1").c_str());
                    face = TopoDS::Face(tdface);
                    isFace = true;
                }
            }
            if (!isFace && isEdge){ //don't bother with edge if we already have a face to work with
                edge = TopoDS::Edge(shape); //isEdge means an edge was selected
            } else {
                if (edgeCount == 1){ //we don't have a face yet and there were no edges in the subobject selection
                    //but since this object only has 1 edge, we use it
                    TopoDS_Shape tdedge = Part::TopoShape(shape).getSubShape(std::string("Edge1").c_str());
                    edge = TopoDS::Edge(tdedge);
                    isEdge = true;
                }
            }

            if (isFace && face.IsNull()) { //ensure we have a good face to work with
                return false;
            }
            if (isEdge && edge.IsNull()){ //ensure we have a good edge to work with
                return false;
            }
            if (!isFace && !isEdge){
                return false;
            }

            if (isFace) {
                BRepAdaptor_Surface adapt(face);
                if (adapt.GetType() != GeomAbs_Plane){
                    return false;
                }
                return true;
            } else {
                if (isEdge){
                    BRepAdaptor_Curve curve(edge);
                    if (!(curve.GetType() == GeomAbs_Circle)) {
                        return false;
                    }
                    return true;
                }
            }
        } //end of if(derived from part::feature)
        return true;
    }//end of allow()

}; //end of class
}; //end of namespace block


/* TRANSLATOR PartGui::Mirroring */

Mirroring::Mirroring(QWidget* parent)
  : QWidget(parent), ui(new Ui_Mirroring)
{
    ui->setupUi(this);
    constexpr double max = std::numeric_limits<double>::max();
    ui->baseX->setRange(-max, max);
    ui->baseY->setRange(-max, max);
    ui->baseZ->setRange(-max, max);
    ui->baseX->setUnit(Base::Unit::Length);
    ui->baseY->setUnit(Base::Unit::Length);
    ui->baseZ->setUnit(Base::Unit::Length);
    findShapes();

    Gui::ItemViewSelection sel(ui->shapes);
    sel.applyFrom(Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId()));
    sel.applyFrom(Gui::Selection().getObjectsOfType(App::Link::getClassTypeId()));
    sel.applyFrom(Gui::Selection().getObjectsOfType(App::Part::getClassTypeId()));

    connect(ui->selectButton, &QPushButton::clicked, this, &Mirroring::onSelectButtonClicked);

    MirrorPlaneSelection* gate = new MirrorPlaneSelection();
    Gui::Selection().addSelectionGate(gate);
}

/*
 *  Destroys the object and frees any allocated resources
 */
Mirroring::~Mirroring() = default;

void Mirroring::onSelectButtonClicked(){
    if (!ui->selectButton->isChecked()){
        Gui::Selection().rmvSelectionGate();
        ui->selectButton->setText(tr("Select reference"));
    } else {
        MirrorPlaneSelection* gate = new MirrorPlaneSelection();
        Gui::Selection().addSelectionGate(gate);
        ui->selectButton->setText(tr("Selecting"));
    }
}

void Mirroring::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

void Mirroring::onSelectionChanged(const Gui::SelectionChanges &msg)
{
    if (ui->selectButton->isChecked()) {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            std::string objName(msg.pObjectName);
            std::string subName(msg.pSubName);
            std::stringstream refStr;
            refStr << objName << " : [" << subName << "]";
            ui->referenceLineEdit->setText(QLatin1String(refStr.str().c_str()));
            ui->comboBox->setCurrentIndex(3);
        }
    }
}

void Mirroring::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc)
        return;
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    if (!activeGui)
        return;

    this->document = QString::fromLatin1(activeDoc->getName());
    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType<App::DocumentObject>();

    for (auto obj : objs) {
        Part::TopoShape shape = Part::Feature::getTopoShape(obj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
        if (!shape.isNull()) {
            QString label = QString::fromUtf8(obj->Label.getValue());
            QString name = QString::fromLatin1(obj->getNameInDocument());

            QTreeWidgetItem* child = new QTreeWidgetItem();
            child->setText(0, label);
            child->setToolTip(0, label);
            child->setData(0, Qt::UserRole, name);
            Gui::ViewProvider* vp = activeGui->getViewProvider(obj);
            if (vp) child->setIcon(0, vp->getIcon());
            ui->shapes->addTopLevelItem(child);
        }
    }
}

bool Mirroring::reject()
{
    Gui::Selection().rmvSelectionGate();
    return true;
}

bool Mirroring::accept()
{
    if (ui->shapes->selectedItems().isEmpty()) {
        QMessageBox::critical(this, windowTitle(),
            tr("Select a shape for mirroring."));
        return false;
    }

    App::Document* activeDoc = App::GetApplication().getDocument((const char*)this->document.toLatin1());
    if (!activeDoc) {
        QMessageBox::critical(this, windowTitle(),
            tr("No such document '%1'.").arg(this->document));
        return false;
    }

    Gui::WaitCursor wc;
    unsigned int count = activeDoc->countObjectsOfType<Part::Mirroring>();
    activeDoc->openTransaction("Mirroring");

    QString shape, label, selectionString;
    QRegularExpression rx(QString::fromLatin1(R"( \(Mirror #\d+\)$)"));
    QList<QTreeWidgetItem *> items = ui->shapes->selectedItems();
    float normx=0, normy=0, normz=0;
    int index = ui->comboBox->currentIndex();
    std::string selection(""); //set MirrorPlane property to empty string unless
                                //user has selected Use selected reference in combobox

    if (index == 0){
        normz = 1.0f;
    } else if (index == 1){
        normy = 1.0f;
    } else if (index == 2){
        normx = 1.0f;
    } else if (index == 3){ //use selected reference
        std::vector<Gui::SelectionObject> selobjs = Gui::Selection().getSelectionEx();
        if (selobjs.size() == 1) {
            selection = selobjs[0].getAsPropertyLinkSubString();
        }
    }
    double basex = ui->baseX->value().getValue();
    double basey = ui->baseY->value().getValue();
    double basez = ui->baseZ->value().getValue();
    for (auto item : items) {
        shape = item->data(0, Qt::UserRole).toString();
        std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(item->text(0).toUtf8());
        label = QString::fromStdString(escapedstr);
        selectionString = QString::fromStdString(selection);

        // if we already have the suffix " (Mirror #<number>)" remove it
        int pos = label.indexOf(rx);
        if (pos > -1)
            label = label.left(pos);
        label.append(QStringLiteral(" (Mirror #%1)").arg(++count));

        QString code = QStringLiteral(
            "__doc__=FreeCAD.getDocument(\"%1\")\n"
            "__doc__.addObject(\"Part::Mirroring\")\n"
            "__doc__.ActiveObject.Source=__doc__.getObject(\"%2\")\n"
            "__doc__.ActiveObject.Label=u\"%3\"\n"
            "__doc__.ActiveObject.Normal=(%4,%5,%6)\n"
            "__doc__.ActiveObject.Base=(%7,%8,%9)\n"
            "__doc__.ActiveObject.MirrorPlane=(%10)\n"
            "del __doc__")
            .arg(this->document, shape, label)
            .arg(normx).arg(normy).arg(normz)
            .arg(basex).arg(basey).arg(basez)
            .arg(selectionString);
        Gui::Command::runCommand(Gui::Command::App, code.toLatin1());
        QByteArray from = shape.toLatin1();
        Gui::Command::copyVisual("ActiveObject", "ShapeAppearance", from);
        Gui::Command::copyVisual("ActiveObject", "LineColor", from);
        Gui::Command::copyVisual("ActiveObject", "PointColor", from);
    }

    activeDoc->commitTransaction();
    activeDoc->recompute();
    Gui::Selection().rmvSelectionGate();
    return true;
}

// ---------------------------------------

TaskMirroring::TaskMirroring()
{
    widget = new Mirroring();
    addTaskBox(Gui::BitmapFactory().pixmap("Part_Mirror.svg"), widget, false);
}

bool TaskMirroring::accept()
{
    return widget->accept();
}

bool TaskMirroring::reject()
{
    return widget->reject();
}

#include "moc_Mirroring.cpp"
