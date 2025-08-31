/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QPainter>
#include <QPaintEvent>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>

#include "DlgInspectAppearance.h"
#include "ui_DlgInspectAppearance.h"


using namespace MatGui;

ColorWidget::ColorWidget(const Base::Color& color, QWidget* parent)
    : QWidget(parent)
{
    _color = color.asValue<QColor>();
}

void ColorWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    // Draw a white background
    auto color = QColor(255, 255, 255);
    auto left = event->rect().left();
    auto width = event->rect().width();
    painter.fillRect(left, event->rect().top(), width, event->rect().height(), QBrush(color));

    // Draw a black border
    color = QColor(0, 0, 0);
    left = event->rect().left() + 2;
    width = event->rect().width() - 4;
    if (event->rect().width() > 75) {
        left += (event->rect().width() - 75) / 2;
        width = 71;
    }
    painter.fillRect(left, event->rect().top() + 2, width, event->rect().height() - 4, QBrush(color));

    // Draw the color
    left = event->rect().left() + 5;
    width = event->rect().width() - 10;
    if (event->rect().width() > 75) {
        left += (event->rect().width() - 75) / 2;
        width = 65;
    }
    painter.fillRect(left, event->rect().top() + 5, width, event->rect().height() - 10, QBrush(_color));
}

/* TRANSLATOR MatGui::DlgInspectAppearance */

DlgInspectAppearance::DlgInspectAppearance(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_DlgInspectAppearance)
{
    ui->setupUi(this);

    std::vector<Gui::ViewProvider*> views = getSelection();
    update(views);

    Gui::Selection().Attach(this);
}

DlgInspectAppearance::~DlgInspectAppearance()
{
    Gui::Selection().Detach(this);
}

bool DlgInspectAppearance::accept()
{
    return true;
}

std::vector<Gui::ViewProvider*> DlgInspectAppearance::getSelection() const
{
    std::vector<Gui::ViewProvider*> views;

    // get a single selection
    std::vector<Gui::SelectionSingleton::SelObj> sel =
        Gui::Selection().getSelection(nullptr, Gui::ResolveMode::OldStyleElement, true);
    for (const auto& it : sel) {
        Gui::ViewProvider* view =
            Gui::Application::Instance->getDocument(it.pDoc)->getViewProvider(it.pObject);
        views.push_back(view);
    }

    return views;
}

/// @cond DOXERR
void DlgInspectAppearance::OnChange(Gui::SelectionSingleton::SubjectType& rCaller,
                                    Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);

    if (Reason.Type == Gui::SelectionChanges::AddSelection
        || Reason.Type == Gui::SelectionChanges::RmvSelection
        || Reason.Type == Gui::SelectionChanges::SetSelection
        || Reason.Type == Gui::SelectionChanges::ClrSelection) {
        std::vector<Gui::ViewProvider*> views = getSelection();
        update(views);
    }
}
/// @endcond

void DlgInspectAppearance::update(std::vector<Gui::ViewProvider*>& views)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc) {
        ui->editDocument->setText(QString::fromUtf8(doc->Label.getValue()));

        if (views.size() == 1) {
            auto view = dynamic_cast<Gui::ViewProviderDocumentObject*>(views[0]);
            if (!view) {
                return;
            }
            auto* obj = view->getObject();
            if (!obj) {
                return;
            }
            auto* labelProp = dynamic_cast<App::PropertyString*>(obj->getPropertyByName("Label"));
            if (labelProp) {
                ui->editObjectLabel->setText(QString::fromUtf8(labelProp->getValue()));
            }
            else {
                ui->editObjectLabel->setText(QStringLiteral(""));
            }
            ui->editObjectName->setText(QLatin1String(obj->getNameInDocument()));

            auto subElement = Gui::Selection().getSelectionEx();
            if (subElement.size() > 0) {
                auto& subObject = subElement[0];
                if (subObject.getSubNames().size() > 0) {
                    ui->editSubShape->setText(QString::fromStdString(subObject.getSubNames()[0]));
                }
                else {
                    ui->editSubShape->setText(QStringLiteral(""));
                }
            }
            else {
                ui->editSubShape->setText(QStringLiteral(""));
            }

            auto subShapeType = QString::fromUtf8(obj->getTypeId().getName());
            subShapeType.remove(subShapeType.indexOf(QStringLiteral("::")), subShapeType.size());
            ui->editSubShapeType->setText(subShapeType);
            ui->editShapeType->setText(QString::fromUtf8(obj->getTypeId().getName()));

            ui->tabAppearance->clear();
            if (labelProp && QString::fromUtf8(labelProp->getValue()).size() > 0) {
                auto* prop =
                    dynamic_cast<App::PropertyMaterialList*>(view->getPropertyByName("ShapeAppearance"));
                if (prop) {
                    for (int index = 0; index < prop->getSize(); index++) {
                        auto& material = (prop->getValues())[index];
                        auto* tab = makeAppearanceTab(material);
                        ui->tabAppearance->addTab(tab, QString::number(index));
                    }
                }
            }
        }
    }
}

