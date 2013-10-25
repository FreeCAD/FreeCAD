/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_VIEWPROVIDERPYTHONFEATURE_H
#define GUI_VIEWPROVIDERPYTHONFEATURE_H

#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/ViewProviderPythonFeaturePy.h>
#include <App/PropertyPythonObject.h>
#include <App/DynamicProperty.h>

class SoSensor;
class SoDragger;
class SoNode;

namespace Gui {
class SoFCSelection;
class SoFCBoundingBox;

class GuiExport ViewProviderPythonFeatureImp
{
public:
    /// constructor.
    ViewProviderPythonFeatureImp(ViewProviderDocumentObject*);
    /// destructor.
    ~ViewProviderPythonFeatureImp();

    // Returns the icon
    QIcon getIcon() const;
    std::vector<App::DocumentObject*> claimChildren(const std::vector<App::DocumentObject*>&) const;
    bool useNewSelectionModel() const;
    std::string getElement(const SoDetail *det) const;
    SoDetail* getDetail(const char*) const;
    std::vector<Base::Vector3d> getSelectionShape(const char* Element) const;
    bool setEdit(int ModNum);
    bool unsetEdit(int ModNum);
    bool doubleClicked(void);

    /** @name Update data methods*/
    //@{
    void attach(App::DocumentObject *pcObject);
    void updateData(const App::Property*);
    void onChanged(const App::Property* prop);
    void startRestoring();
    void finishRestoring();
    //@}

    /** @name Display methods */
    //@{
    /// get the default display mode
    const char* getDefaultDisplayMode() const;
    /// returns a list of all possible modes
    std::vector<std::string> getDisplayModes(void) const;
    /// set the display mode
    std::string setDisplayMode(const char* ModeName);
    //@}

private:
    ViewProviderDocumentObject* object;
};

template <class ViewProviderT>
class ViewProviderPythonFeatureT : public ViewProviderT
{
    PROPERTY_HEADER(Gui::ViewProviderPythonFeatureT<ViewProviderT>);

public:
    /// constructor.
    ViewProviderPythonFeatureT() : _attached(false) {
        ADD_PROPERTY(Proxy,(Py::Object()));
        imp = new ViewProviderPythonFeatureImp(this);
        props = new App::DynamicProperty(this);
    }
    /// destructor.
    virtual ~ViewProviderPythonFeatureT() {
        delete imp;
        delete props;
    }

    // Returns the icon
    QIcon getIcon() const {
        QIcon icon = imp->getIcon();
        if (icon.isNull())
            icon = ViewProviderT::getIcon();
        return icon;
    }

    std::vector<App::DocumentObject*> claimChildren() const {
        return imp->claimChildren(ViewProviderT::claimChildren());
    }

    /** @name Nodes */
    //@{
    virtual SoSeparator* getRoot() {
        return ViewProviderT::getRoot();
    }
    virtual SoSeparator* getFrontRoot() const {
        return ViewProviderT::getFrontRoot();
    }
    // returns the root node of the Provider (3D)
    virtual SoSeparator* getBackRoot() const {
        return ViewProviderT::getBackRoot();
    }
    //@}

    /** @name Selection handling */
    //@{
    virtual bool useNewSelectionModel() const {
        return imp->useNewSelectionModel();
    }
    virtual std::string getElement(const SoDetail *det) const {
        std::string name = imp->getElement(det);
        if (!name.empty()) return name;
        return ViewProviderT::getElement(det);
    }
    virtual SoDetail* getDetail(const char* name) const {
        SoDetail* det = imp->getDetail(name);
        if (det) return det;
        return ViewProviderT::getDetail(name);
    }
    virtual std::vector<Base::Vector3d> getSelectionShape(const char* Element) const {
        return ViewProviderT::getSelectionShape(Element);
    };
    //@}

    /** @name Update data methods*/
    //@{
    virtual void attach(App::DocumentObject *obj) {
        // delay loading of the actual attach() method because the Python
        // view provider class is not attached yet
        ViewProviderT::pcObject = obj;
    }
    virtual void updateData(const App::Property* prop) {
        imp->updateData(prop);
        ViewProviderT::updateData(prop);
    }
    virtual void getTaskViewContent(std::vector<Gui::TaskView::TaskContent*>& c) const {
        ViewProviderT::getTaskViewContent(c);
    }
    //@}

    /** @name Restoring view provider from document load */
    //@{
    virtual void startRestoring() {
        imp->startRestoring();
    }
    virtual void finishRestoring() {
        imp->finishRestoring();
    }
    //@}

    /** @name Display methods */
    //@{
    /// get the default display mode
    virtual const char* getDefaultDisplayMode() const {
        return imp->getDefaultDisplayMode();
    }
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const {
        std::vector<std::string> modes = ViewProviderT::getDisplayModes();
        std::vector<std::string> more_modes = imp->getDisplayModes();
        modes.insert(modes.end(), more_modes.begin(), more_modes.end());
        return modes;
    }
    /// set the display mode
    virtual void setDisplayMode(const char* ModeName) {
        std::string mask = imp->setDisplayMode(ModeName);
        ViewProviderT::setDisplayMaskMode(mask.c_str());
        ViewProviderT::setDisplayMode(ModeName);
    }
    //@}

