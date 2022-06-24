/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>
# include <Standard_math.hxx>
# include <BRep_Builder.hxx>
# include <BRepBuilderAPI_MakePolygon.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopExp_Explorer.hxx>
# include <cfloat>
# include <QFuture>
# include <QFutureWatcher>
# include <QKeyEvent>
# include <QMessageBox>
# include <QtConcurrentMap>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

#include "ui_CrossSections.h"
#include "CrossSections.h"
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Grid.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <App/Document.h>
#include <Base/UnitsApi.h>

using namespace MeshPartGui;
namespace bp = boost::placeholders;

namespace MeshPartGui {
class ViewProviderCrossSections : public Gui::ViewProvider
{
public:
    ViewProviderCrossSections()
    {
        coords = new SoCoordinate3();
        coords->ref();
        planes = new SoLineSet();
        planes->ref();
        SoBaseColor* color = new SoBaseColor();
        color->rgb.setValue(1.0f, 0.447059f, 0.337255f);
        SoDrawStyle* style = new SoDrawStyle();
        style->lineWidth.setValue(2.0f);
        this->pcRoot->addChild(color);
        this->pcRoot->addChild(style);
        this->pcRoot->addChild(coords);
        this->pcRoot->addChild(planes);
    }
    ~ViewProviderCrossSections()
    {
        coords->unref();
        planes->unref();
    }
    void updateData(const App::Property*)
    {
    }
    const char* getDefaultDisplayMode() const
    {
        return "";
    }
    std::vector<std::string> getDisplayModes(void) const
    {
        return std::vector<std::string>();
    }
    void setCoords(const std::vector<Base::Vector3f>& v)
    {
        coords->point.setNum(v.size());
        SbVec3f* p = coords->point.startEditing();
        for (unsigned int i=0; i<v.size(); i++) {
            const Base::Vector3f& pt = v[i];
            p[i].setValue(pt.x,pt.y,pt.z);
        }
        coords->point.finishEditing();
        unsigned int count = v.size()/5;
        planes->numVertices.setNum(count);
        int32_t* l = planes->numVertices.startEditing();
        for (unsigned int i=0; i<count; i++) {
            l[i] = 5;
        }
        planes->numVertices.finishEditing();
    }

private:
    SoCoordinate3* coords;
    SoLineSet* planes;
};

class MeshCrossSection {
public:
    MeshCrossSection(const MeshCore::MeshKernel& mesh,
                     const MeshCore::MeshFacetGrid& grid,
                     double x, double y, double z,
                     bool connectEdges, double eps)
        : mesh(mesh)
        , grid(grid)
        , x(x)
        , y(y)
        , z(z)
        , connectEdges(connectEdges)
        , epsilon(eps)
    {
    }
    std::list<TopoDS_Wire> section(double d)
    {
        Mesh::MeshObject::TPolylines polylines;
        MeshCore::MeshAlgorithm algo(mesh);
        Base::Vector3f p(x*d, y*d, z*d);
        Base::Vector3f n(x, y, z);
        algo.CutWithPlane(p, n, grid, polylines, epsilon, connectEdges);

        std::list<TopoDS_Wire> wires;
        for (auto it = polylines.begin(); it != polylines.end(); ++it) {
            BRepBuilderAPI_MakePolygon mkPoly;
            for (auto jt = it->begin(); jt != it->end(); ++jt) {
                mkPoly.Add(Base::convertTo<gp_Pnt>(*jt));
            }

            if (mkPoly.IsDone())
                wires.push_back(mkPoly.Wire());
        }

        return wires;
    }

private:
    const MeshCore::MeshKernel& mesh;
    const MeshCore::MeshFacetGrid& grid;
    double x,y,z;
    bool connectEdges;
    double epsilon;
};
}

CrossSections::CrossSections(const Base::BoundBox3d& bb, QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl), bbox(bb)
{
    ui = new Ui_CrossSections();
    ui->setupUi(this);
    ui->position->setRange(-DBL_MAX, DBL_MAX);
    ui->position->setUnit(Base::Unit::Length);
    ui->distance->setRange(0, DBL_MAX);
    ui->distance->setUnit(Base::Unit::Length);
    ui->spinEpsilon->setMinimum(0.0001);
    vp = new ViewProviderCrossSections();

    Base::Vector3d c = bbox.GetCenter();
    calcPlane(CrossSections::XY, c.z);
    ui->position->setValue(c.z);

    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    view = qobject_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        view->getViewer()->addViewProvider(vp);
    }
}

