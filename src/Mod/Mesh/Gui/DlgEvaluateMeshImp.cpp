/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QDockWidget>
# include <QMessageBox>
#endif

#include "DlgEvaluateMeshImp.h"

#include <boost/signals.hpp>
#include <boost/bind.hpp>

#include <Base/Interpreter.h>
#include <Base/Sequencer.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/WaitCursor.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/Mesh/App/Core/Evaluation.h>
#include <Mod/Mesh/App/Core/Degeneration.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/FeatureMeshDefects.h>
#include "ViewProviderDefects.h"

using namespace MeshCore;
using namespace Mesh;
using namespace MeshGui;

CleanupHandler::CleanupHandler()
  : QObject(qApp)
{
    // connect to lstWindowClosed signal
    connect(qApp, SIGNAL(lastWindowClosed()), this, SLOT(cleanup()));
}

// The lastWindowClosed signal will be emitted recursively and before the cleanup slot is finished
// therefore all code inside this function must handle this case!
void CleanupHandler::cleanup()
{
    DockEvaluateMeshImp::destruct();
}

// -------------------------------------------------------------

class DlgEvaluateMeshImp::Private
{
public:
    Private() : meshFeature(0), view(0)
    {
    }
    ~Private()
    {
    }

    std::map<std::string, ViewProviderMeshDefects*> vp;
    Mesh::Feature* meshFeature;
    QPointer<Gui::View3DInventor> view;
    std::vector<unsigned long> self_intersections;
};

/* TRANSLATOR MeshGui::DlgEvaluateMeshImp */

void DlgEvaluateMeshImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        this->retranslateUi(this);
        meshNameButton->setItemText(0, tr("No selection"));
    }
    QDialog::changeEvent(e);
}

void DlgEvaluateMeshImp::slotCreatedObject(const App::DocumentObject& Obj)
{
    // add new mesh object to the list
    if (Obj.getTypeId().isDerivedFrom(Mesh::Feature::getClassTypeId())) {
        QString label = QString::fromUtf8(Obj.Label.getValue());
        QString name = QString::fromAscii(Obj.getNameInDocument());
        meshNameButton->addItem(label, name);
    }
}

void DlgEvaluateMeshImp::slotDeletedObject(const App::DocumentObject& Obj)
{
    // remove mesh objects from the list
    if (Obj.getTypeId().isDerivedFrom(Mesh::Feature::getClassTypeId())) {
        int index = meshNameButton->findData(QString::fromAscii(Obj.getNameInDocument()));
        if (index > 0) {
            meshNameButton->removeItem(index);
            meshNameButton->setDisabled(meshNameButton->count() < 2);
        }
    }

    // is it the current mesh object then clear everything
    if (&Obj == d->meshFeature) {
        removeViewProviders();
        d->meshFeature = 0;
        meshNameButton->setCurrentIndex(0);
        cleanInformation();
        d->self_intersections.clear();
    }
}

void DlgEvaluateMeshImp::slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop)
{
    // if the current mesh object was modified update everything
    if (&Obj == d->meshFeature && Prop.getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        removeViewProviders();
        cleanInformation();
        showInformation();
        d->self_intersections.clear();
    }
    else if (Obj.getTypeId().isDerivedFrom(Mesh::Feature::getClassTypeId())) {
        // if the label has changed update the entry in the list
        if (Prop.getTypeId() == App::PropertyString::getClassTypeId() &&
            strcmp(Prop.getName(), "Label") == 0) {
                QString label = QString::fromUtf8(Obj.Label.getValue());
                QString name = QString::fromAscii(Obj.getNameInDocument());
                int index = meshNameButton->findData(name);
                meshNameButton->setItemText(index, label);
        }
    }
}

void DlgEvaluateMeshImp::slotCreatedDocument(const App::Document& Doc)
{
}

void DlgEvaluateMeshImp::slotDeletedDocument(const App::Document& Doc)
{
    if (&Doc == getDocument()) {
        // the view is already destroyed
        for (std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.begin(); it != d->vp.end(); ++it) {
            delete it->second;
        }

        d->vp.clear();    

        // try to attach to the active document
        this->detachDocument();
        d->view = 0;
        on_refreshButton_clicked();
    }
}

