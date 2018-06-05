// dxf.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.
// modified 2018 wandererfan

#pragma once

#include <algorithm>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <math.h>

//Following is required to be defined on Ubuntu with OCC 6.3.1
#ifndef HAVE_IOSTREAM
#define HAVE_IOSTREAM
#endif

typedef int Aci_t; // AutoCAD color index

typedef enum
{
    eUnspecified = 0,   // Unspecified (No units)
    eInches,
    eFeet,
    eMiles,
    eMillimeters,
    eCentimeters,
    eMeters,
    eKilometers,
    eMicroinches,
    eMils,
    eYards,
    eAngstroms,
    eNanometers,
    eMicrons,
    eDecimeters,
    eDekameters,
    eHectometers,
    eGigameters,
    eAstronomicalUnits,
    eLightYears,
    eParsecs
} eDxfUnits_t;


//spline data for reading
struct SplineData
{
    double norm[3];
    int degree;
    int knots;
    int control_points;
    int fit_points;
    int flag;
    std::list<double> starttanx;
    std::list<double> starttany;
    std::list<double> starttanz;
    std::list<double> endtanx;
    std::list<double> endtany;
    std::list<double> endtanz;
    std::list<double> knot;
    std::list<double> weight;
    std::list<double> controlx;
    std::list<double> controly;
    std::list<double> controlz;
    std::list<double> fitx;
    std::list<double> fity;
    std::list<double> fitz;
};

//***************************
//data structures for writing
//added by Wandererfan 2018 (wandererfan@gmail.com) for FreeCAD project
struct point3D
{
    double x;
    double y;
    double z;
};

struct SplineDataOut
{
    point3D norm;
    int degree;
    int knots;
    int control_points;
    int fit_points;
    int flag;
    point3D starttan;
    point3D endtan;
    std::vector<double> knot;
    std::vector<double> weight;
    std::vector<point3D> control;
    std::vector<point3D> fit;
};

struct LWPolyDataOut
{
    double nVert;
    int    Flag;
    double Width;
    double Elev;
    double Thick;
    std::vector<point3D> Verts;
    std::vector<double> StartWidth;
    std::vector<double> EndWidth;
    std::vector<double> Bulge;
    point3D Extr;
};
//********************

class CDxfWrite{
private:
    std::ofstream* m_ofs;
    bool m_fail;
    std::stringstream m_ssBlock;

public:
    CDxfWrite(const char* filepath);
    ~CDxfWrite();

    bool Failed(){return m_fail;}

    void WriteLine(const double* s, const double* e, const char* layer_name );
    void WritePoint(const double*, const char*);
    void WriteArc(const double* s, const double* e, const double* c, bool dir, const char* layer_name );
    void WriteEllipse(const double* c, double major_radius, double minor_radius, 
                      double rotation, double start_angle, double end_angle, bool endIsCW, 
                      const char* layer_name);
    void WriteCircle(const double* c, double radius, const char* layer_name );
    void WriteSpline(SplineDataOut sd, const char* layer_name);
    void WriteLWPolyLine(LWPolyDataOut pd, const char* layer_name);
    void WritePolyline(LWPolyDataOut pd, const char* layer_name);
    void WriteVertex(double x, double y, double z, const char* layer_name);
    void WriteText(const char* text, const double* location1, const double* location2,
                   const double height, const int horizJust, const char* layer_name);
    void WriteLinearDim(const double* textMidPoint, const double* lineDefPoint,
                  const double* extLine1, const double* extLine2,
                  const char* dimText,
                  const char* layer_name);
    void writeLinearDimBlock(const double* textMidPoint, const double* lineDefPoint,
                  const double* extLine1, const double* extLine2,
                  const char* dimText);
    void WriteAngularDim(const double* textMidPoint, const double* lineDefPoint,
                  const double* startExt1, const double* endExt1,
                  const double* startExt2, const double* endExt2,
                  const char* dimText,
                  const char* layer_name);
    void writeAngularDimBlock(const double* textMidPoint, const double* lineDefPoint,
                         const double* startExt1, const double* endExt1,
                         const double* startExt2, const double* endExt2,
                         const char* dimText);
   void WriteRadialDim(const double* centerPoint, const double* textMidPoint, 
                         const double* arcPoint,
                         const char* dimText,
                         const char* layer_name);
    void writeRadialDimBlock(const double* centerPoint, const double* textMidPoint, 
                         const double* arcPoint, const char* dimText);
    void WriteDiametricDim(const double* textMidPoint, 
                         const double* arcPoint1, const double* arcPoint2,
                         const char* dimText,
                         const char* layer_name);
    void writeDiametricDimBlock(const double* textMidPoint, 
                         const double* arcPoint1, const double* arcPoint2,
                         const char* dimText);

