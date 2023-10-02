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

#ifndef FEM_VIEWPROVIDERFEMPOSTFUNCTION_H
#define FEM_VIEWPROVIDERFEMPOSTFUNCTION_H

#include <Inventor/SbBox3f.h>
#include <QWidget>
#include <boost_signals2.hpp>

#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/Fem/App/FemPostFunction.h>


class SoComposeMatrix;
class SoDragger;
class SoGroup;
class SoMatrixTransform;
class SoScale;
class SoSphere;
class SoSurroundScale;
class SoTransformManip;
class Ui_BoxWidget;
class Ui_CylinderWidget;
class Ui_PlaneWidget;
class Ui_SphereWidget;

namespace FemGui
{

class ViewProviderFemPostFunction;

class FemGuiExport FunctionWidget: public QWidget
{
    Q_OBJECT
public:
    FunctionWidget() = default;
    ~FunctionWidget() override = default;

    virtual void applyPythonCode() = 0;
    virtual void setViewProvider(ViewProviderFemPostFunction* view);
    void onObjectsChanged(const App::DocumentObject& obj, const App::Property&);

protected:
    ViewProviderFemPostFunction* getView()
    {
        return m_view;
    }
    Fem::FemPostFunction* getObject()
    {
        return m_object;
    }

    bool blockObjectUpdates()
    {
        return m_block;
    }
    void setBlockObjectUpdates(bool val)
    {
        m_block = val;
    }

    virtual void onChange(const App::Property& p) = 0;

private:
    bool m_block {false};
    ViewProviderFemPostFunction* m_view {nullptr};
    Fem::FemPostFunction* m_object {nullptr};
    boost::signals2::scoped_connection m_connection;
};

class FemGuiExport ViewProviderFemPostFunctionProvider: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostFunction);

public:
    ViewProviderFemPostFunctionProvider();
    ~ViewProviderFemPostFunctionProvider() override;

    App::PropertyFloat SizeX;
    App::PropertyFloat SizeY;
    App::PropertyFloat SizeZ;

    // handling when object is deleted
    bool onDelete(const std::vector<std::string>&) override;
    /// asks view provider if the given object can be deleted
    bool canDelete(App::DocumentObject* obj) const override;

protected:
    std::vector<App::DocumentObject*> claimChildren() const override;
    std::vector<App::DocumentObject*> claimChildren3D() const override;
    void onChanged(const App::Property* prop) override;
    void updateData(const App::Property*) override;

    void updateSize();
};

class FemGuiExport ViewProviderFemPostFunction: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostFunction);

public:
    /// constructor.
    ViewProviderFemPostFunction();
    ~ViewProviderFemPostFunction() override;

    App::PropertyFloat AutoScaleFactorX;
    App::PropertyFloat AutoScaleFactorY;
    App::PropertyFloat AutoScaleFactorZ;

    void attach(App::DocumentObject* pcObject) override;
    bool doubleClicked() override;
    std::vector<std::string> getDisplayModes() const override;

    // creates the widget used in the task dalogs, either for the function itself or for
    // the filter using it
    virtual FunctionWidget* createControlWidget()
    {
        return nullptr;
    }

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    void onChanged(const App::Property* prop) override;

    void setAutoScale(bool value)
    {
        m_autoscale = value;
    }
    bool autoScale()
    {
        return m_autoscale;
    }

    bool isDragging()
    {
        return m_isDragging;
    }
    SbBox3f getBoundingsOfView() const;
    bool findScaleFactor(double& scale) const;

    virtual SoTransformManip* setupManipulator();
    virtual void draggerUpdate(SoDragger*)
    {}
    SoTransformManip* getManipulator()
    {
        return m_manip;
    }
    SoSeparator* getGeometryNode()
    {
        return m_geometrySeperator;
    }
    SoScale* getScaleNode()
    {
        return m_scale;
    }

private:
    static void dragStartCallback(void* data, SoDragger* d);
    static void dragFinishCallback(void* data, SoDragger* d);
    static void dragMotionCallback(void* data, SoDragger* d);

    SoSeparator* m_geometrySeperator;
    SoTransformManip* m_manip;
    SoScale* m_scale;
    bool m_autoscale, m_isDragging, m_autoRecompute;
};

