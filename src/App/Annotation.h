/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef APP_ANNOTATION_H
#define APP_ANNOTATION_H

#include "DocumentObject.h"
#include "PropertyGeo.h"
#include "PropertyStandard.h"


namespace App
{

class AppExport Annotation : public DocumentObject
{
    PROPERTY_HEADER(App::Annotation);

public:
    /// Constructor
    Annotation(void);
    virtual ~Annotation();

    App::PropertyStringList LabelText;
    App::PropertyVector Position;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "Gui::ViewProviderAnnotation";
    }
};

class AppExport AnnotationLabel : public DocumentObject
{
    PROPERTY_HEADER(App::AnnotationLabel);

public:
    /// Constructor
    AnnotationLabel(void);
    virtual ~AnnotationLabel();

    App::PropertyStringList LabelText;
    App::PropertyVector BasePosition;
    App::PropertyVector TextPosition;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "Gui::ViewProviderAnnotationLabel";
    }
};

} //namespace App


#endif // APP_ANNOTATION_H
