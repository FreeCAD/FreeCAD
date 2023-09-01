/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QAction>
# include <QApplication>
# include <QLabel>
# include <QMenu>
# include <QMessageBox>
# include <QPainter>
# include <QSplitter>
# include <QTimer>
# include <QVBoxLayout>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/fields/SoSFImage.h>
# include <Inventor/nodes/SoImage.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/sensors/SoNodeSensor.h>
#endif

#include <App/Document.h>
#include <App/GeoFeature.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SplitView3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/WaitCursor.h>

#include "ManualAlignment.h"
#include "BitmapFactory.h"
#include "SoAxisCrossKit.h"
#include "Tools.h"


using namespace Gui;
namespace sp = std::placeholders;

AlignmentGroup::AlignmentGroup() = default;

AlignmentGroup::~AlignmentGroup() = default;

void AlignmentGroup::addView(App::DocumentObject* pView)
{
    if (pView) {
        App::Document* rDoc = pView->getDocument();
        Gui::Document* pDoc = Gui::Application::Instance->getDocument(rDoc);
        auto pProvider = static_cast<Gui::ViewProviderDocumentObject*>
            (pDoc->getViewProvider(pView));
        this->_views.push_back(pProvider);
    }
}

std::vector<App::DocumentObject*> AlignmentGroup::getViews() const
{
    std::vector<App::DocumentObject*> views;

    std::vector<Gui::ViewProviderDocumentObject*>::const_iterator it;
    for (it = this->_views.begin(); it != this->_views.end(); ++it) {
        App::DocumentObject* pView = (*it)->getObject();
        views.push_back(pView);
    }

    return views;
}

bool AlignmentGroup::hasView(Gui::ViewProviderDocumentObject* pView) const
{
    std::vector<Gui::ViewProviderDocumentObject*>::const_iterator it;
    for (it = this->_views.begin(); it != this->_views.end(); ++it) {
        if (*it == pView)
            return true;
    }

    return false;
}

void AlignmentGroup::removeView(Gui::ViewProviderDocumentObject* pView)
{
    std::vector<Gui::ViewProviderDocumentObject*>::iterator it;
    for (it = this->_views.begin(); it != this->_views.end(); ++it) {
        if (*it == pView) {
            this->_views.erase(it);
            break;
        }
    }
}

void AlignmentGroup::addToViewer(Gui::View3DInventorViewer* viewer) const
{
    std::vector<Gui::ViewProviderDocumentObject*>::const_iterator it;
    for (it = this->_views.begin(); it != this->_views.end(); ++it)
        viewer->addViewProvider(*it);

    viewer->viewAll();
}

void AlignmentGroup::removeFromViewer(Gui::View3DInventorViewer* viewer) const
{
    std::vector<Gui::ViewProviderDocumentObject*>::const_iterator it;
    for (it = this->_views.begin(); it != this->_views.end(); ++it)
        viewer->removeViewProvider(*it);
}

void AlignmentGroup::setRandomColor()
{
    std::vector<Gui::ViewProviderDocumentObject*>::iterator it;
    for (it = this->_views.begin(); it != this->_views.end(); ++it) {
        float r = /*(float)rand()/(float)RAND_MAX*/0.0f;
        float g = (float)rand()/(float)RAND_MAX;
        float b = (float)rand()/(float)RAND_MAX;
        if ((*it)->isDerivedFrom(Gui::ViewProviderGeometryObject::getClassTypeId())) {
            SoSearchAction searchAction;
            searchAction.setType(SoMaterial::getClassTypeId());
            searchAction.setInterest(SoSearchAction::FIRST);
            searchAction.apply((*it)->getRoot());
            SoPath* selectionPath = searchAction.getPath();

            if (selectionPath) {
                auto material = static_cast<SoMaterial*>(selectionPath->getTail());
                material->diffuseColor.setValue(r, g, b);
            }
        }
    }
}

Gui::Document* AlignmentGroup::getDocument() const
{
    if (this->_views.empty())
        return nullptr;
    App::DocumentObject* pView = this->_views[0]->getObject();
    if (pView) {
        App::Document* rDoc = pView->getDocument();
        Gui::Document* pDoc = Gui::Application::Instance->getDocument(rDoc);
        return pDoc;
    }

    return nullptr;
}

void AlignmentGroup::addPoint(const PickedPoint& pnt)
{
    this->_pickedPoints.push_back(pnt);
}

void AlignmentGroup::removeLastPoint()
{
    this->_pickedPoints.pop_back();
}

int AlignmentGroup::countPoints() const
{
    return this->_pickedPoints.size();
}

const std::vector<PickedPoint>& AlignmentGroup::getPoints() const
{
    return this->_pickedPoints;
}

void AlignmentGroup::clearPoints()
{
    this->_pickedPoints.clear();
}

void AlignmentGroup::setAlignable(bool align)
{
    std::vector<Gui::ViewProviderDocumentObject*>::iterator it;
    for (it = this->_views.begin(); it != this->_views.end(); ++it) {
        auto pAlignMode = dynamic_cast<App::PropertyBool*>((*it)->getPropertyByName("AlignMode"));
        if (pAlignMode) {
            pAlignMode->setValue(align);
        }
        // leaving alignment mode
        else if (!align){
            auto pColor = dynamic_cast<App::PropertyColor*>((*it)->getPropertyByName("ShapeColor"));
            if (pColor)
                pColor->touch(); // resets to color defined by property
        }
    }
}

