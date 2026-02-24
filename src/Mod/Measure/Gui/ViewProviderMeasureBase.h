// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <Mod/Measure/MeasureGlobal.h>

#include <QString>

#include <App/Application.h>
#include <App/PropertyStandard.h>
#include <Base/Parameter.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/SoTextLabel.h>
#include <Gui/ViewProviderDocumentObjectGroup.h>

#include <Mod/Measure/App/MeasureBase.h>

// NOLINTBEGIN
class SbVec2s;
class SoFontStyle;
class SoBaseColor;
class SoText2;
class SoTranslation;
class SoPickStyle;
class SoCoordinate3;
class SoIndexedLineSet;
class SoTranslate2Dragger;
// NOLINTEND


namespace MeasureGui
{


class MeasureGuiExport ViewProviderMeasureGroup: public Gui::ViewProviderDocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeasureGui::ViewProviderMeasureGroup);

public:
    ViewProviderMeasureGroup();
    ~ViewProviderMeasureGroup() override;

    bool allowOverride(const App::DocumentObject&) const override
    {
        return true;
    }

    QIcon getIcon() const override;
};


// NOLINTBEGIN
class MeasureGuiExport ViewProviderMeasureBase: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeasureGui::ViewProviderMeasureBase);

public:
    /// constructor.
    ViewProviderMeasureBase();

    /// destructor.
    ~ViewProviderMeasureBase() override;

    // Display properties
    App::PropertyColor TextColor;
    App::PropertyColor TextBackgroundColor;
    App::PropertyColor LineColor;
    App::PropertyInteger FontSize;
    // NOLINTEND

    // Fields
    SoSFFloat fieldFontSize;

    /**
     * Attaches the document object to this view provider.
     */
    bool isPartOfPhysicalObject() const override
    {
        return false;
    };
    void attach(App::DocumentObject* pcObj) override;
    void updateData(const App::Property* prop) override;
    virtual void positionAnno(const Measure::MeasureBase* measureObject);
    void finishRestoring() override;

    bool useNewSelectionModel() const override
    {
        return true;
    }
    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;
    /// Show the annotation in the 3d window
    void show() override;

    virtual void redrawAnnotation();
    Measure::MeasureBase* getMeasureObject();

    virtual bool isSubjectVisible();

    static Base::Vector3d toVector3d(SbVec3f svec)
    {
        return Base::Vector3d(svec[0], svec[1], svec[2]);
    }
    static SbVec3f toSbVec3f(Base::Vector3d vec3)
    {
        return SbVec3f(vec3.x, vec3.y, vec3.z);
    }

    void onSubjectVisibilityChanged(const App::DocumentObject& docObj, const App::Property& prop);
    void connectToSubject(App::DocumentObject* subject);
    void connectToSubject(std::vector<App::DocumentObject*> subject);

protected:
    static void draggerChangedCallback(void* data, SoDragger*);
    void onChanged(const App::Property* prop) override;
    virtual void onLabelMoved() {};
    void setLabelValue(const Base::Quantity& value);
    void setLabelValue(const QString& value);
    void setLabelTranslation(const SbVec3f& position);
    void updateIcon();

    SoPickStyle* getSoPickStyle();
    SoDrawStyle* getSoLineStylePrimary();
    SoDrawStyle* getSoLineStyleSecondary();
    SoSeparator* getSoSeparatorText();

    static constexpr double defaultTolerance = 10e-6;
    virtual Base::Vector3d getTextDirection(
        Base::Vector3d elementDirection,
        double tolerance = defaultTolerance
    );
    float getViewScale();

    // TODO: getters & setters and move variables to private?
    bool _mShowTree = true;

    SoSeparator* pGlobalSeparator;  // Separator in the global coordinate space
    Gui::SoFrameLabel* pLabel;
    SoTranslate2Dragger* pDragger;
    SoTransform* pDraggerOrientation;
    SoTransform* pLabelTranslation;
    SoBaseColor* pColor;
    SoSeparator* pRootSeparator;
    SoSeparator* pTextSeparator;
    SoSeparator* pLineSeparator;
    SoSeparator* pLineSeparatorSecondary;

private:
    fastsignals::connection _mVisibilityChangedConnection;
};


// NOLINTBEGIN
class MeasureGuiExport ViewProviderMeasure: public MeasureGui::ViewProviderMeasureBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeasureGui::ViewProviderMeasure);
    // NOLINTEND

public:
    /// Constructor
    ViewProviderMeasure();
    ~ViewProviderMeasure() override;

    void redrawAnnotation() override;
    void positionAnno(const Measure::MeasureBase* measureObject) override;

protected:
    void onChanged(const App::Property* prop) override;

    virtual Base::Vector3d getBasePosition();
    virtual Base::Vector3d getTextPosition();

private:
    SoCoordinate3* pCoords;
    SoIndexedLineSet* pLines;
};


class ViewProviderMeasureArea: public ViewProviderMeasure
{
    PROPERTY_HEADER(MeasureGui::ViewProviderMeasureArea);

public:
    ViewProviderMeasureArea()
    {
        sPixmap = "Measurement-Area";
    }
};


class ViewProviderMeasureLength: public ViewProviderMeasure
{
    PROPERTY_HEADER(MeasureGui::ViewProviderMeasureLength);

public:
    ViewProviderMeasureLength()
    {
        sPixmap = "Measurement-Distance";
    }
};


class ViewProviderMeasurePosition: public ViewProviderMeasure
{
    PROPERTY_HEADER(MeasureGui::ViewProviderMeasurePosition);

public:
    ViewProviderMeasurePosition()
    {
        sPixmap = "Measurement-Position";
    }
};


class ViewProviderMeasureRadius: public ViewProviderMeasure
{
    PROPERTY_HEADER(MeasureGui::ViewProviderMeasureRadius);

public:
    ViewProviderMeasureRadius()
    {
        sPixmap = "Measurement-Radius";
    }
};

class ViewProviderMeasureDiameter: public ViewProviderMeasure
{
    PROPERTY_HEADER(MeasureGui::ViewProviderMeasureDiameter);

public:
    ViewProviderMeasureDiameter()
    {
        sPixmap = "Measurement-Diameter";
    }
};

class ViewProviderMeasureCOM: public ViewProviderMeasure
{
    PROPERTY_HEADER(MeasureGui::ViewProviderMeasureCOM);

public:
    ViewProviderMeasureCOM()
    {
        sPixmap = "Measurement-CenterOfMass";
    }
};


}  // namespace MeasureGui
