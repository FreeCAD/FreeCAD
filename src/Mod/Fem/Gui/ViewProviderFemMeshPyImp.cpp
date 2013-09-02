
#include "PreCompiled.h"

#include "Mod/Fem/Gui/ViewProviderFemMesh.h"

// inclusion of the generated files (generated out of ViewProviderFemMeshPy.xml)
#include "ViewProviderFemMeshPy.h"
#include "ViewProviderFemMeshPy.cpp"

using namespace FemGui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderFemMeshPy::representation(void) const
{
    return std::string("<ViewProviderFemMesh object>");
}



PyObject* ViewProviderFemMeshPy::animate(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}





Py::Dict ViewProviderFemMeshPy::getNodeColor(void) const
{
    //return Py::List();
    throw Py::AttributeError("Not yet implemented");
}

void ViewProviderFemMeshPy::setNodeColor(Py::Dict arg)
{
    if(arg.size() == 0)
        this->getViewProviderFemMeshPtr()->resetColorByNodeId();
    else {
        std::map<long,App::Color> NodeColorMap;

        for( Py::Dict::iterator it = arg.begin(); it!= arg.end();++it){
            Py::Int id((*it).first);
            Py::Tuple color((*it).second);
            NodeColorMap[id] = App::Color(Py::Float(color[0]),Py::Float(color[1]),Py::Float(color[2]),0);
        }
        this->getViewProviderFemMeshPtr()->setColorByNodeId(NodeColorMap);
	}

    throw Py::AttributeError("Not yet implemented");
}

Py::List ViewProviderFemMeshPy::getHighlightedNodes(void) const
{
    //return Py::List();
    throw Py::AttributeError("Not yet implemented");
}

void  ViewProviderFemMeshPy::setHighlightedNodes(Py::List arg)
{
 /*   std::set<long>& nodeSet;
    for (Py::List::iterator it = arg.begin(); it != arg.end() && index < 16; ++it) {
        nodeSet.i (double)Py::Int(*it);
    }
    setHighlightNodes*/
    throw Py::AttributeError("Not yet implemented");

}



PyObject *ViewProviderFemMeshPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ViewProviderFemMeshPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