void AlignmentGroup::moveTo(AlignmentGroup& that)
{
    std::vector<Gui::ViewProviderDocumentObject*>::iterator it;
    for (it = this->_views.begin(); it != this->_views.end(); ++it)
        that._views.push_back(*it);

    this->_views.clear();
}

void AlignmentGroup::clear()
{
    this->_views.clear();
    this->_pickedPoints.clear();
}

bool AlignmentGroup::isEmpty() const
{
    return this->_views.empty();
}

int AlignmentGroup::count() const
{
    return this->_views.size();
}

Base::BoundBox3d AlignmentGroup::getBoundingBox() const
{
    Base::BoundBox3d box;
    std::vector<Gui::ViewProviderDocumentObject*>::const_iterator it;
    for (it = this->_views.begin(); it != this->_views.end(); ++it) {
        if ((*it)->isDerivedFrom(Gui::ViewProviderGeometryObject::getClassTypeId())) {
            auto geo = static_cast<App::GeoFeature*>((*it)->getObject());
            const App::PropertyComplexGeoData* prop = geo->getPropertyOfGeometry();
            if (prop)
                box.Add(prop->getBoundingBox());
        }
    }
    return box;
}

// ------------------------------------------------------------------

MovableGroup::MovableGroup() = default;

MovableGroup::~MovableGroup() = default;

// ------------------------------------------------------------------

FixedGroup::FixedGroup() = default;

FixedGroup::~FixedGroup() = default;

// ------------------------------------------------------------------

MovableGroupModel::MovableGroupModel() = default;

MovableGroupModel::~MovableGroupModel() = default;

void MovableGroupModel::addGroup(const MovableGroup& grp)
{
    this->_groups.push_back(grp);
}

void MovableGroupModel::addGroups(const std::map<int, MovableGroup>& grps)
{
    for (const auto & grp : grps)
        this->_groups.push_back(grp.second);
}

void MovableGroupModel::removeActiveGroup()
{
    this->_groups.erase(this->_groups.begin());
}

MovableGroup& MovableGroupModel::activeGroup()
{
    // Make sure that the array is not empty
    if (this->_groups.empty())
        throw Base::RuntimeError("Empty group");
    return *(this->_groups.begin());
}

const MovableGroup& MovableGroupModel::activeGroup() const
{
    // Make sure that the array is not empty
    if (this->_groups.empty())
        throw Base::RuntimeError("Empty group");
    return this->_groups.front();
}

void MovableGroupModel::continueAlignment()
{
    if (!isEmpty())
        removeActiveGroup();
}

void MovableGroupModel::clear()
{
    this->_groups.clear();
}

bool MovableGroupModel::isEmpty() const
{
    return this->_groups.empty();
}

int MovableGroupModel::count() const
{
    return this->_groups.size();
}

const MovableGroup& MovableGroupModel::getGroup(int i) const
{
    if (i >= count())
        throw Base::IndexError("Index out of range");
    return this->_groups[i];
}

Base::BoundBox3d MovableGroupModel::getBoundingBox() const
{
    Base::BoundBox3d box;
    std::vector<MovableGroup>::const_iterator it;
    for (it = this->_groups.begin(); it != this->_groups.end(); ++it) {
        box.Add(it->getBoundingBox());
    }
    return box;
}

// ------------------------------------------------------------------

namespace Gui {
class AlignmentView : public Gui::AbstractSplitView
{
public:
    QLabel* myLabel;

    AlignmentView(Gui::Document* pcDocument, QWidget* parent, Qt::WindowFlags wflags=Qt::WindowFlags())
        : AbstractSplitView(pcDocument, parent, wflags)
    {
        //anti-aliasing settings
        bool smoothing = false;
        bool glformat = false;
        int samples = View3DInventorViewer::getNumSamples();
        QtGLFormat f;

        if (samples > 1) {
            glformat = true;
            f.setSamples(samples);
        }
        else if (samples > 0) {
            smoothing = true;
        }

        QSplitter* mainSplitter=nullptr;
        mainSplitter = new QSplitter(Qt::Horizontal, this);
        if (glformat) {
            _viewer.push_back(new View3DInventorViewer(f, mainSplitter));
            _viewer.push_back(new View3DInventorViewer(f, mainSplitter));
        }
        else {
            _viewer.push_back(new View3DInventorViewer(mainSplitter));
            _viewer.push_back(new View3DInventorViewer(mainSplitter));
        }
        setDocumentOfViewers(pcDocument);

        auto vbox = new QFrame(this);
        auto layout = new QVBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        vbox->setLayout(layout);

        myLabel = new QLabel(this);
        myLabel->setAutoFillBackground(true);
        QPalette pal = myLabel->palette();
        pal.setColor(QPalette::Window, Qt::darkGray);
        pal.setColor(QPalette::WindowText, Qt::white);
        myLabel->setPalette(pal);
        mainSplitter->setPalette(pal);
        myLabel->setAlignment(Qt::AlignCenter);
        myLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QFont font = myLabel->font();
        font.setPointSize(14);
        myLabel->setFont(font);
        layout->addWidget(myLabel);
        layout->addWidget(mainSplitter);

        vbox->show();
        setCentralWidget(vbox);

        // apply the user settings
        setupSettings();

        if (smoothing) {
            for (const auto & i : _viewer)
                i->getSoRenderManager()->getGLRenderAction()->setSmoothing(true);
        }

        static_cast<SoGroup*>(getViewer(0)->getSoRenderManager()->getSceneGraph())->
            addChild(setupHeadUpDisplay(tr("Movable object")));
        static_cast<SoGroup*>(getViewer(1)->getSoRenderManager()->getSceneGraph())->
            addChild(setupHeadUpDisplay(tr("Fixed object")));
    }
    ~AlignmentView() override = default;
    PyObject* getPyObject() override
    {
        Py_Return;
    }
    bool canClose() override
    {
        return false;
    }
    SoNode* setupHeadUpDisplay(const QString& text) const
    {
        auto hudRoot = new SoSeparator;
        hudRoot->ref();

        auto hudCam = new SoOrthographicCamera();
        hudCam->viewportMapping = SoCamera::LEAVE_ALONE;

        // Set the position in the window.
        // [0, 0] is in the center of the screen.
        //
        auto hudTrans = new SoTranslation;
        hudTrans->translation.setValue(-0.95f, -0.95f, 0.0f);

        QFont font = this->font();
        font.setPointSize(24);
        QFontMetrics fm(font);

        QColor front;
        front.setRgbF(0.8f, 0.8f, 0.8f);

        int w = QtTools::horizontalAdvance(fm, text);
        int h = fm.height();

        QImage image(w,h,QImage::Format_ARGB32_Premultiplied);
        image.fill(0x00000000);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(front);
        painter.setFont(font);
        painter.drawText(0,0,w,h,Qt::AlignLeft,text);
        painter.end();
        SoSFImage sfimage;
        Gui::BitmapFactory().convert(image, sfimage);
        auto hudImage = new SoImage();
        hudImage->image = sfimage;

        // Assemble the parts...
        //
        hudRoot->addChild(hudCam);
        hudRoot->addChild(hudTrans);
        hudRoot->addChild(hudImage);

        return hudRoot;
    }
};
}

