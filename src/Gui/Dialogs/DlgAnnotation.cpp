// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QEvent>
# include <QMessageBox>
#endif

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "DlgAnnotation.h"
#include "ui_DlgAnnotation.h"
#include <Base/Color.h>
#include <App/Annotation.h>
#include <App/Document.h>
#include <App/GeoFeature.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProviderAnnotation.h>
#include <Gui/Selection/Selection.h>


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgAnnotation */

DlgAnnotation::DlgAnnotation(App::Document* doc, QWidget* parent)
    : QDialog(parent)
    , ui {new Ui_DlgAnnotation()}
    , document {doc}
{
    ui->setupUi(this);
    QFont fn;
    ui->fontSize->setValue(fn.pointSizeF());
    ensureTransaction();

    // clang-format off
    connect(ui->buttonCreate, &QPushButton::clicked,
            this, &DlgAnnotation::createAnnotation);
    // clang-format on
}

DlgAnnotation::~DlgAnnotation() = default;

void DlgAnnotation::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QDialog::changeEvent(event);
    }
}

void DlgAnnotation::ensureTransaction()
{
    if (!document.expired()) {
        if (!document->hasPendingTransaction()) {
            document->openTransaction(QT_TRANSLATE_NOOP("Command", "Add Annotation"));
        }
    }
}

void DlgAnnotation::createAnnotation()
{
    if (!document.expired()) {
        ensureTransaction();

        QString text = ui->annotationText->toPlainText();
        if (text.isEmpty()) {
            QMessageBox::warning(this, tr("No annotation"), tr("Insert an annotation text"));
            ui->annotationText->setFocus();
            return;
        }

        auto obj = document->addObject<App::AnnotationLabel>("Annotation");
        std::vector<std::string> lines;
        boost::algorithm::split(lines, text.toStdString(), boost::is_any_of("\n"));
        obj->LabelText.setValues(lines);

        auto pos = getPosition();
        obj->BasePosition.setValue(pos.base);
        obj->TextPosition.setValue(pos.text);

        auto view = Gui::Application::Instance->getViewProvider<Gui::ViewProviderAnnotationLabel>(obj);
        if (view) {
            QColor fgColor = ui->textColor->color();
            view->TextColor.setValue(Base::Color::fromValue<QColor>(fgColor));
            QColor bgColor = ui->backgroundColor->color();
            view->BackgroundColor.setValue(Base::Color::fromValue<QColor>(bgColor));
            view->FontSize.setValue(ui->fontSize->value());
            view->FontName.setValue(ui->fontComboBox->currentText().toStdString());
            view->Frame.setValue(ui->checkBoxFrame->isChecked());
        }
    }
}

DlgAnnotation::Position DlgAnnotation::getPosition() const
{
    Position pos;

    auto select = Gui::Selection().getSelectionEx();
    if (!select.empty()) {
        const auto& selobj = select.front();
        const auto* obj = selobj.getObject();

        const auto& pts = selobj.getPickedPoints();
        if (!pts.empty()) {
            pos.base = pts.front();
        }
        else if (const auto* geo = freecad_cast<App::GeoFeature*>(obj)) {
            auto plm = geo->globalPlacement();
            pos.base = plm.getPosition();
        }

        if (const auto* geo = freecad_cast<App::GeoFeature*>(obj)) {
            if (const auto* data = geo->getPropertyOfGeometry()) {
                auto bbox = data->getBoundingBox();
                Base::Vector3d cnt = bbox.GetCenter();
                pos.text = pos.base - cnt;
            }
        }
    }

    return pos;
}

void DlgAnnotation::accept()
{
    if (!document.expired()) {
        document->commitTransaction();
    }
    QDialog::accept();
}

void DlgAnnotation::reject()
{
    if (!document.expired()) {
        document->abortTransaction();
    }
    QDialog::reject();
}

// ---------------------------------------

TaskAnnotation::TaskAnnotation(App::Document* doc)
    : dialog {new DlgAnnotation(doc)}
{
    addTaskBox(Gui::BitmapFactory().pixmap("Tree_Annotation"), dialog);
}

TaskAnnotation::~TaskAnnotation() = default;

bool TaskAnnotation::accept()
{
    dialog->accept();
    return (dialog->result() == QDialog::Accepted);
}

bool TaskAnnotation::reject()
{
    dialog->reject();
    return (dialog->result() == QDialog::Rejected);
}

#include "moc_DlgAnnotation.cpp"
