/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QPushButton>
# include <QTextCursor>
# include <QTime>
#endif

#include "RemeshGmsh.h"
#include "ui_RemeshGmsh.h"
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Widgets.h>
#include <Gui/ReportView.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Core/MeshIO.h>

using namespace MeshGui;

class GmshWidget::Private {
public:
    Private(QWidget* parent)
      : gmsh(parent)
    {
    }

    void appendText(const QString& text, bool error) {
        syntax->setParagraphType(error ? Gui::DockWnd::ReportHighlighter::Error
                                       : Gui::DockWnd::ReportHighlighter::Message);
        QTextCursor cursor(ui.outputWindow->document());
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(text);
        cursor.endEditBlock();
        ui.outputWindow->ensureCursorVisible();
    }

public:
    Ui_RemeshGmsh ui;
    QPointer<Gui::StatusWidget> label;
    QPointer<Gui::DockWnd::ReportHighlighter> syntax;
    QProcess gmsh;
    QTime time;
};

GmshWidget::GmshWidget(QWidget* parent, Qt::WindowFlags fl)
  : QWidget(parent, fl)
  , d(new Private(parent))
{
    connect(&d->gmsh, SIGNAL(started()), this, SLOT(started()));
    connect(&d->gmsh, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(finished(int, QProcess::ExitStatus)));
    connect(&d->gmsh, SIGNAL(errorOccurred(QProcess::ProcessError)),
            this, SLOT(errorOccurred(QProcess::ProcessError)));
    connect(&d->gmsh, SIGNAL(readyReadStandardError()),
            this, SLOT(readyReadStandardError()));
    connect(&d->gmsh, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readyReadStandardOutput()));

    d->ui.setupUi(this);
    d->ui.fileChooser->onRestore();
    d->syntax = new Gui::DockWnd::ReportHighlighter(d->ui.outputWindow);
    d->ui.outputWindow->setReadOnly(true);

    // Meshing algorithms
    // 1=MeshAdapt, 2=Automatic, 5=Delaunay, 6=Frontal, 7=BAMG, 8=Frontal Quad
    // 9=Packing of Parallelograms
    d->ui.method->addItem(tr("Automatic"), static_cast<int>(2));
    d->ui.method->addItem(tr("Adaptive"), static_cast<int>(1));
    d->ui.method->addItem(QString::fromLatin1("Delaunay"), static_cast<int>(5));
    d->ui.method->addItem(tr("Frontal"), static_cast<int>(6));
    d->ui.method->addItem(QString::fromLatin1("BAMG"), static_cast<int>(5));
    d->ui.method->addItem(tr("Frontal Quad"), static_cast<int>(6));
    d->ui.method->addItem(tr("Parallelograms"), static_cast<int>(9));
}

GmshWidget::~GmshWidget()
{
    d->ui.fileChooser->onSave();
}

void GmshWidget::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

bool GmshWidget::writeProject(QString& inpFile, QString& outFile)
{
    Q_UNUSED(inpFile)
    Q_UNUSED(outFile)

    return false;
}

bool GmshWidget::loadOutput()
{
    return false;
}

int GmshWidget::meshingAlgorithm() const
{
    return d->ui.method->itemData(d->ui.method->currentIndex()).toInt();
}

double GmshWidget::getAngle() const
{
    return d->ui.angle->value().getValue();
}

double GmshWidget::getMaxSize() const
{
    return d->ui.maxSize->value().getValue();
}

double GmshWidget::getMinSize() const
{
    return d->ui.minSize->value().getValue();
}

void GmshWidget::accept()
{
    if (d->gmsh.state() == QProcess::Running) {
        Base::Console().Warning("Cannot start gmsh because it's already running\n");
        return;
    }

    QString inpFile;
    QString outFile;
    if (writeProject(inpFile, outFile)) {
        // ./gmsh - -bin -2 /tmp/mesh.geo -o /tmp/best.stl
        QString proc = d->ui.fileChooser->fileName();
        QStringList args;
        args << QLatin1String("-")
             << QLatin1String("-bin")
             << QLatin1String("-2")
             << inpFile
             << QLatin1String("-o")
             << outFile;
        d->gmsh.start(proc, args);

        d->time.start();
        d->ui.labelTime->setText(tr("Time:"));
    }
}

