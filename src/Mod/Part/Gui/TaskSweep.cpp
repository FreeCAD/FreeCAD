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
# include <QApplication>
# include <QMessageBox>
# include <QTextStream>
# include <QTimer>
# include <QTreeWidget>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <Precision.hxx>
# include <ShapeAnalysis_FreeBounds.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopTools_HSequenceOfShape.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskSweep.h"
#include "ui_TaskSweep.h"


using namespace PartGui;

class SweepWidget::Private
{
public:
    Ui_TaskSweep ui;
    QString buttonText;
    std::string document;
    Private() = default;
    ~Private() = default;

    class EdgeSelection : public Gui::SelectionFilterGate
    {
    public:
        EdgeSelection()
            : Gui::SelectionFilterGate(nullPointer())
        {
        }
        bool allow(App::Document* /*pDoc*/, App::DocumentObject*pObj, const char*sSubName) override
        {
            if (!sSubName || sSubName[0] == '\0') {
                // If selecting again the same edge the passed sub-element is empty. If the whole
                // shape is an edge or wire we can use it completely.
                Part::TopoShape topoShape = Part::Feature::getTopoShape(pObj);
                if (topoShape.isNull()) {
                    return false;
                }
                const TopoDS_Shape shape = topoShape.getShape();
                if (!shape.IsNull()) {
                    // a single edge
                    if (shape.ShapeType() == TopAbs_EDGE) {
                        return true;
                    }
                    // a single wire
                    if (shape.ShapeType() == TopAbs_WIRE) {
                        return true;
                    }
                    // a compound of only edges or wires
                    if (shape.ShapeType() == TopAbs_COMPOUND) {
                        TopoDS_Iterator it(shape);
                        for (; it.More(); it.Next()) {
                            if (it.Value().IsNull())
                                return false;
                            if ((it.Value().ShapeType() != TopAbs_EDGE) &&
                                    (it.Value().ShapeType() != TopAbs_WIRE))
                                return false;
                        }

                        return true;
                    }
                }
            }
            else {
                std::string element(sSubName);
                return element.substr(0,4) == "Edge";
            }

            return false;
        }
    };
};

/* TRANSLATOR PartGui::SweepWidget */

SweepWidget::SweepWidget(QWidget* parent)
  : d(new Private())
{
    Q_UNUSED(parent);
    Gui::Command::runCommand(Gui::Command::App, "from FreeCAD import Base");
    Gui::Command::runCommand(Gui::Command::App, "import Part");

    d->ui.setupUi(this);
    d->ui.selector->setAvailableLabel(tr("Available profiles"));
    d->ui.selector->setSelectedLabel(tr("Selected profiles"));
    d->ui.labelPath->clear();

    connect(d->ui.buttonPath, &QPushButton::toggled,
            this, &SweepWidget::onButtonPathToggled);
    connect(d->ui.selector->availableTreeWidget(), &QTreeWidget::currentItemChanged,
            this, &SweepWidget::onCurrentItemChanged);
    connect(d->ui.selector->selectedTreeWidget(), &QTreeWidget::currentItemChanged,
            this, &SweepWidget::onCurrentItemChanged);

    findShapes();
}

SweepWidget::~SweepWidget()
{
    delete d;
    Gui::Selection().rmvSelectionGate();
}

void SweepWidget::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    if (!activeGui)
        return;
    d->document = activeDoc->getName();

    std::vector<App::DocumentObject*> objs = activeDoc->getObjectsOfType<App::DocumentObject>();

    for (auto obj : objs) {
        Part::TopoShape topoShape = Part::Feature::getTopoShape(obj);
        if (topoShape.isNull()) {
            continue;
        }
        TopoDS_Shape shape = topoShape.getShape();
        if (shape.IsNull()) continue;

        // also allow compounds with a single face, wire or vertex or
        // if there are only edges building one wire
        if (shape.ShapeType() == TopAbs_COMPOUND) {
            Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
            Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();

            TopoDS_Iterator it(shape);
            int numChilds=0;
            TopoDS_Shape child;
            for (; it.More(); it.Next(), numChilds++) {
                if (!it.Value().IsNull()) {
                    child = it.Value();
                    if (child.ShapeType() == TopAbs_EDGE) {
                        hEdges->Append(child);
                    }
                }
            }

            // a single child
            if (numChilds == 1) {
                shape = child;
            }
            // or all children are edges
            else if (hEdges->Length() == numChilds) {
                ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges,
                    Precision::Confusion(), Standard_False, hWires);
                if (hWires->Length() == 1)
                    shape = hWires->Value(1);
            }
        }

        if (shape.ShapeType() == TopAbs_FACE ||
            shape.ShapeType() == TopAbs_WIRE ||
            shape.ShapeType() == TopAbs_EDGE ||
            shape.ShapeType() == TopAbs_VERTEX) {
            QString label = QString::fromUtf8(obj->Label.getValue());
            QString name = QString::fromLatin1(obj->getNameInDocument());

            QTreeWidgetItem* child = new QTreeWidgetItem();
            child->setText(0, label);
            child->setToolTip(0, label);
            child->setData(0, Qt::UserRole, name);
            Gui::ViewProvider* vp = activeGui->getViewProvider(obj);
            if (vp) child->setIcon(0, vp->getIcon());
            d->ui.selector->availableTreeWidget()->addTopLevelItem(child);
        }
    }
}

