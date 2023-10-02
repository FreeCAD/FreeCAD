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

#include <QDialog>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObserver.h>
#include <Mod/Mesh/App/Types.h>


class QAbstractButton;
class QScrollArea;

namespace Gui
{
class View3DInventor;
}
namespace Mesh
{
class Feature;
}

namespace MeshGui
{
class ViewProviderMeshDefects;

/**
 * The GuiCleanupHandler class is used to cleanup GUI elements from the MeshGui
 * module when the application is about to be closed.
 * @author Werner Mayer
 */
class CleanupHandler: public QObject
{
    Q_OBJECT

public:
    CleanupHandler();

private:
    void cleanup();
};

/**
 * \author Werner Mayer
 */
class DlgEvaluateMeshImp: public QDialog, public App::DocumentObserver
{
    Q_OBJECT

public:
    explicit DlgEvaluateMeshImp(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgEvaluateMeshImp() override;

    void setMesh(Mesh::Feature*);

private:
    /** Checks if the given document is about to be closed */
    void slotDeletedDocument(const App::Document& Doc) override;
    /** Checks if a new object was added. */
    void slotCreatedObject(const App::DocumentObject& Obj) override;
    /** Checks if the given object is about to be removed. */
    void slotDeletedObject(const App::DocumentObject& Obj) override;
    /** The property of an observed object has changed */
    void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop) override;

    void setupConnections();
    void onCheckOrientationButtonClicked();
    void onAnalyzeOrientationButtonClicked();
    void onRepairOrientationButtonClicked();

    void onCheckDuplicatedFacesButtonClicked();
    void onAnalyzeDuplicatedFacesButtonClicked();
    void onRepairDuplicatedFacesButtonClicked();

    void onCheckDuplicatedPointsButtonClicked();
    void onAnalyzeDuplicatedPointsButtonClicked();
    void onRepairDuplicatedPointsButtonClicked();

    void onCheckNonmanifoldsButtonClicked();
    void onAnalyzeNonmanifoldsButtonClicked();
    void onRepairNonmanifoldsButtonClicked();

    void onCheckDegenerationButtonClicked();
    void onAnalyzeDegeneratedButtonClicked();
    void onRepairDegeneratedButtonClicked();

    void onCheckIndicesButtonClicked();
    void onAnalyzeIndicesButtonClicked();
    void onRepairIndicesButtonClicked();

    void onCheckSelfIntersectionButtonClicked();
    void onAnalyzeSelfIntersectionButtonClicked();
    void onRepairSelfIntersectionButtonClicked();

    void onCheckFoldsButtonClicked();
    void onAnalyzeFoldsButtonClicked();
    void onRepairFoldsButtonClicked();

    void onAnalyzeAllTogetherClicked();
    void onRepairAllTogetherClicked();

    void onRefreshButtonClicked();
    void onMeshNameButtonActivated(int);
    void onButtonBoxClicked(QAbstractButton*);

protected:
    void refreshList();
    void showInformation();
    void cleanInformation();
    void addViewProvider(const char* vp, const std::vector<Mesh::ElementIndex>& indices);
    void removeViewProvider(const char* vp);
    void removeViewProviders();
    void changeEvent(QEvent* e) override;

private:
    class Private;
    Private* d;
};

/**
 * The DockEvaluateMeshImp class creates a single instance and embeds it into a dock window.
 * \author Werner Mayer
 */
class DockEvaluateMeshImp: public DlgEvaluateMeshImp  // NOLINT
{
    Q_OBJECT

protected:
    explicit DockEvaluateMeshImp(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DockEvaluateMeshImp() override;
    void closeEvent(QCloseEvent* e) override;

public:
    static DockEvaluateMeshImp* instance();
    static void destruct();
    static bool hasInstance();

    QSize sizeHint() const override;

private:
    QScrollArea* scrollArea;
    static DockEvaluateMeshImp* _instance;
};

}  // namespace MeshGui

#endif  // MESHGUI_DLG_EVALUATE_MESH_IMP_H
