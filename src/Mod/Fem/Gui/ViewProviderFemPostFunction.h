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


#ifndef FEM_VIEWPROVIDERFEMPOSTFUNCTION_H
#define FEM_VIEWPROVIDERFEMPOSTFUNCTION_H

#include <Gui/ViewProviderDocumentObject.h>
#include <Inventor/SbMatrix.h>

class SoScale;
class SoSurroundScale;
class SoTransformManip;
class SoComposeMatrix;
class SoMatrixTransform;
class SoDragger;

namespace FemGui
{
    
class FemGuiExport ViewProviderFemPostFunctionProvider : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER(FemGui::ViewProviderFemPostFunction);

public:    
    ViewProviderFemPostFunctionProvider();
    virtual ~ViewProviderFemPostFunctionProvider();

    virtual std::vector< App::DocumentObject* > claimChildren(void) const;
    virtual std::vector< App::DocumentObject* > claimChildren3D(void) const;
};

class FemGuiExport ViewProviderFemPostFunction : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER(FemGui::ViewProviderFemPostFunction);

public:
    /// constructor.
    ViewProviderFemPostFunction();
    ~ViewProviderFemPostFunction();

    void attach(App::DocumentObject *pcObject);
    std::vector<std::string> getDisplayModes() const;
   
protected:    
    void setAutoScale(bool value) {m_autoscale = value;};
    bool autoScale()              {return m_autoscale;};
    
    bool isDragging() {return m_isDragging;};
    
    virtual SoTransformManip*   setupManipulator();
    virtual void                draggerUpdate(SoDragger* m) {};
    SoTransformManip*           getManipulator() {return m_manip;};
    SoSeparator*                getGeometryNode() {return m_geometrySeperator;};
    
private:
    static void dragStartCallback(void * data, SoDragger * d);
    static void dragFinishCallback(void * data, SoDragger * d);
    static void dragMotionCallback(void * data, SoDragger * d);
    
    SoSeparator*        m_geometrySeperator;
    SoTransformManip*   m_manip;
    SoScale*            m_scale;
    bool                m_autoscale, m_isDragging, m_autoRecompute;
};

class FemGuiExport ViewProviderFemPostPlaneFunction : public ViewProviderFemPostFunction {
  
    PROPERTY_HEADER(FemGui::ViewProviderFemPostPlaneFunction);
    
public:
    ViewProviderFemPostPlaneFunction();     
    virtual ~ViewProviderFemPostPlaneFunction();
    
protected:
    virtual void draggerUpdate(SoDragger* mat);
    virtual void updateData(const App::Property*);
};

} //namespace FemGui


#endif // FEM_VIEWPROVIDERFEMPOSTFUNCTION_H