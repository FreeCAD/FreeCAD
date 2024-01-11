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

namespace Gui
{
class View3DInventorViewer;
}

namespace AssemblyGui
{

class AssemblyGuiExport ViewProviderAssembly: public Gui::ViewProviderPart,
                                              public Gui::SelectionObserver
{
    Q_DECLARE_TR_FUNCTIONS(AssemblyGui::ViewProviderAssembly)
    PROPERTY_HEADER_WITH_OVERRIDE(AssemblyGui::ViewProviderAssembly);

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
    bool isInEditMode();

    /// Ask the view provider if it accepts object deletions while in edit
    bool acceptDeletionsInEdit() override
    {
        return true;
    }

    bool canDragObject(App::DocumentObject*) const override;

    App::DocumentObject* getActivePart();

    enum class MoveMode
    {
        Translation,
        TranslationOnAxis,
        TranslationOnPlane,
        Rotation,
        RotationOnPlane,
        TranslationOnAxisAndRotationOnePlane,
        None,
    };
    MoveMode moveMode;

    /// is called when the provider is in edit and the mouse is moved
    bool mouseMove(const SbVec2s& pos, Gui::View3DInventorViewer* viewer) override;
    /// is called when the Provider is in edit and the mouse is clicked
    bool mouseButtonPressed(int Button,
                            bool pressed,
                            const SbVec2s& cursorPos,
                            const Gui::View3DInventorViewer* viewer) override;
    MoveMode findMoveMode();
    void initMove();
    void endMove();


    bool getSelectedObjectsWithinAssembly();
    App::DocumentObject* getObjectFromSubNames(std::vector<std::string>& subNames);
    std::vector<std::string> parseSubNames(std::string& subNamesStr);

    /// Get the python wrapper for that ViewProvider
    PyObject* getPyObject() override;

    virtual void setEnableMovement(bool enable = true)
    {
        enableMovement = enable;
    }
    virtual bool getEnableMovement() const
    {
        return enableMovement;
    }

    // protected:
    /// get called by the container whenever a property has been changed
    // void onChanged(const App::Property* prop) override;

    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

    bool canStartDragging;
    bool partMoving;
    bool enableMovement;
    bool jointVisibilityBackup;
    int numberOfSel;
    Base::Vector3d initialPosition;
    Base::Vector3d initialPositionRot;
    Base::Placement jcsPlc;
    Base::Placement jcsGlobalPlc;

    App::DocumentObject* movingJoint;

    std::vector<std::pair<App::DocumentObject*, double>> objectMasses;
    std::vector<std::pair<App::DocumentObject*, Base::Placement>> docsToMove;
};

}  // namespace AssemblyGui

#endif  // ASSEMBLYGUI_VIEWPROVIDER_ViewProviderAssembly_H
