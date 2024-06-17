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
#include <QDockWidget>
#include <QMessageBox>
#include <QPointer>
#include <QScrollArea>
#endif

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/WaitCursor.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Core/Evaluation.h>
#include <Mod/Mesh/App/Core/Degeneration.h>

#include "DlgEvaluateMeshImp.h"
#include "ui_DlgEvaluateMesh.h"
#include "DlgEvaluateSettings.h"
#include "ViewProviderDefects.h"


using namespace MeshCore;
using namespace Mesh;
using namespace MeshGui;

CleanupHandler::CleanupHandler()
    : QObject(qApp)
{
    // connect to lstWindowClosed signal
    connect(qApp, &QApplication::lastWindowClosed, this, &CleanupHandler::cleanup);
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
    Private()
        : view(nullptr)
    {}

    void showFoldsFunction(bool on)
    {
        ui.label_9->setVisible(on);
        ui.line_9->setVisible(on);
        ui.checkFoldsButton->setVisible(on);
        ui.analyzeFoldsButton->setVisible(on);
        ui.repairFoldsButton->setVisible(on);
    }

    Ui_DlgEvaluateMesh ui {};
    std::map<std::string, ViewProviderMeshDefects*> vp;
    Mesh::Feature* meshFeature {nullptr};
    QPointer<Gui::View3DInventor> view;
    std::vector<Mesh::FacetIndex> self_intersections;
    bool enableFoldsCheck {false};
    bool checkNonManfoldPoints {false};
    bool strictlyDegenerated {true};
    float epsilonDegenerated {0.0F};
};

/* TRANSLATOR MeshGui::DlgEvaluateMeshImp */

/**
 *  Constructs a DlgEvaluateMeshImp which is a child of 'parent', with the
 *  widget flags set to 'f'.
 */
DlgEvaluateMeshImp::DlgEvaluateMeshImp(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , d(new Private())
{
    d->ui.setupUi(this);
    setupConnections();

    d->ui.line->setFrameShape(QFrame::HLine);
    d->ui.line->setFrameShadow(QFrame::Sunken);
    d->ui.line_2->setFrameShape(QFrame::HLine);
    d->ui.line_2->setFrameShadow(QFrame::Sunken);
    d->ui.line_3->setFrameShape(QFrame::HLine);
    d->ui.line_3->setFrameShadow(QFrame::Sunken);
    d->ui.line_4->setFrameShape(QFrame::HLine);
    d->ui.line_4->setFrameShadow(QFrame::Sunken);
    d->ui.line_5->setFrameShape(QFrame::HLine);
    d->ui.line_5->setFrameShadow(QFrame::Sunken);
    d->ui.line_6->setFrameShape(QFrame::HLine);
    d->ui.line_6->setFrameShadow(QFrame::Sunken);
    d->ui.line_7->setFrameShape(QFrame::HLine);
    d->ui.line_7->setFrameShadow(QFrame::Sunken);
    d->ui.line_8->setFrameShape(QFrame::HLine);
    d->ui.line_8->setFrameShadow(QFrame::Sunken);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Mesh/Evaluation");
    d->checkNonManfoldPoints = hGrp->GetBool("CheckNonManifoldPoints", false);
    d->enableFoldsCheck = hGrp->GetBool("EnableFoldsCheck", false);
    d->strictlyDegenerated = hGrp->GetBool("StrictlyDegenerated", true);
    if (d->strictlyDegenerated) {
        d->epsilonDegenerated = 0.0F;
    }
    else {
        d->epsilonDegenerated = MeshCore::MeshDefinitions::_fMinPointDistanceP2;
    }

    d->showFoldsFunction(d->enableFoldsCheck);

    QPushButton* button = d->ui.buttonBox->button(QDialogButtonBox::Open);
    button->setText(tr("Settings..."));

    // try to attach to the active document
    this->onRefreshButtonClicked();
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgEvaluateMeshImp::~DlgEvaluateMeshImp()
{
    // no need to delete child widgets, Qt does it all for us
    for (const auto& it : d->vp) {
        if (d->view) {
            d->view->getViewer()->removeViewProvider(it.second);
        }
        delete it.second;
    }

    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Mesh/Evaluation");
        hGrp->SetBool("CheckNonManifoldPoints", d->checkNonManfoldPoints);
        hGrp->SetBool("EnableFoldsCheck", d->enableFoldsCheck);
        hGrp->SetBool("StrictlyDegenerated", d->strictlyDegenerated);
    }
    catch (...) {
    }

    d->vp.clear();
    delete d;
}

void DlgEvaluateMeshImp::setupConnections()
{
    // clang-format off
    connect(d->ui.checkOrientationButton, &QCheckBox::clicked,
            this, &DlgEvaluateMeshImp::onCheckOrientationButtonClicked);
    connect(d->ui.analyzeOrientationButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onAnalyzeOrientationButtonClicked);
    connect(d->ui.repairOrientationButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onRepairOrientationButtonClicked);

    connect(d->ui.checkDuplicatedFacesButton, &QCheckBox::clicked,
            this, &DlgEvaluateMeshImp::onCheckDuplicatedFacesButtonClicked);
    connect(d->ui.analyzeDuplicatedFacesButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onAnalyzeDuplicatedFacesButtonClicked);
    connect(d->ui.repairDuplicatedFacesButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onRepairDuplicatedFacesButtonClicked);

    connect(d->ui.checkDuplicatedPointsButton, &QCheckBox::clicked,
            this, &DlgEvaluateMeshImp::onCheckDuplicatedPointsButtonClicked);
    connect(d->ui.analyzeDuplicatedPointsButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onAnalyzeDuplicatedPointsButtonClicked);
    connect(d->ui.repairDuplicatedPointsButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onRepairDuplicatedPointsButtonClicked);

    connect(d->ui.checkNonmanifoldsButton, &QCheckBox::clicked,
            this, &DlgEvaluateMeshImp::onCheckNonmanifoldsButtonClicked);
    connect(d->ui.analyzeNonmanifoldsButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onAnalyzeNonmanifoldsButtonClicked);
    connect(d->ui.repairNonmanifoldsButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onRepairNonmanifoldsButtonClicked);

    connect(d->ui.checkDegenerationButton, &QCheckBox::clicked,
            this, &DlgEvaluateMeshImp::onCheckDegenerationButtonClicked);
    connect(d->ui.analyzeDegeneratedButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onAnalyzeDegeneratedButtonClicked);
    connect(d->ui.repairDegeneratedButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onRepairDegeneratedButtonClicked);

    connect(d->ui.checkIndicesButton, &QCheckBox::clicked,
            this, &DlgEvaluateMeshImp::onCheckIndicesButtonClicked);
    connect(d->ui.analyzeIndicesButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onAnalyzeIndicesButtonClicked);
    connect(d->ui.repairIndicesButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onRepairIndicesButtonClicked);

    connect(d->ui.checkSelfIntersectionButton, &QCheckBox::clicked,
            this, &DlgEvaluateMeshImp::onCheckSelfIntersectionButtonClicked);
    connect(d->ui.analyzeSelfIntersectionButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onAnalyzeSelfIntersectionButtonClicked);
    connect(d->ui.repairSelfIntersectionButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onRepairSelfIntersectionButtonClicked);

    connect(d->ui.checkFoldsButton, &QCheckBox::clicked,
            this, &DlgEvaluateMeshImp::onCheckFoldsButtonClicked);
    connect(d->ui.analyzeFoldsButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onAnalyzeFoldsButtonClicked);
    connect(d->ui.repairFoldsButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onRepairFoldsButtonClicked);

    connect(d->ui.analyzeAllTogether, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onAnalyzeAllTogetherClicked);
    connect(d->ui.repairAllTogether, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onRepairAllTogetherClicked);

    connect(d->ui.refreshButton, &QPushButton::clicked,
            this, &DlgEvaluateMeshImp::onRefreshButtonClicked);
    connect(d->ui.meshNameButton, qOverload<int>(&QComboBox::activated),
            this, &DlgEvaluateMeshImp::onMeshNameButtonActivated);
    connect(d->ui.buttonBox, &QDialogButtonBox::clicked,
            this, &DlgEvaluateMeshImp::onButtonBoxClicked);
    connect(d->ui.buttonBox, &QDialogButtonBox::helpRequested,
            Gui::getMainWindow(), &Gui::MainWindow::whatsThis);
    // clang-format on
}

void DlgEvaluateMeshImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
        d->ui.meshNameButton->setItemText(0, tr("No selection"));
    }
    QDialog::changeEvent(e);
}

