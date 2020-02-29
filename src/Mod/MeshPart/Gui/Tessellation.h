/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESHPARTGUI_TESSELLATION_H
#define MESHPARTGUI_TESSELLATION_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <App/DocumentObserver.h>
#include <Mod/Mesh/Gui/RemeshGmsh.h>
#include <memory>
#include <QPointer>

namespace App {
class Document;
class SubObjectT;
}
namespace MeshPartGui {

/**
 * Non-modal dialog to mesh a shape.
 * @author Werner Mayer
 */
class Mesh2ShapeGmsh : public MeshGui::GmshWidget
{
    Q_OBJECT

public:
    Mesh2ShapeGmsh(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~Mesh2ShapeGmsh();

    void process(App::Document* doc, const std::list<App::SubObjectT>&);

Q_SIGNALS:
    void processed();

protected:
    virtual bool writeProject(QString& inpFile, QString& outFile);
    virtual bool loadOutput();

private:
    class Private;
    std::unique_ptr<Private> d;
};

class Ui_Tessellation;
class Tessellation : public QWidget
{
    Q_OBJECT

    enum {
        Standard,
        Mefisto,
        Netgen,
        Gmsh
    };

public:
    Tessellation(QWidget* parent = 0);
    ~Tessellation();
    bool accept();

protected:
    void changeEvent(QEvent *e);

private Q_SLOTS:
    void meshingMethod(int id);
    void on_estimateMaximumEdgeLength_clicked();
    void on_comboFineness_currentIndexChanged(int);
    void on_checkSecondOrder_toggled(bool);
    void on_checkQuadDominated_toggled(bool);
    void gmshProcessed();

private:
    QString document;
    QPointer<Mesh2ShapeGmsh> gmsh;
    std::unique_ptr<Ui_Tessellation> ui;
};

class TaskTessellation : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskTessellation();
    ~TaskTessellation();

public:
    virtual void open();
    virtual void clicked(int);
    virtual bool accept();
    virtual bool reject();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

private:
    Tessellation* widget;
};

} // namespace MeshPartGui

#endif // MESHPARTGUI_TESSELLATION_H
