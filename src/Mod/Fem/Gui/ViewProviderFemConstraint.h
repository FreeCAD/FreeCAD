/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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

#ifndef GUI_VIEWPROVIDERFEMCONSTRAINT_H
#define GUI_VIEWPROVIDERFEMCONSTRAINT_H

#include <QObject>
#include <QVBoxLayout>

#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/ViewProviderPythonFeature.h>
#include <Mod/Fem/FemGlobal.h>


class SoFontStyle;
class SoText2;
class SoBaseColor;
class SoTranslation;
class SbRotation;
class SoMaterial;

namespace FemGui
{

class TaskFemConstraint;

class FemGuiExport ViewProviderFemConstraint: public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemConstraint);

public:
    /// Constructor
    ViewProviderFemConstraint();
    ~ViewProviderFemConstraint() override;

    // Display properties
    App::PropertyColor TextColor;
    App::PropertyColor FaceColor;
    App::PropertyColor ShapeColor;
    App::PropertyInteger FontSize;
    App::PropertyFloat DistFactor;
    App::PropertyBool Mirror;

    void attach(App::DocumentObject*) override;
    void updateData(const App::Property* prop) override
    {
        Gui::ViewProviderGeometryObject::updateData(prop);
    }
    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;

    std::vector<App::DocumentObject*> claimChildren() const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;

    /// Highlight the references that have been selected
    virtual void highlightReferences(const bool /* on */)
    {}

    static std::string gethideMeshShowPartStr();
    static std::string gethideMeshShowPartStr(const std::string showConstr);

protected:
    void onChanged(const App::Property* prop) override;
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;

    static void createPlacement(SoSeparator* sep, const SbVec3f& base, const SbRotation& r);
    static void updatePlacement(const SoSeparator* sep,
                                const int idx,
                                const SbVec3f& base,
                                const SbRotation& r);
    static void createCone(SoSeparator* sep, const double height, const double radius);
    static SoSeparator* createCone(const double height, const double radius);
    static void
    updateCone(const SoNode* node, const int idx, const double height, const double radius);
    static void createCylinder(SoSeparator* sep, const double height, const double radius);
    static SoSeparator* createCylinder(const double height, const double radius);
    static void
    updateCylinder(const SoNode* node, const int idx, const double height, const double radius);
    static void
    createCube(SoSeparator* sep, const double width, const double length, const double height);
    static SoSeparator* createCube(const double width, const double length, const double height);
    static void updateCube(const SoNode* node,
                           const int idx,
                           const double width,
                           const double length,
                           const double height);
    static void createArrow(SoSeparator* sep, const double length, const double radius);
    static SoSeparator* createArrow(const double length, const double radius);
    static void
    updateArrow(const SoNode* node, const int idx, const double length, const double radius);
    static void createSpring(SoSeparator* sep, const double length, const double width);
    static SoSeparator* createSpring(const double length, const double width);
    static void
    updateSpring(const SoNode* node, const int idx, const double length, const double width);
    static void
    createFixed(SoSeparator* sep, const double height, const double width, const bool gap = false);
    static SoSeparator*
    createFixed(const double height, const double width, const bool gap = false);
    static void updateFixed(const SoNode* node,
                            const int idx,
                            const double height,
                            const double width,
                            const bool gap = false);
    static void createDisplacement(SoSeparator* sep,
                                   const double height,
                                   const double width,
                                   const bool gap = false);
    static SoSeparator*
    createDisplacement(const double height, const double width, const bool gap = false);
    static void updateDisplacement(const SoNode* node,
                                   const int idx,
                                   const double height,
                                   const double width,
                                   const bool gap = false);
    static void createRotation(SoSeparator* sep,
                               const double height,
                               const double width,
                               const bool gap = false);
    static SoSeparator*
    createRotation(const double height, const double width, const bool gap = false);
    static void updateRotation(const SoNode* node,
                               const int idx,
                               const double height,
                               const double width,
                               const bool gap = false);

private:
    SoFontStyle* pFont;
    SoText2* pLabel;
    SoBaseColor* pTextColor;
    SoBaseColor* pMaterials;

protected:
    SoSeparator* pShapeSep;

    // Shaft design wizard integration
protected:
    friend class TaskFemConstraint;
    QVBoxLayout* wizardWidget;
    QVBoxLayout* wizardSubLayout;
    TaskFemConstraint* constraintDialog;

    void checkForWizard();
    static QObject* findChildByName(const QObject* parent, const QString& name);
};

using ViewProviderFemConstraintPython = Gui::ViewProviderPythonFeatureT<ViewProviderFemConstraint>;


}  // namespace FemGui


#endif  // GUI_VIEWPROVIDERFEMCONSTRAINT_H
