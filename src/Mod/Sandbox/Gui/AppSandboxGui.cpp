/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer@users.sourceforge.net>        *
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
# include <Python.h>
# include <Standard_math.hxx>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoCoordinate3.h>
#endif
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/GeometryPyCXX.h>
#include <Base/Reader.h>
#include <Base/VectorPy.h>
#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObserver.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#ifdef HAVE_PART
#include <Mod/Part/App/PropertyGeometryList.h>
#endif

#include "Workbench.h"

// use a different name to CreateCommand()
void CreateSandboxCommands(void);


/* module functions */

class ObjectObserver : public App::DocumentObserver
{
public:
    ObjectObserver(App::DocumentObject* o, SoCoordinate3* c) : object(o), coords(c), radius(25.0)
    {
    }
private:
    void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop)
    {
#ifdef HAVE_PART
        if (object == &Obj && Prop.getTypeId() == Part::PropertyGeometryList::getClassTypeId()) {
            const Part::PropertyGeometryList& geom = static_cast<const Part::PropertyGeometryList&>(Prop);
            const std::vector<Part::Geometry*>& items = geom.getValues();
            if (items.size() != 2)
                return;
            Part::Geometry* g1 = items[0];
            Part::Geometry* g2 = items[1];
            if (!g1 || g1->getTypeId() != Part::GeomArcOfCircle::getClassTypeId())
                return;
            if (!g2 || g2->getTypeId() != Part::GeomLineSegment::getClassTypeId())
                return;
            Part::GeomArcOfCircle* arc = static_cast<Part::GeomArcOfCircle*>(g1);
            Part::GeomLineSegment* seg = static_cast<Part::GeomLineSegment*>(g2);

            try {
                Base::Vector3d m1 = arc->getCenter();
              //Base::Vector3d a3 = arc->getStartPoint();
                Base::Vector3d a3 = arc->getEndPoint(true);
              //Base::Vector3d l1 = seg->getStartPoint();
                Base::Vector3d l2 = seg->getEndPoint();
#if 0
                Py::Module pd("FilletArc");
                Py::Callable method(pd.getAttr(std::string("makeFilletArc")));
                Py::Callable vector(pd.getAttr(std::string("Vector")));

                Py::Tuple xyz(3);
                Py::Tuple args(6);
                xyz.setItem(0, Py::Float(m1.x));
                xyz.setItem(1, Py::Float(m1.y));
                xyz.setItem(2, Py::Float(m1.z));
                args.setItem(0,vector.apply(xyz));

                xyz.setItem(0, Py::Float(a3.x));
                xyz.setItem(1, Py::Float(a3.y));
                xyz.setItem(2, Py::Float(a3.z));
                args.setItem(1,vector.apply(xyz));

                xyz.setItem(0, Py::Float(l2.x));
                xyz.setItem(1, Py::Float(l2.y));
                xyz.setItem(2, Py::Float(l2.z));
                args.setItem(2,vector.apply(xyz));

                xyz.setItem(0, Py::Float((float)0));
                xyz.setItem(1, Py::Float((float)0));
                xyz.setItem(2, Py::Float((float)1));
                args.setItem(3,vector.apply(xyz));

                args.setItem(4,Py::Float(radius));
                args.setItem(5,Py::Int((int)0));
                Py::Tuple ret(method.apply(args));
                Py::Object S1(ret.getItem(0));
                Py::Object S2(ret.getItem(1));
                Py::Object M2(ret.getItem(2));

                Base::Vector3d s1, s2, m2;
                s1.x = (double)Py::Float(S1.getAttr("x"));
                s1.y = (double)Py::Float(S1.getAttr("y"));
                s1.z = (double)Py::Float(S1.getAttr("z"));

                s2.x = (double)Py::Float(S2.getAttr("x"));
                s2.y = (double)Py::Float(S2.getAttr("y"));
                s2.z = (double)Py::Float(S2.getAttr("z"));

                m2.x = (double)Py::Float(M2.getAttr("x"));
                m2.y = (double)Py::Float(M2.getAttr("y"));
                m2.z = (double)Py::Float(M2.getAttr("z"));

                coords->point.set1Value(0, (float)s1.x,(float)s1.y,(float)s1.z);
                coords->point.set1Value(1, (float)m2.x,(float)m2.y,(float)m2.z);
                coords->point.set1Value(2, (float)s2.x,(float)s2.y,(float)s2.z);

                Base::Console().Message("M1=<%.4f,%.4f>\n", m1.x,m1.y);
                Base::Console().Message("M2=<%.4f,%.4f>\n", m2.x,m2.y);
                Base::Console().Message("S1=<%.4f,%.4f>\n", s1.x,s1.y);
                Base::Console().Message("S2=<%.4f,%.4f>\n", s2.x,s2.y);
                Base::Console().Message("P=<%.4f,%.4f>\n", a3.x,a3.y);
                Base::Console().Message("Q=<%.4f,%.4f>\n", l2.x,l2.y);
                Base::Console().Message("\n");
#else
                Py::Module pd("PartDesign");
                Py::Callable method(pd.getAttr(std::string("makeFilletArc")));

                Py::Tuple args(6);
                args.setItem(0,Py::Vector(m1));
                args.setItem(1,Py::Vector(a3));
                args.setItem(2,Py::Vector(l2));
                args.setItem(3,Py::Vector(Base::Vector3d(0,0,1)));
                args.setItem(4,Py::Float(radius));
              //args.setItem(5,Py::Int((int)0));
                args.setItem(5,Py::Long((long)1));
                Py::Tuple ret(method.apply(args));
                Py::Vector S1(ret.getItem(0));
                Py::Vector S2(ret.getItem(1));
                Py::Vector M2(ret.getItem(2));

                Base::Vector3d s1 = S1.toVector();
                Base::Vector3d s2 = S2.toVector();
                Base::Vector3d m2 = M2.toVector();
                coords->point.set1Value(0, (float)s1.x,(float)s1.y,(float)s1.z);
                coords->point.set1Value(1, (float)m2.x,(float)m2.y,(float)m2.z);
                coords->point.set1Value(2, (float)s2.x,(float)s2.y,(float)s2.z);

                Base::Console().Message("M1=<%.4f,%.4f>\n", m1.x,m1.y);
                Base::Console().Message("M2=<%.4f,%.4f>\n", m2.x,m2.y);
                Base::Console().Message("S1=<%.4f,%.4f>\n", s1.x,s1.y);
                Base::Console().Message("S2=<%.4f,%.4f>\n", s2.x,s2.y);
                Base::Console().Message("P=<%.4f,%.4f>\n", a3.x,a3.y);
                Base::Console().Message("Q=<%.4f,%.4f>\n", l2.x,l2.y);
                Base::Console().Message("\n");
#endif
            }
            catch (Py::Exception&) {
                Base::PyException e; // extract the Python error text
                Base::Console().Error("%s\n", e.what());
            }
        }
#else
        (void)Obj;
        (void)Prop;
#endif
    }

    App::DocumentObject* object;
    SoCoordinate3* coords;
    double radius;
};

