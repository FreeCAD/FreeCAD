// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <cstring>
#include <functional>

#include <QApplication>
#include <QMessageBox>
#include <QSignalBlocker>

#include <App/Document.h>
#include <Base/Exception.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/Selection/SelectionObject.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/FeatureProjectOnSurface.h>

#include "ReferenceSelection.h"
#include "ui_TaskProjectOnSurface.h"
#include "TaskProjectOnSurface.h"

using namespace PartDesignGui;

namespace
{
class ProjectOnSurfaceSelectionGate: public Gui::SelectionGate
{
public:
    ProjectOnSurfaceSelectionGate(const App::DocumentObject* feature, bool support)
        : feature(feature)
        , support(support)
    {}

    bool allow(App::Document*, App::DocumentObject* object, const char* subName) override
    {
        if (!feature || !object || object == feature || !feature->testIfLinkDAGCompatible(object)) {
            return false;
        }

        if (!support && (!subName || !subName[0])
            && object->isDerivedFrom<Part::Part2DObject>()) {
            return true;
        }

        try {
            auto shape = Part::Feature::getTopoShape(
                object,
                Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
                    | Part::ShapeOption::Transform,
                subName
            );
            if (shape.isNull()) {
                return false;
            }
            if (support) {
                return shape.getShape().ShapeType() == TopAbs_FACE;
            }
            if (!subName || !subName[0]) {
                return true;
            }
            const auto type = shape.getShape().ShapeType();
            return type == TopAbs_EDGE || type == TopAbs_WIRE || type == TopAbs_FACE;
        }
        catch (const Base::Exception&) {
            return false;
        }
    }

private:
    const App::DocumentObject* feature;
    bool support;
};

bool containsReference(
    const App::PropertyLinkSubList& property,
    App::DocumentObject* object,
    const std::string& subName
)
{
    const auto& objects = property.getValues();
    const auto& subNames = property.getSubValues();
    for (std::size_t index = 0; index < objects.size(); ++index) {
        if (objects[index] == object && subNames[index] == subName) {
            return true;
        }
    }
    return false;
}

void populateList(QListWidget* list, const App::PropertyLinkSubList& property)
{
    list->clear();
    const auto& objects = property.getValues();
    const auto& subNames = property.getSubValues();
    for (std::size_t index = 0; index < objects.size(); ++index) {
        list->addItem(getRefStr(objects[index], {subNames[index]}));
    }
}

void populateList(QListWidget* list, const App::PropertyLinkSub& property)
{
    list->clear();
    if (auto* object = property.getValue()) {
        list->addItem(getRefStr(object, property.getSubValues()));
    }
}

void addPart2DEdges(App::PropertyLinkSubList& property, App::DocumentObject* object)
{
    auto shape = Part::Feature::getTopoShape(
        object,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform,
        ""
    );
    const auto edgeCount = shape.countSubShapes(TopAbs_EDGE);
    for (std::size_t index = 1; index <= edgeCount; ++index) {
        const std::string subName = "Edge" + std::to_string(index);
        if (!containsReference(property, object, subName)) {
            property.addValue(object, {subName});
        }
    }
}
}  // namespace

TaskProjectOnSurface::TaskProjectOnSurface(
    ViewProviderProjectOnSurface* view,
    QWidget* parent
)
    : Gui::TaskView::TaskBox(
          Gui::BitmapFactory().pixmap("Part_ProjectionOnSurface"),
          tr("Project on Surface Parameters"),
          true,
          parent
      )
    , SelectionObserver(view)
    , ui(new Ui_TaskProjectOnSurface)
    , vp(view)
{
    auto* proxy = new QWidget(this);
    ui->setupUi(proxy);
    groupLayout()->addWidget(proxy);

    connect(ui->buttonAddProjection, &QToolButton::toggled, this, [this](bool checked) {
        setSelectionMode(SelectionMode::Projection, checked);
    });
    connect(ui->buttonAddSupport, &QToolButton::toggled, this, [this](bool checked) {
        setSelectionMode(SelectionMode::Support, checked);
    });
    connect(ui->buttonRemoveProjection, &QToolButton::clicked, this, [this] {
        removeSelected(false);
    });
    connect(ui->buttonRemoveSupport, &QToolButton::clicked, this, [this] {
        removeSelected(true);
    });
    connect(ui->comboResult, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (vp.expired()) {
            return;
        }
        auto* feature = vp->getObject<PartDesign::ProjectOnSurface>();
        const char* mode = index == 0 ? Part::ProjectOnSurface::AllMode
            : index == 1             ? Part::ProjectOnSurface::EdgesMode
                                     : Part::ProjectOnSurface::FacesMode;
        feature->Mode.setValue(mode);
        updateFeature();
    });

    updateUI();
}

TaskProjectOnSurface::~TaskProjectOnSurface()
{
    Gui::Selection().rmvSelectionGate();
}

void TaskProjectOnSurface::setSelectionMode(SelectionMode mode, bool enabled)
{
    if (!enabled) {
        if (selectionMode == mode) {
            selectionMode = SelectionMode::None;
            Gui::Selection().rmvSelectionGate();
        }
        return;
    }

    selectionMode = mode;
    if (mode == SelectionMode::Projection) {
        const QSignalBlocker blocker(ui->buttonAddSupport);
        ui->buttonAddSupport->setChecked(false);
    }
    else {
        const QSignalBlocker blocker(ui->buttonAddProjection);
        ui->buttonAddProjection->setChecked(false);
    }

    Gui::Selection().clearSelection();
    if (!vp.expired()) {
        Gui::Selection().addSelectionGate(new ProjectOnSurfaceSelectionGate(
            vp->getObject<PartDesign::ProjectOnSurface>(),
            mode == SelectionMode::Support
        ));
    }
}