    /** @name Access properties */
    //@{
    App::Property* addDynamicProperty(
        const char* type, const char* name=0,
        const char* group=0, const char* doc=0,
        short attr=0, bool ro=false, bool hidden=false) {
        return props->addDynamicProperty(type, name, group, doc, attr, ro, hidden);
    }
    virtual bool removeDynamicProperty(const char* name) {
        return props->removeDynamicProperty(name);
    }
    std::vector<std::string> getDynamicPropertyNames() const {
        return props->getDynamicPropertyNames();
    }
    App::Property *getDynamicPropertyByName(const char* name) const {
        return props->getDynamicPropertyByName(name);
    }
    virtual void addDynamicProperties(const App::PropertyContainer* cont) {
        return props->addDynamicProperties(cont);
    }
    /// get all properties of the class (including parent)
    virtual void getPropertyMap(std::map<std::string,App::Property*> &Map) const {
        return props->getPropertyMap(Map);
    }
    /// find a property by its name
    virtual App::Property *getPropertyByName(const char* name) const {
        return props->getPropertyByName(name);
    }
    /// get the name of a property
    virtual const char* getName(const App::Property* prop) const {
        return props->getName(prop);
    }
    //@}

    /** @name Property attributes */
    //@{
    /// get the Type of a Property
    short getPropertyType(const App::Property* prop) const {
        return props->getPropertyType(prop);
    }
    /// get the Type of a named Property
    short getPropertyType(const char *name) const {
        return props->getPropertyType(name);
    }
    /// get the Group of a Property
    const char* getPropertyGroup(const App::Property* prop) const {
        return props->getPropertyGroup(prop);
    }
    /// get the Group of a named Property
    const char* getPropertyGroup(const char *name) const {
        return props->getPropertyGroup(name);
    }
    /// get the Group of a Property
    const char* getPropertyDocumentation(const App::Property* prop) const {
        return props->getPropertyDocumentation(prop);
    }
    /// get the Group of a named Property
    const char* getPropertyDocumentation(const char *name) const {
        return props->getPropertyDocumentation(name);
    }
    /// check if the property is read-only
    bool isReadOnly(const App::Property* prop) const {
        return props->isReadOnly(prop);
    }
    /// check if the nameed property is read-only
    bool isReadOnly(const char *name) const {
        return props->isReadOnly(name);
    }
    /// check if the property is hidden
    bool isHidden(const App::Property* prop) const {
        return props->isHidden(prop);
    }
    /// check if the named property is hidden
    bool isHidden(const char *name) const {
        return props->isHidden(name);
    }
    //@}

    /** @name Property serialization */
    //@{
    void Save (Base::Writer &writer) const {
        props->Save(writer);
    }
    void Restore(Base::XMLReader &reader) {
        props->Restore(reader);
    }
    //@}

    PyObject* getPyObject() {
        if (!ViewProviderT::pyViewObject)
            ViewProviderT::pyViewObject = new ViewProviderPythonFeaturePy(this);
        ViewProviderT::pyViewObject->IncRef();
        return ViewProviderT::pyViewObject;
    }

protected:
    virtual void onChanged(const App::Property* prop) {
        if (prop == &Proxy) {
            if (ViewProviderT::pcObject && !Proxy.getValue().is(Py::_None())) {
                if (!_attached) {
                    _attached = true;
                    imp->attach(ViewProviderT::pcObject);
                    ViewProviderT::attach(ViewProviderT::pcObject);
                    // needed to load the right display mode after they're known now
                    ViewProviderT::DisplayMode.touch();
                }
                ViewProviderT::updateView();
            }
        }
        else {
            imp->onChanged(prop);
            ViewProviderT::onChanged(prop);
        }
    }
    /// is called by the document when the provider goes in edit mode
    virtual bool setEdit(int ModNum)
    {
        bool ok = imp->setEdit(ModNum);
        if (!ok) ok = ViewProviderT::setEdit(ModNum);
        return ok;
    }
    /// is called when you loose the edit mode
    virtual void unsetEdit(int ModNum)
    {
        bool ok = imp->unsetEdit(ModNum);
        if (!ok) ViewProviderT::unsetEdit(ModNum);
    }

    virtual bool doubleClicked(void)
    {
        bool ok = imp->doubleClicked();
        if (!ok) 
            return ViewProviderT::doubleClicked();
        else 
            return true;
    }

private:
    ViewProviderPythonFeatureImp* imp;
    App::DynamicProperty *props;
    App::PropertyPythonObject Proxy;
    bool _attached;
};

// Special Feature-Python classes
typedef ViewProviderPythonFeatureT<ViewProviderDocumentObject> ViewProviderPythonFeature;
typedef ViewProviderPythonFeatureT<ViewProviderGeometryObject> ViewProviderPythonGeometry;

} // namespace Gui

#endif // GUI_VIEWPROVIDERPYTHONFEATURE_H