// ***************************************************************************
class FemGuiExport BoxWidget: public FunctionWidget
{
    Q_OBJECT
public:
    BoxWidget();
    ~BoxWidget() override;

    void applyPythonCode() override;
    void onChange(const App::Property& p) override;
    void setViewProvider(ViewProviderFemPostFunction* view) override;

private Q_SLOTS:
    void centerChanged(double);
    void lengthChanged(double);
    void widthChanged(double);
    void heightChanged(double);

private:
    std::unique_ptr<Ui_BoxWidget> ui;
};

class FemGuiExport ViewProviderFemPostBoxFunction: public ViewProviderFemPostFunction
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostBoxFunction);

public:
    ViewProviderFemPostBoxFunction();
    ~ViewProviderFemPostBoxFunction() override;

    SoTransformManip* setupManipulator() override;
    FunctionWidget* createControlWidget() override;

protected:
    void draggerUpdate(SoDragger* mat) override;
    void updateData(const App::Property*) override;
};


// ***************************************************************************
class FemGuiExport CylinderWidget: public FunctionWidget
{
    Q_OBJECT
public:
    CylinderWidget();
    ~CylinderWidget() override;

    void applyPythonCode() override;
    void onChange(const App::Property& p) override;
    void setViewProvider(ViewProviderFemPostFunction* view) override;

private Q_SLOTS:
    void centerChanged(double);
    void axisChanged(double);
    void radiusChanged(double);

private:
    std::unique_ptr<Ui_CylinderWidget> ui;
};

class FemGuiExport ViewProviderFemPostCylinderFunction: public ViewProviderFemPostFunction
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostCylinderFunction);

public:
    ViewProviderFemPostCylinderFunction();
    ~ViewProviderFemPostCylinderFunction() override;

    SoTransformManip* setupManipulator() override;
    FunctionWidget* createControlWidget() override;

protected:
    void draggerUpdate(SoDragger* mat) override;
    void updateData(const App::Property*) override;
};


// ***************************************************************************
class FemGuiExport PlaneWidget: public FunctionWidget
{
    Q_OBJECT
public:
    PlaneWidget();
    ~PlaneWidget() override;

    void applyPythonCode() override;
    void onChange(const App::Property& p) override;
    void setViewProvider(ViewProviderFemPostFunction* view) override;

private Q_SLOTS:
    void originChanged(double);
    void normalChanged(double);

private:
    std::unique_ptr<Ui_PlaneWidget> ui;
};

class FemGuiExport ViewProviderFemPostPlaneFunction: public ViewProviderFemPostFunction
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostPlaneFunction);

public:
    ViewProviderFemPostPlaneFunction();
    ~ViewProviderFemPostPlaneFunction() override;

    App::PropertyFloatConstraint Scale;

    SoTransformManip* setupManipulator() override;
    FunctionWidget* createControlWidget() override;

protected:
    void draggerUpdate(SoDragger* mat) override;
    void updateData(const App::Property*) override;
    void onChanged(const App::Property*) override;

private:
    bool m_detectscale;
};


// ***************************************************************************
class FemGuiExport SphereWidget: public FunctionWidget
{
    Q_OBJECT
public:
    SphereWidget();
    ~SphereWidget() override;

    void applyPythonCode() override;
    void onChange(const App::Property& p) override;
    void setViewProvider(ViewProviderFemPostFunction* view) override;

private Q_SLOTS:
    void centerChanged(double);
    void radiusChanged(double);

private:
    std::unique_ptr<Ui_SphereWidget> ui;
};

class FemGuiExport ViewProviderFemPostSphereFunction: public ViewProviderFemPostFunction
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostSphereFunction);

public:
    ViewProviderFemPostSphereFunction();
    ~ViewProviderFemPostSphereFunction() override;

    SoTransformManip* setupManipulator() override;
    FunctionWidget* createControlWidget() override;

protected:
    void draggerUpdate(SoDragger* mat) override;
    void updateData(const App::Property*) override;
};

namespace ShapeNodes
{
SoGroup* postBox();
SoGroup* postCylinder();
SoGroup* postPlane();
SoGroup* postSphere();

}  // namespace ShapeNodes

}  // namespace FemGui


#endif  // FEM_VIEWPROVIDERFEMPOSTFUNCTION_H
