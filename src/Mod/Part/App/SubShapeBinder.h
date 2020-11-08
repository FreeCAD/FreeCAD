/***************************************************************************
 *   Copyright (c) 2021 Zheng Lei (realthunder) <realthunder.dev@gmail.com>*
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


#ifndef PART_SUBSHAPEBINDER_H
#define PART_SUBSHAPEBINDER_H

#include <QString>
#include <boost_signals2.hpp>
#include <App/PropertyLinks.h>
#include <App/DocumentObserver.h>
#include <App/FeaturePython.h>
#include "PartFeature.h"

namespace Data
{
class MappedName;
}

namespace Part
{

class PartExport SubShapeBinder : public Part::Feature {
    PROPERTY_HEADER_WITH_OVERRIDE(Part::SubShapeBinder);
public:
    typedef Part::Feature inherited;

    SubShapeBinder();
    ~SubShapeBinder();

    const char* getViewProviderName(void) const override {
        return "PartGui::ViewProviderSubShapeBinder";
    }

    void setLinks(std::map<App::DocumentObject *, std::vector<std::string> > &&values, bool reset=false);

    App::PropertyXLinkSubList Support;
    App::PropertyBool ClaimChildren;
    App::PropertyBool Relative;
    App::PropertyBool Fuse;
    App::PropertyBool MakeFace;
    App::PropertyEnumeration FillStyle;
    App::PropertyEnumeration BindMode;
    App::PropertyBool PartialLoad;
    App::PropertyXLink Context;
    App::PropertyInteger _Version;
    App::PropertyEnumeration BindCopyOnChange;

    enum UpdateOption {
        UpdateNone = 0,
        UpdateInit = 1,
        UpdateForced = 2,
    };
    void update(UpdateOption options = UpdateNone);

    virtual int canLoadPartial() const override {
        return PartialLoad.getValue()?1:0;
    }

    virtual bool canLinkProperties() const override {return false;}

    virtual App::DocumentObject *getSubObject(const char *subname, PyObject **pyObj=0, 
            Base::Matrix4D *mat=0, bool transform=true, int depth=0) const override;

    virtual App::DocumentObject *getElementOwner(const Data::MappedName & name) const override;

    // virtual App::DocumentObject *getLinkedObject(bool recurse=true,
    //         Base::Matrix4D *mat=0, bool transform=false, int depth=0) const override;
    App::DocumentObject *_getLinkedObject(bool recurse=true,
            Base::Matrix4D *mat=0, bool transform=false, int depth=0) const;

    static App::SubObjectT import(const App::SubObjectT &feature,
                                  const App::SubObjectT &editObj,
                                  bool importWholeObject = true,
                                  bool noSubObject = false,
                                  bool compatible = false);

protected:
    virtual App::DocumentObjectExecReturn* execute(void) override;
    virtual void onChanged(const App::Property *prop) override;

    virtual void handleChangedPropertyType(
            Base::XMLReader &reader, const char * TypeName, App::Property * prop) override;

    virtual void onDocumentRestored() override;
    virtual void setupObject() override;

    void setupCopyOnChange();
    void checkCopyOnChange(const App::Property &prop);
    void clearCopiedObjects();

    void checkPropertyStatus();

    void collapseGeoChildren();

    void slotRecomputedObject(const App::DocumentObject& Obj);

    void slotLabelChanged();

    typedef boost::signals2::scoped_connection Connection;
    Connection connRecomputedObj;
    App::Document *contextDoc = 0;

    Connection connLabelChange;

    std::vector<Connection> copyOnChangeConns;
    bool hasCopyOnChange = true;

    App::PropertyXLinkSub _CopiedLink;
    std::vector<App::DocumentObjectT> _CopiedObjs;
};

typedef App::FeaturePythonT<SubShapeBinder> SubShapeBinderPython;

} //namespace Part


#endif // PART_SUBSHAPEBINDER_H