/*  
 *  Destroys the object and frees any allocated resources
 */
CrossSections::~CrossSections()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
    if (view) {
        view->getViewer()->removeViewProvider(vp);
    }
    delete vp;
}

CrossSections::Plane CrossSections::plane() const
{
    if (ui->xyPlane->isChecked())
        return CrossSections::XY;
    else if (ui->xzPlane->isChecked())
        return CrossSections::XZ;
    else
        return CrossSections::YZ;
}

void CrossSections::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QDialog::changeEvent(e);
    }
}

void CrossSections::keyPressEvent(QKeyEvent* ke)
{
    // The cross-sections dialog is embedded into a task panel
    // which is a parent widget and will handle the event
    ke->ignore();
}

void CrossSections::accept()
{
    apply();
    QDialog::accept();
}

void CrossSections::apply()
{
    std::vector<App::DocumentObject*> obj = Gui::Selection().
        getObjectsOfType(Mesh::Feature::getClassTypeId());

    std::vector<double> d;
    if (ui->sectionsBox->isChecked())
        d = getPlanes();
    else
        d.push_back(ui->position->value().getValue());
    double a=0,b=0,c=0;
    switch (plane()) {
        case CrossSections::XY:
            c = 1.0;
            break;
        case CrossSections::XZ:
            b = 1.0;
            break;
        case CrossSections::YZ:
            a = 1.0;
            break;
    }

    bool connectEdges = ui->checkBoxConnect->isChecked();
    double eps = ui->spinEpsilon->value();

#if 1 // multi-threaded sections
    for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
        const Mesh::MeshObject& mesh = static_cast<Mesh::Feature*>(*it)->Mesh.getValue();

        MeshCore::MeshKernel kernel(mesh.getKernel());
        kernel.Transform(mesh.getTransform());

        MeshCore::MeshFacetGrid grid(kernel);

        MeshCrossSection cs(kernel, grid, a, b, c, connectEdges, eps);
        QFuture< std::list<TopoDS_Wire> > future = QtConcurrent::mapped
            (d, boost::bind(&MeshCrossSection::section, &cs, bp::_1));
        future.waitForFinished();

        TopoDS_Compound comp;
        BRep_Builder builder;
        builder.MakeCompound(comp);

        for (auto ft = future.begin(); ft != future.end(); ++ft) {
            const std::list<TopoDS_Wire>& w = *ft;
            for (std::list<TopoDS_Wire>::const_iterator wt = w.begin(); wt != w.end(); ++wt) {
                if (!wt->IsNull())
                    builder.Add(comp, *wt);
            }
        }

        App::Document* doc = (*it)->getDocument();
        std::string s = (*it)->getNameInDocument();
        s += "_cs";
        Part::Feature* section = static_cast<Part::Feature*>
            (doc->addObject("Part::Feature",s.c_str()));
        section->Shape.setValue(comp);
        section->purgeTouched();
    }
