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
# include <QFontMetrics>
# include <QPointer>
# include <QSet>
# include <BRep_Tool.hxx>
# include <BRepGProp.hxx>
# include <gp_Pnt.hxx>
# include <GProp_GProps.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoRayPickAction.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoSeparator.h>
#endif
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/Tools.h>
#include <Gui/Utilities.h>

#include "TaskFaceColors.h"
#include "ui_TaskFaceColors.h"
#include "SoBrepFaceSet.h"
#include "ViewProviderExt.h"


using namespace PartGui;
namespace sp = std::placeholders;

namespace PartGui {
    class FaceSelection : public Gui::SelectionFilterGate
    {
        const App::DocumentObject* object;
    public:
        explicit FaceSelection(const App::DocumentObject* obj)
            : Gui::SelectionFilterGate(), object(obj)
        {
        }
        bool allow(App::Document* /*pDoc*/, App::DocumentObject* pObj, const char* sSubName) override
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            std::string element(sSubName);
            return element.substr(0, 4) == "Face";
        }
    };
}

class FaceColors::Private
{
public:
    using Connection = boost::signals2::connection;
    Ui_TaskFaceColors* ui;
    QPointer<Gui::View3DInventorViewer> view;
    ViewProviderPartExt* vp;
    App::DocumentObject* obj;
    Gui::Document* doc;
    std::vector<App::Color> perface;
    QSet<int> index;
    bool boxSelection;
    Connection connectDelDoc;
    Connection connectDelObj;
    Connection connectUndoDoc;

    explicit Private(ViewProviderPartExt* vp) : ui(new Ui_TaskFaceColors()), view(nullptr), vp(vp)
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

        std::vector<App::Color> current = vp->DiffuseColor.getValues();
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

            TopExp_Explorer xp_face(shape, TopAbs_FACE);
            while (xp_face.More()) {
                M.Add(xp_face.Current());
                xp_face.Next();
            }

            App::Document* appdoc = doc->getDocument();
            for (Standard_Integer k = 1; k <= M.Extent(); k++) {
                const TopoDS_Shape& face = M(k);

                TopExp_Explorer xp_vertex(face, TopAbs_VERTEX);
                while (xp_vertex.More()) {
                    gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(xp_vertex.Current()));
                    Base::Vector3d pt2d;
                    pt2d = proj(Base::Vector3d(p.X(), p.Y(), p.Z()));
                    if (polygon.Contains(Base::Vector2d(pt2d.x, pt2d.y))) {
                        std::stringstream str;
                        str << "Face" << k;
                        Gui::Selection().addSelection(appdoc->getName(), obj->getNameInDocument(), str.str().c_str());
                        break;
                    }
                    xp_vertex.Next();
                }
            }
        }
        catch (...) {
        }
    }
    static void selectionCallback(void* ud, SoEventCallback* cb)
    {
        Gui::View3DInventorViewer* view = static_cast<Gui::View3DInventorViewer*>(cb->getUserData());
        view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), selectionCallback, ud);
        view->setSelectionEnabled(true);

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
            for (const auto& it : picked)
                polygon.Add(Base::Vector2d(it[0], it[1]));
        }

        FaceColors* self = static_cast<FaceColors*>(ud);
        self->d->view = nullptr;
        if (self->d->obj && self->d->obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            cb->setHandled();
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(self->d->obj)->Shape.getValue();
            self->d->boxSelection = true;
            self->d->addFacesToSelection(view, proj, polygon, shape);
            self->d->boxSelection = false;
            self->d->ui->boxSelection->setChecked(false);
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
    setupConnections();

    d->ui->groupBox->setTitle(QString::fromUtf8(vp->getObject()->Label.getValue()));
    d->ui->colorButton->setDisabled(true);
    d->ui->colorButton->setAllowTransparency(true);

    FaceSelection* gate = new FaceSelection(d->vp->getObject());
    Gui::Selection().addSelectionGate(gate);

    //NOLINTBEGIN
    d->connectDelDoc = Gui::Application::Instance->signalDeleteDocument.connect(std::bind
        (&FaceColors::slotDeleteDocument, this, sp::_1));
    d->connectDelObj = Gui::Application::Instance->signalDeletedObject.connect(std::bind
        (&FaceColors::slotDeleteObject, this, sp::_1));
    d->connectUndoDoc = d->doc->signalUndoDocument.connect(std::bind
        (&FaceColors::slotUndoDocument, this, sp::_1));
    //NOLINTEND
}