class ManualAlignment::Private {
public:
    SoSeparator * picksepLeft;
    SoSeparator * picksepRight;
    SoNodeSensor* sensorCam1{nullptr};
    SoNodeSensor* sensorCam2{nullptr};
    SbRotation rot_cam1, rot_cam2;
    SbVec3f pos_cam1, pos_cam2;

    Private()
    {
        // left view
        picksepLeft = new SoSeparator;
        picksepLeft->ref();
        // right view
        picksepRight = new SoSeparator;
        picksepRight->ref();
    }
    ~Private()
    {
        picksepLeft->unref();
        picksepRight->unref();
        delete sensorCam1;
        delete sensorCam2;
    }

    static
    void  reorientCamera(SoCamera * cam, const SbRotation & rot)
    {
        if (!cam)
            return;

        // Find global coordinates of focal point.
        SbVec3f direction;
        cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
        SbVec3f focalpoint = cam->position.getValue() +
                             cam->focalDistance.getValue() * direction;

        // Set new orientation value by accumulating the new rotation.
        cam->orientation = rot * cam->orientation.getValue();

        // Reposition camera so we are still pointing at the same old focal point.
        cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
        cam->position = focalpoint - cam->focalDistance.getValue() * direction;
    }

    static
    void copyCameraSettings(SoCamera* cam1, SbRotation& rot_cam1, SbVec3f& pos_cam1,
                            SoCamera* cam2, SbRotation& rot_cam2, SbVec3f& pos_cam2)
    {
        Q_UNUSED(pos_cam2);

        // recompute the diff we have applied to the camera's orientation
        SbRotation rot = cam1->orientation.getValue();
        SbRotation dif = rot * rot_cam1.inverse();
        rot_cam1 = rot;

        // copy the values
        cam2->enableNotify(false);
        cam2->nearDistance = cam1->nearDistance;
        cam2->farDistance = cam1->farDistance;
        cam2->focalDistance = cam1->focalDistance;
        reorientCamera(cam2,dif);
        rot_cam2 = cam2->orientation.getValue();

        // reverse engineer the translation part in wc
        SbVec3f pos = cam1->position.getValue();
        SbVec3f difpos = pos - pos_cam1;
        pos_cam1 = pos;
        // the translation in pixel coords
        cam1->orientation.getValue().inverse().multVec(difpos,difpos);
        // the translation again in wc for the second camera
        cam2->orientation.getValue().multVec(difpos,difpos);
        cam2->position.setValue(cam2->position.getValue()+difpos);

        if (cam1->getTypeId() == cam2->getTypeId()) {
            if (cam1->getTypeId() == SoOrthographicCamera::getClassTypeId())
                static_cast<SoOrthographicCamera*>(cam2)->height =
                static_cast<SoOrthographicCamera*>(cam1)->height;
        }

        cam2->enableNotify(true);
    }
    static
    void syncCameraCB(void * data, SoSensor * s)
    {
        auto self = static_cast<ManualAlignment*>(data);
        if (!self->myViewer)
            return; // already destroyed
        SoCamera* cam1 = self->myViewer->getViewer(0)->getSoRenderManager()->getCamera();
        SoCamera* cam2 = self->myViewer->getViewer(1)->getSoRenderManager()->getCamera();
        if (!cam1 || !cam2)
            return; // missing camera
        auto sensor = static_cast<SoNodeSensor*>(s);
        SoNode* node = sensor->getAttachedNode();
        if (node && node->getTypeId().isDerivedFrom(SoCamera::getClassTypeId())) {
            if (node == cam1) {
                Private::copyCameraSettings(cam1, self->d->rot_cam1, self->d->pos_cam1,
                                   cam2, self->d->rot_cam2, self->d->pos_cam2);
                self->myViewer->getViewer(1)->redraw();
            }
            else if (node == cam2) {
                Private::copyCameraSettings(cam2, self->d->rot_cam2, self->d->pos_cam2,
                                   cam1, self->d->rot_cam1, self->d->pos_cam1);
                self->myViewer->getViewer(0)->redraw();
            }
        }
    }

