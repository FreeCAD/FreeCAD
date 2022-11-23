/***************************************************************************
 *   Copyright (c) 2022 Pierre Boyer <pierrelouis.boyer google mail>       *
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


#ifndef GUI_TASKVIEW_RenderingOrderWidget_H
#define GUI_TASKVIEW_RenderingOrderWidget_H

#include <QWidget>
#include <QListWidget>
#include <QLabel>

namespace SketcherGui {

class ViewProviderSketch;

class RenderingOrderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RenderingOrderWidget(QWidget *parent = nullptr, ViewProviderSketch* sketchView = nullptr);
    ~RenderingOrderWidget() override;
    
    bool eventFilter(QObject *object, QEvent *event) override;

    void saveSettings();
    void loadSettings();
    void languageChanged();

protected:
    void changeEvent(QEvent *e) override;
    
private:
    ViewProviderSketch* sketchView;
    QLabel* label;
    QListWidget* list;
};

} //namespace SketcherGui

#endif // GUI_TASKVIEW_RenderingOrderWidget_H
