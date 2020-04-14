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
# include <TopExp_Explorer.hxx>
# include <QMessageBox>
#endif

#include "Tessellation.h"
#include "ui_Tessellation.h"
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Mesh/Gui/ViewProvider.h>
#include <Mod/Part/Gui/ViewProvider.h>

using namespace MeshPartGui;

/* TRANSLATOR MeshPartGui::Tessellation */

Tessellation::Tessellation(QWidget* parent)
  : QWidget(parent), ui(new Ui_Tessellation)
{
    ui->setupUi(this);
    gmsh = new Mesh2ShapeGmsh(this);
    connect(gmsh, SIGNAL(processed()), this, SLOT(gmshProcessed()));

    ui->stackedWidget->addTab(gmsh, tr("gmsh"));

    ParameterGrp::handle handle = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Mod/Mesh/Meshing/Standard");
    double value = ui->spinSurfaceDeviation->value().getValue();
    value = handle->GetFloat("LinearDeflection", value);
    double angle = ui->spinAngularDeviation->value().getValue();
    angle = handle->GetFloat("AngularDeflection", angle);
    bool relative = ui->relativeDeviation->isChecked();
    relative = handle->GetBool("RelativeLinearDeflection", relative);
    ui->relativeDeviation->setChecked(relative);

    ui->spinSurfaceDeviation->setMaximum(INT_MAX);
    ui->spinSurfaceDeviation->setValue(value);
    ui->spinAngularDeviation->setValue(angle);

    ui->spinMaximumEdgeLength->setRange(0, INT_MAX);

    ui->comboFineness->setCurrentIndex(2);
    on_comboFineness_currentIndexChanged(2);

#if !defined (HAVE_MEFISTO)
    ui->stackedWidget->setTabEnabled(Mefisto, false);
#endif
#if !defined (HAVE_NETGEN)
    ui->stackedWidget->setTabEnabled(Netgen, false);
#endif

    Gui::Command::doCommand(Gui::Command::Doc, "import Mesh, Part, PartGui");
    try {
        Gui::Command::doCommand(Gui::Command::Doc, "import MeshPart");
    }
    catch (...) {
        ui->stackedWidget->setTabEnabled(Mefisto, false);
        ui->stackedWidget->setTabEnabled(Netgen, false);
    }
}

Tessellation::~Tessellation()
{
}

void Tessellation::meshingMethod(int id)
{
    ui->stackedWidget->setCurrentIndex(id);
}

void Tessellation::on_comboFineness_currentIndexChanged(int index)
{
    if (index == 5) {
        ui->doubleGrading->setEnabled(true);
        ui->spinEdgeElements->setEnabled(true);
        ui->spinCurvatureElements->setEnabled(true);
    }
    else {
        ui->doubleGrading->setEnabled(false);
        ui->spinEdgeElements->setEnabled(false);
        ui->spinCurvatureElements->setEnabled(false);
    }

    switch (index) {
    case 0: // Very coarse
        ui->doubleGrading->setValue(0.7);
        ui->spinEdgeElements->setValue(0.3);
        ui->spinCurvatureElements->setValue(1.0);
        break;
    case 1: // Coarse
        ui->doubleGrading->setValue(0.5);
        ui->spinEdgeElements->setValue(0.5);
        ui->spinCurvatureElements->setValue(1.5);
        break;
    case 2: // Moderate
        ui->doubleGrading->setValue(0.3);
        ui->spinEdgeElements->setValue(1.0);
        ui->spinCurvatureElements->setValue(2.0);
        break;
    case 3: // Fine
        ui->doubleGrading->setValue(0.2);
        ui->spinEdgeElements->setValue(2.0);
        ui->spinCurvatureElements->setValue(3.0);
        break;
    case 4: // Very fine
        ui->doubleGrading->setValue(0.1);
        ui->spinEdgeElements->setValue(3.0);
        ui->spinCurvatureElements->setValue(5.0);
        break;
    default:
        break;
    }
}

void Tessellation::on_checkSecondOrder_toggled(bool on)
{
    if (on)
        ui->checkQuadDominated->setChecked(false);
}

void Tessellation::on_checkQuadDominated_toggled(bool on)
{
    if (on)
        ui->checkSecondOrder->setChecked(false);
}

void Tessellation::gmshProcessed()
{
    bool doClose = !ui->checkBoxDontQuit->isChecked();
    if (doClose)
        Gui::Control().reject();
}

void Tessellation::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        int index = ui->comboFineness->currentIndex();
        ui->retranslateUi(this);
        ui->comboFineness->setCurrentIndex(index);
    }
    QWidget::changeEvent(e);
}

namespace MeshPartGui {
struct ShapeInfo {
    App::DocumentObjectT obj;
    std::string subname;

