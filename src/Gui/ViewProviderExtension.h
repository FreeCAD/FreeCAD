/***************************************************************************
 *   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2016     *
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


#ifndef GUI_VIEWPROVIDEREXTENSION_H
#define GUI_VIEWPROVIDEREXTENSION_H

#include "App/Extension.h"
#include "ViewProvider.h"
#include "ViewProviderDocumentObject.h"

namespace Gui {
    
/**
 * @brief Extension with special viewprovider calls
 * 
 */
class GuiExport ViewProviderExtension : public App::Extension
{

    //The cass does not have properties itself, but it is important to provide the property access
    //functions.
    EXTENSION_PROPERTY_HEADER(Gui::ViewProviderExtension);

public:

    ViewProviderExtension ();
    virtual ~ViewProviderExtension ();

    Gui::ViewProviderDocumentObject*       getExtendedViewProvider();
    const Gui::ViewProviderDocumentObject* getExtendedViewProvider() const;
   
    virtual std::vector<App::DocumentObject*> extensionClaimChildren3D(void) const { 
        return std::vector<App::DocumentObject*>(); }
        
    virtual bool extensionOnDelete(const std::vector<std::string> &){ return true;}
    virtual void extensionBeforeDelete(){}
 
    virtual std::vector<App::DocumentObject*> extensionClaimChildren(void) const { 
        return std::vector<App::DocumentObject*>(); }

    virtual bool extensionCanDragObjects() const { return false; }
    virtual bool extensionCanDragObject(App::DocumentObject*) const { return true; }
    virtual void extensionDragObject(App::DocumentObject*) { }
    virtual bool extensionCanDropObjects() const { return false; }
    virtual bool extensionCanDropObject(App::DocumentObject*) const { return true; }
    virtual bool extensionCanDragAndDropObject(App::DocumentObject*) const { return true; }
    virtual void extensionDropObject(App::DocumentObject*) { }
    virtual bool extensionCanDropObjectEx(App::DocumentObject *, App::DocumentObject *, 
            const char *, const std::vector<std::string> &) const
        { return false; }
    virtual std::string extensionDropObjectEx(App::DocumentObject *obj, App::DocumentObject *, 
            const char *, const std::vector<std::string> &) 
        { extensionDropObject(obj); return std::string(); }

    virtual int extensionReplaceObject(App::DocumentObject* /*oldValue*/, App::DocumentObject* /*newValue*/) 
        { return -1; }

    /// Hides the view provider
    virtual void extensionHide(void) { }
    /// Shows the view provider
    virtual void extensionShow(void) { }

    virtual void extensionModeSwitchChange(void) { }
    
    virtual SoSeparator* extensionGetFrontRoot(void) const {return nullptr;}
    virtual SoGroup*     extensionGetChildRoot(void) const {return nullptr;}
    virtual SoSeparator* extensionGetBackRoot(void) const {return nullptr;}
    virtual void extensionAttach(App::DocumentObject* ) { }
    virtual void extensionReattach(App::DocumentObject* ) { }
    virtual void extensionSetDisplayMode(const char* ) { }
    virtual std::vector<std::string> extensionGetDisplayModes(void) const {return std::vector<std::string>();}

    //update data of extended opject
    virtual void extensionUpdateData(const App::Property*);

    virtual void extensionStartRestoring() {}
    virtual void extensionFinishRestoring() {}

    virtual bool extensionGetElementPicked(const SoPickedPoint *, std::string &) const {return false;}
    virtual bool extensionGetDetailPath(const char *, SoFullPath *, SoDetail *&) const {return false;}
    
private:
  //Gui::ViewProviderDocumentObject* m_viewBase = nullptr;
};

/**
 * Generic Python extension class which allows to behave every extension
 * derived class as Python extension -- simply by subclassing.
 */
template <class ExtensionT>
class ViewProviderExtensionPythonT : public ExtensionT
{
    EXTENSION_PROPERTY_HEADER(Gui::ViewProviderExtensionPythonT<ExtensionT>);

public:
    typedef ExtensionT Inherited;
    
    ViewProviderExtensionPythonT() {
        ExtensionT::m_isPythonExtension = true;
        ExtensionT::initExtensionType(ViewProviderExtensionPythonT::getExtensionClassTypeId());
        
        EXTENSION_ADD_PROPERTY(ExtensionProxy,(Py::Object()));
    }
    virtual ~ViewProviderExtensionPythonT() {
    }

    App::PropertyPythonObject ExtensionProxy;
};

typedef ViewProviderExtensionPythonT<Gui::ViewProviderExtension> ViewProviderExtensionPython;

} //Gui

#endif // GUI_VIEWPROVIDEREXTENSION_H
