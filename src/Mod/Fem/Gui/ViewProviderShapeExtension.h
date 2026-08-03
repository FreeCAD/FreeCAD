// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Stefan Tröger <stefantroeger@gmx.net>
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <QWidget>
#include <fastsignals/signal.h>
#include <Inventor/SbBox3f.h>


#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/ViewProviderExtension.h>
#include <Gui/ViewProviderExtensionPython.h>

#include <Mod/Fem/FemGlobal.h>

class SoDragger;
class SoGroup;
class SoScale;
class SoTransformManip;

class Ui_BoxWidget;
class Ui_CylinderWidget;
class Ui_PlaneWidget;
class Ui_SphereWidget;

namespace FemGui
{

class FemGuiExport ShapeWidget: public QWidget
{
    Q_OBJECT
public:
    ShapeWidget() = default;
    ~ShapeWidget() override = default;

    virtual void setViewProvider(Gui::ViewProviderDocumentObject* view);
    void onObjectsChanged(const App::DocumentObject& obj, const App::Property&);

protected:
    bool blockObjectUpdates()
    {
        return m_block;
    }
    void setBlockObjectUpdates(bool val)
    {
        m_block = val;
    }

    template<class T>
    T* getObjectExtension() const
    {
        auto extension = m_object->getExtension<T>();
        if (!extension) {
            throw Base::AbortException(
                "Provided viewproviders object does not have the correct extension"
            );
        }
        return extension;
    }

    virtual void applyPythonCode() {};
    virtual void onChange(const App::Property& p) = 0;

private:
    bool m_block {false};
    Gui::ViewProviderDocumentObject* m_view {nullptr};
    App::DocumentObject* m_object {nullptr};
    fastsignals::scoped_connection m_connection;
};


class FemGuiExport ViewProviderShapeExtension: public Gui::ViewProviderExtension
{

    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderShapeExtension);
    using inherited = Gui::ViewProviderExtension;

public:
    ViewProviderShapeExtension();
    ~ViewProviderShapeExtension() override;

    void extensionAttach(App::DocumentObject* pcObject) override;

    virtual ShapeWidget* createShapeWidget()
    {
        return nullptr;
    }

    SoScale* getScaleNode()
    {
        return m_scale;
    }

protected:
    virtual SoTransformManip* setupManipulator()
    {
        return nullptr;
    };
    virtual void draggerUpdate(SoDragger*) {};

    bool isDragging()
    {
        return m_isDragging;
    }
    SoTransformManip* getManipulator()
    {
        return m_manip;
    }
    SoSeparator* getGeometryNode()
    {
        return m_geometrySeperator;
    }

    template<class T>
    T* getObjectExtension() const
    {
        auto extension = getExtendedViewProvider()->getObject()->getExtension<T>();
        if (!extension) {
            throw Base::AbortException(
                "Provided viewproviders object does not have the correct extension"
            );
        }
        return extension;
    }

    SbBox3f getBoundingsOfView() const;
    bool findScaleFactor(double& scale) const;

    // return shape extension python object to expose the createShapeWidget function
    PyObject* getExtensionPyObject() override;

    std::string m_mask_mode;

private:
    static void dragStartCallback(void* data, SoDragger* d);
    static void dragFinishCallback(void* data, SoDragger* d);
    static void dragMotionCallback(void* data, SoDragger* d);

    SoSeparator* m_geometrySeperator;
    SoTransformManip* m_manip;
    SoScale* m_scale;
    bool m_isDragging {false}, m_autoRecompute {false};
};

// ***************************************************************************
class FemGuiExport BoxWidget: public ShapeWidget
{
    Q_OBJECT
public:
    BoxWidget();
    ~BoxWidget() override;

    void onChange(const App::Property& p) override;
    void setViewProvider(Gui::ViewProviderDocumentObject* view) override;

private Q_SLOTS:
    void centerChanged(double);
    void lengthChanged(double);
    void widthChanged(double);
    void heightChanged(double);

private:
    std::unique_ptr<Ui_BoxWidget> ui;
};

class FemGuiExport ViewProviderBoxExtension: public ViewProviderShapeExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderBoxExtension);
    using inherited = ViewProviderShapeExtension;

