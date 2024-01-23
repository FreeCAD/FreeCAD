/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
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

#ifndef GUI_VIEWPROVIDER_MEASUREMENTBASE_H
#define GUI_VIEWPROVIDER_MEASUREMENTBASE_H

#include <Mod/Measure/MeasureGlobal.h>

#include <boost_signals2.hpp>
#include <QString>

#include <App/Application.h>
#include <App/PropertyStandard.h>
#include <Base/Parameter.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/SoTextLabel.h>

#include <Mod/Measure/App/MeasureBase.h>

//NOLINTBEGIN
class SbVec2s;
class SoFontStyle;
class SoBaseColor;
class SoText2;
class SoTranslation;
class SoPickStyle;
class SoCoordinate3;
class SoIndexedLineSet;
class SoTranslate2Dragger;
//NOLINTEND


namespace MeasureGui {

//NOLINTBEGIN
class MeasureGuiExport ViewProviderMeasureBase :public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(ViewProviderMeasureBase);

public:
    /// constructor.
    ViewProviderMeasureBase();

    /// destructor.
    ~ViewProviderMeasureBase() override;

    // Display properties
    App::PropertyColor          TextColor;
    App::PropertyColor          TextBackgroundColor;
    App::PropertyColor          LineColor;
    App::PropertyInteger        FontSize;
//NOLINTEND

    /**
     * Attaches the document object to this view provider.
     */
    bool isPartOfPhysicalObject() const override {return false;};
    void attach(App::DocumentObject *pcObj) override;
    void updateData(const App::Property* prop) override;
    virtual void onGuiInit(const Measure::MeasureBase* measureObject);
    virtual void onGuiUpdate(const Measure::MeasureBase* measureObject);
    void finishRestoring() override;

    bool useNewSelectionModel() const override {return true;}
    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;
    /// Show the annotation in the 3d window
    void show() override;

    virtual void redrawAnnotation();
    Measure::MeasureBase* getMeasureObject();

    virtual bool isSubjectVisible();

    static Base::Vector3d toVector3d(SbVec3f svec) { return Base::Vector3d(svec[0], svec[1], svec[2]); }
    static SbVec3f toSbVec3f(Base::Vector3d vec3) { return SbVec3f(vec3.x, vec3.y, vec3.z); }

    using Connection = boost::signals2::scoped_connection;
    Connection connectVisibilityChanged;
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

    SoPickStyle* getSoPickStyle();
    SoDrawStyle* getSoLineStylePrimary();
    SoDrawStyle* getSoLineStyleSecondary();
    SoSeparator* getSoSeparatorText();

    static constexpr double defaultTolerance = 10e-6;
    virtual Base::Vector3d getTextDirection(Base::Vector3d elementDirection, double tolerance = defaultTolerance);


    // TODO: getters & setters and move variables to private?
    bool _mShowTree = true;

    Gui::SoFrameLabel * pLabel;
    SoTranslate2Dragger* pDragger;
    SoTransform* pDraggerOrientation;
    SoTransform    * pLabelTranslation;
    SoBaseColor      * pColor;
    SoSeparator* pRootSeparator;
    SoSeparator* pTextSeparator;
    SoSeparator* pLineSeparator;
    SoSeparator* pLineSeparatorSecondary;
};

//NOLINTBEGIN
class MeasureGuiExport ViewProviderMeasure : public MeasureGui::ViewProviderMeasureBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeasureGui::ViewProviderMeasure);
//NOLINTEND

public:
    /// Constructor
    ViewProviderMeasure();
    ~ViewProviderMeasure() override;

    void attach(App::DocumentObject * feature) override;
    void redrawAnnotation() override;
    void onGuiInit(const Measure::MeasureBase* measureObject) override;

protected:
    void onChanged(const App::Property* prop) override;

    virtual Base::Vector3d getBasePosition();
    virtual Base::Vector3d getTextPosition();

private:
    SoCoordinate3    * pCoords;
    SoIndexedLineSet * pLines;
};

} // namespace Gui

#endif // GUI_VIEWPROVIDER_MEASUREMENTBASE_H

