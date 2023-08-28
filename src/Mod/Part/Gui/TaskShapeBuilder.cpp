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
# include <QButtonGroup>
# include <QMessageBox>
# include <QTextStream>
# include <sstream>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SelectionObject.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskShapeBuilder.h"
#include "ui_TaskShapeBuilder.h"
#include "BoxSelection.h"


using namespace PartGui;

namespace PartGui {
    class ShapeSelection : public Gui::SelectionFilterGate
    {
    public:
        enum Type {VERTEX, EDGE, FACE, ALL};
        Type mode{ALL};
        ShapeSelection()
            : Gui::SelectionFilterGate(nullPointer())
        {
        }
        void setMode(Type mode)
        {
            this->mode = mode;
        }
        bool allow(App::Document*, App::DocumentObject* obj, const char*sSubName) override
        {
            if (!obj || !obj->isDerivedFrom(Part::Feature::getClassTypeId()))
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return (mode == ALL);
            std::string element(sSubName);
            switch (mode) {
            case VERTEX:
                return element.substr(0,6) == "Vertex";
            case EDGE:
                return element.substr(0,4) == "Edge";
            case FACE:
                return element.substr(0,4) == "Face";
            default:
                return true;
            }
        }
    };
}

class ShapeBuilderWidget::Private
{
public:
    Ui_TaskShapeBuilder ui;
    QButtonGroup bg;
    ShapeSelection* gate;
    BoxSelection selection;
    Private()
    {
        Gui::Command::runCommand(Gui::Command::App, "from FreeCAD import Base");
        Gui::Command::runCommand(Gui::Command::App, "import Part");
    }
    ~Private() = default;
};

/* TRANSLATOR PartGui::ShapeBuilderWidget */

ShapeBuilderWidget::ShapeBuilderWidget(QWidget* parent)
  : d(new Private())
{
    Q_UNUSED(parent);
    d->ui.setupUi(this);
    d->ui.label->setText(QString());
    d->bg.addButton(d->ui.radioButtonEdgeFromVertex, 0);
    d->bg.addButton(d->ui.radioButtonWireFromEdge, 1);
    d->bg.addButton(d->ui.radioButtonFaceFromVertex, 2);
    d->bg.addButton(d->ui.radioButtonFaceFromEdge, 3);
    d->bg.addButton(d->ui.radioButtonShellFromFace, 4);
    d->bg.addButton(d->ui.radioButtonSolidFromShell, 5);
    d->bg.setExclusive(true);

    connect(d->ui.selectButton, &QPushButton::clicked,
            this, &ShapeBuilderWidget::onSelectButtonClicked);
    connect(d->ui.createButton, &QPushButton::clicked,
            this, &ShapeBuilderWidget::onCreateButtonClicked);
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
    connect(&d->bg, qOverload<int>(&QButtonGroup::buttonClicked),
            this, &ShapeBuilderWidget::switchMode);
#else
    connect(&d->bg, &QButtonGroup::idClicked,
            this, &ShapeBuilderWidget::switchMode);
#endif

    d->gate = new ShapeSelection();
    Gui::Selection().addSelectionGate(d->gate);

    d->bg.button(0)->setChecked(true);
    switchMode(0);
}

ShapeBuilderWidget::~ShapeBuilderWidget()
{
    Gui::Selection().rmvSelectionGate();
    delete d;
}

void ShapeBuilderWidget::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (d->ui.checkFaces->isChecked()) {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            std::string subName(msg.pSubName);
            if (!subName.empty()) {
                // From the shape get all faces and add them to the selection
                bool blocked = blockSelection(true);
                App::Document* doc = App::GetApplication().getDocument(msg.pDocName);
                App::DocumentObject* obj = doc->getObject(msg.pObjectName);
                if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                    TopoDS_Shape myShape = static_cast<Part::Feature*>(obj)->Shape.getValue();
                    TopTools_IndexedMapOfShape all_faces;
                    TopExp::MapShapes(myShape, TopAbs_FACE, all_faces);
                    for (int i=1; i<= all_faces.Extent(); i++) {
                        TopoDS_Shape face = all_faces(i);
                        if (!face.IsNull()) {
                            std::stringstream str;
                            str << "Face" << i;
                            Gui::Selection().addSelection(msg.pDocName, msg.pObjectName, str.str().c_str());
                        }
                    }
                }

                blockSelection(blocked);
            }
        }
    }
}

