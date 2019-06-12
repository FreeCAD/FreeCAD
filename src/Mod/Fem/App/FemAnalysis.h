/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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


#ifndef Fem_FemAnalysis_H
#define Fem_FemAnalysis_H


#include <App/DocumentObjectGroup.h>
#include <App/PropertyLinks.h>
#include <App/FeaturePython.h>



namespace Fem
{

class AppFemExport FemAnalysis : public App::DocumentObjectGroup
{
    PROPERTY_HEADER(Fem::FemAnalysis);

public:
    /// Constructor
    FemAnalysis(void);
    virtual ~FemAnalysis();

    /// unique identifier of the Analysis
    App::PropertyUUID    Uid;

    virtual const char* getViewProviderName() const {
        return "FemGui::ViewProviderFemAnalysis";
    }

protected:
    /// Support of backward compatibility
    virtual void handleChangedPropertyName(Base::XMLReader &reader,
                                           const char * TypeName,
                                           const char *PropName);
};

class AppFemExport DocumentObject : public App::DocumentObject
{
    PROPERTY_HEADER(Fem::DocumentObject);
};

typedef App::FeaturePythonT<FemAnalysis> FemAnalysisPython;
typedef App::FeaturePythonT<DocumentObject> FeaturePython;


} //namespace Fem


#endif // Fem_FemAnalysis_H
