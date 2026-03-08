// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#pragma once

#include <Mod/Measure/MeasureGlobal.h>

#include <memory>
#include <QString>

#include <App/DocumentObject.h>
#include <App/MeasureManager.h>
#include <App/DocumentObserver.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/FeaturePython.h>
#include <App/Link.h>
#include <Base/Quantity.h>
#include <Base/Placement.h>
#include <Base/Interpreter.h>

#include <Mod/Part/App/MeasureInfo.h>
#include <Mod/Part/App/MeasureClient.h>  // needed?


namespace Measure
{

class MeasureExport MeasureBase: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Measure::MeasureBase);

public:
    MeasureBase();
    ~MeasureBase() override = default;

    App::PropertyPlacement Placement;

    // fastsignals::signal<void (const MeasureBase*)> signalGuiInit;

    // return PyObject as MeasureBasePy
    PyObject* getPyObject() override;

    // Initialize measurement properties from selection
    virtual void parseSelection(const App::MeasureSelection& selection);


    virtual QString getResultString();

    virtual std::vector<std::string> getInputProps();
    virtual App::Property* getResultProp()
    {
        return {};
    }

    // Return the objects that are measured
    virtual std::vector<App::DocumentObject*> getSubject() const;

private:
    Py::Object getProxyObject() const;

protected:
    void onDocumentRestored() override;
};

// Create a scriptable object based on MeasureBase
using MeasurePython = App::FeaturePythonT<MeasureBase>;

template<typename T>
class MeasureExport MeasureBaseExtendable: public MeasureBase
{

    using GeometryHandler = std::function<Part::MeasureInfoPtr(const App::SubObjectT&)>;
    using HandlerMap = std::map<std::string, GeometryHandler>;


public:
    static void addGeometryHandler(const std::string& module, GeometryHandler callback)
    {
        _mGeometryHandlers[module] = callback;
    }

    static GeometryHandler getGeometryHandler(const std::string& module)
    {

        if (!hasGeometryHandler(module)) {
            return {};
        }

        return _mGeometryHandlers[module];
    }

    static Part::MeasureInfoPtr getMeasureInfo(App::SubObjectT& subObjT)
    {

        // Resolve App::Link
        App::DocumentObject* sub = subObjT.getSubObject();
        if (!sub) {
            return nullptr;
        }

        if (sub->isDerivedFrom<App::Link>()) {
            auto link = static_cast<App::Link*>(sub);
            sub = link->getLinkedObject(true);
        }

        // Get the Geometry handler based on the module
        const char* className = sub->getTypeId().getName();
        std::string mod = Base::Type::getModuleName(className);

        auto handler = getGeometryHandler(mod);
        if (!handler) {
            Base::Console().log(
                "MeasureBaseExtendable::getMeasureInfo: No geometry handler "
                "available for submitted element type"
            );
            return nullptr;
        }

        return handler(subObjT);
    }

    static void addGeometryHandlers(const std::vector<std::string>& modules, GeometryHandler callback)
    {
        // TODO: this will replace a callback with a later one.  Should we check that there isn't
        // already a handler defined for this module?
        for (auto& mod : modules) {
            _mGeometryHandlers[mod] = callback;
        }
    }


    static bool hasGeometryHandler(const std::string& module)
    {
        return (_mGeometryHandlers.count(module) > 0);
    }

private:
    inline static HandlerMap _mGeometryHandlers = MeasureBaseExtendable<T>::HandlerMap();
};


}  // namespace Measure
