#include <iostream>
#include <vector>

#include "Geo.h"
#include "Constraints.h"
#include "InputParser.h"
#include "OutputWriter.h"

#include <fstream>

using namespace std;
using namespace Eigen;

Vector3d Place(GCS::Point p, GCS::Quaternion q, GCS::Displacement di) {

    double a=*q.a, b=*q.b, c=*q.c, d=*q.d;
    double norm = sqrt(pow(a,2)+pow(b,2)+pow(c,2)+pow(d,2));
    double x=a/norm, y=b/norm, z=c/norm, w=d/norm;

    Matrix3d rot;
    rot(0,0) = 1-2*(pow(y,2)+pow(z,2));
    rot(0,1) = -2.0*w*z + 2.0*x*y;
    rot(0,2) = 2.0*w*y + 2.0*x*z;
    rot(1,0) = 2.0*w*z + 2.0*x*y;
    rot(1,1) = 1-2*(pow(x,2)+pow(z,2));
    rot(1,2) = -2.0*w*x + 2.0*y*z;
    rot(2,0) = -2.0*w*y + 2.0*x*z;
    rot(2,1) = 2.0*w*x + 2.0*y*z;
    rot(2,2) = 1-2*(pow(x,2)+pow(y,2));

    Vector3d v(p.x, p.y, p.z);
    Vector3d n = rot*v;

    if (di.x != NULL) {
        n(0) += *di.x;
        n(1) += *di.y;
        n(2) += *di.z;
    }

    return n;
}

std::string Vec(Vector3d vec) {

    std::stringstream s;
    s<<"Base.Vector(" << vec(0) << ", " << vec(1) << ", " << vec(2) <<")";
    return s.str();
}


int main(int argc, char **argv) {

    if (argc<3) return 1;

    cout << endl << "loading file: " <<  argv[1];
    cout << endl << "generating output: " <<  argv[2] << endl << endl;

    vector<double *> variables;
    vector<double *> parameters;
    vector<GCS::Point> points;
    vector<GCS::Normal> norms;
    vector<GCS::Displacement> disps;
    vector<GCS::Quaternion> quats;
    vector<GCS::Solid> solids;
    vector<GCS::Constraint *> constraints;

    string inputfile(argv[1]);
    string outputfile(argv[2]);

    bool d2,d3;

    InputParser parser;
    d3 = parser.readInputFileAS(inputfile, variables, parameters, points, norms, disps, quats, solids, constraints);

    cout << "Variables: " << variables.size() << endl << "parameters: " << parameters.size() << endl;
    cout << "Points: " << points.size() << endl << "Normals: " << norms.size() << endl;
    cout << "Displacements: " << disps.size() << endl;
    cout << "Quaternions: " << quats.size() << endl;
    cout << "Solids: " << solids.size() << endl;
    cout << "Constraints: " << constraints.size() << endl;

    //init output writing
    remove(outputfile.c_str());
    ofstream file;
    file.open (outputfile.c_str());
    if (!file.is_open()) return 0;
    //import all needed modules
    file << "import Part" << endl << "from FreeCAD import Base" << endl << endl;
    //open new document
    file << "App.newDocument(\"solver\")" << endl << endl;
    GCS::Displacement emd;
    for (int i=0; i<solids.size(); i++) {
        file << "plane = Part.makePlane(2,2,"<<Vec(Place(solids[i].p, solids[i].q, solids[i].d))<<",";
        file << Vec(Place(solids[i].n, solids[i].q, emd)) << ")" << endl;
        file << "Part.show(plane)" << endl;
        file << "Gui.ActiveDocument.ActiveObject.ShapeColor = ("<< 0.9-0.05*(double)i <<",0.0,0.0)" << endl;
    }

    cout<<endl<<"solving...";

    GCS::System GCSsys(constraints);

    GCSsys.solve(variables);
    GCSsys.applySolution();


    for (int i=0; i<solids.size(); i++) {
           file << "plane = Part.makePlane(2,2,"<<Vec(Place(solids[i].p, solids[i].q, solids[i].d))<<",";
           file << Vec(Place(solids[i].n, solids[i].q, emd)) << ")" << endl;
           file << "Part.show(plane)" << endl;
           file << "Gui.ActiveDocument.ActiveObject.ShapeColor = (0.0,0.0,"<< 0.9-0.05*(double)i <<")" << endl;
       }

    // Clean up
    GCS::free(variables);
    GCS::free(parameters);
    GCS::free(constraints);

    return 0;
}