    void writeDimBlockPreamble(const char* layer_name);
    void writeHeaderSection(void);
    void writeTablesSection(void);
    void writeBlocksSection(void);
};

// derive a class from this and implement it's virtual functions
class ImportExport CDxfRead{
private:
    std::ifstream* m_ifs;

    bool m_fail;
    char m_str[1024];
    char m_unused_line[1024];
    eDxfUnits_t m_eUnits;
    bool m_measurement_inch;
    char m_layer_name[1024];
    char m_section_name[1024];
    char m_block_name[1024];
    bool m_ignore_errors;


    typedef std::map< std::string,Aci_t > LayerAciMap_t;
    LayerAciMap_t m_layer_aci;  // layer names -> layer color aci map

    bool ReadUnits();
    bool ReadLayer();
    bool ReadLine();
    bool ReadText();
    bool ReadArc();
    bool ReadCircle();
    bool ReadEllipse();
    bool ReadPoint();
    bool ReadSpline();
    bool ReadLwPolyLine();
    bool ReadPolyLine();
    bool ReadVertex(double *pVertex, bool *bulge_found, double *bulge);
    void OnReadArc(double start_angle, double end_angle, double radius, const double* c, double z_extrusion_dir, bool hidden);
    void OnReadCircle(const double* c, double radius, bool hidden);
    void OnReadEllipse(const double* c, const double* m, double ratio, double start_angle, double end_angle);
    bool ReadInsert();
    bool ReadDimension();
    bool ReadBlockInfo();

    void get_line();
    void put_line(const char *value);
    void DerefACI();

protected:
    Aci_t m_aci; // manifest color name or 256 for layer color

public:
    CDxfRead(const char* filepath); // this opens the file
    ~CDxfRead(); // this closes the file

    bool Failed(){return m_fail;}
    void DoRead(const bool ignore_errors = false); // this reads the file and calls the following functions

    double mm( double value ) const;

    bool IgnoreErrors() const { return(m_ignore_errors); }

    virtual void OnReadLine(const double* /*s*/, const double* /*e*/, bool /*hidden*/){}
    virtual void OnReadPoint(const double* /*s*/){}
    virtual void OnReadText(const double* /*point*/, const double /*height*/, const char* /*text*/){}
    virtual void OnReadArc(const double* /*s*/, const double* /*e*/, const double* /*c*/, bool /*dir*/, bool /*hidden*/){}
    virtual void OnReadCircle(const double* /*s*/, const double* /*c*/, bool /*dir*/, bool /*hidden*/){}
    virtual void OnReadEllipse(const double* /*c*/, double /*major_radius*/, double /*minor_radius*/, double /*rotation*/, double /*start_angle*/, double /*end_angle*/, bool /*dir*/){}
    virtual void OnReadSpline(struct SplineData& /*sd*/){}
    virtual void OnReadInsert(const double* /*point*/, const double* /*scale*/, const char* /*name*/, double /*rotation*/){}
    virtual void OnReadDimension(const double* /*s*/, const double* /*e*/, const double* /*point*/, double /*rotation*/){}
    virtual void AddGraphics() const { }

    std::string LayerName() const;

};
