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

#include <App/DocumentObserver.h>
#include <Gui/DocumentObserver.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#include "ViewProviderFemPostFunction.h"


class QComboBox;
class Ui_TaskPostDisplay;
class Ui_TaskPostClip;
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


namespace FemGui {

class ViewProviderPointMarker;
class PointMarker : public QObject
{
    Q_OBJECT

public:
    PointMarker(Gui::View3DInventorViewer* view, std::string ObjName);
    ~PointMarker();

    void addPoint(const SbVec3f&);
    int countPoints() const;

Q_SIGNALS:
    void PointsChanged(double x1, double y1, double z1, double x2, double y2, double z2);

protected:
    void customEvent(QEvent* e);

private:
    Gui::View3DInventorViewer *view;
    ViewProviderPointMarker *vp;
    std::string m_name;
    std::string ObjectInvisible();
};


class FemGuiExport ViewProviderPointMarker : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER(FemGui::ViewProviderPointMarker);

public:
    ViewProviderPointMarker();
    virtual ~ViewProviderPointMarker();

protected:
    SoCoordinate3    * pCoords;
    friend class PointMarker;
};


class ViewProviderDataMarker;
class DataMarker : public QObject
{
    Q_OBJECT

public:
    DataMarker(Gui::View3DInventorViewer* view, std::string ObjName);
    ~DataMarker();

    void addPoint(const SbVec3f&);
    int countPoints() const;

Q_SIGNALS:
    void PointsChanged(double x, double y, double z);

protected:
    void customEvent(QEvent* e);

private:
    Gui::View3DInventorViewer *view;
    ViewProviderDataMarker *vp;
    std::string m_name;
    std::string ObjectInvisible();
};


class FemGuiExport ViewProviderDataMarker : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER(FemGui::ViewProviderDataMarker);

public:
    ViewProviderDataMarker();
    virtual ~ViewProviderDataMarker();

protected:
    SoCoordinate3    * pCoords;
    SoMarkerSet      * pMarker;
    friend class DataMarker;
};

class TaskPostBox : public Gui::TaskView::TaskBox {

    Q_OBJECT

public:
    TaskPostBox(Gui::ViewProviderDocumentObject* view, const QPixmap &icon, const QString &title, QWidget *parent = nullptr);
    ~TaskPostBox();

    virtual void applyPythonCode() = 0;
    virtual bool isGuiTaskOnly() {return false;} //return true if only gui properties are manipulated

protected:
    App::DocumentObject* getObject() const {
        return *m_object;
    }
    template<typename T>
    T* getTypedObject() const {
        return m_object.get<T>();
    }
    Gui::ViewProviderDocumentObject* getView() const {
        return *m_view;
    }
    template<typename T>
    T* getTypedView() const {
        return m_view.get<T>();
    }

    App::Document* getDocument() const;

    bool autoApply();
    void recompute();

    static void updateEnumerationList(App::PropertyEnumeration&, QComboBox* box);

private:
    App::DocumentObjectWeakPtrT m_object;
    Gui::ViewProviderWeakPtrT   m_view;
};


/// simulation dialog for the TaskView
class TaskDlgPost : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgPost(Gui::ViewProviderDocumentObject *view);
    ~TaskDlgPost();
    void connectSlots();

    void appendBox(TaskPostBox* box);
    Gui::ViewProviderDocumentObject* getView() const {
        return *m_view;
    }

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button
    virtual bool isAllowedAlterDocument(void) const
    { return false; }
    virtual void modifyStandardButtons(QDialogButtonBox*);

    /// returns for Close and Help button
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const;

protected:
    void recompute();

protected:
    Gui::ViewProviderWeakPtrT   m_view;
    std::vector<TaskPostBox*>   m_boxes;
};


class TaskPostDisplay : public TaskPostBox
{
    Q_OBJECT

public:
    TaskPostDisplay(Gui::ViewProviderDocumentObject* view, QWidget *parent = nullptr);
    ~TaskPostDisplay();

    virtual void applyPythonCode();
    virtual bool isGuiTaskOnly() {return true;}

private Q_SLOTS:
    void on_Representation_activated(int i);
    void on_Field_activated(int i);
    void on_VectorMode_activated(int i);
    void on_Transparency_valueChanged(int i);
    void slotAddedFunction();

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostDisplay> ui;
};