    static Base::Placement
    transformation2x2(const Base::Vector3d& plane1_base,
                      const Base::Vector3d& plane1_xaxis,
                      const Base::Vector3d& plane2_base,
                      const Base::Vector3d& plane2_xaxis)
    {
        // the transformation is:
        // * move from plane1_base to plane2_base
        // * rotate from plane1_zaxis to plane2_zaxis around plane2_base as center point
        Base::Rotation rot(plane1_xaxis, plane2_xaxis);

        Base::Vector3d pln_base;
        rot.multVec(plane1_base,pln_base);
        Base::Vector3d dif = plane2_base - pln_base;
        return {dif, rot};
    }

    static Base::Placement
    transformation3x3(const Base::Vector3d& plane1_base,
                      const Base::Vector3d& plane1_zaxis,
                      const Base::Vector3d& plane1_xaxis,
                      const Base::Vector3d& plane2_base,
                      const Base::Vector3d& plane2_zaxis,
                      const Base::Vector3d& plane2_xaxis)
    {
        // the transformation is:
        // * move from plane1_base to plane2_base
        // * rotate from plane1_zaxis to plane2_zaxis around plane2_base as center point
        Base::Rotation rot(plane1_zaxis, plane2_zaxis);

        // first transformation to align the plane normals and base points
        Base::Vector3d dif1 = plane1_base;
        rot.multVec(dif1,dif1);
        dif1 = plane2_base - dif1;
        Base::Placement plm1(dif1, rot);

        // second transformation to align the planes' x axes
        Base::Vector3d pln_xaxis;
        rot.multVec(plane1_xaxis,pln_xaxis);
        Base::Rotation rot2(pln_xaxis, plane2_xaxis);
        Base::Vector3d dif2 = plane2_base;
        rot2.multVec(dif2,dif2);
        dif2 = plane2_base - dif2;
        Base::Placement plm2(dif2, rot2);
        plm2 = plm2 * plm1;
        return plm2;
    }
};

/* TRANSLATOR Gui::ManualAlignment */

ManualAlignment* ManualAlignment::_instance = nullptr;

/**
 * Construction.
 */
ManualAlignment::ManualAlignment()
  : myViewer(nullptr), myDocument(nullptr), myPickPoints(3), d(new Private)
{
    //NOLINTBEGIN
    // connect with the application's signal for deletion of documents
    this->connectApplicationDeletedDocument = Gui::Application::Instance->signalDeleteDocument
        .connect(std::bind(&ManualAlignment::slotDeletedDocument, this, sp::_1));
    //NOLINTEND

    // setup sensor connection
    d->sensorCam1 = new SoNodeSensor(Private::syncCameraCB, this);
    d->sensorCam2 = new SoNodeSensor(Private::syncCameraCB, this);
}

/**
 * Destruction.
 */
ManualAlignment::~ManualAlignment()
{
    this->connectDocumentDeletedObject.disconnect();
    this->connectApplicationDeletedDocument.disconnect();
    closeViewer();
    delete d;
    _instance = nullptr;
}

/**
 * Creates the one and only instance of this class.
 */
ManualAlignment* ManualAlignment::instance()
{
    // not initialized?
    if (!_instance)
        _instance = new ManualAlignment();
    return _instance;
}

/**
 * Destructs the one and only instance of this class.
 */
void ManualAlignment::destruct()
{
    if (_instance) {
        ManualAlignment* tmp = _instance;
        _instance = nullptr;
        delete tmp;
    }
}

/**
 * Checks whether the one instance exists.
 */
bool ManualAlignment::hasInstance()
{
    return _instance != nullptr;
}

void ManualAlignment::setMinPoints(int minPoints)
{
    if ((minPoints > 0) && (minPoints <= 3))
        myPickPoints = minPoints;
}

void ManualAlignment::setFixedGroup(const FixedGroup& fixed)
{
    this->myFixedGroup = fixed;
    this->myDocument = fixed.getDocument();
}

void ManualAlignment::setModel(const MovableGroupModel& model)
{
    this->myAlignModel = model;
}

void ManualAlignment::clearAll()
{
    myFixedGroup.clear();
    myAlignModel.clear();
    myDocument = nullptr;
}