    ShapeInfo(const App::DocumentObject *o, const char *s)
        : obj(o), subname(s)
    {}
};
}

void Tessellation::on_estimateMaximumEdgeLength_clicked()
{
    std::list<ShapeInfo> shapeObjects;
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc) {
        return;
    }

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    if (!activeGui) {
        return;
    }

    double edgeLen = 0;
    for (auto &sel : Gui::Selection().getSelection("*",0)) {
        auto shape = Part::Feature::getTopoShape(sel.pObject,sel.SubName);
        if (shape.hasSubShape(TopAbs_FACE)) {
            Base::BoundBox3d bbox = shape.getBoundBox();
            edgeLen = std::max<double>(edgeLen, bbox.LengthX());
            edgeLen = std::max<double>(edgeLen, bbox.LengthY());
            edgeLen = std::max<double>(edgeLen, bbox.LengthZ());
            shapeObjects.emplace_back(sel.pObject, sel.SubName);
        }
    }

    ui->spinMaximumEdgeLength->setValue(edgeLen/10);
}

bool Tessellation::accept()
{
    std::list<ShapeInfo> shapeObjects;
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc) {
        QMessageBox::critical(this, windowTitle(), tr("No active document"));
        return false;
    }

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    if (!activeGui) {
        QMessageBox::critical(this, windowTitle(), tr("No active document"));
        return false;
    }

    this->document = QString::fromLatin1(activeDoc->getName());

    for (auto &sel : Gui::Selection().getSelection("*",0)) {
        auto shape = Part::Feature::getTopoShape(sel.pObject,sel.SubName);
        if (shape.hasSubShape(TopAbs_FACE)) {
            shapeObjects.emplace_back(sel.pObject, sel.SubName);
        }
    }

    if (shapeObjects.empty()) {
        QMessageBox::critical(this, windowTitle(),
            tr("Select a shape for meshing, first."));
        return false;
    }

    bool doClose = !ui->checkBoxDontQuit->isChecked();
    int method = ui->stackedWidget->currentIndex();

    // For gmsh the workflow is very different because it uses an executable
    // and therefore things are asynchronous
    if (method == Gmsh) {
        std::list<App::SubObjectT> obj;
        for (const auto &info : shapeObjects) {
            obj.emplace_back(info.obj, info.subname.c_str());
        }
        gmsh->process(activeDoc, obj);
        return false;
    }

    try {
        QString objname, label, subname;
        Gui::WaitCursor wc;

        // Save parameters
        if (method == Standard) {
            ParameterGrp::handle handle = App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/Mod/Mesh/Meshing/Standard");
            double value = ui->spinSurfaceDeviation->value().getValue();
            handle->SetFloat("LinearDeflection", value);
            double angle = ui->spinAngularDeviation->value().getValue();
            handle->SetFloat("AngularDeflection", angle);
            bool relative = ui->relativeDeviation->isChecked();
            handle->SetBool("RelativeLinearDeflection", relative);
        }

        activeDoc->openTransaction("Meshing");
        for (auto &info : shapeObjects) {
            subname = QString::fromLatin1(info.subname.c_str());
            objname = QString::fromLatin1(info.obj.getObjectName().c_str());

            auto obj = info.obj.getObject();
            if (!obj)
                continue;
            auto sobj = obj->getSubObject(info.subname.c_str());
            if (!sobj)
                continue;
            sobj = sobj->getLinkedObject(true);
            if (!sobj)
                continue;
            label = QString::fromUtf8(sobj->Label.getValue());
            auto svp = Base::freecad_dynamic_cast<PartGui::ViewProviderPartExt>(
                    Gui::Application::Instance->getViewProvider(sobj));

            QString param;
            if (method == Standard) { // Standard
                double devFace = ui->spinSurfaceDeviation->value().getValue();
                double devAngle = ui->spinAngularDeviation->value().getValue();
                devAngle = Base::toRadians<double>(devAngle);
                bool relative = ui->relativeDeviation->isChecked();
                param = QString::fromLatin1("Shape=__shape__, "
                                            "LinearDeflection=%1, "
                                            "AngularDeflection=%2, "
                                            "Relative=%3")
                    .arg(devFace)
                    .arg(devAngle)
                    .arg(relative ? QString::fromLatin1("True") : QString::fromLatin1("False"));
                if (ui->meshShapeColors->isChecked())
                    param += QString::fromLatin1(",Segments=True");
                if (ui->groupsFaceColors->isChecked() && svp) {
                    // TODO: currently, we can only retrieve part feature
                    // color. The problem is that if the feature is linked,
                    // there are potentially many places where the color can
                    // get overridden.
                    //
                    // With topo naming feature merged, it will be possible to
                    // infer more accurate colors from just the shape names,
                    // with static function,
                    //
                    // PartGui::ViewProviderPartExt::getShapeColors().
                    //
                    param += QString::fromLatin1(",GroupColors=Gui.getDocument('%1').getObject('%2').DiffuseColor")
                            .arg(QString::fromLatin1(sobj->getDocument()->getName()),
                                 QString::fromLatin1(sobj->getNameInDocument()));
                }
            }
            else if (method == Mefisto) { // Mefisto
                double maxEdge = ui->spinMaximumEdgeLength->value().getValue();
                if (!ui->spinMaximumEdgeLength->isEnabled())
                    maxEdge = 0;
                param = QString::fromLatin1("Shape=__shape__,MaxLength=%1").arg(maxEdge);
            }
            else if (method == Netgen) { // Netgen
                int fineness = ui->comboFineness->currentIndex();
                double growthRate = ui->doubleGrading->value();
                double nbSegPerEdge = ui->spinEdgeElements->value();
                double nbSegPerRadius = ui->spinCurvatureElements->value();
                bool secondOrder = ui->checkSecondOrder->isChecked();
                bool optimize = ui->checkOptimizeSurface->isChecked();
                bool allowquad = ui->checkQuadDominated->isChecked();
                if (fineness < 5) {
                    param = QString::fromLatin1("Shape=__shape__,"
                        "Fineness=%1,SecondOrder=%2,Optimize=%3,AllowQuad=%4")
                        .arg(fineness)
                        .arg(secondOrder ? 1 : 0)
                        .arg(optimize ? 1 : 0)
                        .arg(allowquad ? 1 : 0);
                }
                else {
                    param = QString::fromLatin1("Shape=__shape__,"
                        "GrowthRate=%1,SegPerEdge=%2,SegPerRadius=%3,SecondOrder=%4,Optimize=%5,AllowQuad=%6")
                        .arg(growthRate)
                        .arg(nbSegPerEdge)
                        .arg(nbSegPerRadius)
                        .arg(secondOrder ? 1 : 0)
                        .arg(optimize ? 1 : 0)
                        .arg(allowquad ? 1 : 0);
                }
            }

            QString cmd = QString::fromLatin1(
                "__doc__=FreeCAD.getDocument(\"%1\")\n"
                "__mesh__=__doc__.addObject(\"Mesh::Feature\",\"Mesh\")\n"
                "__part__=__doc__.getObject(\"%2\")\n"
                "__shape__=Part.getShape(__part__,\"%3\")\n"
                "__mesh__.Mesh=MeshPart.meshFromShape(%4)\n"
                "__mesh__.Label=\"%5 (Meshed)\"\n"
                "del __doc__, __mesh__, __part__, __shape__\n")
                .arg(this->document)
                .arg(objname)
                .arg(subname)
                .arg(param)
                .arg(label);

            Gui::Command::runCommand(Gui::Command::Doc, cmd.toUtf8());

            // if Standard mesher is used and face colors should be applied
            if (method == Standard) { // Standard
                if (ui->meshShapeColors->isChecked()) {
                    Gui::ViewProvider* vpm = Gui::Application::Instance->getViewProvider
                            (activeDoc->getActiveObject());
                    MeshGui::ViewProviderMesh* vpmesh = dynamic_cast<MeshGui::ViewProviderMesh*>(vpm);
                    if (vpmesh && svp) {
                        std::vector<App::Color> diff_col = svp->DiffuseColor.getValues();
                        if (ui->groupsFaceColors->isChecked()) {
                            // unique colors
                            std::set<uint32_t> col_set;
                            for (auto it : diff_col)
                                col_set.insert(it.getPackedValue());
                            diff_col.clear();
                            for (auto it : col_set)
                                diff_col.push_back(App::Color(it));
                        }
                        vpmesh->highlightSegments(diff_col);
                    }
                }
            }
        }
        activeDoc->commitTransaction();
    }
    catch (const Base::Exception& e) {
        activeDoc->abortTransaction();
        Base::Console().Error(e.what());
    }

    return doClose;
}

