/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
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

#ifndef _DrawView_h_
#define _DrawView_h_

#include <boost/signals2.hpp>

#include <QRectF>

#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <App/PropertyGeo.h>
#include <App/FeaturePython.h>

namespace TechDraw
{

class DrawPage;
class DrawViewClip;

/** Base class of all View Features in the drawing module
 */
class TechDrawExport DrawView : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawView);

public:
    /// Constructor
    DrawView(void);
    virtual ~DrawView();

    App::PropertyFloat X;
    App::PropertyFloat Y;
    App::PropertyBool  LockPosition;
    App::PropertyFloatConstraint Scale;

    App::PropertyEnumeration ScaleType;
    App::PropertyFloat Rotation;
    App::PropertyString Caption;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void) override;
    virtual void onDocumentRestored() override;
    virtual short mustExecute() const override;
    //@}
    virtual void handleChangedPropertyType(
        Base::XMLReader &reader, const char * TypeName, App::Property * prop) override;

    bool isInClip();
    DrawViewClip* getClipGroup(void);

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderDrawingView";
    }
    //return PyObject as DrawViewPy
    virtual PyObject *getPyObject(void) override;

    DrawPage* findParentPage() const;
    virtual QRectF getRect() const;                      //must be overridden by derived class
    virtual double autoScale(double w, double h) const;
    virtual bool checkFit(DrawPage*) const;
    virtual void setPosition(double x, double y);
    bool keepUpdated(void);
    boost::signals2::signal<void (const DrawView*)> signalGuiPaint;
    virtual double getScale(void) const;
    void checkScale(void);
    void requestPaint(void);

protected:
    void onChanged(const App::Property* prop) override;
    std::string pageFeatName;
    bool autoPos;
    bool mouseMove;

private:
    static const char* ScaleTypeEnums[];
    static App::PropertyFloatConstraint::Constraints scaleRange;
};

typedef App::FeaturePythonT<DrawView> DrawViewPython;

} //namespace TechDraw

#endif
