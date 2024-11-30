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

#ifndef _FeaturePage_h_
#define _FeaturePage_h_

#include <App/DocumentObjectGroup.h>
#include <App/PropertyFile.h>
#include <Mod/Drawing/DrawingGlobal.h>


namespace Drawing
{

/** Base class of all View Features in the drawing module
 */
class DrawingExport FeaturePage: public App::DocumentObjectGroup
{
    PROPERTY_HEADER(Drawing::FeaturePage);

public:
    /// Constructor
    FeaturePage(void);
    virtual ~FeaturePage();

    App::PropertyFileIncluded PageResult;
    App::PropertyFile Template;
    App::PropertyStringList EditableTexts;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn* execute(void);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const
    {
        return "DrawingGui::ViewProviderDrawingPage";
    }
    virtual std::vector<std::string> getEditableTextsFromTemplate(void) const;

protected:
    void onBeforeChange(const App::Property* prop);
    void onChanged(const App::Property* prop);
    /// get called after a document has been fully restored
    virtual void onDocumentRestored();

private:
    int numChildren;
};


}  // namespace Drawing


#endif
