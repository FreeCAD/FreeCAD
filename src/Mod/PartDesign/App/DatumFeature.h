/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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


#ifndef PARTDESIGN_DATUMFEATURE_H
#define PARTDESIGN_DATUMFEATURE_H

//#include <App/PropertyUnits.h>
#include <App/PropertyLinks.h>
#include "Feature.h"

namespace PartDesign
{

class PartDesignExport Datum : public PartDesign::Feature
{
    PROPERTY_HEADER(PartDesign::Datum);

public:
    Datum();
    virtual ~Datum();

    /// The references defining the datum object, e.g. three planes for a point, two planes for a line
    App::PropertyLinkSubList References;
    /// The values defining the datum object, e.g. the offset from a Reference plane
    App::PropertyFloatList Values;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void) = 0;
    short mustExecute() const;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderDatum";
    }
    //@}    

    virtual const std::set<Base::Type> getHint() = 0;

protected:
    void onChanged (const App::Property* prop);

protected:
    std::multiset<Base::Type> refTypes;

    static const Base::Type getRefType(const App::DocumentObject* obj, const std::string& subname);

};

class PartDesignExport Point : public PartDesign::Datum
{
    PROPERTY_HEADER(PartDesign::Point);

public:
    Point();
    virtual ~Point();

    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    //@}

    static void initHints();
    const std::set<Base::Type> getHint();

private:
    // Hints on what further references are required/possible on this feature for a given set of references
    static std::map<std::multiset<Base::Type>, std::set<Base::Type> > hints;
};

class PartDesignExport Line : public PartDesign::Datum
{
    PROPERTY_HEADER(PartDesign::Line);

public:
    Line();
    virtual ~Line();

    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    //@}

    static void initHints();
    const std::set<Base::Type> getHint();

private:
    // Hints on what further references are required/possible on this feature for a given set of references
    static std::map<std::multiset<Base::Type>, std::set<Base::Type> > hints;
};

class PartDesignExport Plane : public PartDesign::Datum
{
    PROPERTY_HEADER(PartDesign::Plane);

public:
    Plane();

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    //@}

    static void initHints();
    const std::set<Base::Type> getHint();

private:
    // Hints on what further references are required/possible on this feature for a given set of references
    static std::map<std::multiset<Base::Type>, std::set<Base::Type> > hints;
};

} //namespace PartDesign


#endif // PARTDESIGN_DATUMFEATURE_H
