
#include "PreCompiled.h"

#ifndef _PreComp_
#include <SMESHDS_Mesh.hxx>
#include <SMESH_Mesh.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>
#include <Mod/Fem/App/FemMeshObject.h>

#include "ViewProviderFemMesh.h"
// clang-format off
// inclusion of the generated files (generated out of ViewProviderFemMeshPy.xml)
#include "ViewProviderFemMeshPy.h"
#include "ViewProviderFemMeshPy.cpp"
// clang-format off


using namespace FemGui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderFemMeshPy::representation() const
{
    return {"<ViewProviderFemMesh object>"};
}


PyObject* ViewProviderFemMeshPy::applyDisplacement(PyObject* args)
{
    double factor;
    if (!PyArg_ParseTuple(args, "d", &factor)) {
        return nullptr;
    }

    this->getViewProviderFemMeshPtr()->applyDisplacementToNodes(factor);

    Py_Return;
}


App::Color calcColor(double value, double min, double max)
{
    if (max < 0) {
        max = 0;
    }
    if (min > 0) {
        min = 0;
    }

    if (value < min) {
        return App::Color(0.0, 0.0, 1.0);
    }
    if (value > max) {
        return App::Color(1.0, 0.0, 0.0);
    }
    if (value == 0.0) {
        return App::Color(0.0, 1.0, 0.0);
    }
    if (value > max / 2.0) {
        return App::Color(1.0, 1 - ((value - (max / 2.0)) / (max / 2.0)), 0.0);
    }
    if (value > 0.0) {
        return App::Color(value / (max / 2.0), 1.0, 0.0);
    }
    if (value < min / 2.0) {
        return App::Color(0.0, 1 - ((value - (min / 2.0)) / (min / 2.0)), 1.0);
    }
    if (value < 0.0) {
        return App::Color(0.0, 1.0, value / (min / 2.0));
    }
    return App::Color(0, 0, 0);
}


PyObject* ViewProviderFemMeshPy::setNodeColorByScalars(PyObject* args)
{
    double max = -1e12;
    double min = +1e12;
    PyObject* node_ids_py;
    PyObject* values_py;

    if (PyArg_ParseTuple(args, "O!O!", &PyList_Type, &node_ids_py, &PyList_Type, &values_py)) {
        std::vector<long> ids;
        std::vector<double> values;
        int num_items = PyList_Size(node_ids_py);
        if (num_items < 0) {
            PyErr_SetString(PyExc_ValueError, "PyList_Size < 0. That is not a valid list!");
            Py_Return;
        }
        std::vector<App::Color> node_colors(num_items);
        for (int i = 0; i < num_items; i++) {
            PyObject* id_py = PyList_GetItem(node_ids_py, i);
            long id = PyLong_AsLong(id_py);
            ids.push_back(id);
            PyObject* value_py = PyList_GetItem(values_py, i);
            double val = PyFloat_AsDouble(value_py);
            values.push_back(val);
            if (val > max) {
                max = val;
            }
            if (val < min) {
                min = val;
            }
        }
        long i = 0;
        for (std::vector<double>::const_iterator it = values.begin(); it != values.end();
             ++it, i++) {
            node_colors[i] = calcColor(*it, min, max);
        }
        this->getViewProviderFemMeshPtr()->setColorByNodeId(ids, node_colors);
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "PyArg_ParseTuple failed. Invalid arguments used with setNodeByScalars");
        return nullptr;
    }
    Py_Return;
}


PyObject* ViewProviderFemMeshPy::resetNodeColor(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    this->getViewProviderFemMeshPtr()->resetColorByNodeId();
    Py_Return;
}


PyObject* ViewProviderFemMeshPy::setNodeDisplacementByVectors(PyObject* args)
{
    PyObject* node_ids_py;
    PyObject* vectors_py;
    if (PyArg_ParseTuple(args, "O!O!", &PyList_Type, &node_ids_py, &PyList_Type, &vectors_py)) {
        std::vector<long> ids;
        std::vector<Base::Vector3d> vectors;
        int num_items = PyList_Size(node_ids_py);
        if (num_items < 0) {
            PyErr_SetString(PyExc_ValueError, "PyList_Size < 0. That is not a valid list!");
            Py_Return;
        }
        for (int i = 0; i < num_items; i++) {
            PyObject* id_py = PyList_GetItem(node_ids_py, i);
            long id = PyLong_AsLong(id_py);
            ids.push_back(id);
            PyObject* vector_py = PyList_GetItem(vectors_py, i);
            Base::Vector3d vec = Base::getVectorFromTuple<double>(vector_py);
            vectors.push_back(vec);
        }
        this->getViewProviderFemMeshPtr()->setDisplacementByNodeId(ids, vectors);
    }
    else {
        PyErr_SetString(
            PyExc_TypeError,
            "PyArg_ParseTuple failed. Invalid arguments used with setNodeDisplacementByVectors");
        return nullptr;
    }
    Py_Return;
}


PyObject* ViewProviderFemMeshPy::resetNodeDisplacement(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    this->getViewProviderFemMeshPtr()->resetDisplacementByNodeId();
    Py_Return;
}


Py::Dict ViewProviderFemMeshPy::getNodeColor() const
{
    // return Py::List();
    throw Py::AttributeError("Not yet implemented");
}