void ShapeBuilderWidget::onCreateButtonClicked()
{
    int mode = d->bg.checkedId();
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc)
        return;

    try {
        if (mode == 0) {
            createEdgeFromVertex();
        }
        else if (mode == 1) {
            createWireFromEdge();
        }
        else if (mode == 2) {
            createFaceFromVertex();
        }
        else if (mode == 3) {
            createFaceFromEdge();
        }
        else if (mode == 4) {
            createShellFromFace();
        }
        else if (mode == 5) {
            createSolidFromShell();
        }
        doc->getDocument()->recompute();
        Gui::Selection().clearSelection();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
    }
}

void ShapeBuilderWidget::onSelectButtonClicked()
{
    int id = d->bg.checkedId();
    if (id == 0 || id == 2) {
        d->selection.start(TopAbs_VERTEX);
    }
    else if (id == 1 || id == 3) {
        d->selection.start(TopAbs_EDGE);
    }
    else if (id == 4) {
        d->selection.start(TopAbs_FACE);
    }
    else {
        QMessageBox::warning(this, tr("Unsupported"), tr("Box selection for shells is not supported"));
    }
}

void ShapeBuilderWidget::createEdgeFromVertex()
{
    Gui::SelectionFilter vertexFilter  ("SELECT Part::Feature SUBELEMENT Vertex COUNT 2");
    bool matchVertex = vertexFilter.match();
    if (!matchVertex) {
        QMessageBox::critical(this, tr("Wrong selection"), tr("Select two vertices"));
        return;
    }

    std::vector<Gui::SelectionObject> sel = vertexFilter.Result[0];
    std::vector<QString> elements;
    std::vector<Gui::SelectionObject>::iterator it;
    std::vector<std::string>::const_iterator jt;
    for (it=sel.begin();it!=sel.end();++it) {
        for (jt=it->getSubNames().begin();jt!=it->getSubNames().end();++jt) {
            QString line;
            QTextStream str(&line);
            str << "App.ActiveDocument." << it->getFeatName() << ".Shape." << jt->c_str() << ".Point";
            elements.push_back(line);
        }
    }

    // should actually never happen
    if (elements.size() != 2) {
        QMessageBox::critical(this, tr("Wrong selection"), tr("Select two vertices"));
        return;
    }

    QString cmd;
    cmd = QString::fromLatin1(
        "_=Part.makeLine(%1, %2)\n"
        "if _.isNull(): raise RuntimeError('Failed to create edge')\n"
        "App.ActiveDocument.addObject('Part::Feature','Edge').Shape=_\n"
        "del _\n"
    ).arg(elements[0], elements[1]);

    try {
        Gui::Application::Instance->activeDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Edge"));
        Gui::Command::runCommand(Gui::Command::App, cmd.toLatin1());
        Gui::Application::Instance->activeDocument()->commitCommand();
    }
    catch (const Base::Exception&) {
        Gui::Application::Instance->activeDocument()->abortCommand();
        throw;
    }
}

