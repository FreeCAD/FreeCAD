/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRep_Tool.hxx>
# include <BRepGProp.hxx>
# include <GProp_GProps.hxx>
# include <gp_Pnt.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <QFontMetrics>
# include <QMessageBox>
# include <QSet>
# include <Python.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoRayPickAction.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoSeparator.h>
# include <boost/signals2.hpp>
# include <boost/bind.hpp>
#endif

#include "ui_TaskFaceColors.h"
#include "TaskFaceColors.h"
#include "ViewProviderExt.h"
#include "SoBrepFaceSet.h"

#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Mod/Part/App/PartFeature.h>


using namespace PartGui;

namespace PartGui {
    class FaceSelection : public Gui::SelectionFilterGate
    {
        const App::DocumentObject* object;
    public:
        FaceSelection(const App::DocumentObject* obj)
            : Gui::SelectionFilterGate(), object(obj)
        {
        }
        bool allow(App::Document* /*pDoc*/, App::DocumentObject*pObj, const char*sSubName)
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            std::string element(sSubName);
            return element.substr(0,4) == "Face";
        }
    };
}

class FaceColors::Private
{
public:
    typedef boost::signals2::connection Connection;
    Ui_TaskFaceColors* ui;
    Gui::View3DInventorViewer* view;
    ViewProviderPartExt* vp;
    App::DocumentObject* obj;
    Gui::Document* doc;
    std::vector<App::Color> current,perface;
    QSet<int> index;
    bool boxSelection;
    Connection connectDelDoc;
    Connection connectDelObj;