void DlgEvaluateMeshImp::slotCreatedObject(const App::DocumentObject& Obj)
{
    // add new mesh object to the list
    if (Obj.isDerivedFrom<Mesh::Feature>()) {
        QString label = QString::fromUtf8(Obj.Label.getValue());
        QString name = QString::fromLatin1(Obj.getNameInDocument());
        d->ui.meshNameButton->addItem(label, name);
    }
}

void DlgEvaluateMeshImp::slotDeletedObject(const App::DocumentObject& Obj)
{
    // remove mesh objects from the list
    if (Obj.isDerivedFrom<Mesh::Feature>()) {
        int index = d->ui.meshNameButton->findData(QString::fromLatin1(Obj.getNameInDocument()));
        if (index > 0) {
            d->ui.meshNameButton->removeItem(index);
            d->ui.meshNameButton->setDisabled(d->ui.meshNameButton->count() < 2);
        }
    }

    // is it the current mesh object then clear everything
    if (&Obj == d->meshFeature) {
        removeViewProviders();
        d->meshFeature = nullptr;
        d->ui.meshNameButton->setCurrentIndex(0);
        cleanInformation();
        d->self_intersections.clear();
    }
}

void DlgEvaluateMeshImp::slotChangedObject(const App::DocumentObject& Obj,
                                           const App::Property& Prop)
{
    // if the current mesh object was modified update everything
    if (&Obj == d->meshFeature && Prop.is<Mesh::PropertyMeshKernel>()) {
        removeViewProviders();
        cleanInformation();
        showInformation();
        d->self_intersections.clear();
    }
    else if (Obj.isDerivedFrom<Mesh::Feature>()) {
        // if the label has changed update the entry in the list
        if (Prop.is<App::PropertyString>() && strcmp(Prop.getName(), "Label") == 0) {
            QString label = QString::fromUtf8(Obj.Label.getValue());
            QString name = QString::fromLatin1(Obj.getNameInDocument());
            int index = d->ui.meshNameButton->findData(name);
            d->ui.meshNameButton->setItemText(index, label);
        }
    }
}

void DlgEvaluateMeshImp::slotDeletedDocument(const App::Document& Doc)
{
    if (&Doc == getDocument()) {
        // the view is already destroyed
        for (const auto& it : d->vp) {
            delete it.second;
        }

        d->vp.clear();

        // try to attach to the active document
        this->detachDocument();
        d->view = nullptr;
        onRefreshButtonClicked();
    }
}