namespace SandboxGui {
class Module : public Py::ExtensionModule<Module>
{

public:
    Module() : Py::ExtensionModule<Module>("SandboxGui")
    {
        add_varargs_method("interactiveFilletArc",&Module::interactiveFilletArc,
            "Interactive fillet arc");
        add_varargs_method("xmlReader",&Module::xmlReader,
            "Read XML");
        initialize("This module is the SandboxGui module"); // register with Python
    }
    
    virtual ~Module() {}

private:
    Py::Object interactiveFilletArc(const Py::Tuple& /*args*/)
    {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(doc->getActiveView());
            if (view) {
                Gui::View3DInventorViewer* viewer = view->getViewer();
                SoSeparator* scene = static_cast<SoSeparator*>(viewer->getSceneGraph());
                SoSeparator* node = new SoSeparator();
                SoBaseColor* rgb = new SoBaseColor();
                rgb->rgb.setValue(1,1,0);
                node->addChild(rgb);
                SoCoordinate3* coords = new SoCoordinate3();
                node->addChild(coords);
                node->addChild(new SoLineSet());
                scene->addChild(node);

                ObjectObserver* obs = new ObjectObserver(doc->getDocument()->getActiveObject(), coords);
                obs->attachDocument(doc->getDocument());
            }
        }
        return Py::None();
    }
    Py::Object xmlReader(const Py::Tuple& args)
    {
        std::string file = static_cast<std::string>(Py::String(args[0]));
        App::Document* doc = App::GetApplication().newDocument();

        std::ifstream str(file.c_str(), std::ios::in);
        Base::XMLReader reader(file.c_str(), str);
        doc->Restore(reader);
        return Py::None();
    }
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace SandboxGui

/* Python entry */
PyMOD_INIT_FUNC(SandboxGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(0);
    }

    // Load Python modules this module depends on
    try {
        Base::Interpreter().loadModule("Sandbox");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(0);
    }

    // instantiating the commands
    CreateSandboxCommands();
    SandboxGui::Workbench::init();
    SandboxGui::SoWidgetShape::initClass();

    // the following constructor call registers our extension module
    // with the Python runtime system
    PyObject* mod = SandboxGui::initModule();
    Base::Console().Log("Loading GUI of Sandbox module... done\n");
    PyMOD_Return(mod);
}
