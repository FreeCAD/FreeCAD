
#include "PreCompiled.h"

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

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



PyObject* ViewProviderFemMeshPy::animate(PyObject * args)
{
    double factor;
    if (!PyArg_ParseTuple(args, "d", &factor))
        return 0;

    this->getViewProviderFemMeshPtr()->animateNodes(factor);

    Py_Return;
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
}

Py::Dict ViewProviderFemMeshPy::getNodeDisplacement(void) const
{
    //return Py::Dict();
    throw Py::AttributeError("Not yet implemented");
}

void  ViewProviderFemMeshPy::setNodeDisplacement(Py::Dict arg)
{
    if(arg.size() == 0)
        this->getViewProviderFemMeshPtr()->resetColorByNodeId();
    else {
        std::map<long,Base::Vector3d> NodeDispMap;
        union PyType_Object pyType = {&(Base::VectorPy::Type)};
        Py::Type vType(pyType.o);

        for( Py::Dict::iterator it = arg.begin(); it!= arg.end();++it){
            Py::Int id((*it).first);
            if ((*it).second.isType(vType)) {
                Py::Vector p((*it).second);
                NodeDispMap[id] = p.toVector();
            }
        }
        this->getViewProviderFemMeshPtr()->setDisplacementByNodeId(NodeDispMap);
	}
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

Py::List ViewProviderFemMeshPy::getVisibleElementFaces(void) const
{
    const std::vector<unsigned long> & visElmFc = this->getViewProviderFemMeshPtr()->getVisibleElementFaces();
    std::vector<unsigned long> trans;

    // sorting out double faces through higer order elements and null entries
    long elementOld =0, faceOld=0;
    for (std::vector<unsigned long>::const_iterator it = visElmFc.begin();it!=visElmFc.end();++it){
        if(*it == 0)
            continue;

        long element = *it>>3;
        long face    = (*it&7)+1;
        if(element == elementOld && face==faceOld)
            continue;

        trans.push_back(*it);
        elementOld = element;
        faceOld    = face;
    }

    Py::List result( trans.size() );
    int i = 0;
    for (std::vector<unsigned long>::const_iterator it = trans.begin();it!=trans.end();++it,i++){
        Py::Tuple tup(2);
        long element = *it>>3;
        long face    = (*it&7)+1;
        tup.setItem( 0,Py::Int( element ) );
        tup.setItem( 1,Py::Int( face ) );
        result.setItem(i,tup);
    }

    return result;
}


PyObject *ViewProviderFemMeshPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ViewProviderFemMeshPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


