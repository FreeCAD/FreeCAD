/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include "PreCompiled.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "Extension.h"
#include "ExtensionContainer.h"
#include <Base/DocumentReader.h>

#ifndef _PreComp_
#   include <xercesc/dom/DOM.hpp>
//#   include <xercesc/parsers/XercesDOMParser.hpp>
#endif


using namespace App;

TYPESYSTEM_SOURCE(App::ExtensionContainer, App::PropertyContainer)

ExtensionContainer::ExtensionContainer() = default;

ExtensionContainer::~ExtensionContainer() {

    //we need to delete all dynamically added extensions
    for(const auto& entry : _extensions) {
        if(entry.second->isPythonExtension())
            delete entry.second;
    }
}

void ExtensionContainer::registerExtension(Base::Type extension, Extension* ext) {
    if(ext->getExtendedContainer() != this)
        throw Base::ValueError("ExtensionContainer::registerExtension: Extension has not this as base object");

    //no duplicate extensions (including base classes)
    if(hasExtension(extension)) {
        for(const auto& entry : _extensions) {
            if(entry.first == extension || entry.first.isDerivedFrom(extension)) {
                _extensions.erase(entry.first);
                break;
            }
        }
    }

    _extensions[extension] = ext;
}

bool ExtensionContainer::hasExtension(Base::Type t, bool derived) const {

    //check for the exact type
    bool found = _extensions.find(t) != _extensions.end();
    if(!found && derived) {
        //and for types derived from it, as they can be cast to the extension
        for(const auto& entry : _extensions) {
            if(entry.first.isDerivedFrom(t))
                return true;
        }
        return false;
    }
    return found;
}

bool ExtensionContainer::hasExtension(const std::string& name) const {

    //and for types derived from it, as they can be cast to the extension
    for(const auto& entry : _extensions) {
        if(entry.second->name() == name)
            return true;
    }
    return false;
}


Extension* ExtensionContainer::getExtension(Base::Type t, bool derived, bool no_except) const {

    auto result = _extensions.find(t);
    if((result == _extensions.end()) && derived) {
        //we need to check for derived types
        for(const auto& entry : _extensions) {
            if(entry.first.isDerivedFrom(t))
                return entry.second;
        }
        if(no_except)
            return nullptr;
        //if we arrive here we don't have anything matching
        throw Base::TypeError("ExtensionContainer::getExtension: No extension of given type available");
    }
    else if (result != _extensions.end()) {
        return result->second;
    }
    else {
        if(no_except)
            return nullptr;
        //if we arrive here we don't have anything matching
        throw Base::TypeError("ExtensionContainer::getExtension: No extension of given type available");
    }
}

bool ExtensionContainer::hasExtensions() const {

    return !_extensions.empty();
}

Extension* ExtensionContainer::getExtension(const std::string& name) const {

    //and for types derived from it, as they can be cast to the extension
    for(const auto& entry : _extensions) {
        if(entry.second->name() == name)
            return entry.second;
    }
    return nullptr;
}

std::vector< Extension* > ExtensionContainer::getExtensionsDerivedFrom(Base::Type type) const {

    std::vector<Extension*> vec;
    //and for types derived from it, as they can be cast to the extension
    for(const auto& entry : _extensions) {
        if(entry.first.isDerivedFrom(type))
            vec.push_back(entry.second);
    }
    return vec;
}

void ExtensionContainer::getPropertyList(std::vector< Property* >& List) const {
    App::PropertyContainer::getPropertyList(List);
    for(const auto& entry : _extensions)
        entry.second->extensionGetPropertyList(List);
}

void ExtensionContainer::getPropertyMap(std::map< std::string, Property* >& Map) const {
    App::PropertyContainer::getPropertyMap(Map);
    for(const auto& entry : _extensions)
        entry.second->extensionGetPropertyMap(Map);
}

Property* ExtensionContainer::getPropertyByName(const char* name) const {
    auto prop = App::PropertyContainer::getPropertyByName(name);
    if(prop)
        return prop;

    for(const auto& entry : _extensions) {
        auto prop = entry.second->extensionGetPropertyByName(name);
        if(prop)
            return prop;
    }

    return nullptr;
}


