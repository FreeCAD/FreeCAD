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

#include <QString>
#include <App/PropertyLinks.h>
#include "Feature.h"

namespace PartDesign
{

class PartDesignExport Datum : public App::DocumentObject
{
    PROPERTY_HEADER(PartDesign::Datum);

public:
    Datum();
    virtual ~Datum();

    /// The references defining the datum object, e.g. three planes for a point, two planes for a line
    App::PropertyLinkSubList References;
    /// The values defining the datum object, e.g. the offset from a Reference plane
    App::PropertyFloatList Values;

    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);

    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderDatum";
    }

    virtual const std::set<QString> getHint() = 0;

protected:
    void onChanged (const App::Property* prop);
    void onDocumentRestored();

protected:
    std::multiset<QString> refTypes;

    static const QString getRefType(const App::DocumentObject* obj, const std::string& subname);

};

class PartDesignExport Point : public PartDesign::Datum
{
    PROPERTY_HEADER(PartDesign::Point);

public:
    App::PropertyVector _Point;

    Point();
    virtual ~Point();

    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderDatumPoint";
    }

    static void initHints();
    const std::set<QString> getHint();

protected:
    virtual void onChanged(const App::Property* prop);

private:
    // Hints on what further references are required/possible on this feature for a given set of references
    static std::map<std::multiset<QString>, std::set<QString> > hints;
};

class PartDesignExport Line : public PartDesign::Datum
{
    PROPERTY_HEADER(PartDesign::Line);

public:
    App::PropertyVector _Base;
    App::PropertyVector _Direction;

    Line();
    virtual ~Line();

    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderDatumLine";
    }

    static void initHints();
    const std::set<QString> getHint();

protected:
    virtual void onChanged(const App::Property* prop);

private:
    // Hints on what further references are required/possible on this feature for a given set of references
    static std::map<std::multiset<QString>, std::set<QString> > hints;
};

class PartDesignExport Plane : public PartDesign::Datum
{
    PROPERTY_HEADER(PartDesign::Plane);

public:
    App::PropertyVector _Base;
    App::PropertyVector _Normal;

    Plane();

    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderDatumPlane";
    }

    static void initHints();
    const std::set<QString> getHint();

protected:
    virtual void onChanged(const App::Property* prop);

private:
    // Hints on what further references are required/possible on this feature for a given set of references
    static std::map<std::multiset<QString>, std::set<QString> > hints;
};

} //namespace PartDesign


#endif // PARTDESIGN_DATUMFEATURE_H
