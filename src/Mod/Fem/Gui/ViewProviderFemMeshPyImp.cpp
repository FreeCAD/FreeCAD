
#include "PreCompiled.h"

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include <App/DocumentObjectPy.h>

#include "Mod/Fem/Gui/ViewProviderFemMesh.h"
#include "Mod/Fem/App/FemResultVector.h"
#include "Mod/Fem/App/FemResultValue.h"

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

App::Color calcColor(double value,double min, double max)
{
    if (max < 0) max = 0;
    if (min > 0) min = 0;

    if (value < min) 
        return App::Color (0.0,0.0,1.0);    
    if (value > max)
        return App::Color (1.0,0.0,0.0);
    if (value == 0.0)
        return App::Color (0.0,1.0,0.0);
    if ( value > max/2.0 )
        return App::Color (1.0,1-((value-(max/2.0)) / (max/2.0)),0.0);
    if ( value > 0.0 )
        return App::Color (value/(max/2.0),1.0,0.0) ;
    if ( value < min/2.0 )
        return App::Color (0.0,1-((value-(min/2.0)) / (min/2.0)),1.0);
    if ( value < 0.0 )
        return App::Color (0.0,1.0,value/(min/2.0)) ;
    return App::Color (0,0,0);
}


PyObject* ViewProviderFemMeshPy::setNodeColorByResult(PyObject *args)
{
	// statistical values get collected and returned
	double max = -1e12;
    double min = +1e12;
	double avg = 0;

    PyObject *object=0;
    int type = 0;
    if (PyArg_ParseTuple(args,"O!|i",&(App::DocumentObjectPy::Type), &object, &type)) {
        App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
        if (obj && obj->getTypeId().isDerivedFrom(Fem::FemResultValue::getClassTypeId())){
            Fem::FemResultValue *result = static_cast<Fem::FemResultValue*>(obj);
            const std::vector<long> & Ids = result->ElementNumbers.getValues() ;
            const std::vector<double> & Vals = result->Values.getValues() ;
            std::vector<App::Color> NodeColors(Vals.size());
			for(std::vector<double>::const_iterator it= Vals.begin();it!=Vals.end();++it){
                if(*it > max)
                    max = *it;
                if(*it < min)
                    min = *it;
				avg += *it;
			}
			avg /= Vals.size();

            // fill up color vector
            long i=0;
            for(std::vector<double>::const_iterator it= Vals.begin();it!=Vals.end();++it,i++)
                NodeColors[i] = calcColor(*it,0.0,max);    
          
            // set the color to the view-provider 
            this->getViewProviderFemMeshPtr()->setColorByNodeId(Ids,NodeColors);


        }else if (obj && obj->getTypeId().isDerivedFrom(Fem::FemResultVector::getClassTypeId())){
            Fem::FemResultVector *result = static_cast<Fem::FemResultVector*>(obj);
            const std::vector<long> & Ids = result->ElementNumbers.getValues() ;
            const std::vector<Base::Vector3d> & Vecs = result->Values.getValues() ;
            std::vector<App::Color> NodeColors(Vecs.size());

			for(std::vector<Base::Vector3d>::const_iterator it= Vecs.begin();it!=Vecs.end();++it){
                double val;
                if(type == 0)
                    val = it->Length();
                else if (type == 1)
                    val = it->x;
                else if (type == 2)
                    val = it->y;
                else if (type == 3)
                    val = it->z;
                else 
                    val = it->Length();

                if(val > max)
                    max = val;
                if(val < min)
                    min = val;
				avg += val;
            }
			avg /= Vecs.size();

            // fill up color vector
            long i=0;
            for(std::vector<Base::Vector3d>::const_iterator it= Vecs.begin();it!=Vecs.end();++it,i++)
                if(type == 0)
                    NodeColors[i] = calcColor(it->Length(),0.0,max);
                else if (type == 1)
                    NodeColors[i] = calcColor(it->x,min,max);
                else if (type == 2)
                    NodeColors[i] = calcColor(it->y,min,max);
                else if (type == 3)
                    NodeColors[i] = calcColor(it->z,min,max);
                else
                    NodeColors[i] = calcColor(it->Length(),0.0,max);

            // set the color to the view-provider 
            this->getViewProviderFemMeshPtr()->setColorByNodeId(Ids,NodeColors);


        }else{
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Argument has to be a ResultValue or ResultVector!");
            return 0;
        }
    }

	Py::Tuple res(3);
	res[0] = Py::Float(min);
	res[1] = Py::Float(max);
	res[2] = Py::Float(avg);

	return Py::new_reference_to(res);

}