short int ExtensionContainer::getPropertyType(const Property* prop) const {
    short int res = App::PropertyContainer::getPropertyType(prop);
    if(res != 0)
        return res;

    for(const auto& entry : _extensions) {
        res = entry.second->extensionGetPropertyType(prop);
        if(res != 0)
            return res;
    }

    return 0;
}

short int ExtensionContainer::getPropertyType(const char* name) const {

    short int res = App::PropertyContainer::getPropertyType(name);
    if(res != 0)
        return res;

    for(const auto& entry : _extensions) {
        res = entry.second->extensionGetPropertyType(name);
        if(res != 0)
            return res;
    }

    return 0;
}


const char* ExtensionContainer::getPropertyName(const Property* prop) const {

    const char* res = App::PropertyContainer::getPropertyName(prop);
    if (res)
        return res;

    for (const auto& entry : _extensions) {
        res = entry.second->extensionGetPropertyName(prop);
        if (res)
            return res;
    }

    return nullptr;
}

const char* ExtensionContainer::getPropertyGroup(const Property* prop) const {

    const char* res = App::PropertyContainer::getPropertyGroup(prop);
    if (res)
        return res;

    for (const auto& entry : _extensions) {
        res = entry.second->extensionGetPropertyGroup(prop);
        if (res)
            return res;
    }

    return nullptr;
}

const char* ExtensionContainer::getPropertyGroup(const char* name) const {

    const char* res = App::PropertyContainer::getPropertyGroup(name);
    if (res)
        return res;

    for (const auto& entry : _extensions) {
        res = entry.second->extensionGetPropertyGroup(name);
        if (res)
            return res;
    }

    return nullptr;
}


const char* ExtensionContainer::getPropertyDocumentation(const Property* prop) const {

    const char* res = App::PropertyContainer::getPropertyDocumentation(prop);
    if (res)
        return res;

    for (const auto& entry : _extensions) {
        res = entry.second->extensionGetPropertyDocumentation(prop);
        if (res)
            return res;
    }

    return nullptr;
}

const char* ExtensionContainer::getPropertyDocumentation(const char* name) const {

    const char* res = App::PropertyContainer::getPropertyDocumentation(name);
    if (res)
        return res;

    for(const auto& entry : _extensions) {
        res = entry.second->extensionGetPropertyDocumentation(name);
        if (res)
            return res;
    }

    return nullptr;
}

void ExtensionContainer::onChanged(const Property* prop) {

    //inform all extensions about changed property. This includes all properties from the
    //extended object (this) as well as all extension properties
    for(const auto& entry : _extensions)
        entry.second->extensionOnChanged(prop);

    App::PropertyContainer::onChanged(prop);
}

void ExtensionContainer::Save(Base::Writer& writer) const {

    //Note: save extensions must be called first to ensure that the extension element is always the
    //      very first inside the object element. This is needed since extension element works together with
    //      an object attribute, and if another element would be read first the object attributes would be
    //      cleared.
    saveExtensions(writer);
    App::PropertyContainer::Save(writer);
}

void ExtensionContainer::Restore(Base::XMLReader& reader) {

    //restore dynamic extensions.
    //Note 1: The extension element must be read first, before all other object elements. That is
    //        needed as the element works together with an object element attribute, which would be
    //        cleared if another attribute is read first
    //Note 2: This must happen before the py object of this container is used, as only in the
    //        pyobject constructor the extension methods are added to the container.
    restoreExtensions(reader);
    App::PropertyContainer::Restore(reader);
}

void ExtensionContainer::Restore(Base::DocumentReader& reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *containerEl) {
    //restore dynamic extensions.
    //Note 1: The extension element must be read first, before all other object elements. That is
    //        needed as the element works together with an object element attribute, which would be
    //        cleared if another attribute is read first
    //Note 2: This must happen before the py object of this container is used, as only in the
    //        pyobject constructor the extension methods are added to the container.
    restoreExtensions(reader,containerEl);
    App::PropertyContainer::Restore(reader,containerEl);
}

