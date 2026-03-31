// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Boyer Pierre-Louis                                 *
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


#pragma once

#include <QWidgetAction>
#include <QCoreApplication>

#include <Base/Parameter.h>

class QCheckBox;
class QLabel;
class QWidget;
class QObject;
class QListWidget;

namespace Gui
{
class Command;
class QuantitySpinBox;
}  // namespace Gui

namespace SketcherGui
{

class ViewProviderSketch;

class GridSpaceAction: public QWidgetAction
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::GridSpaceAction)
public:
    GridSpaceAction(QObject* parent);

    void updateWidget();

    void languageChange();

protected:
    QWidget* createWidget(QWidget* parent) override;

private:
    ViewProviderSketch* getView();

    ParameterGrp::handle getParameterPath();

private:
    QCheckBox* gridShow;
    QCheckBox* gridAutoSpacing;
    QCheckBox* snapToGrid;
    QLabel* sizeLabel;
    Gui::QuantitySpinBox* gridSizeBox;
};


class SnapSpaceAction: public QWidgetAction
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::SnapSpaceAction)
public:
    SnapSpaceAction(QObject* parent);

    void updateWidget(bool snapenabled);

    void languageChange();

protected:
    QWidget* createWidget(QWidget* parent) override;

private:
    ParameterGrp::handle getParameterPath();

private:
    QCheckBox* snapToObjects;
    QLabel* angleLabel;
    Gui::QuantitySpinBox* snapAngle;
};


class RenderingOrderAction: public QWidgetAction
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::RenderingOrderAction)
public:
    RenderingOrderAction(QObject* parent);

    void updateWidget();

    void languageChange();

protected:
    QWidget* createWidget(QWidget* parent) override;

private:
    ParameterGrp::handle getParameterPath();

private:
    QListWidget* list;
};

}  // namespace SketcherGui