/**
 *  Constructs a DlgEvaluateMeshImp which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgEvaluateMeshImp::DlgEvaluateMeshImp(QWidget* parent, Qt::WFlags fl)
  : QDialog(parent, fl), d(new Private())
{
    this->setupUi(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line_2->setFrameShape(QFrame::HLine);
    line_2->setFrameShadow(QFrame::Sunken);
    line_3->setFrameShape(QFrame::HLine);
    line_3->setFrameShadow(QFrame::Sunken);
    line_4->setFrameShape(QFrame::HLine);
    line_4->setFrameShadow(QFrame::Sunken);
    line_5->setFrameShape(QFrame::HLine);
    line_5->setFrameShadow(QFrame::Sunken);
    line_6->setFrameShape(QFrame::HLine);
    line_6->setFrameShadow(QFrame::Sunken);
    line_7->setFrameShape(QFrame::HLine);
    line_7->setFrameShadow(QFrame::Sunken);
    line_8->setFrameShape(QFrame::HLine);
    line_8->setFrameShadow(QFrame::Sunken);

    connect(buttonHelp,  SIGNAL (clicked()),
            Gui::getMainWindow(), SLOT (whatsThis()));

    // try to attach to the active document
    this->on_refreshButton_clicked();
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgEvaluateMeshImp::~DlgEvaluateMeshImp()
{
    // no need to delete child widgets, Qt does it all for us
    for (std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.begin(); it != d->vp.end(); ++it) {
        if (d->view)
            d->view->getViewer()->removeViewProvider(it->second);
        delete it->second;
    }

    d->vp.clear();
    delete d;
}

void DlgEvaluateMeshImp::setMesh(Mesh::Feature* m)
{
    App::Document* doc = m->getDocument();
    if (doc != getDocument())
        attachDocument(doc);
  
    refreshList();

    int ct = meshNameButton->count();
    QString objName = QString::fromAscii(m->getNameInDocument());
    for (int i=1; i<ct; i++) {
        if (meshNameButton->itemData(i).toString() == objName) {
            meshNameButton->setCurrentIndex(i);
            on_meshNameButton_activated(i);
            break;
        }
    }
}

void DlgEvaluateMeshImp::addViewProvider(const char* name, const std::vector<unsigned long>& indices)
{
    removeViewProvider(name);

    if (d->view) {
        ViewProviderMeshDefects* vp = static_cast<ViewProviderMeshDefects*>(Base::Type::createInstanceByName(name));
        assert(vp->getTypeId().isDerivedFrom(Gui::ViewProvider::getClassTypeId()));
        vp->attach(d->meshFeature);
        d->view->getViewer()->addViewProvider( vp );
        vp->showDefects(indices);
        d->vp[name] = vp;
    }
}

void DlgEvaluateMeshImp::removeViewProvider(const char* name)
{
    std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.find(name);
    if (it != d->vp.end()) {
        if (d->view)
            d->view->getViewer()->removeViewProvider(it->second);
        delete it->second;
        d->vp.erase(it);
    }
}

void DlgEvaluateMeshImp::removeViewProviders()
{
    for (std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.begin(); it != d->vp.end(); ++it) {
        if (d->view)
            d->view->getViewer()->removeViewProvider(it->second);
        delete it->second;
    }
    d->vp.clear();
}

void DlgEvaluateMeshImp::on_meshNameButton_activated(int i)
{
    QString item = meshNameButton->itemData(i).toString();

    d->meshFeature = 0;
    std::vector<App::DocumentObject*> objs = getDocument()->getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        if (item == QLatin1String((*it)->getNameInDocument())) {
            d->meshFeature = (Mesh::Feature*)(*it);
            break;
        }
    }

    if (i== 0) {
        cleanInformation();
    }
    else {
        showInformation();
    }
}

void DlgEvaluateMeshImp::refreshList()
{
    QList<QPair<QString, QString> > items;
    if (this->getDocument()) {
        std::vector<App::DocumentObject*> objs = this->getDocument()->getObjectsOfType(Mesh::Feature::getClassTypeId());
        for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
            items.push_back(qMakePair(QString::fromUtf8((*it)->Label.getValue()),
                                      QString::fromAscii((*it)->getNameInDocument())));
        }
    }

    meshNameButton->clear();
    meshNameButton->addItem(tr("No selection"));
    for (QList<QPair<QString, QString> >::iterator it = items.begin(); it != items.end(); ++it)
        meshNameButton->addItem(it->first, it->second);
    meshNameButton->setDisabled(items.empty());
    cleanInformation();
}

void DlgEvaluateMeshImp::showInformation()
{
    analyzeOrientationButton->setEnabled(true);
    analyzeDuplicatedFacesButton->setEnabled(true);
    analyzeDuplicatedPointsButton->setEnabled(true);
    analyzeNonmanifoldsButton->setEnabled(true);
    analyzeDegeneratedButton->setEnabled(true);
    analyzeIndicesButton->setEnabled(true);
    analyzeSelfIntersectionButton->setEnabled(true);
    analyzeFoldsButton->setEnabled(true);
    analyzeAllTogether->setEnabled(true);

    const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
    textLabel4->setText(QString::fromAscii("%1").arg(rMesh.CountFacets()));
    textLabel5->setText(QString::fromAscii("%1").arg(rMesh.CountEdges()));
    textLabel6->setText(QString::fromAscii("%1").arg(rMesh.CountPoints()));
}

void DlgEvaluateMeshImp::cleanInformation()
{
    textLabel4->setText( tr("No information") );
    textLabel5->setText( tr("No information") );
    textLabel6->setText( tr("No information") );
    checkOrientationButton->setText( tr("No information") );
    checkDuplicatedFacesButton->setText( tr("No information") );
    checkDuplicatedPointsButton->setText( tr("No information") );
    checkNonmanifoldsButton->setText( tr("No information") );
    checkDegenerationButton->setText( tr("No information") );
    checkIndicesButton->setText( tr("No information") );
    checkSelfIntersectionButton->setText( tr("No information") );
    checkFoldsButton->setText( tr("No information") );
    analyzeOrientationButton->setDisabled(true);
    repairOrientationButton->setDisabled(true);
    analyzeDuplicatedFacesButton->setDisabled(true);
    repairDuplicatedFacesButton->setDisabled(true);
    analyzeDuplicatedPointsButton->setDisabled(true);
    repairDuplicatedPointsButton->setDisabled(true);
    analyzeNonmanifoldsButton->setDisabled(true);
    repairNonmanifoldsButton->setDisabled(true);
    analyzeDegeneratedButton->setDisabled(true);
    repairDegeneratedButton->setDisabled(true);
    analyzeIndicesButton->setDisabled(true);
    repairIndicesButton->setDisabled(true);
    analyzeSelfIntersectionButton->setDisabled(true);
    repairSelfIntersectionButton->setDisabled(true);
    analyzeFoldsButton->setDisabled(true);
    repairFoldsButton->setDisabled(true);
    analyzeAllTogether->setDisabled(true);
    repairAllTogether->setDisabled(true);
}

void DlgEvaluateMeshImp::on_refreshButton_clicked()
{
    // Connect to application and active document
    Gui::Document* gui = Gui::Application::Instance->activeDocument();
    if (gui) {
        App::Document* doc = gui->getDocument();

        // switch to the active document
        if (doc && doc != this->getDocument()) {
            attachDocument(doc);
            removeViewProviders();
            d->view = dynamic_cast<Gui::View3DInventor*>(gui->getActiveView());
        }
    }

    refreshList();
}

void DlgEvaluateMeshImp::on_checkOrientationButton_clicked()
{
    std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.find("MeshGui::ViewProviderMeshOrientation");
    if (it != d->vp.end()) {
        if (checkOrientationButton->isChecked())
            it->second->show();
        else
            it->second->hide();
    }
}

void DlgEvaluateMeshImp::on_analyzeOrientationButton_clicked()
{
    if (d->meshFeature) {
        analyzeOrientationButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalOrientation eval(rMesh);
        std::vector<unsigned long> inds = eval.GetIndices();
        if (inds.empty() && !eval.Evaluate()) {
            checkOrientationButton->setText(tr("Flipped normals found"));
            MeshEvalFoldOversOnSurface f_eval(rMesh);
            if (!f_eval.Evaluate()) {
                qApp->restoreOverrideCursor();
                QMessageBox::warning(this, tr("Orientation"),
                    tr("Check failed due to folds on the surface.\n"
                    "Please run the command to repair folds first"));
                qApp->setOverrideCursor(Qt::WaitCursor);
            }
        }
        else if (inds.empty()) {
            checkOrientationButton->setText( tr("No flipped normals") );
            checkOrientationButton->setChecked(false);
            repairOrientationButton->setEnabled(false);
            removeViewProvider( "MeshGui::ViewProviderMeshOrientation" );
        }
        else {
            checkOrientationButton->setText( tr("%1 flipped normals").arg(inds.size()) );
            checkOrientationButton->setChecked(true);
            repairOrientationButton->setEnabled(true);
            repairAllTogether->setEnabled(true);
            addViewProvider( "MeshGui::ViewProviderMeshOrientation", eval.GetIndices());
        }

        qApp->restoreOverrideCursor();
        analyzeOrientationButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::on_repairOrientationButton_clicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand("Harmonize normals");
        try {
            Gui::Application::Instance->runCommand(
                true, "App.getDocument(\"%s\").getObject(\"%s\").harmonizeNormals()"
                    , docName, objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Orientation"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();
    
        repairOrientationButton->setEnabled(false);
        checkOrientationButton->setChecked(false);
        removeViewProvider( "MeshGui::ViewProviderMeshOrientation" );
    }
}

void DlgEvaluateMeshImp::on_checkNonmanifoldsButton_clicked()
{
    std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.find("MeshGui::ViewProviderMeshNonManifolds");
    if (it != d->vp.end()) {
        if (checkNonmanifoldsButton->isChecked())
            it->second->show();
        else
            it->second->hide();
    }
}

void DlgEvaluateMeshImp::on_analyzeNonmanifoldsButton_clicked()
{
    if (d->meshFeature) {
        analyzeNonmanifoldsButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalTopology f_eval(rMesh);
        MeshEvalPointManifolds p_eval(rMesh);
        bool ok1 = f_eval.Evaluate();
        bool ok2 = p_eval.Evaluate();
    
        if (ok1 && ok2) {
            checkNonmanifoldsButton->setText(tr("No non-manifolds"));
            checkNonmanifoldsButton->setChecked(false);
            repairNonmanifoldsButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshNonManifolds");
        }
        else {
            checkNonmanifoldsButton->setText(tr("%1 non-manifolds").arg(f_eval.CountManifolds()+p_eval.CountManifolds()));
            checkNonmanifoldsButton->setChecked(true);
            repairNonmanifoldsButton->setEnabled(true);
            repairAllTogether->setEnabled(true);
            if (!ok1) {
                const std::vector<std::pair<unsigned long, unsigned long> >& inds = f_eval.GetIndices();
                std::vector<unsigned long> indices;
                indices.reserve(2*inds.size());
                std::vector<std::pair<unsigned long, unsigned long> >::const_iterator it;
                for (it = inds.begin(); it != inds.end(); ++it) {
                    indices.push_back(it->first);
                    indices.push_back(it->second);
                }

                addViewProvider("MeshGui::ViewProviderMeshNonManifolds", indices);
            }
            if (!ok2) {
                addViewProvider("MeshGui::ViewProviderMeshNonManifoldPoints", p_eval.GetIndices());
            }
        }

        qApp->restoreOverrideCursor();
        analyzeNonmanifoldsButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::on_repairNonmanifoldsButton_clicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand("Remove non-manifolds");
        try {
            Gui::Application::Instance->runCommand(
                true, "App.getDocument(\"%s\").getObject(\"%s\").removeNonManifolds()"
                    , docName, objName);
        } 
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Non-manifolds"), QString::fromLatin1(e.what()));
        }
        catch (...) {
            QMessageBox::warning(this, tr("Non-manifolds"), tr("Cannot remove non-manifolds"));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();
    
        repairNonmanifoldsButton->setEnabled(false);
        checkNonmanifoldsButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshNonManifolds");
    }
}

void DlgEvaluateMeshImp::on_checkIndicesButton_clicked()
{
    std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.find("MeshGui::ViewProviderMeshIndices");
    if (it != d->vp.end()) {
        if (checkIndicesButton->isChecked())
            it->second->show();
        else
            it->second->hide();
    }
}

void DlgEvaluateMeshImp::on_analyzeIndicesButton_clicked()
{
    if (d->meshFeature) {
        analyzeIndicesButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalRangeFacet rf(rMesh);
        MeshEvalRangePoint rp(rMesh);
        MeshEvalCorruptedFacets cf(rMesh);
        MeshEvalNeighbourhood nb(rMesh);
        
        if (!rf.Evaluate()) {
            checkIndicesButton->setText(tr("Invalid face indices"));
            checkIndicesButton->setChecked(true);
            repairIndicesButton->setEnabled(true);
            repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshIndices", rf.GetIndices());
        }
        else if (!rp.Evaluate()) {
            checkIndicesButton->setText(tr("Invalid point indices"));
            checkIndicesButton->setChecked(true);
            repairIndicesButton->setEnabled(true);
            repairAllTogether->setEnabled(true);
            //addViewProvider("MeshGui::ViewProviderMeshIndices", rp.GetIndices());
        }
        else if (!cf.Evaluate()) {
            checkIndicesButton->setText(tr("Multiple point indices"));
            checkIndicesButton->setChecked(true);
            repairIndicesButton->setEnabled(true);
            repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshIndices", cf.GetIndices());
        }
        else if (!nb.Evaluate()) {
            checkIndicesButton->setText(tr("Invalid neighbour indices"));
            checkIndicesButton->setChecked(true);
            repairIndicesButton->setEnabled(true);
            repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshIndices", nb.GetIndices());
        }
        else {
            checkIndicesButton->setText(tr("No invalid indices"));
            checkIndicesButton->setChecked(false);
            repairIndicesButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshIndices");
        }

        qApp->restoreOverrideCursor();
        analyzeIndicesButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::on_repairIndicesButton_clicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand("Fix indices");
        try {
            Gui::Application::Instance->runCommand(
                true, "App.getDocument(\"%s\").getObject(\"%s\").fixIndices()"
                    , docName, objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Indices"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();
    
        repairIndicesButton->setEnabled(false);
        checkIndicesButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshIndices");
    }
}

void DlgEvaluateMeshImp::on_checkDegenerationButton_clicked()
{
    std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.find("MeshGui::ViewProviderMeshDegenerations");
    if (it != d->vp.end()) {
        if (checkDegenerationButton->isChecked())
            it->second->show();
        else
            it->second->hide();
    }
}

void DlgEvaluateMeshImp::on_analyzeDegeneratedButton_clicked()
{
    if (d->meshFeature) {
        analyzeDegeneratedButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalDegeneratedFacets eval(rMesh);
        std::vector<unsigned long> degen = eval.GetIndices();
        
        if (degen.empty()) {
            checkDegenerationButton->setText(tr("No degenerations"));
            checkDegenerationButton->setChecked(false);
            repairDegeneratedButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshDegenerations");
        }
        else {
            checkDegenerationButton->setText(tr("%1 degenerated faces").arg(degen.size()));
            checkDegenerationButton->setChecked(true);
            repairDegeneratedButton->setEnabled(true);
            repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshDegenerations", degen);
        }

        qApp->restoreOverrideCursor();
        analyzeDegeneratedButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::on_repairDegeneratedButton_clicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand("Remove degenerated faces");
        try {
            Gui::Application::Instance->runCommand(
                true, "App.getDocument(\"%s\").getObject(\"%s\").fixDegenerations()"
                    , docName, objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Degenerations"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();

        repairDegeneratedButton->setEnabled(false);
        checkDegenerationButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshDegenerations");
    }
}

void DlgEvaluateMeshImp::on_checkDuplicatedFacesButton_clicked()
{
    std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.find("MeshGui::ViewProviderMeshDuplicatedFaces");
    if (it != d->vp.end()) {
        if (checkDuplicatedFacesButton->isChecked())
            it->second->show();
        else
            it->second->hide();
    }
}

void DlgEvaluateMeshImp::on_analyzeDuplicatedFacesButton_clicked()
{
    if (d->meshFeature) {
        analyzeDuplicatedFacesButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalDuplicateFacets eval(rMesh);
        std::vector<unsigned long> dupl = eval.GetIndices();
    
        if (dupl.empty()) {
            checkDuplicatedFacesButton->setText(tr("No duplicated faces"));
            checkDuplicatedFacesButton->setChecked(false);
            repairDuplicatedFacesButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshDuplicatedFaces");
        }
        else {
            checkDuplicatedFacesButton->setText(tr("%1 duplicated faces").arg(dupl.size()));
            checkDuplicatedFacesButton->setChecked(true);
            repairDuplicatedFacesButton->setEnabled(true);
            repairAllTogether->setEnabled(true);

            addViewProvider("MeshGui::ViewProviderMeshDuplicatedFaces", dupl);
        }

        qApp->restoreOverrideCursor();
        analyzeDuplicatedFacesButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::on_repairDuplicatedFacesButton_clicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand("Remove duplicated faces");
        try {
            Gui::Application::Instance->runCommand(
                true, "App.getDocument(\"%s\").getObject(\"%s\").removeDuplicatedFacets()"
                    , docName, objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Duplicated faces"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();
    
        repairDuplicatedFacesButton->setEnabled(false);
        checkDuplicatedFacesButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshDuplicatedFaces");
    }
}

void DlgEvaluateMeshImp::on_checkDuplicatedPointsButton_clicked()
{
    std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.find("MeshGui::ViewProviderMeshDuplicatedPoints");
    if (it != d->vp.end()) {
        if ( checkDuplicatedPointsButton->isChecked() )
            it->second->show();
        else
            it->second->hide();
    }
}

void DlgEvaluateMeshImp::on_analyzeDuplicatedPointsButton_clicked()
{
    if (d->meshFeature) {
        analyzeDuplicatedPointsButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalDuplicatePoints eval(rMesh);
    
        if (eval.Evaluate()) {
            checkDuplicatedPointsButton->setText(tr("No duplicated points"));
            checkDuplicatedPointsButton->setChecked(false);
            repairDuplicatedPointsButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshDuplicatedPoints");
        }
        else {
            checkDuplicatedPointsButton->setText(tr("Duplicated points"));
            checkDuplicatedPointsButton->setChecked(true);
            repairDuplicatedPointsButton->setEnabled(true);
            repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshDuplicatedPoints", eval.GetIndices());
        }

        qApp->restoreOverrideCursor();
        analyzeDuplicatedPointsButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::on_repairDuplicatedPointsButton_clicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand("Remove duplicated points");
        try {
            Gui::Application::Instance->runCommand(
                true, "App.getDocument(\"%s\").getObject(\"%s\").removeDuplicatedPoints()"
                    , docName, objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Duplicated points"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();
    
        repairDuplicatedPointsButton->setEnabled(false);
        checkDuplicatedPointsButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshDuplicatedPoints");
    }
}

void DlgEvaluateMeshImp::on_checkSelfIntersectionButton_clicked()
{
    std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.find("MeshGui::ViewProviderMeshSelfIntersections");
    if (it != d->vp.end()) {
        if ( checkSelfIntersectionButton->isChecked() )
            it->second->show();
        else
            it->second->hide();
    }
}

void DlgEvaluateMeshImp::on_analyzeSelfIntersectionButton_clicked()
{
    if (d->meshFeature) {
        analyzeSelfIntersectionButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalSelfIntersection eval(rMesh);
        std::vector<std::pair<unsigned long, unsigned long> > intersection;
        try {
            eval.GetIntersections(intersection);
        }
        catch (const Base::AbortException&) {
            Base::Console().Message("The self-intersection analyse was aborted by the user\n");
        }

        if (intersection.empty()) {
            checkSelfIntersectionButton->setText(tr("No self-intersections"));
            checkSelfIntersectionButton->setChecked(false);
            repairSelfIntersectionButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshSelfIntersections");
        }
        else {
            checkSelfIntersectionButton->setText(tr("Self-intersections"));
            checkSelfIntersectionButton->setChecked(true);
            repairSelfIntersectionButton->setEnabled(true);
            repairAllTogether->setEnabled(true);
            std::vector<unsigned long> indices;
            indices.reserve(2*intersection.size());
            std::vector<std::pair<unsigned long, unsigned long> >::iterator it;
            for (it = intersection.begin(); it != intersection.end(); ++it) {
                indices.push_back(it->first);
                indices.push_back(it->second);
            }

            addViewProvider("MeshGui::ViewProviderMeshSelfIntersections", indices);
            d->self_intersections.swap(indices);
        }

        qApp->restoreOverrideCursor();
        analyzeSelfIntersectionButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::on_repairSelfIntersectionButton_clicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
#if 0
        const char* objName = d->meshFeature->getNameInDocument();
#endif
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand("Fix self-intersections");
#if 0
        try {
            Gui::Application::Instance->runCommand(
                true, "App.getDocument(\"%s\").getObject(\"%s\").fixSelfIntersections()"
                    , docName, objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Self-intersections"), QString::fromLatin1(e.what()));
        }
#else
        Mesh::MeshObject* mesh = d->meshFeature->Mesh.startEditing();
        mesh->removeSelfIntersections(d->self_intersections);
        d->meshFeature->Mesh.finishEditing();
#endif

        doc->commitCommand();
        doc->getDocument()->recompute();
    
        repairSelfIntersectionButton->setEnabled(false);
        checkSelfIntersectionButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshSelfIntersections");
    }
}

void DlgEvaluateMeshImp::on_checkFoldsButton_clicked()
{
    std::map<std::string, ViewProviderMeshDefects*>::iterator it = d->vp.find("MeshGui::ViewProviderMeshFolds");
    if (it != d->vp.end()) {
        if (checkFoldsButton->isChecked())
            it->second->show();
        else
            it->second->hide();
    }
}

void DlgEvaluateMeshImp::on_analyzeFoldsButton_clicked()
{
    if (d->meshFeature) {
        analyzeFoldsButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalFoldsOnSurface s_eval(rMesh);
        MeshEvalFoldsOnBoundary b_eval(rMesh);
        MeshEvalFoldOversOnSurface f_eval(rMesh);
        bool ok1 = s_eval.Evaluate();
        bool ok2 = b_eval.Evaluate();
        bool ok3 = f_eval.Evaluate();
    
        if (ok1 && ok2 && ok3) {
            checkFoldsButton->setText(tr("No folds on surface"));
            checkFoldsButton->setChecked(false);
            repairFoldsButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshFolds");
        }
        else {
            std::vector<unsigned long> inds  = f_eval.GetIndices();
            std::vector<unsigned long> inds1 = s_eval.GetIndices();
            std::vector<unsigned long> inds2 = b_eval.GetIndices();
            inds.insert(inds.end(), inds1.begin(), inds1.end());
            inds.insert(inds.end(), inds2.begin(), inds2.end());

            // remove duplicates
            std::sort(inds.begin(), inds.end());
            inds.erase(std::unique(inds.begin(), inds.end()), inds.end());
            
            checkFoldsButton->setText(tr("%1 folds on surface").arg(inds.size()));
            checkFoldsButton->setChecked(true);
            repairFoldsButton->setEnabled(true);
            repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshFolds", inds);
        }

        qApp->restoreOverrideCursor();
        analyzeFoldsButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::on_repairFoldsButton_clicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        qApp->setOverrideCursor(Qt::WaitCursor);
        doc->openCommand("Remove folds");
        try {
            Gui::Application::Instance->runCommand(
                true, "App.getDocument(\"%s\").getObject(\"%s\").removeFoldsOnSurface()"
                    , docName, objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Folds"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();
    
        qApp->restoreOverrideCursor();
        repairFoldsButton->setEnabled(false);
        checkFoldsButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshFolds");
    }
}

void DlgEvaluateMeshImp::on_analyzeAllTogether_clicked()
{
    on_analyzeOrientationButton_clicked();
    on_analyzeDuplicatedFacesButton_clicked();
    on_analyzeDuplicatedPointsButton_clicked();
    on_analyzeNonmanifoldsButton_clicked();
    on_analyzeDegeneratedButton_clicked();
    on_analyzeIndicesButton_clicked();
    on_analyzeSelfIntersectionButton_clicked();
    on_analyzeFoldsButton_clicked();
}

void DlgEvaluateMeshImp::on_repairAllTogether_clicked()
{
    if (d->meshFeature) {
        Gui::WaitCursor wc;
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand("Repair mesh");

        bool run = false;
        bool self = true;
        int max_iter=10;
        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        try {
            do {
                run = false;
                {
                    MeshEvalSelfIntersection eval(rMesh);
                    if (self && !eval.Evaluate()) {
                        Gui::Application::Instance->runCommand(true,
                            "App.getDocument(\"%s\").getObject(\"%s\").fixSelfIntersections()",
                            docName, objName);
                        run = true;
                    }
                    else {
                        self = false; // once no self-intersections found do not repeat it later on
                    }
                    qApp->processEvents();
                }
                {
                    MeshEvalFoldsOnSurface s_eval(rMesh);
                    MeshEvalFoldsOnBoundary b_eval(rMesh);
                    MeshEvalFoldOversOnSurface f_eval(rMesh);
                    if (!s_eval.Evaluate() || !b_eval.Evaluate() || !f_eval.Evaluate()) {
                        Gui::Application::Instance->runCommand(true,
                            "App.getDocument(\"%s\").getObject(\"%s\").removeFoldsOnSurface()",
                            docName, objName);
                        run = true;
                    }
                    qApp->processEvents();
                }
                {
                    MeshEvalOrientation eval(rMesh);
                    if (!eval.Evaluate()) {
                        Gui::Application::Instance->runCommand(true,
                            "App.getDocument(\"%s\").getObject(\"%s\").harmonizeNormals()",
                            docName, objName);
                        run = true;
                    }
                    qApp->processEvents();
                }
                {
                    MeshEvalTopology eval(rMesh);
                    if (!eval.Evaluate()) {
                        Gui::Application::Instance->runCommand(true,
                            "App.getDocument(\"%s\").getObject(\"%s\").removeNonManifolds()",
                            docName, objName);
                        run = true;
                    }
                    qApp->processEvents();
                }
                {
                    MeshEvalRangeFacet rf(rMesh);
                    MeshEvalRangePoint rp(rMesh);
                    MeshEvalCorruptedFacets cf(rMesh);
                    MeshEvalNeighbourhood nb(rMesh);
                    if (!rf.Evaluate() || !rp.Evaluate() || !cf.Evaluate() || !nb.Evaluate()) {
                        Gui::Application::Instance->runCommand(true,
                            "App.getDocument(\"%s\").getObject(\"%s\").fixIndices()",
                            docName, objName);
                        run = true;
                    }
                }
                {
                    MeshEvalDegeneratedFacets eval(rMesh);
                    if (!eval.Evaluate()) {
                        Gui::Application::Instance->runCommand(true,
                            "App.getDocument(\"%s\").getObject(\"%s\").fixDegenerations()",
                            docName, objName);
                        run = true;
                    }
                    qApp->processEvents();
                }
                {
                    MeshEvalDuplicateFacets eval(rMesh);
                    if (!eval.Evaluate()) {
                        Gui::Application::Instance->runCommand(true,
                            "App.getDocument(\"%s\").getObject(\"%s\").removeDuplicatedFacets()",
                            docName, objName);
                        run = true;
                    }
                    qApp->processEvents();
                }
                {
                    MeshEvalDuplicatePoints eval(rMesh);
                    if (!eval.Evaluate()) {
                        Gui::Application::Instance->runCommand(true,
                            "App.getDocument(\"%s\").getObject(\"%s\").removeDuplicatedPoints()",
                            docName, objName);
                        run = true;
                    }
                    qApp->processEvents();
                }
            } while(checkRepeatButton->isChecked() && run && (--max_iter > 0));
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Mesh repair"), QString::fromLatin1(e.what()));
        }
        catch (...) {
            QMessageBox::warning(this, tr("Mesh repair"), QString::fromLatin1("Unknown error occurred."));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();
    }
}

// -------------------------------------------------------------

/* TRANSLATOR MeshGui::DockEvaluateMeshImp */