void ManualAlignment::setViewingDirections(const Base::Vector3d& view1, const Base::Vector3d& up1,
                                           const Base::Vector3d& view2, const Base::Vector3d& up2)
{
    if (myViewer.isNull())
        return;

    {
        SbVec3f vz(-view1.x, -view1.y, -view1.z);
        vz.normalize();
        SbVec3f vy(up1.x, up1.y, up1.z);
        vy.normalize();
        SbVec3f vx = vy.cross(vz);
        vy = vz.cross(vx);

        SbMatrix rot = SbMatrix::identity();
        rot[0][0] = vx[0];
        rot[0][1] = vx[1];
        rot[0][2] = vx[2];

        rot[1][0] = vy[0];
        rot[1][1] = vy[1];
        rot[1][2] = vy[2];

        rot[2][0] = vz[0];
        rot[2][1] = vz[1];
        rot[2][2] = vz[2];

        SbRotation total(rot);
        myViewer->getViewer(0)->getSoRenderManager()->getCamera()->orientation.setValue(total);
        myViewer->getViewer(0)->viewAll();
    }

    {
        SbVec3f vz(-view2.x, -view2.y, -view2.z);
        vz.normalize();
        SbVec3f vy(up2.x, up2.y, up2.z);
        vy.normalize();
        SbVec3f vx = vy.cross(vz);
        vy = vz.cross(vx);

        SbMatrix rot = SbMatrix::identity();
        rot[0][0] = vx[0];
        rot[0][1] = vx[1];
        rot[0][2] = vx[2];

        rot[1][0] = vy[0];
        rot[1][1] = vy[1];
        rot[1][2] = vy[2];

        rot[2][0] = vz[0];
        rot[2][1] = vz[1];
        rot[2][2] = vz[2];

        SbRotation total(rot);
        myViewer->getViewer(1)->getSoRenderManager()->getCamera()->orientation.setValue(total);
        myViewer->getViewer(1)->viewAll();
    }
}

/**
 * Performs the alignment for the specified aligned and non-aligned views specified by setModel() and setFixedGroup().
 */
void ManualAlignment::startAlignment(Base::Type mousemodel)
{
    // allow only one alignment at a time
    if (!myViewer.isNull()) {
        QMessageBox::warning(qApp->activeWindow(), tr("Manual alignment"), tr("The alignment is already in progress."));
        return;
    }

    myTransform = Base::Placement();

    if (myFixedGroup.isEmpty())
        return;
    if (myAlignModel.isEmpty())
        return;

    // create a split window for picking the points
    myViewer = new AlignmentView(myDocument,Gui::getMainWindow());
    myViewer->setWindowTitle(tr("Alignment[*]"));
    myViewer->setWindowIcon(QApplication::windowIcon());
    myViewer->resize(400, 300);
    Gui::getMainWindow()->addWindow(myViewer);
    myViewer->showMaximized();
    int n = this->myPickPoints;
    QString msg = n == 1
        ? tr("Please, select at least one point in the left and the right view")
        : tr("Please, select at least %1 points in the left and the right view").arg(n);
    myViewer->myLabel->setText(msg);

    connect(myViewer, &QObject::destroyed, this, &ManualAlignment::reset);

    // show all aligned views in the 2nd view
    myFixedGroup.addToViewer(myViewer->getViewer(1));
    myFixedGroup.setAlignable(true);

    // set picked points root
    SoNode* node1 = myViewer->getViewer(0)->getSceneGraph();
    if (node1->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())){
        ((SoGroup*)node1)->addChild(d->picksepLeft);
    }
    SoNode* node2 = myViewer->getViewer(1)->getSceneGraph();
    if (node2->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())){
        ((SoGroup*)node2)->addChild(d->picksepRight);
    }

    myViewer->getViewer(0)->setEditing(true);
    myViewer->getViewer(0)->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
        ManualAlignment::probePickedCallback);
    myViewer->getViewer(1)->setEditing(true);
    myViewer->getViewer(1)->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
        ManualAlignment::probePickedCallback);
    // apply the mouse model
    myViewer->getViewer(0)->setNavigationType(mousemodel);
    myViewer->getViewer(1)->setNavigationType(mousemodel);

    // Connect to the document's signal as we want to be notified when something happens
    if (this->connectDocumentDeletedObject.connected())
        this->connectDocumentDeletedObject.disconnect();
    //NOLINTBEGIN
    this->connectDocumentDeletedObject = myDocument->signalDeletedObject.connect(std::bind
        (&ManualAlignment::slotDeletedObject, this, sp::_1));
    //NOLINTEND

    continueAlignment();
}

/**
 * If still one view needs to be aligned then it is shown in the first window. If all views are aligned the process will be terminated.
 */
void ManualAlignment::continueAlignment()
{
    myFixedGroup.clearPoints();
    coinRemoveAllChildren(d->picksepLeft);
    coinRemoveAllChildren(d->picksepRight);

    if (!myAlignModel.isEmpty()) {
        AlignmentGroup& grp = myAlignModel.activeGroup();
        grp.clearPoints();
        grp.addToViewer(myViewer->getViewer(0));
        grp.setAlignable(true);

        Gui::getMainWindow()->showMessage(tr("Please pick points in the left and right view"));

        myViewer->getViewer(0)->setEditingCursor(QCursor(Qt::PointingHandCursor));
        myViewer->getViewer(1)->setEditingCursor(QCursor(Qt::PointingHandCursor));
    }
    else {
        finish();
    }
}

void ManualAlignment::closeViewer()
{
    if (!myViewer)
        return;
    // Close the viewer
    if (myViewer->parentWidget())
        myViewer->parentWidget()->deleteLater();
    myViewer = nullptr;
}

/**
 * Make all views unpickable and resets internal data.
 */
void ManualAlignment::reset()
{
    if (!myAlignModel.isEmpty()) {
        myAlignModel.activeGroup().setAlignable(false);
        myAlignModel.activeGroup().clear();
        myAlignModel.clear();
    }

    myFixedGroup.setAlignable(false);
    myFixedGroup.clear();

    coinRemoveAllChildren(d->picksepLeft);
    coinRemoveAllChildren(d->picksepRight);

    if (myDocument) {
        this->connectDocumentDeletedObject.disconnect();
        myDocument = nullptr;
    }
}