#else
    try {
        Gui::Command::runCommand(Gui::Command::App, "import Mesh, Part\n");
        Gui::Command::runCommand(Gui::Command::App, "from FreeCAD import Base\n");

        std::stringstream str;
        str << "[";
        for (std::vector<double>::iterator jt = d.begin(); jt != d.end(); ++jt) {
            double d = *jt;
            str << "("
                << "App.Vector(" << a*d << ", " << b*d << ", " << c*d << "), "
                << "App.Vector(" << a   << ", " << b   << ", " << c   << ")"
                << "), ";
        }
        str << "]";

        QString planes = QString::fromStdString(str.str());
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
            App::Document* doc = (*it)->getDocument();
            std::string s = (*it)->getNameInDocument();
            s += "_cs";
            Gui::Command::runCommand(Gui::Command::App, QString::fromLatin1(
                "points=FreeCAD.getDocument(\"%1\").%2.Mesh.crossSections(%3, %4, %5)\n"
                "wires=[]\n"
                "for i in points:\n"
                "    wires.extend([Part.makePolygon(j) for j in i])\n")
                .arg(QLatin1String(doc->getName()))
                .arg(QLatin1String((*it)->getNameInDocument()))
                .arg(planes)
                .arg(eps)
                .arg(connectEdges ? QLatin1String("True") : QLatin1String("False"))
                .toLatin1());

            Gui::Command::runCommand(Gui::Command::App, QString::fromLatin1(
                "comp=Part.Compound(wires)\n"
                "slice=FreeCAD.getDocument(\"%1\").addObject(\"Part::Feature\",\"%2\")\n"
                "slice.Shape=comp\n"
                "slice.purgeTouched()\n"
                "del slice,comp,wires,points")
                .arg(QLatin1String(doc->getName()))
                .arg(QLatin1String(s.c_str())).toLatin1());
        }
    }
    catch (const Base::Exception& e) {
        QMessageBox::critical(this, tr("Failure"), QString::fromLatin1(e.what()));
    }
#endif
}

void CrossSections::on_xyPlane_clicked()
{
    Base::Vector3d c = bbox.GetCenter();
    ui->position->setValue(c.z);
    if (!ui->sectionsBox->isChecked()) {
        calcPlane(CrossSections::XY, c.z);
    }
    else {
        double dist = bbox.LengthZ() / ui->countSections->value();
        if (!ui->checkBothSides->isChecked())
            dist *= 0.5f;
        ui->distance->setValue(dist);
        calcPlanes(CrossSections::XY);
    }
}

void CrossSections::on_xzPlane_clicked()
{
    Base::Vector3d c = bbox.GetCenter();
    ui->position->setValue(c.y);
    if (!ui->sectionsBox->isChecked()) {
        calcPlane(CrossSections::XZ, c.y);
    }
    else {
        double dist = bbox.LengthY() / ui->countSections->value();
        if (!ui->checkBothSides->isChecked())
            dist *= 0.5f;
        ui->distance->setValue(dist);
        calcPlanes(CrossSections::XZ);
    }
}

void CrossSections::on_yzPlane_clicked()
{
    Base::Vector3d c = bbox.GetCenter();
    ui->position->setValue(c.x);
    if (!ui->sectionsBox->isChecked()) {
        calcPlane(CrossSections::YZ, c.x);
    }
    else {
        double dist = bbox.LengthX() / ui->countSections->value();
        if (!ui->checkBothSides->isChecked())
            dist *= 0.5f;
        ui->distance->setValue(dist);
        calcPlanes(CrossSections::YZ);
    }
}

void CrossSections::on_position_valueChanged(double v)
{
    if (!ui->sectionsBox->isChecked()) {
        calcPlane(plane(), v);
    }
    else {
        calcPlanes(plane());
    }
}

void CrossSections::on_sectionsBox_toggled(bool b)
{
    if (b) {
        on_countSections_valueChanged(ui->countSections->value());
    }
    else {
        CrossSections::Plane type = plane();
        Base::Vector3d c = bbox.GetCenter();
        double value = 0;
        switch (type) {
            case CrossSections::XY:
                value = c.z;
                break;
            case CrossSections::XZ:
                value = c.y;
                break;
            case CrossSections::YZ:
                value = c.x;
                break;
        }

        ui->position->setValue(value);
        calcPlane(type, value);
    }
}

void CrossSections::on_checkBothSides_toggled(bool b)
{
    double d = ui->distance->value().getValue();
    d = b ? 2.0 * d : 0.5 * d;
    ui->distance->setValue(d);
    calcPlanes(plane());
}

void CrossSections::on_countSections_valueChanged(int v)
{
    CrossSections::Plane type = plane();
    double dist = 0;
    switch (type) {
        case CrossSections::XY:
            dist = bbox.LengthZ() / v;
            break;
        case CrossSections::XZ:
            dist = bbox.LengthY() / v;
            break;
        case CrossSections::YZ:
            dist = bbox.LengthX() / v;
            break;
    }
    if (!ui->checkBothSides->isChecked())
        dist *= 0.5f;
    ui->distance->setValue(dist);
    calcPlanes(type);
}