void DlgEvaluateMeshImp::setMesh(Mesh::Feature* m)
{
    App::Document* doc = m->getDocument();
    if (doc != getDocument()) {
        attachDocument(doc);
    }

    refreshList();

    int ct = d->ui.meshNameButton->count();
    QString objName = QString::fromLatin1(m->getNameInDocument());
    for (int i = 1; i < ct; i++) {
        if (d->ui.meshNameButton->itemData(i).toString() == objName) {
            d->ui.meshNameButton->setCurrentIndex(i);
            onMeshNameButtonActivated(i);
            break;
        }
    }
}

void DlgEvaluateMeshImp::addViewProvider(const char* name,
                                         const std::vector<Mesh::ElementIndex>& indices)
{
    removeViewProvider(name);

    if (d->view) {
        auto vp = static_cast<ViewProviderMeshDefects*>(Base::Type::createInstanceByName(name));
        assert(vp->isDerivedFrom<Gui::ViewProvider>());
        vp->attach(d->meshFeature);
        d->view->getViewer()->addViewProvider(vp);
        vp->showDefects(indices);
        d->vp[name] = vp;
    }
}

void DlgEvaluateMeshImp::removeViewProvider(const char* name)
{
    auto it = d->vp.find(name);
    if (it != d->vp.end()) {
        if (d->view) {
            d->view->getViewer()->removeViewProvider(it->second);
        }
        delete it->second;
        d->vp.erase(it);
    }
}

void DlgEvaluateMeshImp::removeViewProviders()
{
    for (const auto& it : d->vp) {
        if (d->view) {
            d->view->getViewer()->removeViewProvider(it.second);
        }
        delete it.second;
    }
    d->vp.clear();
}

void DlgEvaluateMeshImp::onMeshNameButtonActivated(int i)
{
    QString item = d->ui.meshNameButton->itemData(i).toString();

    d->meshFeature = nullptr;
    std::vector<App::DocumentObject*> objs =
        getDocument()->getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (auto obj : objs) {
        if (item == QLatin1String(obj->getNameInDocument())) {
            d->meshFeature = static_cast<Mesh::Feature*>(obj);
            break;
        }
    }

    if (i == 0) {
        cleanInformation();
    }
    else {
        showInformation();
    }
}

void DlgEvaluateMeshImp::refreshList()
{
    QVector<QPair<QString, QString>> items;
    if (this->getDocument()) {
        std::vector<App::DocumentObject*> objs =
            this->getDocument()->getObjectsOfType(Mesh::Feature::getClassTypeId());
        for (auto obj : objs) {
            items.push_back(qMakePair(QString::fromUtf8(obj->Label.getValue()),
                                      QString::fromLatin1(obj->getNameInDocument())));
        }
    }

    d->ui.meshNameButton->clear();
    d->ui.meshNameButton->addItem(tr("No selection"));
    for (const auto& item : items) {
        d->ui.meshNameButton->addItem(item.first, item.second);
    }
    d->ui.meshNameButton->setDisabled(items.empty());
    cleanInformation();
}

void DlgEvaluateMeshImp::showInformation()
{
    d->ui.analyzeOrientationButton->setEnabled(true);
    d->ui.analyzeDuplicatedFacesButton->setEnabled(true);
    d->ui.analyzeDuplicatedPointsButton->setEnabled(true);
    d->ui.analyzeNonmanifoldsButton->setEnabled(true);
    d->ui.analyzeDegeneratedButton->setEnabled(true);
    d->ui.analyzeIndicesButton->setEnabled(true);
    d->ui.analyzeSelfIntersectionButton->setEnabled(true);
    d->ui.analyzeFoldsButton->setEnabled(true);
    d->ui.analyzeAllTogether->setEnabled(true);

    if (d->meshFeature) {
        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        d->ui.textLabel4->setText(QString::fromLatin1("%1").arg(rMesh.CountFacets()));
        d->ui.textLabel5->setText(QString::fromLatin1("%1").arg(rMesh.CountEdges()));
        d->ui.textLabel6->setText(QString::fromLatin1("%1").arg(rMesh.CountPoints()));
    }
}

void DlgEvaluateMeshImp::cleanInformation()
{
    d->ui.textLabel4->setText(tr("No information"));
    d->ui.textLabel5->setText(tr("No information"));
    d->ui.textLabel6->setText(tr("No information"));
    d->ui.checkOrientationButton->setText(tr("No information"));
    d->ui.checkDuplicatedFacesButton->setText(tr("No information"));
    d->ui.checkDuplicatedPointsButton->setText(tr("No information"));
    d->ui.checkNonmanifoldsButton->setText(tr("No information"));
    d->ui.checkDegenerationButton->setText(tr("No information"));
    d->ui.checkIndicesButton->setText(tr("No information"));
    d->ui.checkSelfIntersectionButton->setText(tr("No information"));
    d->ui.checkFoldsButton->setText(tr("No information"));
    d->ui.analyzeOrientationButton->setDisabled(true);
    d->ui.repairOrientationButton->setDisabled(true);
    d->ui.analyzeDuplicatedFacesButton->setDisabled(true);
    d->ui.repairDuplicatedFacesButton->setDisabled(true);
    d->ui.analyzeDuplicatedPointsButton->setDisabled(true);
    d->ui.repairDuplicatedPointsButton->setDisabled(true);
    d->ui.analyzeNonmanifoldsButton->setDisabled(true);
    d->ui.repairNonmanifoldsButton->setDisabled(true);
    d->ui.analyzeDegeneratedButton->setDisabled(true);
    d->ui.repairDegeneratedButton->setDisabled(true);
    d->ui.analyzeIndicesButton->setDisabled(true);
    d->ui.repairIndicesButton->setDisabled(true);
    d->ui.analyzeSelfIntersectionButton->setDisabled(true);
    d->ui.repairSelfIntersectionButton->setDisabled(true);
    d->ui.analyzeFoldsButton->setDisabled(true);
    d->ui.repairFoldsButton->setDisabled(true);
    d->ui.analyzeAllTogether->setDisabled(true);
    d->ui.repairAllTogether->setDisabled(true);
}

