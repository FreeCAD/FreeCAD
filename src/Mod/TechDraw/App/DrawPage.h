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

#ifndef _DrawPage_h_
#define _DrawPage_h_

#include <boost_signals2.hpp>

#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>


namespace TechDraw
{

class TechDrawExport DrawPage: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawPage);

public:
    DrawPage(void);
    virtual ~DrawPage();

    App::PropertyLinkList Views;
    App::PropertyLink Template;
    App::PropertyBool KeepUpdated;

    App::PropertyFloatConstraint Scale;
    App::PropertyEnumeration ProjectionType; // First or Third Angle
    
    App::PropertyInteger  NextBalloonIndex;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void) override;
    //@}
    virtual void handleChangedPropertyType(
            Base::XMLReader &reader, const char * TypeName, App::Property * prop) override;

    int addView(App::DocumentObject *docObj);
    int removeView(App::DocumentObject* docObj);
    short mustExecute() const override;
    boost::signals2::signal<void (const DrawPage*)> signalGuiPaint;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderPage";
    }

    PyObject *getPyObject(void) override;

//App::DocumentObjectExecReturn * recompute(void);

    /// Check whether we've got a valid template
    /*!
     * \return boolean answer to the question: "Doest thou have a valid template?"
     */
    bool hasValidTemplate() const;
    /// Returns width of the template
    /*!
     * \throws Base::Exception if no template is loaded.
     */
    double getPageWidth() const;
    /// Returns height of the template
    /*!
     * \throws Base::Exception if no template is loaded.
     */
    double getPageHeight() const;
    const char* getPageOrientation() const;
    bool isUnsetting(void) { return nowUnsetting; }
    void requestPaint(void);
    std::vector<App::DocumentObject*> getAllViews(void) ;
    DrawViewPart *balloonParent;    //could be many balloons on page? 
    
    int getNextBalloonIndex(void);
    
    void updateAllViews(void);
    static bool GlobalUpdateDrawings(void);
    static bool AllowPageOverride(void);
    void forceRedraw(bool b) { m_forceRedraw = b; }
    bool forceRedraw(void)   { return m_forceRedraw; }
    void redrawCommand();

    bool canUpdate() const;

protected:
    void onBeforeChange(const App::Property* prop) override;
    void onChanged(const App::Property* prop) override;
    virtual void onDocumentRestored() override;
    virtual void unsetupObject() override;

    bool m_forceRedraw;

private:
    static const char* ProjectionTypeEnums[];
    bool nowUnsetting;
    static App::PropertyFloatConstraint::Constraints scaleRange;

};

typedef App::FeaturePythonT<DrawPage> DrawPagePython;

} //namespace TechDraw


#endif

