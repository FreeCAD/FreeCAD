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

#include <Gui/ViewProviderSuppressibleExtension.h>


class SbRotation;
class SoMultipleCopy;
class SoTransform;

namespace FemGui
{

class TaskFemConstraint;

class FemGuiExport ViewProviderFemConstraint: public Gui::ViewProviderGeometryObject,
                                              public Gui::ViewProviderSuppressibleExtension
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemConstraint);

public:
    /// Constructor
    ViewProviderFemConstraint();
    ~ViewProviderFemConstraint() override;

    void attach(App::DocumentObject*) override;
    void updateData(const App::Property* prop) override;
    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;

    std::vector<App::DocumentObject*> claimChildren() const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;

    PyObject* getPyObject() override;

    /// Highlight the references that have been selected
    virtual void highlightReferences(const bool /* on */)
    {}

    SoSeparator* getSymbolSeparator() const;
    SoSeparator* getExtraSymbolSeparator() const;
    SoTransform* getExtraSymbolTransform() const;
    // Apply rotation on copies of the constraint symbol
    void setRotateSymbol(bool rotate);
    bool getRotateSymbol() const;

    /** Load constraint symbol from Open Inventor file
     * The file structure should be as follows:
     * A separator containing a separator with the symbol used in multiple
     * copies at points on the surface and an optional separator with a symbol
     * excluded from multiple copies.
     */
    void loadSymbol(const char* fileName);

    static std::string gethideMeshShowPartStr();
    static std::string gethideMeshShowPartStr(const std::string showConstr);

protected:
    void onChanged(const App::Property* prop) override;
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    void handleChangedPropertyName(Base::XMLReader& reader,
                                   const char* typeName,
                                   const char* propName) override;

    void updateSymbol();
    virtual void
    transformSymbol(const Base::Vector3d& point, const Base::Vector3d& normal, SbMatrix& mat) const;
    virtual void transformExtraSymbol() const;

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
    bool rotateSymbol;

protected:
    SoSeparator* pShapeSep;
    SoSeparator* pSymbol;
    SoSeparator* pExtraSymbol;
    SoTransform* pExtraTrans;
    SoMultipleCopy* pMultCopy;
    const char* ivFile;

    static std::string resourceSymbolDir;

    // Shaft design wizard integration
protected:
    friend class TaskFemConstraint;
    QVBoxLayout* wizardWidget;
    QVBoxLayout* wizardSubLayout;
    TaskFemConstraint* constraintDialog;

    void checkForWizard();
    static QObject* findChildByName(const QObject* parent, const QString& name);
};


inline SoSeparator* ViewProviderFemConstraint::getSymbolSeparator() const
{
    return pSymbol;
}

inline SoSeparator* ViewProviderFemConstraint::getExtraSymbolSeparator() const
{
    return pExtraSymbol;
}

inline SoTransform* ViewProviderFemConstraint::getExtraSymbolTransform() const
{
    return pExtraTrans;
}

inline bool ViewProviderFemConstraint::getRotateSymbol() const
{
    return rotateSymbol;
}

inline void ViewProviderFemConstraint::setRotateSymbol(bool rotate)
{
    rotateSymbol = rotate;
}

using ViewProviderFemConstraintPython = Gui::ViewProviderPythonFeatureT<ViewProviderFemConstraint>;


}  // namespace FemGui


#endif  // GUI_VIEWPROVIDERFEMCONSTRAINT_H