class TaskPostFunction : public TaskPostBox {

    Q_OBJECT

public:
    TaskPostFunction(Gui::ViewProviderDocumentObject* view, QWidget* parent = nullptr);
    virtual ~TaskPostFunction();

    virtual void applyPythonCode();
};


class TaskPostClip : public TaskPostBox {

    Q_OBJECT

public:
    TaskPostClip(Gui::ViewProviderDocumentObject* view, App::PropertyLink* function, QWidget* parent = nullptr);
    virtual ~TaskPostClip();

    virtual void applyPythonCode();

private Q_SLOTS:
    void on_CreateButton_triggered(QAction*);
    void on_FunctionBox_currentIndexChanged(int idx);
    void on_InsideOut_toggled(bool val);
    void on_CutCells_toggled(bool val);

Q_SIGNALS:
    void emitAddedFunction();

private:
    void collectImplicitFunctions();

  //App::PropertyLink* m_functionProperty;
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostClip> ui;
    FunctionWidget* fwidget;
};


class TaskPostDataAlongLine: public TaskPostBox {

    Q_OBJECT

public:
    TaskPostDataAlongLine(Gui::ViewProviderDocumentObject* view, QWidget* parent = nullptr);
    virtual ~TaskPostDataAlongLine();

    virtual void applyPythonCode();
    static void pointCallback(void * ud, SoEventCallback * n);

private Q_SLOTS:
    void on_SelectPoints_clicked();
    void on_CreatePlot_clicked();
    void on_Representation_activated(int i);
    void on_Field_activated(int i);
    void on_VectorMode_activated(int i);
    void point2Changed(double);
    void point1Changed(double);
    void resolutionChanged(int val);
    void onChange(double x1, double y1, double z1, double x2, double y2, double z2);


private:
    std::string Plot();
    std::string ObjectVisible();
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostDataAlongLine> ui;
};


class TaskPostDataAtPoint: public TaskPostBox {

    Q_OBJECT

public:
    TaskPostDataAtPoint(Gui::ViewProviderDocumentObject* view, QWidget* parent = nullptr);
    virtual ~TaskPostDataAtPoint();

    virtual void applyPythonCode();
    static void pointCallback(void * ud, SoEventCallback * n);

private Q_SLOTS:
    void on_SelectPoint_clicked();
    void on_Field_activated(int i);
    void centerChanged(double);
    void onChange(double x, double y, double z);

private:
    std::string toString(double val) const;
    void showValue(double value, const char* unit);


private:
    std::string ObjectVisible();
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostDataAtPoint> ui;
};


class TaskPostScalarClip : public TaskPostBox {

    Q_OBJECT

public:
    TaskPostScalarClip(Gui::ViewProviderDocumentObject* view, QWidget* parent = nullptr);
    virtual ~TaskPostScalarClip();

    virtual void applyPythonCode();

private Q_SLOTS:
    void on_Slider_valueChanged(int v);
    void on_Value_valueChanged(double v);
    void on_Scalar_currentIndexChanged(int idx);
    void on_InsideOut_toggled(bool val);

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostScalarClip> ui;
};


class TaskPostWarpVector : public TaskPostBox {

    Q_OBJECT

public:
    TaskPostWarpVector(Gui::ViewProviderDocumentObject* view, QWidget* parent = nullptr);
    virtual ~TaskPostWarpVector();

    virtual void applyPythonCode();

private Q_SLOTS:
    void on_Slider_valueChanged(int v);
    void on_Value_valueChanged(double v);
    void on_Max_valueChanged(double);
    void on_Min_valueChanged(double);
    void on_Vector_currentIndexChanged(int idx);

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostWarpVector> ui;
};


class TaskPostCut : public TaskPostBox {

    Q_OBJECT

public:
    TaskPostCut(Gui::ViewProviderDocumentObject* view, App::PropertyLink* function, QWidget* parent = nullptr);
    virtual ~TaskPostCut();

    virtual void applyPythonCode();

private Q_SLOTS:
    void on_CreateButton_triggered(QAction*);
    void on_FunctionBox_currentIndexChanged(int idx);

Q_SIGNALS:
    void emitAddedFunction();

private:
    void collectImplicitFunctions();

  //App::PropertyLink* m_functionProperty;
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPostCut> ui;
    FunctionWidget* fwidget;
};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskPostDisplay_H
