/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#ifndef GUI_TASKVIEW_TaskPostDisplay_H
#define GUI_TASKVIEW_TaskPostDisplay_H

#include <Gui/DocumentObserver.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/ViewProviderDocumentObject.h>

#include "ViewProviderFemPostFunction.h"


class QComboBox;
class Ui_TaskPostDisplay;
class Ui_TaskPostClip;
class Ui_TaskPostContours;
class Ui_TaskPostDataAlongLine;
class Ui_TaskPostDataAtPoint;
class Ui_TaskPostScalarClip;
class Ui_TaskPostWarpVector;
class Ui_TaskPostCut;

class SoFontStyle;
class SoText2;
class SoBaseColor;
class SoTranslation;
class SoCoordinate3;
class SoIndexedLineSet;
class SoEventCallback;
class SoMarkerSet;

namespace Fem
{
class FemPostDataAlongLineFilter;
class FemPostDataAtPointFilter;
}  // namespace Fem

namespace FemGui
{

// ***************************************************************************
// point marker
class ViewProviderPointMarker;

class PointMarker: public QObject
{
    Q_OBJECT

public:
    PointMarker(Gui::View3DInventorViewer* view, App::DocumentObject* obj);
    ~PointMarker() override;

    void addPoint(const SbVec3f&);
    void clearPoints() const;
    int countPoints() const;
    SbVec3f getPoint(int idx) const;
    void setPoint(int idx, const SbVec3f& pt) const;
    Gui::View3DInventorViewer* getView() const;
    App::DocumentObject* getObject() const;
    template<class T>
    T* getObject() const
    {
        return Base::freecad_dynamic_cast<T>(getObject());
    }
    QMetaObject::Connection connSelectPoint;

protected:
    std::string ObjectInvisible();

private:
    Gui::View3DInventorViewer* view;
    App::DocumentObject* obj;
    ViewProviderPointMarker* vp;
};


class FemGuiExport ViewProviderPointMarker: public Gui::ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderPointMarker);

public:
    ViewProviderPointMarker();
    ~ViewProviderPointMarker() override;

protected:
    SoCoordinate3* pCoords;
    SoMarkerSet* pMarker;
    friend class PointMarker;
};


// ***************************************************************************
// DataAlongLine markers
class DataAlongLineMarker: public PointMarker
{
    Q_OBJECT

public:
    DataAlongLineMarker(Gui::View3DInventorViewer* view, Fem::FemPostDataAlongLineFilter* obj);

Q_SIGNALS:
    void PointsChanged(double x1, double y1, double z1, double x2, double y2, double z2);

protected:
    void customEvent(QEvent* e) override;
};


// ***************************************************************************
// main task dialog
class TaskPostBox: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskPostBox(Gui::ViewProviderDocumentObject* view,
                const QPixmap& icon,
                const QString& title,
                QWidget* parent = nullptr);
    ~TaskPostBox() override;

    virtual void applyPythonCode() = 0;
    virtual bool isGuiTaskOnly()
    {
        return false;
    }  // return true if only gui properties are manipulated

protected:
    App::DocumentObject* getObject() const
    {
        return *m_object;
    }
    template<class T>
    T* getObject() const
    {
        return Base::freecad_dynamic_cast<T>(getObject());
    }
    template<typename T>
    T* getTypedObject() const
    {
        return m_object.get<T>();
    }
    Gui::ViewProviderDocumentObject* getView() const
    {
        return *m_view;
    }
    template<typename T>
    T* getTypedView() const
    {
        return m_view.get<T>();
    }

    App::Document* getDocument() const;

    bool autoApply();
    void recompute();

    static void updateEnumerationList(App::PropertyEnumeration&, QComboBox* box);

private:
    App::DocumentObjectWeakPtrT m_object;
    Gui::ViewProviderWeakPtrT m_view;
};


// ***************************************************************************
// simulation dialog for the TaskView
class TaskDlgPost: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgPost(Gui::ViewProviderDocumentObject* view);
    ~TaskDlgPost() override;
    void connectSlots();

    void appendBox(TaskPostBox* box);
    Gui::ViewProviderDocumentObject* getView() const
    {
        return *m_view;
    }

public:
    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user presses the help button
    bool isAllowedAlterDocument() const override
    {
        return false;
    }
    void modifyStandardButtons(QDialogButtonBox*) override;

    /// returns for Close and Help button
    QDialogButtonBox::StandardButtons getStandardButtons() const override;

protected:
    void recompute();

protected:
    Gui::ViewProviderWeakPtrT m_view;
    std::vector<TaskPostBox*> m_boxes;
};


// ***************************************************************************
// box to set the coloring
class ViewProviderFemPostObject;

class TaskPostDisplay: public TaskPostBox
{
    Q_OBJECT

public:
    explicit TaskPostDisplay(ViewProviderFemPostObject* view, QWidget* parent = nullptr);
    ~TaskPostDisplay() override;

    void applyPythonCode() override;
    bool isGuiTaskOnly() override
    {
        return true;
    }

private:
    void setupConnections();
    void onRepresentationActivated(int i);
    void onFieldActivated(int i);
    void onVectorModeActivated(int i);
    void onTransparencyValueChanged(int i);
    void slotAddedFunction();

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostDisplay> ui;
};


// ***************************************************************************
// functions
class ViewProviderFemPostFunction;

class TaskPostFunction: public TaskPostBox
{
    Q_OBJECT

public:
    explicit TaskPostFunction(ViewProviderFemPostFunction* view, QWidget* parent = nullptr);
    ~TaskPostFunction() override;

    void applyPythonCode() override;
};


// ***************************************************************************
// in the following, the different filters sorted alphabetically
// ***************************************************************************


