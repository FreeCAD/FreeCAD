/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <cfloat>
# include <QFuture>
# include <QKeyEvent>

# include <BRep_Builder.hxx>
# include <Standard_math.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>

# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoSeparator.h>
#endif
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <App/Document.h>
#include <Base/Sequencer.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/CrossSection.h>
#include <Mod/Part/App/PartFeature.h>

#include "CrossSections.h"
#include "ui_CrossSections.h"


using namespace PartGui;
namespace sp = std::placeholders;
#undef CS_FUTURE // multi-threading causes some problems

namespace PartGui {
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
    ~ViewProviderCrossSections() override
    {
        coords->unref();
        planes->unref();
    }
    void updateData(const App::Property*) override
    {
    }
    const char* getDefaultDisplayMode() const override
    {
        return "";
    }
    std::vector<std::string> getDisplayModes() const override
    {
        return {};
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
}

CrossSections::CrossSections(const Base::BoundBox3d& bb, QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl)
  , ui(new Ui_CrossSections)
  , bbox(bb)
{
    ui->setupUi(this);
    setupConnections();

    ui->position->setRange(-DBL_MAX, DBL_MAX);
    ui->position->setUnit(Base::Unit::Length);
    ui->distance->setRange(0, DBL_MAX);
    ui->distance->setUnit(Base::Unit::Length);
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
    if (view) {
        view->getViewer()->removeViewProvider(vp);
    }
    delete vp;
}

void CrossSections::setupConnections()
{
    connect(ui->xyPlane, &QRadioButton::clicked,
            this, &CrossSections::xyPlaneClicked);
    connect(ui->xzPlane, &QRadioButton::clicked,
            this, &CrossSections::xzPlaneClicked);
    connect(ui->yzPlane, &QRadioButton::clicked,
            this, &CrossSections::yzPlaneClicked);
    connect(ui->position, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &CrossSections::positionValueChanged);
    connect(ui->distance, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &CrossSections::distanceValueChanged);
    connect(ui->countSections, qOverload<int>(&QSpinBox::valueChanged),
            this, &CrossSections::countSectionsValueChanged);
    connect(ui->checkBothSides, &QCheckBox::toggled,
            this, &CrossSections::checkBothSidesToggled);
    connect(ui->sectionsBox, &QGroupBox::toggled,
            this, &CrossSections::sectionsBoxToggled);
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
    std::vector<App::DocumentObject*> docobjs = Gui::Selection().
            getObjectsOfType(App::DocumentObject::getClassTypeId());
    std::vector<App::DocumentObject*> obj;
    for (auto it : docobjs) {
        if (!Part::Feature::getTopoShape(it).isNull()) {
            obj.push_back(it);
        }
    }

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

#ifdef CS_FUTURE
    Standard::SetReentrant(Standard_True);
    for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
        Part::CrossSection cs(a,b,c,static_cast<Part::Feature*>(*it)->Shape.getValue());
        QFuture< std::list<TopoDS_Wire> > future = QtConcurrent::mapped
            (d, std::bind(&Part::CrossSection::section, &cs, sp::_1));
        future.waitForFinished();
        QFuture< std::list<TopoDS_Wire> >::const_iterator ft;
        TopoDS_Compound comp;
        BRep_Builder builder;
        builder.MakeCompound(comp);

        for (ft = future.begin(); ft != future.end(); ++ft) {
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
    Base::SequencerLauncher seq("Cross-sections...", obj.size() * (d.size() +1));
    Gui::Command::runCommand(Gui::Command::App, "import Part\n");
    Gui::Command::runCommand(Gui::Command::App, "from FreeCAD import Base\n");
    for (auto it : obj) {
        App::Document* doc = it->getDocument();
        std::string s = it->getNameInDocument();
        s += "_cs";
        Gui::Command::runCommand(Gui::Command::App, QString::fromLatin1(
            "wires=list()\n"
            "shape=FreeCAD.getDocument(\"%1\").%2.Shape\n")
            .arg(QLatin1String(doc->getName()),
                 QLatin1String(it->getNameInDocument())).toLatin1());

        for (double jt : d) {
            Gui::Command::runCommand(Gui::Command::App, QString::fromLatin1(
                "for i in shape.slice(Base.Vector(%1,%2,%3),%4):\n"
                "    wires.append(i)\n"
                ).arg(a).arg(b).arg(c).arg(jt).toLatin1());
            seq.next();
        }

        Gui::Command::runCommand(Gui::Command::App, QString::fromLatin1(
            "comp=Part.Compound(wires)\n"
            "slice=FreeCAD.getDocument(\"%1\").addObject(\"Part::Feature\",\"%2\")\n"
            "slice.Shape=comp\n"
            "slice.purgeTouched()\n"
            "del slice,comp,wires,shape")
            .arg(QLatin1String(doc->getName()),
                 QLatin1String(s.c_str())).toLatin1());

        seq.next();
    }
#endif
}

void CrossSections::xyPlaneClicked()
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

void CrossSections::xzPlaneClicked()
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

void CrossSections::yzPlaneClicked()
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

void CrossSections::positionValueChanged(double v)
{
    if (!ui->sectionsBox->isChecked()) {
        calcPlane(plane(), v);
    }
    else {
        calcPlanes(plane());
    }
}

void CrossSections::sectionsBoxToggled(bool b)
{
    if (b) {
        countSectionsValueChanged(ui->countSections->value());
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

void CrossSections::checkBothSidesToggled(bool b)
{
    double d = ui->distance->value().getValue();
    d = b ? 2.0 * d : 0.5 * d;
    ui->distance->setValue(d);
    calcPlanes(plane());
}

void CrossSections::countSectionsValueChanged(int v)
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

void CrossSections::distanceValueChanged(double)
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
    for (double it : d) {
        Base::Vector3f v[4];
        switch (type) {
            case XY:
                v[0].Set(bound[0],bound[2],it);
                v[1].Set(bound[1],bound[2],it);
                v[2].Set(bound[1],bound[3],it);
                v[3].Set(bound[0],bound[3],it);
                break;
            case XZ:
                v[0].Set(bound[0],it,bound[2]);
                v[1].Set(bound[1],it,bound[2]);
                v[2].Set(bound[1],it,bound[3]);
                v[3].Set(bound[0],it,bound[3]);
                break;
            case YZ:
                v[0].Set(it,bound[0],bound[2]);
                v[1].Set(it,bound[1],bound[2]);
                v[2].Set(it,bound[1],bound[3]);
                v[3].Set(it,bound[0],bound[3]);
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
        Gui::BitmapFactory().pixmap("Part_CrossSections"),
        widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
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
