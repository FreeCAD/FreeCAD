// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef ASSEMBLYGUI_VIEWPROVIDER_ViewProviderAssembly_H
#define ASSEMBLYGUI_VIEWPROVIDER_ViewProviderAssembly_H

#include <QCoreApplication>

#include <Mod/Assembly/AssemblyGlobal.h>

#include <Gui/Selection.h>
#include <Gui/ViewProviderPart.h>

class SoSwitch;
class SoSensor;
class SoDragger;
class SoFieldSensor;

namespace Gui
{
class SoFCCSysDragger;
class View3DInventorViewer;
}  // namespace Gui

namespace AssemblyGui
{

class AssemblyGuiExport ViewProviderAssembly: public Gui::ViewProviderPart,
                                              public Gui::SelectionObserver
{
    Q_DECLARE_TR_FUNCTIONS(AssemblyGui::ViewProviderAssembly)
    PROPERTY_HEADER_WITH_OVERRIDE(AssemblyGui::ViewProviderAssembly);

    enum class DragMode
    {
        Translation,
        TranslationOnAxis,
        TranslationOnPlane,
        Rotation,
        RotationOnPlane,
        TranslationOnAxisAndRotationOnePlane,
        Ball,
        None,
    };

public:
    ViewProviderAssembly();
    ~ViewProviderAssembly() override;

    /// deliver the icon shown in the tree view. Override from ViewProvider.h
    QIcon getIcon() const override;

    bool doubleClicked() override;
    bool onDelete(const std::vector<std::string>& subNames) override;

    /** @name enter/exit edit mode */
    //@{
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    void setEditViewer(Gui::View3DInventorViewer*, int ModNum) override;
    bool isInEditMode() const;

    /// Ask the view provider if it accepts object deletions while in edit
    bool acceptDeletionsInEdit() override
    {
        return true;
    }

    bool canDragObject(App::DocumentObject*) const override;

    App::DocumentObject* getActivePart() const;


    /// is called when the Provider is in edit and a key event ocours. Only ESC ends edit.
    bool keyPressed(bool pressed, int key) override;
    /// is called when the provider is in edit and the mouse is moved
    bool mouseMove(const SbVec2s& pos, Gui::View3DInventorViewer* viewer) override;
    /// is called when the Provider is in edit and the mouse is clicked
    bool mouseButtonPressed(int Button,
                            bool pressed,
                            const SbVec2s& cursorPos,
                            const Gui::View3DInventorViewer* viewer) override;

    /// Finds what drag mode should be used based on the user selection.
    DragMode findDragMode();
    void initMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer);
    void endMove();
    virtual void setEnableMovement(bool enable = true)
    {
        enableMovement = enable;
    }
    virtual bool getEnableMovement() const
    {
        return enableMovement;
    }


    bool getSelectedObjectsWithinAssembly(bool addPreselection = true, bool onlySolids = false);
    App::DocumentObject* getObjectFromSubNames(std::vector<std::string>& subNames);
    std::vector<std::string> parseSubNames(std::string& subNamesStr);

    /// Get the python wrapper for that ViewProvider
    PyObject* getPyObject() override;

    // protected:
    /// get called by the container whenever a property has been changed
    // void onChanged(const App::Property* prop) override;

    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

    // Dragger controls:
    void initMoveDragger();
    void endMoveDragger();
    static void draggerMotionCallback(void* data, SoDragger* d);

    void setDragger();
    void unsetDragger();
    void setDraggerVisibility(bool val);
    bool getDraggerVisibility();
    void setDraggerPlacement(Base::Placement plc);
    Base::Placement getDraggerPlacement();
    Gui::SoFCCSysDragger* getDragger();

    static Base::Vector3d getCenterOfBoundingBox(const std::vector<App::DocumentObject*>& objs,
                                                 const std::vector<App::DocumentObject*>& parts);

    DragMode dragMode;
    bool canStartDragging;
    bool partMoving;
    bool enableMovement;
    bool jointVisibilityBackup;
    bool ctrlPressed;
    int numberOfSel;
    Base::Vector3d prevPosition;
    Base::Vector3d initialPosition;
    Base::Vector3d initialPositionRot;
    Base::Placement jcsPlc;
    Base::Placement jcsGlobalPlc;
    Base::Placement draggerInitPlc;

    App::DocumentObject* movingJoint;

    std::vector<std::pair<App::DocumentObject*, double>> objectMasses;
    std::vector<std::pair<App::DocumentObject*, Base::Placement>> docsToMove;

    Gui::SoFCCSysDragger* asmDragger = nullptr;
    SoSwitch* asmDraggerSwitch = nullptr;
    SoFieldSensor* translationSensor = nullptr;
    SoFieldSensor* rotationSensor = nullptr;
};

}  // namespace AssemblyGui

#endif  // ASSEMBLYGUI_VIEWPROVIDER_ViewProviderAssembly_H