// ***************************************************************************
// data along line filter
class ViewProviderFemPostDataAlongLine;

class TaskPostDataAlongLine: public TaskPostBox
{
    Q_OBJECT

public:
    explicit TaskPostDataAlongLine(ViewProviderFemPostDataAlongLine* view,
                                   QWidget* parent = nullptr);
    ~TaskPostDataAlongLine() override;

    void applyPythonCode() override;
    static void pointCallback(void* ud, SoEventCallback* n);

private:
    void setupConnectionsStep1();
    void setupConnectionsStep2();
    void onSelectPointsClicked();
    void onCreatePlotClicked();
    void onRepresentationActivated(int i);
    void onFieldActivated(int i);
    void onVectorModeActivated(int i);
    void point2Changed(double);
    void point1Changed(double);
    void resolutionChanged(int val);
    void onChange(double x1, double y1, double z1, double x2, double y2, double z2);

private:
    std::string Plot();
    std::string ObjectVisible();
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostDataAlongLine> ui;
    DataAlongLineMarker* marker;
};


// ***************************************************************************
// data at point filter
class ViewProviderFemPostDataAtPoint;

class TaskPostDataAtPoint: public TaskPostBox
{
    Q_OBJECT

public:
    explicit TaskPostDataAtPoint(ViewProviderFemPostDataAtPoint* view, QWidget* parent = nullptr);
    ~TaskPostDataAtPoint() override;

    void applyPythonCode() override;
    static void pointCallback(void* ud, SoEventCallback* n);

Q_SIGNALS:
    void PointsChanged(double x, double y, double z);

protected:
    Gui::View3DInventorViewer* viewer;
    QMetaObject::Connection connSelectPoint;

private:
    void setupConnections();
    void onSelectPointClicked();
    void onFieldActivated(int i);
    void centerChanged(double);
    void onChange(double x, double y, double z);

    std::string toString(double val) const;
    void showValue(double value, const char* unit);
    std::string objectVisible(bool visible) const;
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostDataAtPoint> ui;
};


// ***************************************************************************
// clip filter
class ViewProviderFemPostClip;

class TaskPostClip: public TaskPostBox
{
    Q_OBJECT

public:
    TaskPostClip(ViewProviderFemPostClip* view,
                 App::PropertyLink* function,
                 QWidget* parent = nullptr);
    ~TaskPostClip() override;

    void applyPythonCode() override;

private:
    void setupConnections();
    void onCreateButtonTriggered(QAction*);
    void onFunctionBoxCurrentIndexChanged(int idx);
    void onInsideOutToggled(bool val);
    void onCutCellsToggled(bool val);

Q_SIGNALS:
    void emitAddedFunction();

private:
    void collectImplicitFunctions();

    // App::PropertyLink* m_functionProperty;
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostClip> ui;
    FunctionWidget* fwidget;
};


// ***************************************************************************
// contours filter
class ViewProviderFemPostContours;

class TaskPostContours: public TaskPostBox
{
    Q_OBJECT

public:
    explicit TaskPostContours(ViewProviderFemPostContours* view, QWidget* parent = nullptr);
    ~TaskPostContours() override;

    void applyPythonCode() override;

private:
    void onFieldsChanged(int idx);
    void onVectorModeChanged(int idx);
    void onNumberOfContoursChanged(int number);
    void onNoColorChanged(bool state);
    void onSmoothingChanged(bool state);
    void onRelaxationChanged(double v);

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostContours> ui;
    bool blockVectorUpdate = false;
    void updateFields();
};


// ***************************************************************************
// cut filter
class ViewProviderFemPostCut;

class TaskPostCut: public TaskPostBox
{
    Q_OBJECT

public:
    TaskPostCut(ViewProviderFemPostCut* view,
                App::PropertyLink* function,
                QWidget* parent = nullptr);
    ~TaskPostCut() override;

    void applyPythonCode() override;

private:
    void setupConnections();
    void onCreateButtonTriggered(QAction*);
    void onFunctionBoxCurrentIndexChanged(int idx);

Q_SIGNALS:
    void emitAddedFunction();

private:
    void collectImplicitFunctions();

    // App::PropertyLink* m_functionProperty;
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostCut> ui;
    FunctionWidget* fwidget;
};


// ***************************************************************************
// scalar clip filter
class ViewProviderFemPostScalarClip;

class TaskPostScalarClip: public TaskPostBox
{
    Q_OBJECT

public:
    explicit TaskPostScalarClip(ViewProviderFemPostScalarClip* view, QWidget* parent = nullptr);
    ~TaskPostScalarClip() override;

    void applyPythonCode() override;

private:
    void setupConnections();
    void onSliderValueChanged(int v);
    void onValueValueChanged(double v);
    void onScalarCurrentIndexChanged(int idx);
    void onInsideOutToggled(bool val);

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostScalarClip> ui;
};


// ***************************************************************************
// warp vector filter
class ViewProviderFemPostWarpVector;

class TaskPostWarpVector: public TaskPostBox
{
    Q_OBJECT

public:
    explicit TaskPostWarpVector(ViewProviderFemPostWarpVector* view, QWidget* parent = nullptr);
    ~TaskPostWarpVector() override;

    void applyPythonCode() override;

private:
    void setupConnections();
    void onSliderValueChanged(int v);
    void onValueValueChanged(double v);
    void onMaxValueChanged(double);
    void onMinValueChanged(double);
    void onVectorCurrentIndexChanged(int idx);

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostWarpVector> ui;
};

}  // namespace FemGui

#endif  // GUI_TASKVIEW_TaskPostDisplay_H