/**
 * Terminates the process and closes the windows.
 */
void ManualAlignment::finish()
{
    if (myViewer.isNull())
        return;

    if (myDocument)
        myDocument->getDocument()->recompute();
    closeViewer();
    reset();

    Gui::getMainWindow()->showMessage(tr("The alignment has finished"));

    // If an event receiver has been defined send the manual alignment finished event to it
    Q_EMIT emitFinished();
}

/**
 * Cancels the process and closes the windows without performing an alignment.
 */
void ManualAlignment::cancel()
{
    if (myViewer.isNull())
        return;

    closeViewer();
    myTransform = Base::Placement();
    reset();

    Gui::getMainWindow()->showMessage(tr("The alignment has been canceled"));

    // If an event receiver has been defined send the manual alignment cancelled event to it
    Q_EMIT emitCanceled();
}

void ManualAlignment::align()
{
    // Now we can start the actual alignment
    if (myAlignModel.activeGroup().countPoints() < myPickPoints) {
        QMessageBox::warning(myViewer, tr("Manual alignment"),
                tr("Too few points picked in the left view."
                   " At least %1 points are needed.").arg(myPickPoints));
    }
    else if (myFixedGroup.countPoints() < myPickPoints) {
        QMessageBox::warning(myViewer, tr("Manual alignment"),
                tr("Too few points picked in the right view."
                  " At least %1 points are needed.").arg(myPickPoints));
    }
    else if (myAlignModel.activeGroup().countPoints() != myFixedGroup.countPoints()) {
        QMessageBox::warning(myViewer, tr("Manual alignment"),
                tr("Different number of points picked in left and right view.\n"
                   "On the left view %1 points are picked,\n"
                   "on the right view %2 points are picked.")
                .arg(myAlignModel.activeGroup().countPoints())
                            .arg(myFixedGroup.countPoints()));
    }
    else {
        // do not allow to pick further points
        myAlignModel.activeGroup().removeFromViewer(myViewer->getViewer(0));
        myAlignModel.activeGroup().setAlignable(false);
        std::vector<App::DocumentObject*> pViews = myAlignModel.activeGroup().getViews();
        Gui::getMainWindow()->showMessage(tr("Try to align group of views"));

        // Compute alignment
        bool ok = computeAlignment(myAlignModel.activeGroup().getPoints(), myFixedGroup.getPoints());
        if (ok && myDocument) {
            // Align views
            myDocument->openCommand(QT_TRANSLATE_NOOP("Command", "Align"));
            for (const auto & pView : pViews)
                alignObject(pView);
            myDocument->commitCommand();

            // the alignment was successful so show it in the right view now
            //myAlignModel.activeGroup().setRandomColor();
            myAlignModel.activeGroup().setAlignable(true);
            myAlignModel.activeGroup().addToViewer(myViewer->getViewer(1));
            myAlignModel.activeGroup().moveTo(myFixedGroup);
            myAlignModel.continueAlignment();
        }
        else {
            // Inform user that alignment failed
            auto ret = QMessageBox::critical(myViewer, tr("Manual alignment"),
                tr("The alignment failed.\nHow do you want to proceed?"),
                QMessageBox::Retry | QMessageBox::Ignore | QMessageBox::Abort);
            if ( ret == QMessageBox::Ignore ) {
                myAlignModel.continueAlignment();
            }
            else if ( ret == QMessageBox::Abort ) {
                finish();
                return;
            }
        }

        continueAlignment();
    }
}

void ManualAlignment::showInstructions()
{
    // Now we can start the actual alignment
    if (myAlignModel.activeGroup().countPoints() < myPickPoints) {
        Gui::getMainWindow()->showMessage(
            tr("Too few points picked in the left view."
               " At least %1 points are needed.").arg(myPickPoints));
    }
    else if (myFixedGroup.countPoints() < myPickPoints) {
        Gui::getMainWindow()->showMessage(
            tr("Too few points picked in the right view."
               " At least %1 points are needed.").arg(myPickPoints));
    }
    else if (myAlignModel.activeGroup().countPoints() != myFixedGroup.countPoints()) {
        Gui::getMainWindow()->showMessage(
            tr("Different number of points picked in left and right view. "
               "On the left view %1 points are picked, "
               "on the right view %2 points are picked.")
            .arg(myAlignModel.activeGroup().countPoints())
            .arg(myFixedGroup.countPoints()));
    }
}

bool ManualAlignment::canAlign() const
{
    if (myAlignModel.activeGroup().countPoints() == myFixedGroup.countPoints()) {
        if (myFixedGroup.countPoints() >= myPickPoints)
            return true;
    }

    return false;
}

/**
 * This method computes the alignment. For the calculation of the alignment the picked points of both views
 * are taken. If the alignment fails false is returned, true otherwise.
 */