void TaskProjectOnSurface::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == SelectionMode::None || vp.expired()
        || msg.Type != Gui::SelectionChanges::AddSelection) {
        return;
    }

    Gui::SelectionObject selected(msg);
    auto* object = selected.getObject();
    auto* feature = vp->getObject<PartDesign::ProjectOnSurface>();
    if (!object || object == feature || !feature->testIfLinkDAGCompatible(object)) {
        return;
    }

    auto subNames = selected.getSubNames();
    if (subNames.empty()) {
        subNames.emplace_back();
    }

    for (const auto& subName : subNames) {
        if (selectionMode == SelectionMode::Projection && subName.empty()
            && object->isDerivedFrom<Part::Part2DObject>()) {
            try {
                addPart2DEdges(feature->Projection, object);
            }
            catch (const Base::Exception&) {
            }
            continue;
        }

        try {
            auto shape = Part::Feature::getTopoShape(
                object,
                Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
                    | Part::ShapeOption::Transform,
                subName.c_str()
            );
            if (shape.isNull()) {
                continue;
            }
            const auto type = shape.getShape().ShapeType();
            if (selectionMode == SelectionMode::Support && type != TopAbs_FACE) {
                continue;
            }
            if (selectionMode == SelectionMode::Projection && !subName.empty()
                && type != TopAbs_EDGE && type != TopAbs_WIRE && type != TopAbs_FACE) {
                continue;
            }
            if (selectionMode == SelectionMode::Support) {
                feature->SupportFace.setValue(object, {subName});
                break;
            }
            if (!containsReference(feature->Projection, object, subName)) {
                feature->Projection.addValue(object, {subName});
            }
        }
        catch (const Base::Exception&) {
        }
    }

    updateUI();
    updateFeature();
    Gui::Selection().clearSelection();
}

void TaskProjectOnSurface::removeSelected(bool support)
{
    if (vp.expired()) {
        return;
    }
    auto* feature = vp->getObject<PartDesign::ProjectOnSurface>();
    auto* list = support ? ui->listSupport : ui->listProjection;

    if (support) {
        if (!list->selectedItems().empty()) {
            feature->SupportFace.setValue(nullptr);
            updateUI();
            updateFeature();
        }
        return;
    }

    auto objects = feature->Projection.getValues();
    auto subNames = feature->Projection.getSubValues();
    std::vector<int> rows;
    for (auto* item : list->selectedItems()) {
        rows.push_back(list->row(item));
    }
    std::ranges::sort(rows, std::greater());
    for (int row : rows) {
        if (row >= 0 && static_cast<std::size_t>(row) < objects.size()) {
            objects.erase(objects.begin() + row);
            subNames.erase(subNames.begin() + row);
        }
    }
    feature->Projection.setValues(std::move(objects), std::move(subNames));
    updateUI();
    updateFeature();
}

void TaskProjectOnSurface::updateUI()
{
    if (vp.expired()) {
        return;
    }
    auto* feature = vp->getObject<PartDesign::ProjectOnSurface>();
    populateList(ui->listProjection, feature->Projection);
    populateList(ui->listSupport, feature->SupportFace);

    const QSignalBlocker blocker(ui->comboResult);
    const char* mode = feature->Mode.getValueAsString();
    ui->comboResult->setCurrentIndex(
        strcmp(mode, Part::ProjectOnSurface::AllMode) == 0 ? 0
            : strcmp(mode, Part::ProjectOnSurface::EdgesMode) == 0 ? 1
                                                                    : 2
    );
}

void TaskProjectOnSurface::updateFeature()
{
    if (vp.expired()) {
        return;
    }
    auto* feature = vp->getObject<PartDesign::ProjectOnSurface>();
    feature->recomputeFeature();
}

TaskDlgProjectOnSurface::TaskDlgProjectOnSurface(ViewProviderProjectOnSurface* view)
    : parameter(new TaskProjectOnSurface(view))
    , vp(view)
{
    Content.push_back(parameter);
}

bool TaskDlgProjectOnSurface::accept()
{
    if (vp.expired()) {
        return true;
    }

    auto* feature = vp->getObject<PartDesign::ProjectOnSurface>();
    try {
        if (feature->Projection.getSize() == 0) {
            throw Base::ValueError("Select at least one sketch, edge, wire, or face to project");
        }
        if (!feature->SupportFace.getValue() || feature->SupportFace.getSubValues().size() != 1) {
            throw Base::ValueError("Select a target face");
        }
        Gui::cmdAppDocument(feature, "recompute()");
        if (!feature->isValid() || feature->Shape.getValue().IsNull()) {
            throw Base::RuntimeError(feature->getStatusString());
        }
        Gui::cmdGuiDocument(feature, "resetEdit()");
        vp->getDocument()->commitCommand();
        return true;
    }
    catch (const Base::Exception& error) {
        QMessageBox::warning(
            parameter,
            tr("Project on Surface"),
            QApplication::translate("Exception", error.what())
        );
        return false;
    }
}

bool TaskDlgProjectOnSurface::reject()
{
    if (!vp.expired()) {
        auto* document = vp->getObject()->getDocument();
        vp->getDocument()->abortCommand();
        Gui::cmdGuiDocument(document, "resetEdit()");
        Gui::cmdAppDocument(document, "recompute()");
    }
    return true;
}

#include "moc_TaskProjectOnSurface.cpp"
