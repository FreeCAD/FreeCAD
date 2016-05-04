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

#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <App/PropertyGeo.h>
#include <App/FeaturePython.h>

namespace TechDraw
{

class DrawPage;

/** Base class of all View Features in the drawing module
 */
class TechDrawExport DrawView : public App::DocumentObject
{
    PROPERTY_HEADER(TechDraw::DrawView);

public:
    /// Constructor
    DrawView(void);
    virtual ~DrawView();

    App::PropertyFloat X;
    App::PropertyFloat Y;
    App::PropertyFloat Scale;

    App::PropertyEnumeration ScaleType;
    App::PropertyFloat Rotation;
    App::PropertyBool Visible;

    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *recompute(void);
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual void onDocumentRestored();
    //@}

    bool isInClip();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderDrawingView";
    }
    //return PyObject as DrawViewPy
    virtual PyObject *getPyObject(void);

    DrawPage* findParentPage() const;
    bool allowAutoPos() {return autoPos;};                //sb in DPGI??
    void setAutoPos(bool state) {autoPos = state;};

protected:
    void onChanged(const App::Property* prop);
    std::string pageFeatName;
    bool autoPos;

private:
    static const char* ScaleTypeEnums[];
};

typedef App::FeaturePythonT<DrawView> DrawViewPython;

} //namespace TechDraw

#endif
