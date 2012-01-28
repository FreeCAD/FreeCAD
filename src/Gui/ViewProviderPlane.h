/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2012     *
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


#ifndef GUI_ViewProviderPlane_H
#define GUI_ViewProviderPlane_H

#include "ViewProviderGeometryObject.h"
#include <QObject>

class SoFontStyle;
class SoText2;
class SoBaseColor;
class SoTranslation;
class SoCoordinate3;
class SoIndexedLineSet;
class SoEventCallback;
class SoMaterial;

namespace Gui
{


class GuiExport ViewProviderPlane : public ViewProviderGeometryObject
{
    PROPERTY_HEADER(Gui::ViewProviderPlane);

public:
    /// Constructor
    ViewProviderPlane(void);
    virtual ~ViewProviderPlane();

    void attach(App::DocumentObject *);
    void updateData(const App::Property*);
    std::vector<std::string> getDisplayModes(void) const;
    void setDisplayMode(const char* ModeName);

   /// indicates if the ViewProvider use the new Selection model
    virtual bool useNewSelectionModel(void) const {return true;}
    /// indicates if the ViewProvider can be selected
    virtual bool isSelectable(void) const ;
    /// return a hit element to the selection path or 0
    virtual std::string getElement(const SoDetail *) const;
    virtual SoDetail* getDetail(const char*) const;

protected:
    void onChanged(const App::Property* prop);

private:
    SoCoordinate3    * pCoords;
    SoMaterial       * pMat;
    SoIndexedLineSet * pLines;
};

} //namespace Gui


#endif // GUI_ViewProviderPlane_H