#if 0 // needed for Qt's lupdate utility
    qApp->translate("QDockWidget", "Evaluate & Repair Mesh");
#endif

DockEvaluateMeshImp* DockEvaluateMeshImp::_instance=0;

DockEvaluateMeshImp* DockEvaluateMeshImp::instance()
{
    // not initialized?
    if(!_instance) {
        _instance = new DockEvaluateMeshImp(Gui::getMainWindow());
        _instance->setSizeGripEnabled(false);
    }

    return _instance;
}

void DockEvaluateMeshImp::destruct ()
{
    if (_instance != 0) {
        DockEvaluateMeshImp *pTmp = _instance;
        _instance = 0;
        delete pTmp;
    }
}

bool DockEvaluateMeshImp::hasInstance()
{
    return _instance != 0;
}

/**
 *  Constructs a DockEvaluateMeshImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DockEvaluateMeshImp::DockEvaluateMeshImp( QWidget* parent, Qt::WFlags fl )
  : DlgEvaluateMeshImp( parent, fl )
{
    // embed this dialog into a dockable widget container
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    // use Qt macro for preparing for translation stuff (but not translating yet)
    QDockWidget* dw = pDockMgr->addDockWindow("Evaluate & Repair Mesh",
        this, Qt::RightDockWidgetArea);
    //dw->setAttribute(Qt::WA_DeleteOnClose);
    dw->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    dw->show();
}

/**
 *  Destroys the object and frees any allocated resources
 */
DockEvaluateMeshImp::~DockEvaluateMeshImp()
{
    _instance = 0;
}

/**
 * Destroys the dock window this object is embedded into without destroying itself.
 */
void DockEvaluateMeshImp::closeEvent(QCloseEvent* e)
{
    // closes the dock window
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    pDockMgr->removeDockWindow(this);
}

/**
 * Returns an appropriate size hint for the dock window.
 */
QSize DockEvaluateMeshImp::sizeHint () const
{
    return QSize(371, 579);
}

#include "moc_DlgEvaluateMeshImp.cpp"

