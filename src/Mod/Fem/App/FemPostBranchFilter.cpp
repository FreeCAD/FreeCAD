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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <Python.h>
#include <vtkDataSetReader.h>
#include <vtkImageData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLPUnstructuredGridReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#endif

#include <Base/Console.h>

#include "FemMesh.h"
#include "FemMeshObject.h"
#include "FemPostBranchFilter.h"
#include "FemPostBranchFilterPy.h"
#include "FemVTKTools.h"


using namespace Fem;
using namespace App;

PROPERTY_SOURCE_WITH_EXTENSIONS(Fem::FemPostBranchFilter, Fem::FemPostFilter);

const char* FemPostBranchFilter::OutputEnums[] = {"Passthrough", "Append", nullptr};

FemPostBranchFilter::FemPostBranchFilter() : Fem::FemPostFilter(), Fem::FemPostGroupExtension()
{
    FemPostGroupExtension::initExtension(this);

    ADD_PROPERTY_TYPE(Output,
                      (long(0)),
                      "Pipeline",
                      App::Prop_None,
                      "Selects what the output of the branch itself is\n"
                      "In passthrough the branchs output is equal its imput.\n"
                      "In append, all child filter outputs gets appended as the branches output");

    Output.setEnums(OutputEnums);

    /* We always have a passthrough filter. This allows to connect our children
     * dependend on the Mode setting, without worrying about the connection to our input
     * filter. We do not care if the input filter changes, as this is affecting only the passthrough
     * input and does not affect our child connections.
     * Dependent on our output mode, the passthrough is also used as output, but potentially
     * the append filter is used. in this case our children need to be connected into the append filter.
     * Here the same holds as before: Append filter output can be connected to arbitrary other filters
     * in the pipeline, not affecting our internal connections to our children.
     */

    m_append = vtkSmartPointer<vtkAppendFilter>::New();
    m_passthrough = vtkSmartPointer<vtkPassThrough>::New();
    m_transform_filter->SetInputConnection(m_passthrough->GetOutputPort(0));

    FilterPipeline passthrough;
    passthrough.source = m_passthrough;
    passthrough.target = m_passthrough;
    addFilterPipeline(passthrough, "passthrough");

    FilterPipeline append;
    append.source = m_passthrough;
    append.target = m_append;
    addFilterPipeline(append, "append");

    setTransformLocation(TransformLocation::input);
    setActiveFilterPipeline("passthrough");
}

FemPostBranchFilter::~FemPostBranchFilter() = default;

short FemPostBranchFilter::mustExecute() const
{
    if (Mode.isTouched()) {
        return 1;
    }

    return FemPostFilter::mustExecute();
}

void FemPostBranchFilter::setupPipeline()
{
    // we check if all connections are right and add new ones if needed
    std::vector<App::DocumentObject*> objs = Group.getValues();

    if (objs.empty()) {
        return;
    }


    // prepare output filter: we make all connections new!
    m_append->RemoveAllInputConnections(0);

    FemPostFilter* filter = NULL;
    std::vector<App::DocumentObject*>::iterator it = objs.begin();
    for (; it != objs.end(); ++it) {

        // prepare the filter: make all connections new
        FemPostFilter* nextFilter = static_cast<FemPostFilter*>(*it);
        nextFilter->getFilterInput()->RemoveAllInputConnections(0);

        // handle input modes
        if (Mode.getValue() == 0) {
            // serial: the next filter gets the previous output, the first one gets our input
            if (filter == NULL) {
                nextFilter->getFilterInput()->SetInputConnection(m_passthrough->GetOutputPort());
            } else {
                nextFilter->getFilterInput()->SetInputConnection(filter->getFilterOutput()->GetOutputPort());
            }

        }
        else if (Mode.getValue() == 1) {
            // parallel: all filters get out input
            nextFilter->getFilterInput()->SetInputConnection(m_passthrough->GetOutputPort());
        }

        // handle append filter
        m_append->AddInputConnection(0, nextFilter->getFilterOutput()->GetOutputPort());

        filter = nextFilter;
    };
}

void FemPostBranchFilter::onChanged(const Property* prop)
{
    /* onChanged handles the Pipeline setup: we connect the inputs and outputs
     * of our child filters correctly according to the new settings
     */

    if (prop == &Group || prop == &Mode) {
        setupPipeline();
    }

    if (prop == &Frame) {
        //Update all children with the new step
        for (const auto& obj : Group.getValues()) {
            if (obj->isDerivedFrom(FemPostFilter::getClassTypeId())) {
                static_cast<Fem::FemPostFilter*>(obj)->Frame.setValue(Frame.getValue());
            }
        }
    }

    if (prop == &Output) {
        if (Output.getValue() == 0) {
            setActiveFilterPipeline("passthrough");
        }
        else {
            setActiveFilterPipeline("append");
        }
        // inform toplevel pipeline we changed
        App::DocumentObject* group = FemPostGroupExtension::getGroupOfObject(this);
        if (!group) {
            return;
        }
        if (group->hasExtension(FemPostGroupExtension::getExtensionClassTypeId())) {
            auto postgroup = group->getExtensionByType<FemPostGroupExtension>();
            postgroup->filterChanged(this);
        }
    }

    FemPostFilter::onChanged(prop);
}

void FemPostBranchFilter::filterChanged(FemPostFilter* filter)
{

    //we only need to update the following children if we are in serial mode
    if (Mode.getValue() == 0) {

        std::vector<App::DocumentObject*> objs = Group.getValues();

        if (objs.empty()) {
            return;
        }
        bool started = false;
        std::vector<App::DocumentObject*>::iterator it = objs.begin();
        for (; it != objs.end(); ++it) {

            if (started) {
                (*it)->touch();
            }

            if (*it == filter) {
                started = true;
            }
        }
    }

    // if we append as output, we need to inform the parent object that we are isTouched
    if (Output.getValue() == 1) {
        //make sure we inform our parent object that we changed, it then can inform others if needed
        App::DocumentObject* group = FemPostGroupExtension::getGroupOfObject(this);
        if (!group) {
            return;
        }
        if (group->hasExtension(FemPostGroupExtension::getExtensionClassTypeId())) {
            auto postgroup = group->getExtensionByType<FemPostGroupExtension>();
            postgroup->filterChanged(this);
        }
    }
}

void FemPostBranchFilter::filterPipelineChanged(FemPostFilter*) {
    // one of our filters has changed its active pipeline. We need to reconnect it properly.
    // As we are cheap we just reconnect everything
    // TODO: Do more efficiently
    onChanged(&Group);
}


PyObject* FemPostBranchFilter::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FemPostBranchFilterPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