    Private(ViewProviderPartExt* vp) : ui(new Ui_TaskFaceColors()), view(0), vp(vp)
    {
        obj = vp->getObject();
        doc = Gui::Application::Instance->getDocument(obj->getDocument());

        // build up map edge->face
        TopTools_IndexedMapOfShape mapOfShape;
        TopExp_Explorer xp(static_cast<Part::Feature*>(obj)->Shape.getValue(), TopAbs_FACE);
        while (xp.More()) {
            mapOfShape.Add(xp.Current());
            xp.Next();
        }

        current = vp->DiffuseColor.getValues();
        if (current.empty())
            current.push_back(vp->ShapeColor.getValue());
        perface = current;
        perface.resize(mapOfShape.Extent(), perface.front());

        boxSelection = false;
    }
    ~Private()
    {
        delete ui;
    }
    bool isVisibleFace(int faceIndex, const SbVec2f& pos, Gui::View3DInventorViewer* viewer)
    {
        SoSeparator* root = new SoSeparator;
        root->ref();
        root->addChild(viewer->getSoRenderManager()->getCamera());
        root->addChild(vp->getRoot());

        SoSearchAction searchAction;
        searchAction.setType(PartGui::SoBrepFaceSet::getClassTypeId());
        searchAction.setInterest(SoSearchAction::FIRST);
        searchAction.apply(root);
        SoPath* selectionPath = searchAction.getPath();

        SoRayPickAction rp(viewer->getSoRenderManager()->getViewportRegion());
        rp.setNormalizedPoint(pos);
        rp.apply(selectionPath);
        root->unref();

        SoPickedPoint* pick = rp.getPickedPoint();
        if (pick) {
            const SoDetail* detail = pick->getDetail();
            if (detail && detail->isOfType(SoFaceDetail::getClassTypeId())) {
                int index = static_cast<const SoFaceDetail*>(detail)->getPartIndex();
                if (faceIndex != index)
                    return false;
                SbVec3f dir = viewer->getViewDirection();
                const SbVec3f& nor = pick->getNormal();
                if (dir.dot(nor) > 0)
                    return false; // bottom side points to user
                return true;
            }
        }

        return false;
    }
    void addFacesToSelection(Gui::View3DInventorViewer* /*viewer*/,
                             const Gui::ViewVolumeProjection& proj,
                             const Base::Polygon2d& polygon,
                             const TopoDS_Shape& shape)
    {
        try {
            TopTools_IndexedMapOfShape M;

            TopExp_Explorer xp_face(shape,TopAbs_FACE);
            while (xp_face.More()) {
                M.Add(xp_face.Current());
                xp_face.Next();
            }

            App::Document* appdoc = doc->getDocument();
            for (Standard_Integer k = 1; k <= M.Extent(); k++) {
                const TopoDS_Shape& face = M(k);

                TopExp_Explorer xp_vertex(face,TopAbs_VERTEX);
                while (xp_vertex.More()) {
                    gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(xp_vertex.Current()));
                    Base::Vector3d pt2d;
                    pt2d = proj(Base::Vector3d(p.X(), p.Y(), p.Z()));
                    if (polygon.Contains(Base::Vector2d(pt2d.x, pt2d.y))) {
#if 0
                        // TODO
                        if (isVisibleFace(k-1, SbVec2f(pt2d.x, pt2d.y), viewer))
#endif
                        {
                            std::stringstream str;
                            str << "Face" << k;
                            Gui::Selection().addSelection(appdoc->getName(), obj->getNameInDocument(), str.str().c_str());
                            break;
                        }
                    }
                    xp_vertex.Next();
                }

                //GProp_GProps props;
                //BRepGProp::SurfaceProperties(face, props);
                //gp_Pnt c = props.CentreOfMass();
                //Base::Vector3d pt2d;
                //pt2d = proj(Base::Vector3d(c.X(), c.Y(), c.Z()));
                //if (polygon.Contains(Base::Vector2d(pt2d.x, pt2d.y))) {
                //    if (isVisibleFace(k-1, SbVec2f(pt2d.x, pt2d.y), viewer)) {
                //        std::stringstream str;
                //        str << "Face" << k;
                //        Gui::Selection().addSelection(appdoc->getName(), obj->getNameInDocument(), str.str().c_str());
                //    }
                //}
            }
        }
        catch (...) {
        }
    }
    static void selectionCallback(void * ud, SoEventCallback * cb)
    {
        Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(cb->getUserData());
        view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), selectionCallback, ud);
        SoNode* root = view->getSceneGraph();
        static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(true);

        std::vector<SbVec2f> picked = view->getGLPolygon();
        SoCamera* cam = view->getSoRenderManager()->getCamera();
        SbViewVolume vv = cam->getViewVolume();
        Gui::ViewVolumeProjection proj(vv);
        Base::Polygon2d polygon;
        if (picked.size() == 2) {
            SbVec2f pt1 = picked[0];
            SbVec2f pt2 = picked[1];
            polygon.Add(Base::Vector2d(pt1[0], pt1[1]));
            polygon.Add(Base::Vector2d(pt1[0], pt2[1]));
            polygon.Add(Base::Vector2d(pt2[0], pt2[1]));
            polygon.Add(Base::Vector2d(pt2[0], pt1[1]));
        }
        else {
            for (std::vector<SbVec2f>::const_iterator it = picked.begin(); it != picked.end(); ++it)
                polygon.Add(Base::Vector2d((*it)[0],(*it)[1]));
        }

        FaceColors* self = reinterpret_cast<FaceColors*>(ud);
        self->d->view = 0;
        if (self->d->obj && self->d->obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            cb->setHandled();
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(self->d->obj)->Shape.getValue();
            self->d->boxSelection = true;
            self->d->addFacesToSelection(view, proj, polygon, shape);
            self->d->boxSelection = false;
            self->updatePanel();
            view->redraw();
        }
    }
};

/* TRANSLATOR PartGui::TaskFaceColors */

FaceColors::FaceColors(ViewProviderPartExt* vp, QWidget* parent)
  : d(new Private(vp))
{
    Q_UNUSED(parent);
    d->ui->setupUi(this);
    d->ui->groupBox->setTitle(QString::fromUtf8(vp->getObject()->Label.getValue()));
    d->ui->colorButton->setDisabled(true);

    FaceSelection* gate = new FaceSelection(d->vp->getObject());
    Gui::Selection().addSelectionGate(gate);

    d->connectDelDoc = Gui::Application::Instance->signalDeleteDocument.connect(boost::bind
        (&FaceColors::slotDeleteDocument, this, _1));
    d->connectDelObj = Gui::Application::Instance->signalDeletedObject.connect(boost::bind
        (&FaceColors::slotDeleteObject, this, _1));
}

FaceColors::~FaceColors()
{
    if (d->view) {
        d->view->stopSelection();
        d->view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(),
            Private::selectionCallback, this);
        SoNode* root = d->view->getSceneGraph();
        static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(true);
    }
    Gui::Selection().rmvSelectionGate();
    d->connectDelDoc.disconnect();
    d->connectDelObj.disconnect();
    delete d;
}

void FaceColors::slotDeleteDocument(const Gui::Document& Doc)
{
    if (d->doc == &Doc)
        Gui::Control().closeDialog();
}

void FaceColors::slotDeleteObject(const Gui::ViewProvider& obj)
{
    if (d->vp == &obj)
        Gui::Control().closeDialog();
}