// ---------------------------------------

class Mesh2ShapeGmsh::Private {
public:
    std::string label;
    std::list<App::SubObjectT> shapes;
    App::DocumentT doc;
    std::string cadFile;
    std::string stlFile;
    std::string geoFile;
};

Mesh2ShapeGmsh::Mesh2ShapeGmsh(QWidget* parent, Qt::WindowFlags fl)
  : GmshWidget(parent, fl)
  , d(new Private())
{
    d->cadFile = App::Application::getTempFileName() + "mesh.brep";
    d->stlFile = App::Application::getTempFileName() + "mesh.stl";
    d->geoFile = App::Application::getTempFileName() + "mesh.geo";
}

Mesh2ShapeGmsh::~Mesh2ShapeGmsh()
{
}

void Mesh2ShapeGmsh::process(App::Document* doc, const std::list<App::SubObjectT>& objs)
{
    d->doc = doc;
    d->shapes = objs;

    doc->openTransaction("Meshing");
    accept();
}

bool Mesh2ShapeGmsh::writeProject(QString& inpFile, QString& outFile)
{
    if (!d->shapes.empty()) {
        App::SubObjectT sub = d->shapes.front();
        d->shapes.pop_front();

        App::DocumentObject* part = sub.getObject();
        if (part) {
            Part::TopoShape shape = Part::Feature::getTopoShape(part, sub.getSubName().c_str());
            shape.exportBrep(d->cadFile.c_str());
            d->label = part->Label.getStrValue() + " (Meshed)";

            // Parameters
            int algorithm = meshingAlgorithm();
            double maxSize = getMaxSize();
            if (maxSize == 0.0)
                maxSize = 1.0e22;
            double minSize = getMinSize();

            // gmsh geo file
            Base::FileInfo geo(d->geoFile);
            Base::ofstream geoOut(geo, std::ios::out);
            geoOut << "// geo file for meshing with Gmsh meshing software created by FreeCAD\n"
                << "// open brep geometry\n"
                << "Merge \"" << d->cadFile << "\";\n\n"
                << "// Characteristic Length\n"
                << "// no boundary layer settings for this mesh\n"
                << "// min, max Characteristic Length\n"
                << "Mesh.CharacteristicLengthMax = " << maxSize << ";\n"
                << "Mesh.CharacteristicLengthMin = " << minSize << ";\n\n"
                << "// optimize the mesh\n"
                << "Mesh.Optimize = 1;\n"
                << "Mesh.OptimizeNetgen = 0;\n"
                << "// for more HighOrderOptimize parameter check http://gmsh.info/doc/texinfo/gmsh.html\n"
                << "Mesh.HighOrderOptimize = 0;\n\n"
                << "// mesh order\n"
                << "Mesh.ElementOrder = 2;\n"
                << "// Second order nodes are created by linear interpolation instead by curvilinear\n"
                << "Mesh.SecondOrderLinear = 1;\n\n"
                << "// mesh algorithm, only a few algorithms are usable with 3D boundary layer generation\n"
                << "// 2D mesh algorithm (1=MeshAdapt, 2=Automatic, 5=Delaunay, 6=Frontal, 7=BAMG, 8=DelQuad)\n"
                << "Mesh.Algorithm = " << algorithm << ";\n"
                << "// 3D mesh algorithm (1=Delaunay, 2=New Delaunay, 4=Frontal, 5=Frontal Delaunay, 6=Frontal Hex, 7=MMG3D, 9=R-tree)\n"
                << "Mesh.Algorithm3D = 1;\n\n"
                << "// meshing\n"
                << "// set geometrical tolerance (also used for merging nodes)\n"
                << "Geometry.Tolerance = 1e-06;\n"
                << "Mesh  2;\n"
                << "Coherence Mesh; // Remove duplicate vertices\n";
            geoOut.close();

            inpFile = QString::fromUtf8(d->geoFile.c_str());
            outFile = QString::fromUtf8(d->stlFile.c_str());

            return true;
        }
    }
    else {
        App::Document* doc = d->doc.getDocument();
        if (doc)
            doc->commitTransaction();

        Q_EMIT processed();
    }

    return false;
}

