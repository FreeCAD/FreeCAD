/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef PARTDESIGN_AuxGroup_H
#define PARTDESIGN_AuxGroup_H

#include <boost/signals2.hpp>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>

namespace PartDesign
{
class Body;

/** For grouping non PartDesign::Feature objects in Body
 */
class PartDesignExport AuxGroup : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::AuxGroup);

    typedef App::DocumentObject inherited;

public:
    AuxGroup();

    App::PropertyLinkList Group;
    App::PropertyLinkHidden _Body;

    virtual const char* getViewProviderName(void) const override {
        return "PartDesignGui::ViewProviderAuxGroup";
    }

    virtual void onDocumentRestored() override;
    virtual void onChanged(const App::Property* prop) override;

    bool isObjectAllowed(const App::DocumentObject *obj) const;

    enum GroupType {
        UnknownGroup,
        SketchGroup,
        DatumGroup,
        MiscGroup,
        OtherGroup,
    };
    GroupType getGroupType() const;
    PartDesign::Body * getBody() const;

protected:
    void refresh();
    void attachBody();

private:
    boost::signals2::scoped_connection connBody;
    mutable GroupType groupType = UnknownGroup;
};

} //namespace PartDesign


#endif // PARTDESIGN_AuxGroup_H