void ShapeBuilderWidget::createWireFromEdge()
{
    Gui::SelectionFilter edgeFilter  ("SELECT Part::Feature SUBELEMENT Edge COUNT 1..");
    bool matchEdge = edgeFilter.match();
    if (!matchEdge) {
        QMessageBox::critical(this, tr("Wrong selection"), tr("Select one or more edges"));
        return;
    }

    std::vector<Gui::SelectionObject> sel = edgeFilter.Result[0];
    std::vector<Gui::SelectionObject>::iterator it;
    std::vector<std::string>::const_iterator jt;

    QString list;
    QTextStream str(&list);
    str << "[";
    for (it=sel.begin();it!=sel.end();++it) {
        for (jt=it->getSubNames().begin();jt!=it->getSubNames().end();++jt) {
            str << "App.ActiveDocument." << it->getFeatName() << ".Shape." << jt->c_str() << ", ";
        }
    }
    str << "]";

    QString cmd;
    cmd = QString::fromLatin1(
        "_=Part.Wire(Part.__sortEdges__(%1))\n"
        "if _.isNull(): raise RuntimeError('Failed to create a wire')\n"
        "App.ActiveDocument.addObject('Part::Feature','Wire').Shape=_\n"
        "del _\n"
    ).arg(list);
    try {
        Gui::Application::Instance->activeDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Wire"));
        Gui::Command::runCommand(Gui::Command::App, cmd.toLatin1());
        Gui::Application::Instance->activeDocument()->commitCommand();
    }
    catch (const Base::Exception&) {
        Gui::Application::Instance->activeDocument()->abortCommand();
        throw;
    }
}

void ShapeBuilderWidget::createFaceFromVertex()
{
    Gui::SelectionFilter vertexFilter  ("SELECT Part::Feature SUBELEMENT Vertex COUNT 3..");
    bool matchVertex = vertexFilter.match();
    if (!matchVertex) {
        QMessageBox::critical(this, tr("Wrong selection"), tr("Select three or more vertices"));
        return;
    }

    std::vector<Gui::SelectionObject> sel = vertexFilter.Result[0];
    std::vector<Gui::SelectionObject>::iterator it;
    std::vector<std::string>::const_iterator jt;

    QString list;
    QTextStream str(&list);
    str << "[";
    for (it=sel.begin();it!=sel.end();++it) {
        for (jt=it->getSubNames().begin();jt!=it->getSubNames().end();++jt) {
            str << "App.ActiveDocument." << it->getFeatName() << ".Shape." << jt->c_str() << ".Point, ";
        }
    }
    str << "]";

    QString cmd;
    if (d->ui.checkPlanar->isChecked()) {
        cmd = QString::fromLatin1(
            "_=Part.Face(Part.makePolygon(%1, True))\n"
            "if _.isNull(): raise RuntimeError('Failed to create face')\n"
            "App.ActiveDocument.addObject('Part::Feature','Face').Shape=_\n"
            "del _\n"
        ).arg(list);
    }
    else {
        cmd = QString::fromLatin1(
            "_=Part.makeFilledFace(Part.makePolygon(%1, True).Edges)\n"
            "if _.isNull(): raise RuntimeError('Failed to create face')\n"
            "App.ActiveDocument.addObject('Part::Feature','Face').Shape=_\n"
            "del _\n"
        ).arg(list);
    }

    try {
        Gui::Application::Instance->activeDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Face"));
        Gui::Command::runCommand(Gui::Command::App, cmd.toLatin1());
        Gui::Application::Instance->activeDocument()->commitCommand();
    }
    catch (const Base::Exception&) {
        Gui::Application::Instance->activeDocument()->abortCommand();
        throw;
    }
}

void ShapeBuilderWidget::createFaceFromEdge()
{
    Gui::SelectionFilter edgeFilter  ("SELECT Part::Feature SUBELEMENT Edge COUNT 1..");
    bool matchEdge = edgeFilter.match();
    if (!matchEdge) {
        QMessageBox::critical(this, tr("Wrong selection"), tr("Select one or more edges"));
        return;
    }

    std::vector<Gui::SelectionObject> sel = edgeFilter.Result[0];
    std::vector<Gui::SelectionObject>::iterator it;
    std::vector<std::string>::const_iterator jt;

    QString list;
    QTextStream str(&list);
    str << "[";
    for (it=sel.begin();it!=sel.end();++it) {
        for (jt=it->getSubNames().begin();jt!=it->getSubNames().end();++jt) {
            str << "App.ActiveDocument." << it->getFeatName() << ".Shape." << jt->c_str() << ", ";
        }
    }
    str << "]";

    QString cmd;
    if (d->ui.checkPlanar->isChecked()) {
        cmd = QString::fromLatin1(
            "_=Part.Face(Part.Wire(Part.__sortEdges__(%1)))\n"
            "if _.isNull(): raise RuntimeError('Failed to create face')\n"
            "App.ActiveDocument.addObject('Part::Feature','Face').Shape=_\n"
            "del _\n"
        ).arg(list);
    }
    else {
        cmd = QString::fromLatin1(
            "_=Part.makeFilledFace(Part.__sortEdges__(%1))\n"
            "if _.isNull(): raise RuntimeError('Failed to create face')\n"
            "App.ActiveDocument.addObject('Part::Feature','Face').Shape=_\n"
            "del _\n"
        ).arg(list);
    }

    try {
        Gui::Application::Instance->activeDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Face"));
        Gui::Command::runCommand(Gui::Command::App, cmd.toLatin1());
        Gui::Application::Instance->activeDocument()->commitCommand();
    }
    catch (const Base::Exception&) {
        Gui::Application::Instance->activeDocument()->abortCommand();
        throw;
    }
}

