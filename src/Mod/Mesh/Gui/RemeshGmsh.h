/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESHGUI_REMESHGMSH_H
#define MESHGUI_REMESHGMSH_H

#include <memory>
#include <QDialog>
#include <QPointer>
#include <QProcess>
#include <App/DocumentObserver.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>

namespace Mesh {
class Feature;
}

namespace Gui {
class StatusWidget;
}

namespace MeshGui {

/**
 * Non-modal dialog to remesh an existing mesh.
 * @author Werner Mayer
 */
class MeshGuiExport GmshWidget : public QWidget
{
    Q_OBJECT

public:
    GmshWidget(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~GmshWidget();
    void accept();
    void reject();

protected:
    void changeEvent(QEvent *e);
    int meshingAlgorithm() const;
    double getAngle() const;
    double getMaxSize() const;
    double getMinSize() const;
    virtual bool writeProject(QString& inpFile, QString& outFile);
    virtual bool loadOutput();

private Q_SLOTS:
    void started();
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void errorOccurred(QProcess::ProcessError error);
    void on_killButton_clicked();
    void on_clearButton_clicked();

    void readyReadStandardError();
    void readyReadStandardOutput();

private:
    class Private;
    std::unique_ptr<Private> d;
};

/**
 * Non-modal dialog to remesh an existing mesh.
 * @author Werner Mayer
 */
class MeshGuiExport RemeshGmsh : public GmshWidget
{
    Q_OBJECT

public:
    RemeshGmsh(Mesh::Feature* mesh, QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~RemeshGmsh();

protected:
    virtual bool writeProject(QString& inpFile, QString& outFile);
    virtual bool loadOutput();

private:
    class Private;
    std::unique_ptr<Private> d;
};

/**
 * Embed the panel into a task dialog.
 */
class TaskRemeshGmsh : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskRemeshGmsh(Mesh::Feature* mesh);
    ~TaskRemeshGmsh();

public:
    void clicked(int);

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Apply | QDialogButtonBox::Close; }
    virtual bool isAllowedAlterDocument(void) const
    { return true; }

private:
    RemeshGmsh* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}

#endif // MESHGUI_REMESHGMSH_H