public:
    /// Constructor
    ViewProviderBoxExtension();
    ~ViewProviderBoxExtension() override;

protected:
    SoTransformManip* setupManipulator() override;
    ShapeWidget* createShapeWidget() override;
    void draggerUpdate(SoDragger* mat) override;
    void extensionUpdateData(const App::Property*) override;
};

using ViewProviderBoxExtensionPython = Gui::ViewProviderExtensionPythonT<ViewProviderBoxExtension>;


// ***************************************************************************
class FemGuiExport CylinderWidget: public ShapeWidget
{
    Q_OBJECT
public:
    CylinderWidget();
    ~CylinderWidget() override;

    void onChange(const App::Property& p) override;
    void setViewProvider(Gui::ViewProviderDocumentObject* view) override;

private Q_SLOTS:
    void centerChanged(double);
    void axisChanged(double);
    void radiusChanged(double);

private:
    std::unique_ptr<Ui_CylinderWidget> ui;
};

class FemGuiExport ViewProviderCylinderExtension: public ViewProviderShapeExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderCylinderExtension);
    using inherited = ViewProviderShapeExtension;

public:
    /// Constructor
    ViewProviderCylinderExtension();
    ~ViewProviderCylinderExtension() override;

protected:
    SoTransformManip* setupManipulator() override;
    ShapeWidget* createShapeWidget() override;
    void draggerUpdate(SoDragger* mat) override;
    void extensionUpdateData(const App::Property*) override;
};

using ViewProviderCylinderExtensionPython
    = Gui::ViewProviderExtensionPythonT<ViewProviderCylinderExtension>;

// ***************************************************************************
class FemGuiExport SphereWidget: public ShapeWidget
{
    Q_OBJECT
public:
    SphereWidget();
    ~SphereWidget() override;

    void onChange(const App::Property& p) override;
    void setViewProvider(Gui::ViewProviderDocumentObject* view) override;

private Q_SLOTS:
    void centerChanged(double);
    void radiusChanged(double);

private:
    std::unique_ptr<Ui_SphereWidget> ui;
};

class FemGuiExport ViewProviderSphereExtension: public ViewProviderShapeExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderSphereExtension);
    using inherited = ViewProviderShapeExtension;

public:
    /// Constructor
    ViewProviderSphereExtension();
    ~ViewProviderSphereExtension() override;

protected:
    SoTransformManip* setupManipulator() override;
    ShapeWidget* createShapeWidget() override;
    void draggerUpdate(SoDragger* mat) override;
    void extensionUpdateData(const App::Property*) override;
};

using ViewProviderSphereExtensionPython
    = Gui::ViewProviderExtensionPythonT<ViewProviderSphereExtension>;

// ***************************************************************************
class FemGuiExport PlaneWidget: public ShapeWidget
{
    Q_OBJECT
public:
    PlaneWidget();
    ~PlaneWidget() override;

    void onChange(const App::Property& p) override;
    void setViewProvider(Gui::ViewProviderDocumentObject* view) override;

private Q_SLOTS:
    void originChanged(double);
    void normalChanged(double);

private:
    std::unique_ptr<Ui_PlaneWidget> ui;
};

class FemGuiExport ViewProviderPlaneExtension: public ViewProviderShapeExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderPlaneExtension);
    using inherited = ViewProviderShapeExtension;

public:
    /// Constructor
    ViewProviderPlaneExtension();
    ~ViewProviderPlaneExtension() override;

    App::PropertyFloatConstraint Scale;

protected:
    SoTransformManip* setupManipulator() override;
    ShapeWidget* createShapeWidget() override;
    void draggerUpdate(SoDragger* mat) override;
    void extensionUpdateData(const App::Property*) override;
    void extensionOnChanged(const App::Property*) override;

private:
    bool m_detectscale {false};
};

using ViewProviderPlaneExtensionPython = Gui::ViewProviderExtensionPythonT<ViewProviderPlaneExtension>;


namespace ShapeNodes
{
SoGroup* postBox();
SoGroup* postCylinder();
SoGroup* postPlane();
SoGroup* postSphere();
}  // namespace ShapeNodes

}  // namespace FemGui