bool ManualAlignment::computeAlignment(const std::vector<PickedPoint>& movPts,
                                       const std::vector<PickedPoint>& fixPts)
{
    assert((int)movPts.size() >= myPickPoints);
    assert((int)fixPts.size() >= myPickPoints);
    assert((int)movPts.size() == (int)fixPts.size());
    myTransform = Base::Placement();

    if (movPts.size() == 1) {
        // 1 point partial solution: Simple translation only
        myTransform.setPosition(fixPts[0].point - movPts[0].point);
    }
    else if (movPts.size() == 2) {
        const Base::Vector3d& p1 = movPts[0].point;
        const Base::Vector3d& p2 = movPts[1].point;
        Base::Vector3d d1 = p2-p1;
        d1.Normalize();

        const Base::Vector3d& q1 = fixPts[0].point;
        const Base::Vector3d& q2 = fixPts[1].point;
        Base::Vector3d d2 = q2-q1;
        d2.Normalize();

        myTransform = Private::transformation2x2(p1, d1, q1, d2);
    }
    else if (movPts.size() >= 3) {
        const Base::Vector3d& p1 = movPts[0].point;
        const Base::Vector3d& p2 = movPts[1].point;
        const Base::Vector3d& p3 = movPts[2].point;
        Base::Vector3d d1 = p2-p1;
        d1.Normalize();
        Base::Vector3d n1 = (p2-p1) % (p3-p1);
        n1.Normalize();

        const Base::Vector3d& q1 = fixPts[0].point;
        const Base::Vector3d& q2 = fixPts[1].point;
        const Base::Vector3d& q3 = fixPts[2].point;
        Base::Vector3d d2 = q2-q1;
        d2.Normalize();
        Base::Vector3d n2 = (q2-q1) % (q3-q1);
        n2.Normalize();

        myTransform = Private::transformation3x3(p1, d1, n1, q1, d2, n2);
    }

    return true;
}

/**
 * This method performs the actual alignment of view \a pView.
 */
void ManualAlignment::alignObject(App::DocumentObject *obj)
{
    if (obj->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())) {
        auto geom = static_cast<App::GeoFeature*>(obj);
        geom->transformPlacement(this->myTransform);
    }
}

/**
 * Creates a point element as visible feedback for the user.
 */
SoNode* ManualAlignment::pickedPointsSubGraph(const SbVec3f& p, const SbVec3f& n, int id)
{
    static const float color_table [10][3] = {
        {1.0f,0.0f,0.0f}, // red
        {0.0f,1.0f,0.0f}, // green
        {0.0f,0.0f,1.0f}, // blue
        {1.0f,1.0f,0.0f}, // yellow
        {0.0f,1.0f,1.0f}, // cyan
        {0.7f,0.0f,0.0f},
        {0.0f,0.7f,0.0f},
        {0.7f,0.7f,0.0f},
        {0.7f,0.0f,0.5f},
        {1.0f,0.7f,0.0f}
    };

    int index = (id-1) % 10;

    auto probe = new SoRegPoint();
    probe->base.setValue(p);
    probe->normal.setValue(n);
    probe->color.setValue(color_table[index][0],color_table[index][1],color_table[index][2]);
    SbString s(tr("Point_%1").arg(id).toStdString().c_str());
    probe->text.setValue(s);
    return probe;
}

/**
 * Handle if the current document is about to being closed.
 */
void ManualAlignment::slotDeletedDocument(const Gui::Document& Doc)
{
    if (&Doc == this->myDocument)
        reset();
}

/**
 * Handle if the a view provider is about to being destroyed.
 */
void ManualAlignment::slotDeletedObject(const Gui::ViewProvider& Obj)
{
    // remove the view provider either from the left or the right view
    if (Obj.getTypeId().isDerivedFrom(Gui::ViewProviderDocumentObject::getClassTypeId())) {
        // remove the view provider immediately from the split window
        bool found = false;
        auto vp = const_cast<Gui::ViewProviderDocumentObject*>
                                      (static_cast<const Gui::ViewProviderDocumentObject*>(&Obj));
        if (myAlignModel.activeGroup().hasView(vp)) {
            myViewer->getViewer(0)->removeViewProvider(vp);
            found = true;
        }
        if (myFixedGroup.hasView(vp)) {
            myViewer->getViewer(1)->removeViewProvider(vp);
            found = true;
        }

        if (found)
            cancel();
    }
}

void ManualAlignment::onAlign()
{
    align();
}

void ManualAlignment::onRemoveLastPointMoveable()
{
    int nPoints = myAlignModel.activeGroup().countPoints();
    if (nPoints > 0) {
        myAlignModel.activeGroup().removeLastPoint();
        d->picksepLeft->removeChild(nPoints-1);
    }
}

void ManualAlignment::onRemoveLastPointFixed()
{
    int nPoints = myFixedGroup.countPoints();
    if (nPoints > 0) {
        myFixedGroup.removeLastPoint();
        d->picksepRight->removeChild(nPoints-1);
    }
}

void ManualAlignment::onClear()
{
    myAlignModel.activeGroup().clear();
    myFixedGroup.clear();

    coinRemoveAllChildren(d->picksepLeft);
    coinRemoveAllChildren(d->picksepRight);
}

void ManualAlignment::onCancel()
{
    cancel();
}