bool Mesh2ShapeGmsh::loadOutput()
{
    App::Document* doc = d->doc.getDocument();
    if (!doc)
        return false;

    // Now read-in the mesh
    Base::FileInfo stl(d->stlFile);
    Base::FileInfo geo(d->geoFile);

    Mesh::MeshObject kernel;
    MeshCore::MeshInput input(kernel.getKernel());
    Base::ifstream stlIn(stl, std::ios::in | std::ios::binary);
    input.LoadBinarySTL(stlIn);
    stlIn.close();
    kernel.harmonizeNormals();

    Mesh::Feature* fea = static_cast<Mesh::Feature*>(doc->addObject("Mesh::Feature", "Mesh"));
    fea->Label.setValue(d->label);
    fea->Mesh.setValue(kernel.getKernel());
    stl.deleteFile();
    geo.deleteFile();

    // process next object
    accept();

    return true;
}

// ---------------------------------------

TaskTessellation::TaskTessellation()
{
    widget = new Tessellation();
    Gui::TaskView::TaskBox* taskbox = new Gui::TaskView::TaskBox(
        QPixmap()/*Gui::BitmapFactory().pixmap("MeshPart_Mesher")*/,
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskTessellation::~TaskTessellation()
{
    // automatically deleted in the sub-class
}

void TaskTessellation::open()
{
}

void TaskTessellation::clicked(int)
{
}

bool TaskTessellation::accept()
{
    return widget->accept();
}

bool TaskTessellation::reject()
{
    return true;
}

#include "moc_Tessellation.cpp"