void ExtensionContainer::saveExtensions(Base::Writer& writer) const {

    //we don't save anything if there are no dynamic extensions
    if(!hasExtensions())
        return;

    //save dynamic extensions
    writer.incInd(); // indentation for 'Extensions'
    writer.Stream() << writer.ind() << "<Extensions Count=\"" << _extensions.size() << "\">" << std::endl;
    for(const auto& entry : _extensions) {

        auto ext = entry.second;
        writer.incInd(); // indentation for 'Extension name'
        writer.Stream() << writer.ind() << "<Extension"
        << " type=\"" << ext->getExtensionTypeId().getName() <<"\""
        << " name=\"" << ext->name() << "\">" << std::endl;
        writer.incInd(); // indentation for the actual Extension
        try {
            // We must make sure to handle all exceptions accordingly so that
            // the project file doesn't get invalidated. In the error case this
            // means to proceed instead of aborting the write operation.
            ext->extensionSave(writer);
        }
        catch (const Base::Exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const std::exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const char* e) {
            Base::Console().Error("%s\n", e);
        }
#ifndef FC_DEBUG
        catch (...) {
            Base::Console().Error("ExtensionContainer::Save: Unknown C++ exception thrown. Try to continue...\n");
        }
#endif
        writer.decInd(); // indentation for the actual extension
        writer.Stream() << writer.ind() << "</Extension>" << std::endl;
        writer.decInd(); // indentation for 'Extension name'
    }
    writer.Stream() << writer.ind() << "</Extensions>" << std::endl;
    writer.decInd();
}

void ExtensionContainer::restoreExtensions(Base::DocumentReader& reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *containerEl) {
    //Dynamic extensions are optional (also because they are introduced late into the document format)
    //and hence it is possible that the element does not exist. As we cannot check for the existence of
    //an element a object attribute is set if extensions are available. Here we check that
    //attribute, and only if it exists the extensions element will be available.
    const char* expanded_cstr = reader.GetAttribute(containerEl,"expanded");
    if(!expanded_cstr)
    	return;
    auto ExtensionsDOM = reader.FindElement(containerEl,"Extensions");
    if(ExtensionsDOM){
    	const char* cnt_cstr = reader.GetAttribute(ExtensionsDOM,"Count");
    	if(cnt_cstr){
    		long Cnt = reader.ContentToInt( cnt_cstr );
    		auto prev_ExtensionDOM = reader.FindElement(ExtensionsDOM,"Extension");
    		readExtension(reader,prev_ExtensionDOM);
    		for (int i=1 ;i<Cnt ;i++) {
    			auto ExtensionDOM_i = reader.FindNextElement(prev_ExtensionDOM,"Extension");
    			readExtension(reader,ExtensionDOM_i);
				prev_ExtensionDOM = ExtensionDOM_i;
			}
			
			
    	}
    }
}

void ExtensionContainer::readExtension(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ExtensionDOM){
	const char* type_cstr = reader.GetAttribute(ExtensionDOM,"type");
	const char* name_cstr = reader.GetAttribute(ExtensionDOM,"name");
	try {
	    App::Extension* ext = getExtension(name_cstr);
	    if(!ext) {
	        //get the extension type asked for
	        Base::Type extension =  Base::Type::fromName(type_cstr);
	        if (extension.isBad() || !extension.isDerivedFrom(App::Extension::getExtensionClassTypeId())) {
	            std::stringstream str;
	            str << "No extension found of type '" << type_cstr << "'" << std::ends;
	            throw Base::TypeError(str.str());
	        }

	        //register the extension
	        ext = static_cast<App::Extension*>(extension.createInstance());
	        //check if this really is a python extension!
	        if (!ext->isPythonExtension()) {
	            delete ext;
	            std::stringstream str;
	            str << "Extension is not a python addable version: '" << type_cstr << "'" << std::ends;
	            throw Base::TypeError(str.str());
	        }
	        ext->initExtension(this);
	    }
	    if (ext && strcmp(ext->getExtensionTypeId().getName(), type_cstr) == 0)
	    	ext->extensionRestore(reader);
	}
	catch (const Base::XMLParseException&) {
	    throw; // re-throw
	}
	catch (const Base::Exception &e) {
	    Base::Console().Error("%s\n", e.what());
	}
	catch (const std::exception &e) {
	    Base::Console().Error("%s\n", e.what());
	}
	catch (const char* e) {
	    Base::Console().Error("%s\n", e);
	}
#ifndef FC_DEBUG
	catch (...) {
	    Base::Console().Error("ExtensionContainer::Restore: Unknown C++ exception thrown\n");
	}
#endif

}