void ShapeBuilderWidget::createShellFromFace()
{
    Gui::SelectionFilter faceFilter  ("SELECT Part::Feature SUBELEMENT Face COUNT 2..");
    bool matchFace = faceFilter.match();
    if (!matchFace) {
        QMessageBox::critical(this, tr("Wrong selection"), tr("Select two or more faces"));
        return;
    }

    std::vector<Gui::SelectionObject> sel = faceFilter.Result[0];

    QString list;
    QTextStream str(&list);
    if (d->ui.checkFaces->isChecked()) {
        std::set<const App::DocumentObject*> obj;
        for (const auto& it : sel)
            obj.insert(it.getObject());
        str << "[]";
        for (auto it : obj) {
            str << "+ App.ActiveDocument." << it->getNameInDocument() << ".Shape.Faces";
        }
    }
    else {
        str << "[";
        for (const auto& it : sel) {
            for (const auto& jt : it.getSubNames()) {
                str << "App.ActiveDocument." << it.getFeatName() << ".Shape." << jt.c_str() << ", ";
            }
        }
        str << "]";
    }

    QString cmd;
    if (d->ui.checkRefine->isEnabled() && d->ui.checkRefine->isChecked()) {
        cmd = QString::fromLatin1(
            "_=Part.Shell(%1)\n"
            "if _.isNull(): raise RuntimeError('Failed to create shell')\n"
            "App.ActiveDocument.addObject('Part::Feature','Shell').Shape=_.removeSplitter()\n"
            "del _\n"
        ).arg(list);
    }
    else {
        cmd = QString::fromLatin1(
            "_=Part.Shell(%1)\n"
            "if _.isNull(): raise RuntimeError('Failed to create shell')\n"
            "App.ActiveDocument.addObject('Part::Feature','Shell').Shape=_\n"
            "del _\n"
        ).arg(list);
    }

    try {
        Gui::Application::Instance->activeDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Shell"));
        Gui::Command::runCommand(Gui::Command::App, cmd.toLatin1());
        Gui::Application::Instance->activeDocument()->commitCommand();
    }
    catch (const Base::Exception&) {
        Gui::Application::Instance->activeDocument()->abortCommand();
        throw;
    }
}

