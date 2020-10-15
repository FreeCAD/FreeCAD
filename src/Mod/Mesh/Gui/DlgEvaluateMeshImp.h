/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESHGUI_DLG_EVALUATE_MESH_IMP_H
#define MESHGUI_DLG_EVALUATE_MESH_IMP_H

#include <map>
#include <QPointer>
#include <QDialog>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObserver.h>

class QAbstractButton;

namespace Gui {
class View3DInventor;
}
namespace Mesh {
  class Feature;
}

namespace MeshGui {
class ViewProviderMeshDefects;

/**
 * The GuiCleanupHandler class is used to cleanup GUI elements from the MeshGui
 * module when the application is about to be closed.
 * @author Werner Mayer
 */
class CleanupHandler : public QObject
{
    Q_OBJECT

public:
    CleanupHandler();

public Q_SLOTS:
    void cleanup();
};

/**
 * \author Werner Mayer
 */
class DlgEvaluateMeshImp : public QDialog, public App::DocumentObserver
{ 
    Q_OBJECT

public:
    DlgEvaluateMeshImp(QWidget* parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgEvaluateMeshImp();

    void setMesh(Mesh::Feature*);

private:
    /** Checks if the given document is about to be closed */
    void slotDeletedDocument(const App::Document& Doc);
    /** Checks if a new object was added. */
    void slotCreatedObject(const App::DocumentObject& Obj);
    /** Checks if the given object is about to be removed. */
    void slotDeletedObject(const App::DocumentObject& Obj);
    /** The property of an observed object has changed */
    void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop);

protected Q_SLOTS:
    void on_checkOrientationButton_clicked();
    void on_analyzeOrientationButton_clicked();
    void on_repairOrientationButton_clicked();

    void on_checkDuplicatedFacesButton_clicked();
    void on_analyzeDuplicatedFacesButton_clicked();
    void on_repairDuplicatedFacesButton_clicked();

    void on_checkDuplicatedPointsButton_clicked();
    void on_analyzeDuplicatedPointsButton_clicked();
    void on_repairDuplicatedPointsButton_clicked();

    void on_checkNonmanifoldsButton_clicked();
    void on_analyzeNonmanifoldsButton_clicked();
    void on_repairNonmanifoldsButton_clicked();

    void on_checkDegenerationButton_clicked();
    void on_analyzeDegeneratedButton_clicked();
    void on_repairDegeneratedButton_clicked();

    void on_checkIndicesButton_clicked();
    void on_analyzeIndicesButton_clicked();
    void on_repairIndicesButton_clicked();

    void on_checkSelfIntersectionButton_clicked();
    void on_analyzeSelfIntersectionButton_clicked();
    void on_repairSelfIntersectionButton_clicked();

    void on_checkFoldsButton_clicked();
    void on_analyzeFoldsButton_clicked();
    void on_repairFoldsButton_clicked();

    void on_analyzeAllTogether_clicked();
    void on_repairAllTogether_clicked();

    void on_refreshButton_clicked();
    void on_meshNameButton_activated(int);
    void on_buttonBox_clicked(QAbstractButton *);

protected:
    void refreshList();
    void showInformation();
    void cleanInformation();
    void addViewProvider(const char* vp, const std::vector<unsigned long>& indices);
    void removeViewProvider(const char* vp);
    void removeViewProviders();
    void changeEvent(QEvent *e);

private:
    class Private;
    Private* d;
};

/**
 * The DockEvaluateMeshImp class creates a single instance and embeds it into a dock window.
 * \author Werner Mayer
 */
class DockEvaluateMeshImp : public DlgEvaluateMeshImp
{ 
    Q_OBJECT

protected:
    DockEvaluateMeshImp( QWidget* parent = 0, Qt::WindowFlags fl = Qt::WindowFlags() );
    ~DockEvaluateMeshImp();
    void closeEvent(QCloseEvent* e);

public:
    static DockEvaluateMeshImp* instance();
    static void destruct();
    static bool hasInstance();
  
    QSize sizeHint () const;

private:
    static DockEvaluateMeshImp* _instance;
};

} // namespace MeshGui

#endif // MESHGUI_DLG_EVALUATE_MESH_IMP_H