bool SweepWidget::isPathValid(const Gui::SelectionObject& sel) const
{
    const App::DocumentObject* path = sel.getObject();
    const std::vector<std::string>& sub = sel.getSubNames();

    TopoDS_Shape pathShape;
    const Part::TopoShape& shape = Part::Feature::getTopoShape(path);
    if (shape.isNull()){
        return false;
    }
    if (!sub.empty()) {
        try {
            BRepBuilderAPI_MakeWire mkWire;
            for (const auto & it : sub) {
                TopoDS_Shape subshape = shape.getSubShape(it.c_str());
                mkWire.Add(TopoDS::Edge(subshape));
            }
            pathShape = mkWire.Wire();
        }
        catch (...) {
            return false;
        }
    }
    else if (shape.getShape().ShapeType() == TopAbs_EDGE) {
        pathShape = shape.getShape();
    }
    else if (shape.getShape().ShapeType() == TopAbs_WIRE) {
        BRepBuilderAPI_MakeWire mkWire(TopoDS::Wire(shape.getShape()));
        pathShape = mkWire.Wire();
    }
    else if (shape.getShape().ShapeType() == TopAbs_COMPOUND) {
        try {
            TopoDS_Iterator it(shape.getShape());
            for (; it.More(); it.Next()) {
                if ((it.Value().ShapeType() != TopAbs_EDGE) &&
                    (it.Value().ShapeType() != TopAbs_WIRE)) {
                    return false;
                }
            }
            Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
            Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
            for (TopExp_Explorer xp(shape.getShape(), TopAbs_EDGE); xp.More(); xp.Next())
                hEdges->Append(xp.Current());

            ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges, Precision::Confusion(), Standard_True, hWires);
            int len = hWires->Length();
            if (len != 1)
                return false;
            pathShape = hWires->Value(1);
        }
        catch (...) {
            return false;
        }
    }

    return (!pathShape.IsNull());
}

bool SweepWidget::accept()
{
    if (d->ui.buttonPath->isChecked())
        return false;
    const App::DocumentObject* docobj = nullptr;
    std::string selection;
    const std::vector<Gui::SelectionObject> selobjs = Gui::Selection().getSelectionEx();
    std::vector<Part::TopoShape> subShapes;
    Part::TopoShape topoShape = Part::TopoShape();
    std::string spineObject, spineLabel;

    bool ok = true;
    if (selobjs.size() == 1) {
        selection = selobjs[0].getAsPropertyLinkSubString();
        const std::vector<std::string>& subnames = selobjs[0].getSubNames();
        docobj = selobjs[0].getObject();
        spineObject = selobjs[0].getFeatName();
        spineLabel = docobj->Label.getValue();
        topoShape = Part::Feature::getTopoShape(docobj);
        if (!topoShape.isNull()) {
            for (std::vector<std::string>::const_iterator it = subnames.begin(); it != subnames.end(); ++it) {
                subShapes.push_back(Part::Feature::getTopoShape(docobj, subnames[0].c_str(), true /*need element*/));
            }
            for (const auto & it : subShapes) {
                TopoDS_Shape dsShape = it.getShape();
                if (dsShape.IsNull() || dsShape.ShapeType() != TopAbs_EDGE) { //only edge selection allowed
                    ok = false;
                }
            }
        } else { //could be not a part::feature or app:link to non-part::feature or app::part without a visible part::feature
            ok = false;
        }

    } else { //not just one object selected
        ok = false;
    }

    QString list, solid, frenet;
    if (d->ui.checkSolid->isChecked())
        solid = QString::fromLatin1("True");
    else
        solid = QString::fromLatin1("False");

    if (d->ui.checkFrenet->isChecked())
        frenet = QString::fromLatin1("True");
    else
        frenet = QString::fromLatin1("False");

    QTextStream str(&list);

    int count = d->ui.selector->selectedTreeWidget()->topLevelItemCount();
    if (count < 1) {
        QMessageBox::critical(this, tr("Too few elements"), tr("At least one edge or wire is required."));
        return false;
    }
    if (!ok) {
        QMessageBox::critical(this, tr("Invalid selection"), tr("Select one or more edges from a single object."));
        return false;
    }
    for (int i=0; i<count; i++) {
        QTreeWidgetItem* child = d->ui.selector->selectedTreeWidget()->topLevelItem(i);
        QString name = child->data(0, Qt::UserRole).toString();
        if (name == QLatin1String(spineObject.c_str())) {
            QMessageBox::critical(this, tr("Wrong selection"), tr("'%1' cannot be used as profile and path.")
                .arg(QString::fromUtf8(spineLabel.c_str())));
            return false;
        }
        str << "App.getDocument('" << d->document.c_str() << "')." << name << ", ";
    }

    try {
        Gui::WaitCursor wc;
        QString cmd;
        cmd = QString::fromLatin1(
            "App.getDocument('%5').addObject('Part::Sweep','Sweep')\n"
            "App.getDocument('%5').ActiveObject.Sections=[%1]\n"
            "App.getDocument('%5').ActiveObject.Spine=%2\n"
            "App.getDocument('%5').ActiveObject.Solid=%3\n"
            "App.getDocument('%5').ActiveObject.Frenet=%4\n"
            )
            .arg(list,
                 QLatin1String(selection.c_str()),
                 solid,
                 frenet,
                 QString::fromLatin1(d->document.c_str()));

        Gui::Document* doc = Gui::Application::Instance->getDocument(d->document.c_str());
        if (!doc)
            throw Base::RuntimeError("Document doesn't exist anymore");
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Sweep"));
        Gui::Command::runCommand(Gui::Command::App, cmd.toLatin1());
        doc->getDocument()->recompute();
        App::DocumentObject* obj = doc->getDocument()->getActiveObject();
        if (obj && !obj->isValid()) {
            std::string msg = obj->getStatusString();
            doc->abortCommand();
            throw Base::RuntimeError(msg);
        }
        doc->commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(this, tr("Input error"), QCoreApplication::translate("Exception", e.what()));
        return false;
    }

    return true;
}