FaceColors::~FaceColors()
{
    if (d->view) {
        d->view->stopSelection();
        d->view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(),
            Private::selectionCallback, this);
        d->view->setSelectionEnabled(true);
    }
    Gui::Selection().rmvSelectionGate();
    d->connectDelDoc.disconnect();
    d->connectDelObj.disconnect();
    d->connectUndoDoc.disconnect();
    delete d;
}

void FaceColors::setupConnections()
{
    connect(d->ui->colorButton, &Gui::ColorButton::changed,
            this, &FaceColors::onColorButtonChanged);
    connect(d->ui->defaultButton, &QPushButton::clicked,
            this, &FaceColors::onDefaultButtonClicked);
    connect(d->ui->boxSelection, &QPushButton::toggled,
            this, &FaceColors::onBoxSelectionToggled);
}

void FaceColors::slotUndoDocument(const Gui::Document& Doc)
{
    if (d->doc == &Doc) {
        d->doc->resetEdit();
        Gui::Control().closeDialog();
    }
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

void FaceColors::onBoxSelectionToggled(bool checked)
{
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
    // toggle the button state and feature
    d->boxSelection = checked;
    if (!checked) {
        // end box selection mode
        if (view)
            view->getViewer()->stopSelection();
    }

    if (view && checked) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        if (!viewer->isSelecting()) {
            viewer->startSelection(Gui::View3DInventorViewer::Rubberband);
            viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), Private::selectionCallback, this);
            // avoid that the selection node handles the event otherwise the callback function won't be
            // called immediately
            viewer->setSelectionEnabled(false);
            d->view = viewer;
        }
    }
}

void FaceColors::onDefaultButtonClicked()
{
    std::fill(d->perface.begin(), d->perface.end(), d->vp->ShapeColor.getValue());
    d->vp->DiffuseColor.setValues(d->perface);
}

void FaceColors::onColorButtonChanged()
{
    if (!d->index.isEmpty()) {
        QColor color = d->ui->colorButton->color();
        for (int it : d->index) {
            // alpha of App::Color is contrary to the one of QColor
            d->perface[it].set(color.redF(), color.greenF(), color.blueF(), (1.0 - color.alphaF()));
        }
        d->vp->DiffuseColor.setValues(d->perface);
        // new color has been applied, unselect so that users can see this
        onSelectionChanged(Gui::SelectionChanges::ClrSelection);
        Gui::Selection().clearSelection();
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
        if (docname == msg.pDocName && objname == msg.pObjectName) {
            int index = std::atoi(msg.pSubName + 4) - 1;
            d->index.insert(index);
            const App::Color& faceColor = d->perface[index];
            QColor color;
            // alpha of App::Color is contrary to the one of QColor
            color.setRgbF(faceColor.r, faceColor.g, faceColor.b, (1.0 - faceColor.a));
            d->ui->colorButton->setColor(color);
            selection_changed = true;
        }
    }
    else if (msg.Type == Gui::SelectionChanges::RmvSelection) {
        App::Document* doc = d->obj->getDocument();
        std::string docname = doc->getName();
        std::string objname = d->obj->getNameInDocument();
        if (docname == msg.pDocName && objname == msg.pObjectName) {
            int index = std::atoi(msg.pSubName + 4) - 1;
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
    for (int it : d->index) {
        faces += QString::number(it + 1);
        if (--size > 0)
            faces += QString::fromLatin1(",");
    }
    faces += QString::fromLatin1("]");

    int maxWidth = d->ui->labelElement->width();
    QFontMetrics fm(d->ui->labelElement->font());
    if (Gui::QtTools::horizontalAdvance(fm, faces) > maxWidth) {
        faces = fm.elidedText(faces, Qt::ElideMiddle, maxWidth);
    }

    d->ui->labelElement->setText(faces);
    d->ui->colorButton->setDisabled(d->index.isEmpty());
}

void FaceColors::open()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(d->vp->getObject()->getDocument());
    doc->openCommand(QT_TRANSLATE_NOOP("Command", "Change face colors"));
}

bool FaceColors::accept()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(d->vp->getObject()->getDocument());
    doc->commitCommand();
    doc->resetEdit();
    return true;
}

bool FaceColors::reject()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(d->vp->getObject()->getDocument());
    doc->abortCommand();
    doc->resetEdit();
    return true;
}

void FaceColors::changeEvent(QEvent* e)
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
        QPixmap(), widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskFaceColors::~TaskFaceColors() = default;

void TaskFaceColors::open()
{
    widget->open();
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