void FaceColors::on_boxSelection_clicked()
{
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        if (!viewer->isSelecting()) {
            viewer->startSelection(Gui::View3DInventorViewer::Rubberband);
            viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), Private::selectionCallback, this);
            // avoid that the selection node handles the event otherwise the callback function won't be
            // called immediately
            SoNode* root = viewer->getSceneGraph();
            static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(false);
            d->view = viewer;
        }
    }
}

void FaceColors::on_defaultButton_clicked()
{
    std::fill(d->perface.begin(), d->perface.end(), d->vp->ShapeColor.getValue());
    d->vp->DiffuseColor.setValues(d->perface);
}

void FaceColors::on_colorButton_changed()
{
    if (!d->index.isEmpty()) {
        float alpha = static_cast<float>(d->vp->Transparency.getValue())/100;
        QColor c = d->ui->colorButton->color();
        for (QSet<int>::iterator it = d->index.begin(); it != d->index.end(); ++it) {
            d->perface[*it].set(c.redF(), c.greenF(), c.blueF(), alpha);
        }
        d->vp->DiffuseColor.setValues(d->perface);
    }
}

void FaceColors::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // no object selected in the combobox or no sub-element was selected
    if (!msg.pSubName)
        return;
    bool selection_changed = false;
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        // when adding a sub-element to the selection check
        // whether this is the currently handled object
        App::Document* doc = d->obj->getDocument();
        std::string docname = doc->getName();
        std::string objname = d->obj->getNameInDocument();
        if (docname==msg.pDocName && objname==msg.pObjectName) {
            int index = std::atoi(msg.pSubName+4)-1;
            d->index.insert(index);
            const App::Color& c = d->perface[index];
            QColor color;
            color.setRgbF(c.r,c.g,c.b);
            d->ui->colorButton->setColor(color);
            selection_changed = true;
        }
    }
    else if (msg.Type == Gui::SelectionChanges::RmvSelection) {
        App::Document* doc = d->obj->getDocument();
        std::string docname = doc->getName();
        std::string objname = d->obj->getNameInDocument();
        if (docname==msg.pDocName && objname==msg.pObjectName) {
            int index = std::atoi(msg.pSubName+4)-1;
            d->index.remove(index);
            selection_changed = true;
        }
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        d->index.clear();
        selection_changed = true;
    }

    if (selection_changed && !d->boxSelection) {
        updatePanel();
    }
}

void FaceColors::updatePanel()
{
    QString faces = QString::fromLatin1("[");
    int size = d->index.size();
    for (QSet<int>::iterator it = d->index.begin(); it != d->index.end(); ++it) {
        faces += QString::number(*it + 1);
        if (--size > 0)
            faces += QString::fromLatin1(",");
    }
    faces += QString::fromLatin1("]");

    int maxWidth = d->ui->labelElement->width();
    QFontMetrics fm(d->ui->labelElement->font());
    if (fm.width(faces) > maxWidth) {
        faces = fm.elidedText(faces, Qt::ElideMiddle, maxWidth);
    }

    d->ui->labelElement->setText(faces);
    d->ui->colorButton->setDisabled(d->index.isEmpty());
}

bool FaceColors::accept()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(d->vp->getObject()->getDocument());
    doc->resetEdit();
    return true;
}

bool FaceColors::reject()
{
    int ret = QMessageBox::question(this, tr("Face colors"), tr("Do you really want to cancel?"),
        QMessageBox::Yes, QMessageBox::No|QMessageBox::Default|QMessageBox::Escape);
    if (ret == QMessageBox::Yes) {
        Gui::Document* doc = Gui::Application::Instance->getDocument(d->vp->getObject()->getDocument());
        doc->resetEdit();
        d->vp->DiffuseColor.setValues(d->current);
        return true;
    }
    else {
        return false;
    }
}

void FaceColors::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui->retranslateUi(this);
    }
}


/* TRANSLATOR PartGui::TaskFaceColors */

TaskFaceColors::TaskFaceColors(ViewProviderPartExt* vp)
{
    widget = new FaceColors(vp);
    taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskFaceColors::~TaskFaceColors()
{
}

void TaskFaceColors::open()
{
}

void TaskFaceColors::clicked(int)
{
}

bool TaskFaceColors::accept()
{
    return widget->accept();
}

bool TaskFaceColors::reject()
{
    return widget->reject();
}

#include "moc_TaskFaceColors.cpp"