bool SweepWidget::reject()
{
    if (d->ui.buttonPath->isChecked())
        return false;
    return true;
}

void SweepWidget::onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    if (previous) {
        Gui::Selection().rmvSelection(d->document.c_str(),
            (const char*)previous->data(0,Qt::UserRole).toByteArray());
    }
    if (current) {
        Gui::Selection().addSelection(d->document.c_str(),
            (const char*)current->data(0,Qt::UserRole).toByteArray());
    }
}

void SweepWidget::onButtonPathToggled(bool on)
{
    if (on) {
        QList<QWidget*> c = this->findChildren<QWidget*>();
        for (auto it : c)
            it->setEnabled(false);
        d->buttonText = d->ui.buttonPath->text();
        d->ui.buttonPath->setText(tr("Done"));
        d->ui.buttonPath->setEnabled(true);
        d->ui.labelPath->setText(tr("Select one or more connected edges in the 3d view and press 'Done'"));
        d->ui.labelPath->setEnabled(true);

        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new Private::EdgeSelection());
    }
    else {
        QList<QWidget*> c = this->findChildren<QWidget*>();
        for (auto it : c)
            it->setEnabled(true);
        d->ui.buttonPath->setText(d->buttonText);
        d->ui.labelPath->clear();
        Gui::Selection().rmvSelectionGate();

        Gui::SelectionFilter edgeFilter  ("SELECT Part::Feature SUBELEMENT Edge COUNT 1..");
        Gui::SelectionFilter partFilter  ("SELECT Part::Feature COUNT 1");
        bool matchEdge = edgeFilter.match();
        bool matchPart = partFilter.match();
        if (matchEdge) {
            // check if path is valid
            const std::vector<Gui::SelectionObject>& result = edgeFilter.Result[0];
            if (!isPathValid(result.front())) {
                QMessageBox::critical(this, tr("Sweep path"), tr("The selected sweep path is invalid."));
                Gui::Selection().clearSelection();
            }
        }
        else if (matchPart) {
            // check if path is valid
            const std::vector<Gui::SelectionObject>& result = partFilter.Result[0];
            if (!isPathValid(result.front())) {
                QMessageBox::critical(this, tr("Sweep path"), tr("The selected sweep path is invalid."));
                Gui::Selection().clearSelection();
            }
        }
    }
}

void SweepWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
        d->ui.selector->setAvailableLabel(tr("Vertex/Wire"));
        d->ui.selector->setSelectedLabel(tr("Sweep"));
    }
}


/* TRANSLATOR PartGui::TaskSweep */

TaskSweep::TaskSweep() : label(nullptr)
{
    widget = new SweepWidget();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Sweep"),
        widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskSweep::~TaskSweep()
{
    delete label;
}

void TaskSweep::open()
{
}

void TaskSweep::clicked(int id)
{
    if (id == QDialogButtonBox::Help) {
        QString help = QApplication::translate("PartGui::TaskSweep",
            "Select one or more profiles and select an edge or wire\n"
            "in the 3D view for the sweep path.");
        if (!label) {
            label = new Gui::StatusWidget(widget);
            label->setStatusText(help);
        }

        label->show();
        QTimer::singleShot(3000, label, &Gui::StatusWidget::hide);
    }
}

bool TaskSweep::accept()
{
    return widget->accept();
}

bool TaskSweep::reject()
{
    return widget->reject();
}

#include "moc_TaskSweep.cpp"