void CrossSections::on_distance_valueChanged(double)
{
    calcPlanes(plane());
}

void CrossSections::calcPlane(Plane type, double pos)
{
    double bound[4];
    switch (type) {
        case XY:
            bound[0] = bbox.MinX;
            bound[1] = bbox.MaxX;
            bound[2] = bbox.MinY;
            bound[3] = bbox.MaxY;
            break;
        case XZ:
            bound[0] = bbox.MinX;
            bound[1] = bbox.MaxX;
            bound[2] = bbox.MinZ;
            bound[3] = bbox.MaxZ;
            break;
        case YZ:
            bound[0] = bbox.MinY;
            bound[1] = bbox.MaxY;
            bound[2] = bbox.MinZ;
            bound[3] = bbox.MaxZ;
            break;
    }

    std::vector<double> d;
    d.push_back(pos);
    makePlanes(type, d, bound);
}

void CrossSections::calcPlanes(Plane type)
{
    double bound[4];
    switch (type) {
        case XY:
            bound[0] = bbox.MinX;
            bound[1] = bbox.MaxX;
            bound[2] = bbox.MinY;
            bound[3] = bbox.MaxY;
            break;
        case XZ:
            bound[0] = bbox.MinX;
            bound[1] = bbox.MaxX;
            bound[2] = bbox.MinZ;
            bound[3] = bbox.MaxZ;
            break;
        case YZ:
            bound[0] = bbox.MinY;
            bound[1] = bbox.MaxY;
            bound[2] = bbox.MinZ;
            bound[3] = bbox.MaxZ;
            break;
    }

    std::vector<double> d = getPlanes();
    makePlanes(type, d, bound);
}

std::vector<double> CrossSections::getPlanes() const
{
    int count = ui->countSections->value();
    double pos = ui->position->value().getValue();
    double stp = ui->distance->value().getValue();
    bool both = ui->checkBothSides->isChecked();

    std::vector<double> d;
    if (both) {
        double start = pos-0.5f*(count-1)*stp;
        for (int i=0; i<count; i++) {
            d.push_back(start+i*stp);
        }
    }
    else {
        for (int i=0; i<count; i++) {
            d.push_back(pos+i*stp);
        }
    }
    return d;
}

void CrossSections::makePlanes(Plane type, const std::vector<double>& d, double bound[4])
{
    std::vector<Base::Vector3f> points;
    for (std::vector<double>::const_iterator it = d.begin(); it != d.end(); ++it) {
        Base::Vector3f v[4];
        switch (type) {
            case XY:
                v[0].Set(bound[0],bound[2],*it);
                v[1].Set(bound[1],bound[2],*it);
                v[2].Set(bound[1],bound[3],*it);
                v[3].Set(bound[0],bound[3],*it);
                break;
            case XZ:
                v[0].Set(bound[0],*it,bound[2]);
                v[1].Set(bound[1],*it,bound[2]);
                v[2].Set(bound[1],*it,bound[3]);
                v[3].Set(bound[0],*it,bound[3]);
                break;
            case YZ:
                v[0].Set(*it,bound[0],bound[2]);
                v[1].Set(*it,bound[1],bound[2]);
                v[2].Set(*it,bound[1],bound[3]);
                v[3].Set(*it,bound[0],bound[3]);
                break;
        }

        points.push_back(v[0]);
        points.push_back(v[1]);
        points.push_back(v[2]);
        points.push_back(v[3]);
        points.push_back(v[0]);
    }
    vp->setCoords(points);
}

// ---------------------------------------

TaskCrossSections::TaskCrossSections(const Base::BoundBox3d& bb)
{
    widget = new CrossSections(bb);
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Mesh_CrossSections"),
        widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskCrossSections::~TaskCrossSections()
{
    // automatically deleted in the sub-class
}

bool TaskCrossSections::accept()
{
    widget->accept();
    return (widget->result() == QDialog::Accepted);
}

void TaskCrossSections::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        widget->apply();
    }
}

#include "moc_CrossSections.cpp"
