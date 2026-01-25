// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <QPointer>
#include <memory>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Mesh/Gui/RemeshGmsh.h>


namespace App
{
class Document;
class SubObjectT;
}  // namespace App
namespace MeshPartGui
{

/**
 * Non-modal dialog to mesh a shape.
 * @author Werner Mayer
 */
class Mesh2ShapeGmsh: public MeshGui::GmshWidget
{
    Q_OBJECT

public:
    explicit Mesh2ShapeGmsh(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~Mesh2ShapeGmsh() override;

    void process(App::Document* doc, const std::list<App::SubObjectT>&);

Q_SIGNALS:
    void processed();

protected:
    bool writeProject(QString& inpFile, QString& outFile) override;
    bool loadOutput() override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

class Ui_Tessellation;
class Tessellation: public QWidget
{
    Q_OBJECT

    enum
    {
        Standard,
        Mefisto,
        Netgen,
        Gmsh
    };

    enum
    {
        VeryCoarse = 0,
        Coarse = 1,
        Moderate = 2,
        Fine = 3,
        VeryFine = 4
    };

public:
    explicit Tessellation(QWidget* parent = nullptr);
    ~Tessellation() override;
    bool accept();

protected:
    void changeEvent(QEvent* e) override;
    void process(int method, App::Document* doc, const std::list<App::SubObjectT>&);
    void saveParameters(int method);
    void setFaceColors(int method, App::Document* doc, App::DocumentObject* obj);
    void addFaceColors(Mesh::Feature* mesh, const std::vector<Base::Color>& colorPerSegm);
    QString getMeshingParameters(int method, App::DocumentObject* obj) const;
    QString getStandardParameters(App::DocumentObject* obj) const;
    QString getMefistoParameters() const;
    QString getNetgenParameters() const;
    std::vector<Base::Color> getUniqueColors(const std::vector<Base::Color>& colors) const;

private:
    void setupConnections();
    void meshingMethod(int id);
    void onEstimateMaximumEdgeLengthClicked();
    void onComboFinenessCurrentIndexChanged(int);
    void onCheckSecondOrderToggled(bool);
    void onCheckQuadDominatedToggled(bool);
    void gmshProcessed();

private:
    QString document;
    QPointer<Mesh2ShapeGmsh> gmsh;
    std::unique_ptr<Ui_Tessellation> ui;
};

class TaskTessellation: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskTessellation();

public:
    void open() override;
    void clicked(int) override;
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    Tessellation* widget;
};

}  // namespace MeshPartGui