void ExtensionContainer::restoreExtensions(Base::XMLReader& reader) {

    //Dynamic extensions are optional (also because they are introduced late into the document format)
    //and hence it is possible that the element does not exist. As we cannot check for the existence of
    //an element a object attribute is set if extensions are available. Here we check that
    //attribute, and only if it exists the extensions element will be available.
    if(!reader.hasAttribute("Extensions"))
        return;

    reader.readElement("Extensions");
    int Cnt = reader.getAttributeAsInteger("Count");

    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Extension");
        const char* Type = reader.getAttribute("type");
        const char* Name = reader.getAttribute("name");
        try {
            App::Extension* ext = getExtension(Name);
            if(!ext) {
                //get the extension type asked for
                Base::Type extension =  Base::Type::fromName(Type);
                if (extension.isBad() || !extension.isDerivedFrom(App::Extension::getExtensionClassTypeId())) {
                    std::stringstream str;
                    str << "No extension found of type '" << Type << "'" << std::ends;
                    throw Base::TypeError(str.str());
                }

                //register the extension
                ext = static_cast<App::Extension*>(extension.createInstance());
                //check if this really is a python extension!
                if (!ext->isPythonExtension()) {
                    delete ext;
                    std::stringstream str;
                    str << "Extension is not a python addable version: '" << Type << "'" << std::ends;
                    throw Base::TypeError(str.str());
                }

                ext->initExtension(this);
            }
            if (ext && strcmp(ext->getExtensionTypeId().getName(), Type) == 0)
                ext->extensionRestore(reader);
        }
        catch (const Base::XMLParseException&) {
            throw; // re-throw
        }
        catch (const Base::Exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const std::exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const char* e) {
            Base::Console().Error("%s\n", e);
        }
#ifndef FC_DEBUG
        catch (...) {
            Base::Console().Error("ExtensionContainer::Restore: Unknown C++ exception thrown\n");
        }
#endif

        reader.readEndElement("Extension");
    }
    reader.readEndElement("Extensions");
}

void ExtensionContainer::handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName)
{
    //inform all extensions about changed property name. This includes all properties from the
    //extended object (this) as well as all extension properties
    for(const auto& entry : _extensions) {
        bool handled = entry.second->extensionHandleChangedPropertyName(reader, TypeName, PropName);

        if(handled)
            return; // one property change needs only be handled once
    }

    PropertyContainer::handleChangedPropertyName(reader, TypeName, PropName);
}

void ExtensionContainer::handleChangedPropertyName(Base::DocumentReader &reader, const char * TypeName, const char *PropName)
{
    //inform all extensions about changed property name. This includes all properties from the
    //extended object (this) as well as all extension properties
    for(const auto& entry : _extensions) {
        bool handled = entry.second->extensionHandleChangedPropertyName(reader, TypeName, PropName);

        if(handled)
            return; // one property change needs only be handled once
    }

    PropertyContainer::handleChangedPropertyName(reader, TypeName, PropName);
}

void ExtensionContainer::handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, Property * prop)
{
    //inform all extensions about changed property type. This includes all properties from the
    //extended object (this) as well as all extension properties
    for(const auto& entry : _extensions) {
        bool handled = entry.second->extensionHandleChangedPropertyType(reader, TypeName, prop);

        if(handled)
            return; // one property change needs only be handled once
    }

    PropertyContainer::handleChangedPropertyType(reader, TypeName, prop);
}

void ExtensionContainer::handleChangedPropertyType(Base::DocumentReader &reader, const char * TypeName, Property * prop)
{
    //inform all extensions about changed property type. This includes all properties from the
    //extended object (this) as well as all extension properties
    for(const auto& entry : _extensions) {
        bool handled = entry.second->extensionHandleChangedPropertyType(reader, TypeName, prop);

        if(handled)
            return; // one property change needs only be handled once
    }

    PropertyContainer::handleChangedPropertyType(reader, TypeName, prop);
}