void DlgEvaluateMeshImp::onRefreshButtonClicked()
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

void DlgEvaluateMeshImp::onCheckOrientationButtonClicked()
{
    auto it = d->vp.find("MeshGui::ViewProviderMeshOrientation");
    if (it != d->vp.end()) {
        if (d->ui.checkOrientationButton->isChecked()) {
            it->second->show();
        }
        else {
            it->second->hide();
        }
    }
}

void DlgEvaluateMeshImp::onAnalyzeOrientationButtonClicked()
{
    if (d->meshFeature) {
        d->ui.analyzeOrientationButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalOrientation eval(rMesh);
        std::vector<MeshCore::FacetIndex> inds = eval.GetIndices();

        if (inds.empty()) {
            d->ui.checkOrientationButton->setText(tr("No flipped normals"));
            d->ui.checkOrientationButton->setChecked(false);
            d->ui.repairOrientationButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshOrientation");
        }
        else {
            d->ui.checkOrientationButton->setText(tr("%1 flipped normals").arg(inds.size()));
            d->ui.checkOrientationButton->setChecked(true);
            d->ui.repairOrientationButton->setEnabled(true);
            d->ui.repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshOrientation", eval.GetIndices());
        }

        qApp->restoreOverrideCursor();
        d->ui.analyzeOrientationButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::onRepairOrientationButtonClicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Harmonize normals"));
        try {
            Gui::Command::doCommand(Gui::Command::App,
                                    R"(App.getDocument("%s").getObject("%s").harmonizeNormals())",
                                    docName,
                                    objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Orientation"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();

        d->ui.repairOrientationButton->setEnabled(false);
        d->ui.checkOrientationButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshOrientation");
    }
}

void DlgEvaluateMeshImp::onCheckNonmanifoldsButtonClicked()
{
    // non-manifold edges
    std::map<std::string, ViewProviderMeshDefects*>::iterator it;
    it = d->vp.find("MeshGui::ViewProviderMeshNonManifolds");
    if (it != d->vp.end()) {
        if (d->ui.checkNonmanifoldsButton->isChecked()) {
            it->second->show();
        }
        else {
            it->second->hide();
        }
    }

    // non-manifold points
    it = d->vp.find("MeshGui::ViewProviderMeshNonManifoldPoints");
    if (it != d->vp.end()) {
        if (d->ui.checkNonmanifoldsButton->isChecked()) {
            it->second->show();
        }
        else {
            it->second->hide();
        }
    }
}

void DlgEvaluateMeshImp::onAnalyzeNonmanifoldsButtonClicked()
{
    if (d->meshFeature) {
        d->ui.analyzeNonmanifoldsButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalTopology f_eval(rMesh);
        bool ok1 = f_eval.Evaluate();
        bool ok2 = true;
        std::vector<Mesh::PointIndex> point_indices;

        if (d->checkNonManfoldPoints) {
            MeshEvalPointManifolds p_eval(rMesh);
            ok2 = p_eval.Evaluate();
            if (!ok2) {
                point_indices = p_eval.GetIndices();
            }
        }

        if (ok1 && ok2) {
            d->ui.checkNonmanifoldsButton->setText(tr("No non-manifolds"));
            d->ui.checkNonmanifoldsButton->setChecked(false);
            d->ui.repairNonmanifoldsButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshNonManifolds");
            removeViewProvider("MeshGui::ViewProviderMeshNonManifoldPoints");
        }
        else {
            d->ui.checkNonmanifoldsButton->setText(
                tr("%1 non-manifolds").arg(f_eval.CountManifolds() + point_indices.size()));
            d->ui.checkNonmanifoldsButton->setChecked(true);
            d->ui.repairNonmanifoldsButton->setEnabled(true);
            d->ui.repairAllTogether->setEnabled(true);

            if (!ok1) {
                const std::vector<std::pair<Mesh::FacetIndex, Mesh::FacetIndex>>& inds =
                    f_eval.GetIndices();
                std::vector<Mesh::FacetIndex> indices;
                indices.reserve(2 * inds.size());
                std::vector<std::pair<Mesh::FacetIndex, Mesh::FacetIndex>>::const_iterator it;
                for (it = inds.begin(); it != inds.end(); ++it) {
                    indices.push_back(it->first);
                    indices.push_back(it->second);
                }

                addViewProvider("MeshGui::ViewProviderMeshNonManifolds", indices);
            }

            if (!ok2) {
                addViewProvider("MeshGui::ViewProviderMeshNonManifoldPoints", point_indices);
            }
        }

        qApp->restoreOverrideCursor();
        d->ui.analyzeNonmanifoldsButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::onRepairNonmanifoldsButtonClicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Remove non-manifolds"));
        try {
            Gui::Command::doCommand(Gui::Command::App,
                                    R"(App.getDocument("%s").getObject("%s").removeNonManifolds())",
                                    docName,
                                    objName);

            if (d->checkNonManfoldPoints) {
                Gui::Command::doCommand(
                    Gui::Command::App,
                    R"(App.getDocument("%s").getObject("%s").removeNonManifoldPoints())",
                    docName,
                    objName);
            }
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Non-manifolds"), QString::fromLatin1(e.what()));
        }
        catch (...) {
            QMessageBox::warning(this, tr("Non-manifolds"), tr("Cannot remove non-manifolds"));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();

        d->ui.repairNonmanifoldsButton->setEnabled(false);
        d->ui.checkNonmanifoldsButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshNonManifolds");
        removeViewProvider("MeshGui::ViewProviderMeshNonManifoldsPoints");
    }
}

void DlgEvaluateMeshImp::onCheckIndicesButtonClicked()
{
    auto it = d->vp.find("MeshGui::ViewProviderMeshIndices");
    if (it != d->vp.end()) {
        if (d->ui.checkIndicesButton->isChecked()) {
            it->second->show();
        }
        else {
            it->second->hide();
        }
    }
}

void DlgEvaluateMeshImp::onAnalyzeIndicesButtonClicked()
{
    if (d->meshFeature) {
        d->ui.analyzeIndicesButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalRangeFacet rf(rMesh);
        MeshEvalRangePoint rp(rMesh);
        MeshEvalCorruptedFacets cf(rMesh);
        MeshEvalNeighbourhood nb(rMesh);

        if (!rf.Evaluate()) {
            d->ui.checkIndicesButton->setText(tr("Invalid face indices"));
            d->ui.checkIndicesButton->setChecked(true);
            d->ui.repairIndicesButton->setEnabled(true);
            d->ui.repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshIndices", rf.GetIndices());
        }
        else if (!rp.Evaluate()) {
            d->ui.checkIndicesButton->setText(tr("Invalid point indices"));
            d->ui.checkIndicesButton->setChecked(true);
            d->ui.repairIndicesButton->setEnabled(true);
            d->ui.repairAllTogether->setEnabled(true);
            // addViewProvider("MeshGui::ViewProviderMeshIndices", rp.GetIndices());
        }
        else if (!cf.Evaluate()) {
            d->ui.checkIndicesButton->setText(tr("Multiple point indices"));
            d->ui.checkIndicesButton->setChecked(true);
            d->ui.repairIndicesButton->setEnabled(true);
            d->ui.repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshIndices", cf.GetIndices());
        }
        else if (!nb.Evaluate()) {
            d->ui.checkIndicesButton->setText(tr("Invalid neighbour indices"));
            d->ui.checkIndicesButton->setChecked(true);
            d->ui.repairIndicesButton->setEnabled(true);
            d->ui.repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshIndices", nb.GetIndices());
        }
        else {
            d->ui.checkIndicesButton->setText(tr("No invalid indices"));
            d->ui.checkIndicesButton->setChecked(false);
            d->ui.repairIndicesButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshIndices");
        }

        qApp->restoreOverrideCursor();
        d->ui.analyzeIndicesButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::onRepairIndicesButtonClicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Fix indices"));
        try {
            Gui::Command::doCommand(Gui::Command::App,
                                    R"(App.getDocument("%s").getObject("%s").fixIndices())",
                                    docName,
                                    objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Indices"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();

        d->ui.repairIndicesButton->setEnabled(false);
        d->ui.checkIndicesButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshIndices");
    }
}

void DlgEvaluateMeshImp::onCheckDegenerationButtonClicked()
{
    auto it = d->vp.find("MeshGui::ViewProviderMeshDegenerations");
    if (it != d->vp.end()) {
        if (d->ui.checkDegenerationButton->isChecked()) {
            it->second->show();
        }
        else {
            it->second->hide();
        }
    }
}

void DlgEvaluateMeshImp::onAnalyzeDegeneratedButtonClicked()
{
    if (d->meshFeature) {
        d->ui.analyzeDegeneratedButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalDegeneratedFacets eval(rMesh, d->epsilonDegenerated);
        std::vector<Mesh::FacetIndex> degen = eval.GetIndices();

        if (degen.empty()) {
            d->ui.checkDegenerationButton->setText(tr("No degenerations"));
            d->ui.checkDegenerationButton->setChecked(false);
            d->ui.repairDegeneratedButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshDegenerations");
        }
        else {
            d->ui.checkDegenerationButton->setText(tr("%1 degenerated faces").arg(degen.size()));
            d->ui.checkDegenerationButton->setChecked(true);
            d->ui.repairDegeneratedButton->setEnabled(true);
            d->ui.repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshDegenerations", degen);
        }

        qApp->restoreOverrideCursor();
        d->ui.analyzeDegeneratedButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::onRepairDegeneratedButtonClicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Remove degenerated faces"));
        try {
            Gui::Command::doCommand(Gui::Command::App,
                                    R"(App.getDocument("%s").getObject("%s").fixDegenerations(%f))",
                                    docName,
                                    objName,
                                    d->epsilonDegenerated);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Degenerations"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();

        d->ui.repairDegeneratedButton->setEnabled(false);
        d->ui.checkDegenerationButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshDegenerations");
    }
}

void DlgEvaluateMeshImp::onCheckDuplicatedFacesButtonClicked()
{
    auto it = d->vp.find("MeshGui::ViewProviderMeshDuplicatedFaces");
    if (it != d->vp.end()) {
        if (d->ui.checkDuplicatedFacesButton->isChecked()) {
            it->second->show();
        }
        else {
            it->second->hide();
        }
    }
}

void DlgEvaluateMeshImp::onAnalyzeDuplicatedFacesButtonClicked()
{
    if (d->meshFeature) {
        d->ui.analyzeDuplicatedFacesButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalDuplicateFacets eval(rMesh);
        std::vector<Mesh::FacetIndex> dupl = eval.GetIndices();

        if (dupl.empty()) {
            d->ui.checkDuplicatedFacesButton->setText(tr("No duplicated faces"));
            d->ui.checkDuplicatedFacesButton->setChecked(false);
            d->ui.repairDuplicatedFacesButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshDuplicatedFaces");
        }
        else {
            d->ui.checkDuplicatedFacesButton->setText(tr("%1 duplicated faces").arg(dupl.size()));
            d->ui.checkDuplicatedFacesButton->setChecked(true);
            d->ui.repairDuplicatedFacesButton->setEnabled(true);
            d->ui.repairAllTogether->setEnabled(true);

            addViewProvider("MeshGui::ViewProviderMeshDuplicatedFaces", dupl);
        }

        qApp->restoreOverrideCursor();
        d->ui.analyzeDuplicatedFacesButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::onRepairDuplicatedFacesButtonClicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Remove duplicated faces"));
        try {
            Gui::Command::doCommand(
                Gui::Command::App,
                R"(App.getDocument("%s").getObject("%s").removeDuplicatedFacets())",
                docName,
                objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Duplicated faces"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();

        d->ui.repairDuplicatedFacesButton->setEnabled(false);
        d->ui.checkDuplicatedFacesButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshDuplicatedFaces");
    }
}

void DlgEvaluateMeshImp::onCheckDuplicatedPointsButtonClicked()
{
    auto it = d->vp.find("MeshGui::ViewProviderMeshDuplicatedPoints");
    if (it != d->vp.end()) {
        if (d->ui.checkDuplicatedPointsButton->isChecked()) {
            it->second->show();
        }
        else {
            it->second->hide();
        }
    }
}

void DlgEvaluateMeshImp::onAnalyzeDuplicatedPointsButtonClicked()
{
    if (d->meshFeature) {
        d->ui.analyzeDuplicatedPointsButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalDuplicatePoints eval(rMesh);

        if (eval.Evaluate()) {
            d->ui.checkDuplicatedPointsButton->setText(tr("No duplicated points"));
            d->ui.checkDuplicatedPointsButton->setChecked(false);
            d->ui.repairDuplicatedPointsButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshDuplicatedPoints");
        }
        else {
            d->ui.checkDuplicatedPointsButton->setText(tr("Duplicated points"));
            d->ui.checkDuplicatedPointsButton->setChecked(true);
            d->ui.repairDuplicatedPointsButton->setEnabled(true);
            d->ui.repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshDuplicatedPoints", eval.GetIndices());
        }

        qApp->restoreOverrideCursor();
        d->ui.analyzeDuplicatedPointsButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::onRepairDuplicatedPointsButtonClicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Remove duplicated points"));
        try {
            Gui::Command::doCommand(
                Gui::Command::App,
                R"(App.getDocument("%s").getObject("%s").removeDuplicatedPoints())",
                docName,
                objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Duplicated points"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();

        d->ui.repairDuplicatedPointsButton->setEnabled(false);
        d->ui.checkDuplicatedPointsButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshDuplicatedPoints");
    }
}

void DlgEvaluateMeshImp::onCheckSelfIntersectionButtonClicked()
{
    auto it = d->vp.find("MeshGui::ViewProviderMeshSelfIntersections");
    if (it != d->vp.end()) {
        if (d->ui.checkSelfIntersectionButton->isChecked()) {
            it->second->show();
        }
        else {
            it->second->hide();
        }
    }
}

void DlgEvaluateMeshImp::onAnalyzeSelfIntersectionButtonClicked()
{
    if (d->meshFeature) {
        d->ui.analyzeSelfIntersectionButton->setEnabled(false);
        qApp->processEvents();
        qApp->setOverrideCursor(Qt::WaitCursor);

        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        MeshEvalSelfIntersection eval(rMesh);
        std::vector<std::pair<Mesh::FacetIndex, Mesh::FacetIndex>> intersection;
        try {
            eval.GetIntersections(intersection);
        }
        catch (const Base::AbortException&) {
            Base::Console().Message("The self-intersection analysis was aborted by the user\n");
        }

        if (intersection.empty()) {
            d->ui.checkSelfIntersectionButton->setText(tr("No self-intersections"));
            d->ui.checkSelfIntersectionButton->setChecked(false);
            d->ui.repairSelfIntersectionButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshSelfIntersections");
        }
        else {
            d->ui.checkSelfIntersectionButton->setText(tr("Self-intersections"));
            d->ui.checkSelfIntersectionButton->setChecked(true);
            d->ui.repairSelfIntersectionButton->setEnabled(true);
            d->ui.repairAllTogether->setEnabled(true);

            std::vector<Mesh::FacetIndex> indices;
            indices.reserve(2 * intersection.size());
            std::vector<std::pair<Mesh::FacetIndex, Mesh::FacetIndex>>::iterator it;
            for (it = intersection.begin(); it != intersection.end(); ++it) {
                indices.push_back(it->first);
                indices.push_back(it->second);
            }

            addViewProvider("MeshGui::ViewProviderMeshSelfIntersections", indices);
            d->self_intersections.swap(indices);
        }

        qApp->restoreOverrideCursor();
        d->ui.analyzeSelfIntersectionButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::onRepairSelfIntersectionButtonClicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Fix self-intersections"));

        Mesh::MeshObject* mesh = d->meshFeature->Mesh.startEditing();
        mesh->removeSelfIntersections(d->self_intersections);
        d->meshFeature->Mesh.finishEditing();
        doc->commitCommand();
        doc->getDocument()->recompute();

        d->ui.repairSelfIntersectionButton->setEnabled(false);
        d->ui.checkSelfIntersectionButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshSelfIntersections");
    }
}

void DlgEvaluateMeshImp::onCheckFoldsButtonClicked()
{
    auto it = d->vp.find("MeshGui::ViewProviderMeshFolds");
    if (it != d->vp.end()) {
        if (d->ui.checkFoldsButton->isChecked()) {
            it->second->show();
        }
        else {
            it->second->hide();
        }
    }
}

void DlgEvaluateMeshImp::onAnalyzeFoldsButtonClicked()
{
    if (d->meshFeature) {
        d->ui.analyzeFoldsButton->setEnabled(false);
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
            d->ui.checkFoldsButton->setText(tr("No folds on surface"));
            d->ui.checkFoldsButton->setChecked(false);
            d->ui.repairFoldsButton->setEnabled(false);
            removeViewProvider("MeshGui::ViewProviderMeshFolds");
        }
        else {
            std::vector<Mesh::FacetIndex> inds = f_eval.GetIndices();
            std::vector<Mesh::FacetIndex> inds1 = s_eval.GetIndices();
            std::vector<Mesh::FacetIndex> inds2 = b_eval.GetIndices();
            inds.insert(inds.end(), inds1.begin(), inds1.end());
            inds.insert(inds.end(), inds2.begin(), inds2.end());

            // remove duplicates
            std::sort(inds.begin(), inds.end());
            inds.erase(std::unique(inds.begin(), inds.end()), inds.end());

            d->ui.checkFoldsButton->setText(tr("%1 folds on surface").arg(inds.size()));
            d->ui.checkFoldsButton->setChecked(true);
            d->ui.repairFoldsButton->setEnabled(true);
            d->ui.repairAllTogether->setEnabled(true);
            addViewProvider("MeshGui::ViewProviderMeshFolds", inds);
        }

        qApp->restoreOverrideCursor();
        d->ui.analyzeFoldsButton->setEnabled(true);
    }
}

void DlgEvaluateMeshImp::onRepairFoldsButtonClicked()
{
    if (d->meshFeature) {
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        qApp->setOverrideCursor(Qt::WaitCursor);
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Remove folds"));
        try {
            Gui::Command::doCommand(
                Gui::Command::App,
                R"(App.getDocument("%s").getObject("%s").removeFoldsOnSurface())",
                docName,
                objName);
        }
        catch (const Base::Exception& e) {
            QMessageBox::warning(this, tr("Folds"), QString::fromLatin1(e.what()));
        }

        doc->commitCommand();
        doc->getDocument()->recompute();

        qApp->restoreOverrideCursor();
        d->ui.repairFoldsButton->setEnabled(false);
        d->ui.checkFoldsButton->setChecked(false);
        removeViewProvider("MeshGui::ViewProviderMeshFolds");
    }
}

void DlgEvaluateMeshImp::onAnalyzeAllTogetherClicked()
{
    onAnalyzeOrientationButtonClicked();
    onAnalyzeDuplicatedFacesButtonClicked();
    onAnalyzeDuplicatedPointsButtonClicked();
    onAnalyzeNonmanifoldsButtonClicked();
    onAnalyzeDegeneratedButtonClicked();
    onAnalyzeIndicesButtonClicked();
    onAnalyzeSelfIntersectionButtonClicked();
    if (d->enableFoldsCheck) {
        onAnalyzeFoldsButtonClicked();
    }
}

void DlgEvaluateMeshImp::onRepairAllTogetherClicked()
{
    // clang-format off
    if (d->meshFeature) {
        Gui::WaitCursor wc;
        const char* docName = App::GetApplication().getDocumentName(d->meshFeature->getDocument());
        const char* objName = d->meshFeature->getNameInDocument();
        Gui::Document* doc = Gui::Application::Instance->getDocument(docName);
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Repair mesh"));

        bool run = false;
        bool self = true;
        int max_iter = 10;
        const MeshKernel& rMesh = d->meshFeature->Mesh.getValue().getKernel();
        try {
            do {
                run = false;
                {
                    MeshEvalSelfIntersection eval(rMesh);
                    if (self && !eval.Evaluate()) {
                        Gui::Command::doCommand(Gui::Command::App,
                            "App.getDocument(\"%s\").getObject(\"%s\").fixSelfIntersections()",
                            docName, objName);
                        run = true;
                    }
                    else {
                        self = false; // once no self-intersections found do not repeat it later on
                    }
                    qApp->processEvents();
                }
                if (d->enableFoldsCheck) {
                    MeshEvalFoldsOnSurface s_eval(rMesh);
                    MeshEvalFoldsOnBoundary b_eval(rMesh);
                    MeshEvalFoldOversOnSurface f_eval(rMesh);
                    if (!s_eval.Evaluate() || !b_eval.Evaluate() || !f_eval.Evaluate()) {
                        Gui::Command::doCommand(Gui::Command::App,
                            "App.getDocument(\"%s\").getObject(\"%s\").removeFoldsOnSurface()",
                            docName, objName);
                        run = true;
                    }
                    qApp->processEvents();
                }
                {
                    MeshEvalOrientation eval(rMesh);
                    if (!eval.Evaluate()) {
                        Gui::Command::doCommand(Gui::Command::App,
                            "App.getDocument(\"%s\").getObject(\"%s\").harmonizeNormals()",
                            docName, objName);
                        run = true;
                    }
                    qApp->processEvents();
                }
                {
                    MeshEvalTopology eval(rMesh);
                    if (!eval.Evaluate()) {
                        Gui::Command::doCommand(Gui::Command::App,
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
                        Gui::Command::doCommand(Gui::Command::App,
                            "App.getDocument(\"%s\").getObject(\"%s\").fixIndices()",
                            docName, objName);
                        run = true;
                    }
                }
                {
                    MeshEvalDegeneratedFacets eval(rMesh, d->epsilonDegenerated);
                    if (!eval.Evaluate()) {
                        Gui::Command::doCommand(Gui::Command::App,
                            "App.getDocument(\"%s\").getObject(\"%s\").fixDegenerations(%f)",
                            docName, objName, d->epsilonDegenerated);
                        run = true;
                    }
                    qApp->processEvents();
                }
                {
                    MeshEvalDuplicateFacets eval(rMesh);
                    if (!eval.Evaluate()) {
                        Gui::Command::doCommand(Gui::Command::App,
                            "App.getDocument(\"%s\").getObject(\"%s\").removeDuplicatedFacets()",
                            docName, objName);
                        run = true;
                    }
                    qApp->processEvents();
                }
                {
                    MeshEvalDuplicatePoints eval(rMesh);
                    if (!eval.Evaluate()) {
                        Gui::Command::doCommand(Gui::Command::App,
                            "App.getDocument(\"%s\").getObject(\"%s\").removeDuplicatedPoints()",
                            docName, objName);
                        run = true;
                    }
                    qApp->processEvents();
                }
            } while(d->ui.checkRepeatButton->isChecked() && run && (--max_iter > 0));
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
    // clang-format on
}

void DlgEvaluateMeshImp::onButtonBoxClicked(QAbstractButton* button)
{
    QDialogButtonBox::StandardButton type = d->ui.buttonBox->standardButton(button);
    if (type == QDialogButtonBox::Open) {
        DlgEvaluateSettings dlg(this);
        dlg.setNonmanifoldPointsChecked(d->checkNonManfoldPoints);
        dlg.setFoldsChecked(d->enableFoldsCheck);
        dlg.setDegeneratedFacetsChecked(d->strictlyDegenerated);
        if (dlg.exec() == QDialog::Accepted) {
            d->checkNonManfoldPoints = dlg.isNonmanifoldPointsChecked();
            d->enableFoldsCheck = dlg.isFoldsChecked();
            d->showFoldsFunction(d->enableFoldsCheck);
            d->strictlyDegenerated = dlg.isDegeneratedFacetsChecked();
            if (d->strictlyDegenerated) {
                d->epsilonDegenerated = 0.0F;
            }
            else {
                d->epsilonDegenerated = MeshCore::MeshDefinitions::_fMinPointDistanceP2;
            }
        }
    }
    else if (type == QDialogButtonBox::Reset) {
        removeViewProviders();
        cleanInformation();
        showInformation();
        d->self_intersections.clear();
        QList<QCheckBox*> cbs = this->findChildren<QCheckBox*>();
        Q_FOREACH (QCheckBox* cb, cbs) {
            cb->setChecked(false);
        }
    }
}

// -------------------------------------------------------------

/* TRANSLATOR MeshGui::DockEvaluateMeshImp */

#if 0  // needed for Qt's lupdate utility
    qApp->translate("QDockWidget", "Evaluate & Repair Mesh");
#endif

DockEvaluateMeshImp* DockEvaluateMeshImp::_instance = nullptr;

DockEvaluateMeshImp* DockEvaluateMeshImp::instance()
{
    // not initialized?
    if (!_instance) {
        _instance = new DockEvaluateMeshImp(Gui::getMainWindow());
        _instance->setSizeGripEnabled(false);
    }

    return _instance;
}

void DockEvaluateMeshImp::destruct()
{
    if (_instance) {
        DockEvaluateMeshImp* pTmp = _instance;
        _instance = nullptr;
        delete pTmp;
    }
}

bool DockEvaluateMeshImp::hasInstance()
{
    return _instance != nullptr;
}

/**
 *  Constructs a DockEvaluateMeshImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DockEvaluateMeshImp::DockEvaluateMeshImp(QWidget* parent, Qt::WindowFlags fl)
    : DlgEvaluateMeshImp(parent, fl)
{
    scrollArea = new QScrollArea();  // NOLINT
    scrollArea->setObjectName(QLatin1String("scrollArea"));
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setFrameShadow(QFrame::Plain);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(this);

    // embed this dialog into a dockable widget container
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    // use Qt macro for preparing for translation stuff (but not translating yet)
    QDockWidget* dw =
        pDockMgr->addDockWindow("Evaluate & Repair Mesh", scrollArea, Qt::RightDockWidgetArea);
    dw->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dw->show();
}

/**
 *  Destroys the object and frees any allocated resources
 */
DockEvaluateMeshImp::~DockEvaluateMeshImp()
{
    _instance = nullptr;
}

/**
 * Destroys the dock window this object is embedded into without destroying itself.
 */
void DockEvaluateMeshImp::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event)
    // closes the dock window
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    pDockMgr->removeDockWindow(scrollArea);

    // make sure to also delete the scroll area
    scrollArea->setWidget(nullptr);
    scrollArea->deleteLater();
}

/**
 * Returns an appropriate size hint for the dock window.
 */
QSize DockEvaluateMeshImp::sizeHint() const
{
    return {371, 579};
}

#include "moc_DlgEvaluateMeshImp.cpp"