void ShapeBuilderWidget::createSolidFromShell()
{
    Gui::SelectionFilter partFilter  ("SELECT Part::Feature COUNT 1");
    bool matchPart = partFilter.match();
    if (!matchPart) {
        QMessageBox::critical(this, tr("Wrong selection"), tr("Select only one part object"));
        return;
    }

    QString line;
    QTextStream str(&line);

    std::vector<Gui::SelectionObject> sel = partFilter.Result[0];
    std::vector<Gui::SelectionObject>::iterator it;
    for (it=sel.begin();it!=sel.end();++it) {
        str << "App.ActiveDocument." << it->getFeatName() << ".Shape";
        break;
    }

    QString cmd;
    if (d->ui.checkRefine->isEnabled() && d->ui.checkRefine->isChecked()) {
        cmd = QString::fromLatin1(
            "shell=%1\n"
            "if shell.ShapeType != 'Shell': raise RuntimeError('Part object is not a shell')\n"
            "_=Part.Solid(shell)\n"
            "if _.isNull(): raise RuntimeError('Failed to create solid')\n"
            "App.ActiveDocument.addObject('Part::Feature','Solid').Shape=_.removeSplitter()\n"
            "del _\n"
        ).arg(line);
    }
    else {
        cmd = QString::fromLatin1(
            "shell=%1\n"
            "if shell.ShapeType != 'Shell': raise RuntimeError('Part object is not a shell')\n"
            "_=Part.Solid(shell)\n"
            "if _.isNull(): raise RuntimeError('Failed to create solid')\n"
            "App.ActiveDocument.addObject('Part::Feature','Solid').Shape=_\n"
            "del _\n"
        ).arg(line);
    }

    try {
        Gui::Application::Instance->activeDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Solid"));
        Gui::Command::runCommand(Gui::Command::App, cmd.toLatin1());
        Gui::Application::Instance->activeDocument()->commitCommand();
    }
    catch (const Base::Exception&) {
        Gui::Application::Instance->activeDocument()->abortCommand();
        throw;
    }
}

void ShapeBuilderWidget::switchMode(int mode)
{
    Gui::Selection().clearSelection();
    if (mode == 0) {
        d->gate->setMode(ShapeSelection::VERTEX);
        d->ui.label->setText(tr("Select two vertices to create an edge"));
        d->ui.checkPlanar->setEnabled(false);
        d->ui.checkFaces->setEnabled(false);
        d->ui.checkRefine->setEnabled(false);
    }
    else if (mode == 1) {
        d->gate->setMode(ShapeSelection::EDGE);
        d->ui.label->setText(tr("Select adjacent edges"));
        d->ui.checkPlanar->setEnabled(true);
        d->ui.checkFaces->setEnabled(false);
        d->ui.checkRefine->setEnabled(false);
    }
    else if (mode == 2) {
        d->gate->setMode(ShapeSelection::VERTEX);
        d->ui.label->setText(tr("Select a list of vertices"));
        d->ui.checkPlanar->setEnabled(true);
        d->ui.checkFaces->setEnabled(false);
        d->ui.checkRefine->setEnabled(false);
    }
    else if (mode == 3) {
        d->gate->setMode(ShapeSelection::EDGE);
        d->ui.label->setText(tr("Select a closed set of edges"));
        d->ui.checkPlanar->setEnabled(true);
        d->ui.checkFaces->setEnabled(false);
        d->ui.checkRefine->setEnabled(false);
    }
    else if (mode == 4) {
        d->gate->setMode(ShapeSelection::FACE);
        d->ui.label->setText(tr("Select adjacent faces"));
        d->ui.checkPlanar->setEnabled(false);
        d->ui.checkFaces->setEnabled(true);
        d->ui.checkRefine->setEnabled(true);
    }
    else {
        d->gate->setMode(ShapeSelection::ALL);
        d->ui.label->setText(tr("All shape types can be selected"));
        d->ui.checkPlanar->setEnabled(false);
        d->ui.checkFaces->setEnabled(false);
        d->ui.checkRefine->setEnabled(true);
    }
}

bool ShapeBuilderWidget::accept()
{
    return true;
}

bool ShapeBuilderWidget::reject()
{
    return true;
}

void ShapeBuilderWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
    }
}


/* TRANSLATOR PartGui::TaskShapeBuilder */

TaskShapeBuilder::TaskShapeBuilder()
{
    widget = new ShapeBuilderWidget();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Shapebuilder"),
        widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskShapeBuilder::~TaskShapeBuilder() = default;

void TaskShapeBuilder::open()
{
}

void TaskShapeBuilder::clicked(int)
{
}

bool TaskShapeBuilder::accept()
{
    return widget->accept();
}

bool TaskShapeBuilder::reject()
{
    return widget->reject();
}

#include "moc_TaskShapeBuilder.cpp"
