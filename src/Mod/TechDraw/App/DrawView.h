/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef DrawView_h_
#define DrawView_h_

#include <boost_signals2.hpp>
#include <QCoreApplication>
#include <QRectF>

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyUnits.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDraw
{

class DrawPage;
class DrawViewClip;
class DrawLeaderLine;
/*class CosmeticVertex;*/

/** Base class of all View Features in the drawing module
 */
class TechDrawExport DrawView : public App::DocumentObject
{
    Q_DECLARE_TR_FUNCTIONS(TechDraw::DrawView)
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawView);

public:
    /// Constructor
    DrawView();
    ~DrawView() override;

    App::PropertyDistance X;
    App::PropertyDistance Y;
    App::PropertyBool LockPosition;
    App::PropertyFloatConstraint Scale;

    App::PropertyEnumeration ScaleType;
    App::PropertyAngle Rotation;
    App::PropertyString Caption;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute() override;
    void onDocumentRestored() override;
    short mustExecute() const override;
    //@}
    void handleChangedPropertyType(
        Base::XMLReader &reader, const char * TypeName, App::Property * prop) override;

    bool isInClip();
    DrawViewClip* getClipGroup();

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderDrawingView";
    }
    //return PyObject as DrawViewPy
    PyObject *getPyObject() override;

    virtual DrawPage* findParentPage() const;
    virtual std::vector<DrawPage*> findAllParentPages() const;
    virtual int countParentPages() const;
    virtual QRectF getRect() const;                      //must be overridden by derived class
    QRectF getRectAligned() const;
    virtual double autoScale() const;
    virtual double autoScale(double w, double h) const;
    virtual bool checkFit() const;
    virtual bool checkFit(DrawPage*) const;
    virtual void setPosition(double x, double y, bool force = false);
    virtual bool keepUpdated(void);

    boost::signals2::signal<void (const DrawView*)> signalGuiPaint;
    boost::signals2::signal<void (const DrawView*, std::string, std::string)> signalProgressMessage;
    void requestPaint(void);
    void showProgressMessage(std::string featureName, std::string text);

    virtual double getScale(void) const;
    void checkScale(void);

    virtual void handleXYLock(void);
    virtual bool isLocked(void) const;
    virtual bool showLock(void) const;

    std::vector<TechDraw::DrawLeaderLine*> getLeaders(void) const;

    void setScaleAttribute();

    void overrideKeepUpdated(bool s) { m_overrideKeepUpdated = s; }
    bool overrideKeepUpdated(void) { return m_overrideKeepUpdated; }

    void translateLabel(std::string context, std::string baseName, std::string uniqueName);
    static std::string translateArbitrary(std::string context, std::string baseName, std::string uniqueName);



protected:
    void onChanged(const App::Property* prop) override;
    virtual void validateScale();
    std::string pageFeatName;
    bool autoPos;
    bool mouseMove;

    int prefScaleType();
    double prefScale();

private:
    static const char* ScaleTypeEnums[];
    static App::PropertyFloatConstraint::Constraints scaleRange;

    bool m_overrideKeepUpdated;
};

using DrawViewPython = App::FeaturePythonT<DrawView>;

} //namespace TechDraw

#endif