QWidget* DlgInspectAppearance::makeAppearanceTab(const App::Material& material)
{
    QWidget* tab = new QWidget(this);

    auto* grid = new QGridLayout();
    tab->setLayout(grid);

    int row = 0;
    auto* labelDiffuse = new QLabel();
    labelDiffuse->setText(tr("Diffuse color"));
    auto* colorDiffuse = new ColorWidget(material.diffuseColor);
    colorDiffuse->setMaximumHeight(23);

    grid->addWidget(labelDiffuse, row, 0);
    grid->addWidget(colorDiffuse, row, 1);
    row += 1;

    auto* labelAmbient = new QLabel();
    labelAmbient->setText(tr("Ambient color"));
    auto* colorAmbient = new ColorWidget(material.ambientColor);
    colorAmbient->setMaximumHeight(23);

    grid->addWidget(labelAmbient, row, 0);
    grid->addWidget(colorAmbient, row, 1);
    row += 1;

    auto* labelEmissive = new QLabel();
    labelEmissive->setText(tr("Emissive color"));
    auto* colorEmissive = new ColorWidget(material.emissiveColor);
    colorEmissive->setMaximumHeight(23);

    grid->addWidget(labelEmissive, row, 0);
    grid->addWidget(colorEmissive, row, 1);
    row += 1;

    auto* labelSpecular = new QLabel();
    labelSpecular->setText(tr("Specular color"));
    auto* colorSpecular = new ColorWidget(material.specularColor);
    colorSpecular->setMaximumHeight(23);

    grid->addWidget(labelSpecular, row, 0);
    grid->addWidget(colorSpecular, row, 1);
    row += 1;

    auto* labelShininess = new QLabel();
    labelShininess->setText(tr("Shininess"));
    auto* editShininess = new QLineEdit();
    editShininess->setText(QString::number(material.shininess));
    editShininess->setEnabled(false);

    grid->addWidget(labelShininess, row, 0);
    grid->addWidget(editShininess, row, 1);
    row += 1;

    auto* labelTransparency = new QLabel();
    labelTransparency->setText(tr("Transparency"));
    auto* editTransparency = new QLineEdit();
    editTransparency->setText(QString::number(material.transparency));
    editTransparency->setEnabled(false);

    grid->addWidget(labelTransparency, row, 0);
    grid->addWidget(editTransparency, row, 1);

    return tab;
}


/* TRANSLATOR MatGui::TaskInspectAppearance */

TaskInspectAppearance::TaskInspectAppearance()
{
    widget = new DlgInspectAppearance();
    addTaskBox(Gui::BitmapFactory().pixmap("Part_Loft"), widget);
}

TaskInspectAppearance::~TaskInspectAppearance() = default;

void TaskInspectAppearance::open()
{}

void TaskInspectAppearance::clicked(int)
{}

bool TaskInspectAppearance::accept()
{
    return widget->accept();
}

#include "moc_DlgInspectAppearance.cpp"