void GmshWidget::readyReadStandardError()
{
    QByteArray msg = d->gmsh.readAllStandardError();
    if (msg.startsWith("\0[1m\0[31m")) {
        msg = msg.mid(9);
    }
    if (msg.endsWith("\0[0m")) {
        msg.chop(5);
    }

    QString text = QString::fromUtf8(msg.data());
    d->appendText(text, true);
}

void GmshWidget::readyReadStandardOutput()
{
    QByteArray msg = d->gmsh.readAllStandardOutput();
    QString text = QString::fromUtf8(msg.data());
    d->appendText(text, false);
}

void GmshWidget::on_killButton_clicked()
{
    if (d->gmsh.state() == QProcess::Running) {
        d->gmsh.kill();
        d->gmsh.waitForFinished(1000);
        d->ui.killButton->setDisabled(true);
    }
}

void GmshWidget::on_clearButton_clicked()
{
    d->ui.outputWindow->clear();
}

void GmshWidget::started()
{
    d->ui.killButton->setEnabled(true);
    if (!d->label) {
        d->label = new Gui::StatusWidget(this);
        d->label->setAttribute(Qt::WA_DeleteOnClose);
        d->label->setStatusText(tr("Running gmsh..."));
        d->label->show();
    }
}

void GmshWidget::finished(int /*exitCode*/, QProcess::ExitStatus exitStatus)
{
    d->ui.killButton->setDisabled(true);
    if (d->label)
        d->label->close();

    d->ui.labelTime->setText(QString::fromLatin1("%1 %2 ms").arg(tr("Time:")).arg(d->time.elapsed()));
    if (exitStatus == QProcess::NormalExit) {
        loadOutput();
    }
}

void GmshWidget::errorOccurred(QProcess::ProcessError error)
{
    QString msg;
    switch (error) {
    case QProcess::FailedToStart:
        msg = tr("Failed to start");
        break;
    default:
        break;
    }

    if (!msg.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), msg);
    }
}

void GmshWidget::reject()
{
    on_killButton_clicked();
}

// -------------------------------------------------

class RemeshGmsh::Private {
public:
    Private(Mesh::Feature* mesh)
      : mesh(mesh)
    {
    }

public:
    App::DocumentObjectWeakPtrT mesh;
    MeshCore::MeshKernel copy;
    std::string stlFile;
    std::string geoFile;
};

RemeshGmsh::RemeshGmsh(Mesh::Feature* mesh, QWidget* parent, Qt::WindowFlags fl)
  : GmshWidget(parent, fl)
  , d(new Private(mesh))
{
    // Copy mesh that is used each time when applying gmsh's remeshing function
    d->copy = mesh->Mesh.getValue().getKernel();
    d->stlFile = App::Application::getTempFileName() + "mesh.stl";
    d->geoFile = App::Application::getTempFileName() + "mesh.geo";
}

RemeshGmsh::~RemeshGmsh()
{
}

