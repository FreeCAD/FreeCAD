/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef PARTDESIGN_DATUMSHAPE_H
#define PARTDESIGN_DATUMSHAPE_H

#include <QString>
#include <boost/signals2.hpp>
#include <App/PropertyLinks.h>
#include <Mod/Part/App/DatumFeature.h>

namespace PartDesign
{

/* This feature is not really a classical datum. It is a fully defined shape and not
 * infinite geometry like a plane and a line. Also it is not calculated by references and hence
 * is not "attached" to anything. Furthermore real shapes must be visualized. This makes it hard
 * to reuse the existing datum infrastructure and a special handling for this type is
 * created.
 */
// TODO Add better documentation (2015-09-11, Fat-Zer)

class PartDesignExport ShapeBinder : public Part::Feature
{
    PROPERTY_HEADER(PartDesign::ShapeBinder);

public:
    ShapeBinder();
    virtual ~ShapeBinder();

    App::PropertyLinkSubListGlobal    Support;
    App::PropertyBool TraceSupport;

    static void getFilteredReferences(App::PropertyLinkSubList* prop, Part::Feature*& object, std::vector< std::string >& subobjects);
    static Part::TopoShape buildShapeFromReferences(Feature* obj, std::vector< std::string > subs);

    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderShapeBinder";
    }

protected:
    virtual void handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop);
    virtual short int mustExecute(void) const;
    virtual App::DocumentObjectExecReturn* execute(void);

private:
    void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop);
    virtual void onSettingDocument();

    typedef boost::signals2::connection Connection;
    Connection connectDocumentChangedObject;
};

class PartDesignExport SubShapeBinder : public Part::Feature {
    PROPERTY_HEADER(PartDesign::SubShapeBinder);
public:
    typedef Part::Feature inherited;

    SubShapeBinder();
    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderSubShapeBinder";
    }

    void setLinks(std::map<App::DocumentObject *, std::vector<std::string> > &&values, bool reset=false);

    App::PropertyXLinkSubList Support;
    App::PropertyBool ClaimChildren;
    App::PropertyBool Relative;
    App::PropertyBool Fuse;
    App::PropertyBool MakeFace;
    App::PropertyEnumeration BindMode;
    App::PropertyBool PartialLoad;
    App::PropertyXLink Context;
    App::PropertyInteger _Version;

    void update();

    virtual int canLoadPartial() const override {
        return PartialLoad.getValue()?1:0;
    }

    virtual bool canLinkProperties() const {return false;}

protected:
    virtual App::DocumentObjectExecReturn* execute(void) override;
    virtual void onChanged(const App::Property *prop) override;

    virtual void handleChangedPropertyType(
            Base::XMLReader &reader, const char * TypeName, App::Property * prop) override;

    virtual void onDocumentRestored() override;
    virtual void setupObject() override;

    void checkPropertyStatus();

    void slotRecomputedObject(const App::DocumentObject& Obj);

    typedef boost::signals2::scoped_connection Connection;
    Connection connRecomputedObj;
    App::Document *contextDoc=0;

    std::vector<Part::TopoShape> _Cache;
};

} //namespace PartDesign


#endif // PARTDESIGN_DATUMSHAPE_H