void ManualAlignment::probePickedCallback(void * ud, SoEventCallback * n)
{
    Q_UNUSED(ud);

    auto view  = static_cast<Gui::View3DInventorViewer*>(n->getUserData());
    const SoEvent* ev = n->getEvent();
    if (ev->getTypeId() == SoMouseButtonEvent::getClassTypeId()) {
        // set as handled
        n->getAction()->setHandled();
        n->setHandled();

        auto mbe = static_cast<const SoMouseButtonEvent *>(ev);
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN) {
            // if we are in 'align' mode then handle the click event
            ManualAlignment* self = ManualAlignment::instance();
            // Get the closest point to the camera of the whole scene.
            // This point doesn't need to be part of this view provider.
            Gui::WaitCursor wc;
            const SoPickedPoint * point = view->getPickedPoint(n);
            if (point) {
                auto vp = static_cast<Gui::ViewProvider*>(view->getViewProviderByPath(point->getPath()));
                if (vp && vp->getTypeId().isDerivedFrom(Gui::ViewProviderDocumentObject::getClassTypeId())) {
                    auto that = static_cast<Gui::ViewProviderDocumentObject*>(vp);
                    if (self->applyPickedProbe(that, point)) {
                        const SbVec3f& vec = point->getPoint();
                        Gui::getMainWindow()->showMessage(
                            tr("Point picked at (%1,%2,%3)")
                            .arg(vec[0]).arg(vec[1]).arg(vec[2]));
                    }
                    else {
                        Gui::getMainWindow()->showMessage(
                            tr("No point was found on model"));
                    }
                }
            }
            else {
                Gui::getMainWindow()->showMessage(
                    tr("No point was picked"));
            }
        }
        else if (mbe->getButton() == SoMouseButtonEvent::BUTTON2 && mbe->getState() == SoButtonEvent::UP) {
            ManualAlignment* self = ManualAlignment::instance();
            if (self->myAlignModel.isEmpty() || self->myFixedGroup.isEmpty())
                return;
            self->showInstructions();
            int nPoints;
            if (view == self->myViewer->getViewer(0))
                nPoints = self->myAlignModel.activeGroup().countPoints();
            else
                nPoints = self->myFixedGroup.countPoints();
            QMenu menu;
            QAction* fi = menu.addAction(tr("&Align"));
            QAction* rem = menu.addAction(tr("&Remove last point"));
            //QAction* cl = menu.addAction("C&lear");
            QAction* ca = menu.addAction(tr("&Cancel"));
            fi->setEnabled(self->canAlign());
            rem->setEnabled(nPoints > 0);
            menu.addSeparator();
            QAction* sync = menu.addAction(tr("&Synchronize views"));
            sync->setCheckable(true);
            if (self->d->sensorCam1->getAttachedNode())
                sync->setChecked(true);
            QAction* id = menu.exec(QCursor::pos());
            if (id == fi) {
                // call align->align();
                QTimer::singleShot(300, self, &ManualAlignment::onAlign);
            }
            else if ((id == rem) && (view == self->myViewer->getViewer(0))) {
                QTimer::singleShot(300, self, &ManualAlignment::onRemoveLastPointMoveable);
            }
            else if ((id == rem) && (view == self->myViewer->getViewer(1))) {
                QTimer::singleShot(300, self, &ManualAlignment::onRemoveLastPointFixed);
            }
            //else if (id == cl) {
            //    // call align->clear();
            //    QTimer::singleShot(300, self, &ManualAlignment::onClear);
            //}
            else if (id == ca) {
                // call align->cancel();
                QTimer::singleShot(300, self, &ManualAlignment::onCancel);
            }
            else if (id == sync) {
                // setup sensor connection
                if (sync->isChecked()) {
                    SoCamera* cam1 = self->myViewer->getViewer(0)->getSoRenderManager()->getCamera();
                    SoCamera* cam2 = self->myViewer->getViewer(1)->getSoRenderManager()->getCamera();
                    if (cam1 && cam2) {
                        self->d->sensorCam1->attach(cam1);
                        self->d->rot_cam1 = cam1->orientation.getValue();
                        self->d->pos_cam1 = cam1->position.getValue();
                        self->d->sensorCam2->attach(cam2);
                        self->d->rot_cam2 = cam2->orientation.getValue();
                        self->d->pos_cam2 = cam2->position.getValue();
                    }
                }
                else {
                    self->d->sensorCam1->detach();
                    self->d->sensorCam2->detach();
                }
            }
        }
    }
}

/**
 * This method stores the picked point \a pnt from the view provider \a prov. If enough points in both windows have been picked
 * the alignment gets invoked.
 */
bool ManualAlignment::applyPickedProbe(Gui::ViewProviderDocumentObject* prov, const SoPickedPoint* pnt)
{
    const SbVec3f& vec = pnt->getPoint();
    const SbVec3f& nor = pnt->getNormal();

    // add to the list for the non-aligned view in the left view
    if (myAlignModel.activeGroup().hasView(prov)) {
        std::vector<Base::Vector3d> pts = prov->getModelPoints(pnt);
        if (pts.empty())
            return false;
        PickedPoint pp;
        pp.point = pts.front();
        myAlignModel.activeGroup().addPoint(pp);
        // Adds a point marker for the picked point.
        d->picksepLeft->addChild(pickedPointsSubGraph(vec, nor, myAlignModel.activeGroup().countPoints()));
        return true;
    }
    else if (myFixedGroup.hasView(prov)) {
        std::vector<Base::Vector3d> pts = prov->getModelPoints(pnt);
        if (pts.empty())
            return false;
        PickedPoint pp;
        pp.point = pts.front();
        myFixedGroup.addPoint(pp);
        // Adds a point marker for the picked point.
        d->picksepRight->addChild(pickedPointsSubGraph(vec, nor, myFixedGroup.countPoints()));
        return true;
    }

    return false;
}

#include "moc_ManualAlignment.cpp"