bool RemeshGmsh::writeProject(QString& inpFile, QString& outFile)
{
    if (!d->mesh.expired()) {
        Base::FileInfo stl(d->stlFile);
        MeshCore::MeshOutput output(d->copy);
        Base::ofstream stlOut(stl, std::ios::out | std::ios::binary);
        output.SaveBinarySTL(stlOut);
        stlOut.close();

        // Parameters
        int algorithm = meshingAlgorithm();
        double maxSize = getMaxSize();
        if (maxSize == 0.0)
            maxSize = 1.0e22;
        double minSize = getMinSize();
        double angle = getAngle();
        int maxAngle = 120;
        int minAngle = 20;

        // gmsh geo file
        Base::FileInfo geo(d->geoFile);
        Base::ofstream geoOut(geo, std::ios::out);
        // Examples on how to use gmsh: https://sfepy.org/doc-devel/preprocessing.html
        // http://gmsh.info//doc/texinfo/gmsh.html
        // https://docs.salome-platform.org/latest/gui/GMSHPLUGIN/gmsh_2d_3d_hypo_page.html
        geoOut << "// geo file for meshing with Gmsh meshing software created by FreeCAD\n"
            << "If(GMSH_MAJOR_VERSION < 4)\n"
            << "   Error(\"Too old gmsh version %g.%g. At least 4.x is required\", GMSH_MAJOR_VERSION, GMSH_MINOR_VERSION);\n"
            << "   Exit;\n"
            << "EndIf\n"
            << "Merge \"" << stl.filePath() << "\";\n\n"
            << "// 2D mesh algorithm (1=MeshAdapt, 2=Automatic, 5=Delaunay, 6=Frontal, 7=BAMG, 8=Frontal Quad)\n"
            << "Mesh.Algorithm = " << algorithm << ";\n\n"
            << "// 3D mesh algorithm (1=Delaunay, 2=New Delaunay, 4=Frontal, 5=Frontal Delaunay, 6=Frontal Hex, 7=MMG3D, 9=R-tree)\n"
            << "// Mesh.Algorithm3D = 1;\n\n"
            << "Mesh.CharacteristicLengthMax = " << maxSize << ";\n"
            << "Mesh.CharacteristicLengthMin = " << minSize << ";\n\n"
            << "// We first classify (\"color\") the surfaces by splitting the original surface\n"
            << "// along sharp geometrical features. This will create new discrete surfaces,\n"
            << "// curves and points.\n"
            << "angle = DefineNumber[" << angle << ", Min " << minAngle << ", Max " << maxAngle << ", Step 1,\n"
            << "  Name \"Parameters/Angle for surface detection\" ];\n\n"
            << "forceParametrizablePatches = DefineNumber[0, Choices{0,1},\n"
            << "  Name \"Parameters/Create surfaces guaranteed to be parametrizable\"];\n\n"
            << "includeBoundary = 1;\n"
            << "ClassifySurfaces{angle * Pi/180, includeBoundary, forceParametrizablePatches};\n"
            << "// Create a geometry for all the discrete curves and surfaces in the mesh, by\n"
            << "// computing a parametrization for each one\n"
            << "CreateGeometry;\n\n"
            << "// Create a volume as usual\n"
            << "Surface Loop(1) = Surface{:};\n"
            << "Volume(1) = {1};\n";
        geoOut.close();

        inpFile = QString::fromUtf8(d->geoFile.c_str());
        outFile = QString::fromUtf8(d->stlFile.c_str());

        return true;
    }

    return false;
}

bool RemeshGmsh::loadOutput()
{
    if (d->mesh.expired())
        return false;

    // Now read-in modified mesh
    Base::FileInfo stl(d->stlFile);
    Base::FileInfo geo(d->geoFile);

    Mesh::MeshObject kernel;
    MeshCore::MeshInput input(kernel.getKernel());
    Base::ifstream stlIn(stl, std::ios::in | std::ios::binary);
    input.LoadBinarySTL(stlIn);
    stlIn.close();
    kernel.harmonizeNormals();

    Mesh::Feature* fea = d->mesh.get<Mesh::Feature>();
    App::Document* doc = fea->getDocument();
    doc->openTransaction("Remesh");
    fea->Mesh.setValue(kernel.getKernel());
    doc->commitTransaction();
    stl.deleteFile();
    geo.deleteFile();

    return true;
}

// -------------------------------------------------

/* TRANSLATOR MeshGui::TaskRemeshGmsh */

TaskRemeshGmsh::TaskRemeshGmsh(Mesh::Feature* mesh)
{
    widget = new RemeshGmsh(mesh);
    taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), false, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskRemeshGmsh::~TaskRemeshGmsh()
{
    // automatically deleted in the sub-class
}

void TaskRemeshGmsh::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        widget->accept();
    }
    else if (id == QDialogButtonBox::Close) {
        widget->reject();
    }
}

#include "moc_RemeshGmsh.cpp"
