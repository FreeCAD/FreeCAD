/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "Selection.h"
#include "ui_Selection.h"


using namespace MeshGui;

/* TRANSLATOR MeshGui::Selection */

Selection::Selection(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_Selection())
{
    ui->setupUi(this);
    setupConnections();
    ui->addSelection->installEventFilter(this);
    ui->clearSelection->installEventFilter(this);

    meshSel.setCheckOnlyVisibleTriangles(ui->visibleTriangles->isChecked());
    meshSel.setCheckOnlyPointToUserTriangles(ui->screenTriangles->isChecked());
    meshSel.setEnabledViewerSelection(false);
}

/*
 *  Destroys the object and frees any allocated resources
 */
Selection::~Selection()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
    meshSel.clearSelection();
    meshSel.setEnabledViewerSelection(true);
}

void Selection::setupConnections()
{
    // clang-format off
    connect(ui->addSelection, &QPushButton::clicked,
            this, &Selection::onAddSelectionClicked);
    connect(ui->clearSelection, &QPushButton::clicked,
            this, &Selection::onClearSelectionClicked);
    connect(ui->visibleTriangles, &QPushButton::clicked,
            this, &Selection::onVisibleTrianglesToggled);
    connect(ui->screenTriangles, &QPushButton::clicked,
            this, &Selection::onScreenTrianglesToggled);
    // clang-format on
}

void Selection::setObjects(const std::vector<Gui::SelectionObject>& o)
{
    meshSel.setObjects(o);
}

std::vector<App::DocumentObject*> Selection::getObjects() const
{
    return meshSel.getObjects();
}

bool Selection::eventFilter(QObject* o, QEvent* e)
{
    if (e->type() == QEvent::HoverEnter) {
        if (o == ui->addSelection) {
            ui->label->setText(tr("Use a brush tool to select the area"));
        }
        else if (o == ui->clearSelection) {
            ui->label->setText(tr("Clears completely the selected area"));
        }
    }
    else if (e->type() == QEvent::HoverLeave) {
        if (o == ui->addSelection) {
            ui->label->clear();
        }
        else if (o == ui->clearSelection) {
            ui->label->clear();
        }
    }

    return false;
}

void Selection::onAddSelectionClicked()
{
    meshSel.startSelection();
}

void Selection::onClearSelectionClicked()
{
    meshSel.clearSelection();
}

void Selection::onVisibleTrianglesToggled(bool on)
{
    meshSel.setCheckOnlyVisibleTriangles(on);
}

void Selection::onScreenTrianglesToggled(bool on)
{
    meshSel.setCheckOnlyPointToUserTriangles(on);
}

#include "moc_Selection.cpp"
