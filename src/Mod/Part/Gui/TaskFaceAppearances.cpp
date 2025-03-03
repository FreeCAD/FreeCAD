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

#include <App/Document.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Dialogs/DlgMaterialPropertiesImp.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Tools.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/Material/Gui/MaterialTreeWidget.h>

#include "TaskFaceAppearances.h"
#include "ui_TaskFaceAppearances.h"
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
            if (Base::Tools::isNullOrEmpty(sSubName))
                return false;
            std::string element(sSubName);
            return element.substr(0, 4) == "Face";
        }
    };
}

class FaceAppearances::Private
{
public:
    using Connection = boost::signals2::connection;
    Ui_TaskFaceAppearances* ui;
    QPointer<Gui::View3DInventorViewer> view;
    ViewProviderPartExt* vp;
    App::DocumentObject* obj;
    Gui::Document* doc;
    std::vector<App::Material> perface;
    QSet<int> index;
    bool boxSelection;
    Connection connectDelDoc;
    Connection connectDelObj;
    Connection connectUndoDoc;

    explicit Private(ViewProviderPartExt* vp) : ui(new Ui_TaskFaceAppearances()), view(nullptr), vp(vp)
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

        std::vector<App::Material> current = vp->ShapeAppearance.getValues();
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

        FaceAppearances* self = static_cast<FaceAppearances*>(ud);
        self->d->view = nullptr;
        if (self->d->obj && self->d->obj->isDerivedFrom<Part::Feature>()) {
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

/* TRANSLATOR PartGui::TaskFaceAppearances */

FaceAppearances::FaceAppearances(ViewProviderPartExt* vp, QWidget* parent)
    : d(new Private(vp))
{
    Q_UNUSED(parent);
    d->ui->setupUi(this);
    setupConnections();

    d->ui->groupBox->setTitle(QString::fromUtf8(vp->getObject()->Label.getValue()));
    d->ui->buttonCustomAppearance->setDisabled(true);

    FaceSelection* gate = new FaceSelection(d->vp->getObject());
    Gui::Selection().addSelectionGate(gate);

    //NOLINTBEGIN
    d->connectDelDoc = Gui::Application::Instance->signalDeleteDocument.connect(std::bind
        (&FaceAppearances::slotDeleteDocument, this, sp::_1));
    d->connectDelObj = Gui::Application::Instance->signalDeletedObject.connect(std::bind
        (&FaceAppearances::slotDeleteObject, this, sp::_1));
    d->connectUndoDoc = d->doc->signalUndoDocument.connect(std::bind
        (&FaceAppearances::slotUndoDocument, this, sp::_1));
    //NOLINTEND
}

FaceAppearances::~FaceAppearances()
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

void FaceAppearances::setupConnections()
{
    // clang-format off
    connect(d->ui->defaultButton, &QPushButton::clicked,
            this, &FaceAppearances::onDefaultButtonClicked);
    connect(d->ui->boxSelection, &QPushButton::toggled,
            this, &FaceAppearances::onBoxSelectionToggled);
    connect(d->ui->widgetMaterial, &MatGui::MaterialTreeWidget::materialSelected,
            this, &FaceAppearances::onMaterialSelected);
    connect(d->ui->buttonCustomAppearance, &QPushButton::clicked,
            this, &FaceAppearances::onButtonCustomAppearanceClicked);
    // clang-format on
}

void FaceAppearances::slotUndoDocument(const Gui::Document& Doc)
{
    if (d->doc == &Doc) {
        d->doc->resetEdit();
        Gui::Control().closeDialog();
    }
}

void FaceAppearances::slotDeleteDocument(const Gui::Document& Doc)
{
    if (d->doc == &Doc)
        Gui::Control().closeDialog();
}

void FaceAppearances::slotDeleteObject(const Gui::ViewProvider& obj)
{
    if (d->vp == &obj)
        Gui::Control().closeDialog();
}

void FaceAppearances::onBoxSelectionToggled(bool checked)
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

void FaceAppearances::onDefaultButtonClicked()
{
    std::fill(d->perface.begin(), d->perface.end(), d->vp->ShapeAppearance[0]);
    d->vp->ShapeAppearance.setValues(d->perface);
}

void FaceAppearances::onMaterialSelected(const std::shared_ptr<Materials::Material>& material)
{
    if (!d->index.isEmpty()) {
        for (int it : d->index) {
            d->perface[it] = material->getMaterialAppearance();
        }
        d->vp->ShapeAppearance.setValues(d->perface);
        // new color has been applied, unselect so that users can see this
        onSelectionChanged(Gui::SelectionChanges::ClrSelection);
        Gui::Selection().clearSelection();
    }
}

void FaceAppearances::onSelectionChanged(const Gui::SelectionChanges& msg)
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
            const Base::Color& faceColor = d->perface[index].diffuseColor;
            QColor color;
            // alpha of Base::Color is contrary to the one of QColor
            color.setRgbF(faceColor.r, faceColor.g, faceColor.b, (1.0 - faceColor.a));
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

void FaceAppearances::updatePanel()
{
    QString faces = QStringLiteral("[");
    int size = d->index.size();
    for (int it : d->index) {
        faces += QString::number(it + 1);
        if (--size > 0)
            faces += QStringLiteral(",");
    }
    faces += QStringLiteral("]");

    int maxWidth = d->ui->labelElement->width();
    QFontMetrics fm(d->ui->labelElement->font());
    if (Gui::QtTools::horizontalAdvance(fm, faces) > maxWidth) {
        faces = fm.elidedText(faces, Qt::ElideMiddle, maxWidth);
    }

    d->ui->labelElement->setText(faces);
    d->ui->buttonCustomAppearance->setDisabled(d->index.isEmpty());
}

int FaceAppearances::getFirstIndex() const
{
    if (!d->index.isEmpty()) {
        return *(d->index.begin());
    }

    return 0;
}

/**
 * Opens a dialog that allows one to modify the 'ShapeMaterial' property of all selected view providers.
 */
void FaceAppearances::onButtonCustomAppearanceClicked()
{
    Gui::Dialog::DlgMaterialPropertiesImp dlg(this);
    App::Material mat = d->perface[getFirstIndex()];
    dlg.setCustomMaterial(mat);
    dlg.setDefaultMaterial(mat);
    dlg.exec();

    // Set the face appearance
    if (!d->index.isEmpty()) {
        for (int it : d->index) {
            d->perface[it] = dlg.getCustomMaterial();
        }
        d->vp->ShapeAppearance.setValues(d->perface);
        // new color has been applied, unselect so that users can see this
        onSelectionChanged(Gui::SelectionChanges::ClrSelection);
        Gui::Selection().clearSelection();
    }
}

void FaceAppearances::open()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(d->vp->getObject()->getDocument());
    doc->openCommand(QT_TRANSLATE_NOOP("Command", "Change face colors"));
}

bool FaceAppearances::accept()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(d->vp->getObject()->getDocument());
    doc->commitCommand();
    doc->resetEdit();
    return true;
}

bool FaceAppearances::reject()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(d->vp->getObject()->getDocument());
    doc->abortCommand();
    doc->resetEdit();
    return true;
}

void FaceAppearances::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui->retranslateUi(this);
    }
}


/* TRANSLATOR PartGui::TaskFaceAppearances */

TaskFaceAppearances::TaskFaceAppearances(ViewProviderPartExt* vp)
{
    widget = new FaceAppearances(vp);
    addTaskBox(widget);
}

TaskFaceAppearances::~TaskFaceAppearances() = default;

void TaskFaceAppearances::open()
{
    widget->open();
}

void TaskFaceAppearances::clicked(int)
{
}

bool TaskFaceAppearances::accept()
{
    return widget->accept();
}

bool TaskFaceAppearances::reject()
{
    return widget->reject();
}

#include "moc_TaskFaceAppearances.cpp"