void ViewProviderFemMeshPy::setNodeColor(Py::Dict arg)
{
    long size = arg.size();
    if (size == 0) {
        this->getViewProviderFemMeshPtr()->resetColorByNodeId();
    }
    else {
        Base::TimeInfo Start;
        Base::Console().Log(
            "Start: ViewProviderFemMeshPy::setNodeColor() =================================\n");
        // std::map<long,App::Color> NodeColorMap;

        // for( Py::Dict::iterator it = arg.begin(); it!= arg.end();++it){
        //     Py::Long id((*it).first);
        //     Py::Tuple color((*it).second);
        //     NodeColorMap[id] =
        //     App::Color(Py::Float(color[0]),Py::Float(color[1]),Py::Float(color[2]),0);
        // }
        std::vector<long> NodeIds(size);
        std::vector<App::Color> NodeColors(size);

        long i = 0;
        for (Py::Dict::iterator it = arg.begin(); it != arg.end(); ++it, i++) {
            Py::Long id((*it).first);
            Py::Tuple color((*it).second);
            NodeIds[i] = id;
            NodeColors[i] =
                App::Color(Py::Float(color[0]), Py::Float(color[1]), Py::Float(color[2]), 0);
        }
        Base::Console().Log("    %f: Start ViewProviderFemMeshPy::setNodeColor() call \n",
                            Base::TimeInfo::diffTimeF(Start, Base::TimeInfo()));

        // this->getViewProviderFemMeshPtr()->setColorByNodeId(NodeColorMap);
        this->getViewProviderFemMeshPtr()->setColorByNodeId(NodeIds, NodeColors);
        Base::Console().Log("    %f: Finish ViewProviderFemMeshPy::setNodeColor() call \n",
                            Base::TimeInfo::diffTimeF(Start, Base::TimeInfo()));
    }
}


Py::Dict ViewProviderFemMeshPy::getElementColor() const
{
    // return Py::List();
    throw Py::AttributeError("Not yet implemented");
}


void ViewProviderFemMeshPy::setElementColor(Py::Dict arg)
{
    if (arg.size() == 0) {
        this->getViewProviderFemMeshPtr()->resetColorByNodeId();
    }
    else {
        std::map<long, App::Color> NodeColorMap;

        for (Py::Dict::iterator it = arg.begin(); it != arg.end(); ++it) {
            Py::Long id((*it).first);
            Py::Tuple color((*it).second);
            NodeColorMap[id] =
                App::Color(Py::Float(color[0]), Py::Float(color[1]), Py::Float(color[2]), 0);
        }
        this->getViewProviderFemMeshPtr()->setColorByElementId(NodeColorMap);
    }
}


Py::Dict ViewProviderFemMeshPy::getNodeDisplacement() const
{
    // return Py::Dict();
    throw Py::AttributeError("Not yet implemented");
}


void ViewProviderFemMeshPy::setNodeDisplacement(Py::Dict arg)
{
    if (arg.size() == 0) {
        this->getViewProviderFemMeshPtr()->resetColorByNodeId();
    }
    else {
        std::map<long, Base::Vector3d> NodeDispMap;
        Py::Type vType(Base::getTypeAsObject(&Base::VectorPy::Type));

        for (Py::Dict::iterator it = arg.begin(); it != arg.end(); ++it) {
            Py::Long id((*it).first);
            if ((*it).second.isType(vType)) {
                Py::Vector p((*it).second);
                NodeDispMap[id] = p.toVector();
            }
        }
        this->getViewProviderFemMeshPtr()->setDisplacementByNodeId(NodeDispMap);
    }
}


Py::List ViewProviderFemMeshPy::getHighlightedNodes() const
{
    Py::List list;
    ViewProviderFemMesh* vp = this->getViewProviderFemMeshPtr();
    std::set<long> nodeIds = vp->getHighlightNodes();
    for (auto it : nodeIds) {
        list.append(Py::Long(it));
    }
    return list;
}


void ViewProviderFemMeshPy::setHighlightedNodes(Py::List arg)
{
    ViewProviderFemMesh* vp = this->getViewProviderFemMeshPtr();
    const SMESHDS_Mesh* data = static_cast<Fem::FemMeshObject*>(vp->getObject())
                                   ->FemMesh.getValue()
                                   .getSMesh()
                                   ->GetMeshDS();

    std::set<long> res;
    for (Py::List::iterator it = arg.begin(); it != arg.end(); ++it) {
        long id = static_cast<long>(Py::Long(*it));
        const SMDS_MeshNode* node = data->FindNode(id);
        if (node) {
            res.insert(id);
        }
    }

    this->getViewProviderFemMeshPtr()->setHighlightNodes(res);
}


PyObject* ViewProviderFemMeshPy::resetHighlightedNodes(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    this->getViewProviderFemMeshPtr()->resetHighlightNodes();
    Py_Return;
}


Py::List ViewProviderFemMeshPy::getVisibleElementFaces() const
{
    const std::vector<unsigned long>& visElmFc =
        this->getViewProviderFemMeshPtr()->getVisibleElementFaces();
    std::vector<unsigned long> trans;

    // sorting out double faces through higher order elements and null entries
    long elementOld = 0, faceOld = 0;
    for (unsigned long it : visElmFc) {
        if (it == 0) {
            continue;
        }

        long element = it >> 3;
        long face = (it & 7) + 1;
        if (element == elementOld && face == faceOld) {
            continue;
        }

        trans.push_back(it);
        elementOld = element;
        faceOld = face;
    }

    Py::List result(trans.size());
    int i = 0;
    for (std::vector<unsigned long>::const_iterator it = trans.begin(); it != trans.end();
         ++it, i++) {
        Py::Tuple tup(2);
        long element = *it >> 3;
        long face = (*it & 7) + 1;
        tup.setItem(0, Py::Long(element));
        tup.setItem(1, Py::Long(face));
        result.setItem(i, tup);
    }

    return result;
}


PyObject* ViewProviderFemMeshPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}


int ViewProviderFemMeshPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