PyObject* ViewProviderFemMeshPy::setNodeDisplacementByResult(PyObject *args)
{
    PyObject *object=0;
    if (PyArg_ParseTuple(args,"O!",&(App::DocumentObjectPy::Type), &object)) {
        App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
        if (obj && obj->getTypeId().isDerivedFrom(Fem::FemResultVector::getClassTypeId())){
            Fem::FemResultVector *result = static_cast<Fem::FemResultVector*>(obj);
            const std::vector<long> & Ids = result->ElementNumbers.getValues() ;
            const std::vector<Base::Vector3d> & Vecs = result->Values.getValues() ;
            // set the displacement to the view-provider 
            this->getViewProviderFemMeshPtr()->setDisplacementByNodeId(Ids,Vecs);


        }else{
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Argument has to be a ResultVector!");
            return 0;
        }
    }

    Py_Return;

}
Py::Dict ViewProviderFemMeshPy::getNodeColor(void) const
{
    //return Py::List();
    throw Py::AttributeError("Not yet implemented");
}

void ViewProviderFemMeshPy::setNodeColor(Py::Dict arg)
{
    long size = arg.size();
    if(size == 0)
        this->getViewProviderFemMeshPtr()->resetColorByNodeId();
    else {
        Base::TimeInfo Start;
        Base::Console().Log("Start: ViewProviderFemMeshPy::setNodeColor() =================================\n");
        //std::map<long,App::Color> NodeColorMap;

        //for( Py::Dict::iterator it = arg.begin(); it!= arg.end();++it){
        //    Py::Int id((*it).first);
        //    Py::Tuple color((*it).second);
        //    NodeColorMap[id] = App::Color(Py::Float(color[0]),Py::Float(color[1]),Py::Float(color[2]),0);
        //}
        std::vector<long> NodeIds(size);
        std::vector<App::Color> NodeColors(size);

        long i = 0;
        for( Py::Dict::iterator it = arg.begin(); it!= arg.end();++it,i++){
            Py::Int id((*it).first);
            Py::Tuple color((*it).second);
            NodeIds[i]    = id;
            NodeColors[i] = App::Color(Py::Float(color[0]),Py::Float(color[1]),Py::Float(color[2]),0);
        }
        Base::Console().Log("    %f: Start ViewProviderFemMeshPy::setNodeColor() call \n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));

        //this->getViewProviderFemMeshPtr()->setColorByNodeId(NodeColorMap);
        this->getViewProviderFemMeshPtr()->setColorByNodeId(NodeIds,NodeColors);
        Base::Console().Log("    %f: Finish ViewProviderFemMeshPy::setNodeColor() call \n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));
	}
}

Py::Dict ViewProviderFemMeshPy::getElementColor(void) const
{
    //return Py::List();
    throw Py::AttributeError("Not yet implemented");
}

void ViewProviderFemMeshPy::setElementColor(Py::Dict arg)
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
        this->getViewProviderFemMeshPtr()->setColorByElementId(NodeColorMap);
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
    std::set<long> res;

    for( Py::List::iterator it = arg.begin(); it!= arg.end();++it){
        Py::Int id(*it);
        if(id)
            res.insert(id);
    }
    this->getViewProviderFemMeshPtr()->setHighlightNodes(res);
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


