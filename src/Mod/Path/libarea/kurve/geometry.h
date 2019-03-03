
/////////////////////////////////////////////////////////////////////////////////////////

//                    geometry.lib header

//                    g.j.hawkesford August 2003
//						modified with 2d & 3d vector methods 2006
//
// This program is released under the BSD license. See the file COPYING for details.
//
/////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#ifdef WIN32
#pragma warning( disable : 4996 )
#ifndef WINVER
	#define WINVER 0x501
#endif
#endif

#include <math.h>
#include <algorithm>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <string.h>

using namespace std;


namespace geoff_geometry {

	// offset methods
	enum OFFSET_METHODS {
		NO_ELIMINATION = 0,
		BASIC_OFFSET,
		ROLLINGBALL_OFFSET // unfinished
	};

	enum SPAN_IDS {
		UNMARKED = 0xe0000000,
		ROLL_AROUND,
		INTERSECTION,
		FULL_CIRCLE_KURVE
		};


	class Vector2d;
	class Vector3d;
	class Point;
	class Point3d;
	class CLine;
	class Circle;
	class Span;
	class Kurve;
	class Line;


	enum UNITS_TYPE{
		MM = 0,
		METRES,
		INCHES
	};

	extern int	  UNITS;					// may be enum UNITS_TYPE (MM METRES or INCHES)
	extern double TOLERANCE;				// CAD Geometry resolution (inexact, eg. from import)
	extern double TOLERANCE_SQ;				// tolerance squared for faster coding.
	extern double TIGHT_TOLERANCE;
	extern double UNIT_VECTOR_TOLERANCE;
	extern double SMALL_ANGLE;				// small angle tangency test eg isConvex
	extern double SIN_SMALL_ANGLE;
	extern double COS_SMALL_ANGLE;
	extern double RESOLUTION;				// CNC resolution

	void set_Tolerances(int mode);
	double mm(double value);				// convert to current units from mm

inline bool FEQ(double a, double b, double tolerance = TOLERANCE) {return fabs(a - b) <= tolerance;}
inline bool FNE(double a, double b, double tolerance = TOLERANCE) {return fabs(a - b) > tolerance;}

inline bool FEQZ(double a, double tolerance = TIGHT_TOLERANCE) {return fabs(a) <= tolerance;}
inline bool FNEZ(double a, double tolerance = TIGHT_TOLERANCE) {return fabs(a) > tolerance;}

#define PI 3.1415926535897932384626433832795e0
#define DegreesToRadians (PI / 180.0e0)
#define RadiansToDegrees (180.0e0 / PI)
#define NEARLY_ONE 0.99999999999e0
#define CPTANGENTTOL 1.0e-04			// normalised vector crossproduct tolerance sin A  so A = .0057deg

#define TANTO		-1
#define ANTITANTO	1

#define TANGENT		0

#define NEARINT		1
#define FARINT		-1

#define LEFTINT		1
#define RIGHTINT	-1

#define CFILLET			0	// corner fillet
#define CHAMFER			1	// chamfer

#define GEOFF_LEFT 1
#define NONE 0
#define GEOFF_RIGHT -1


#define LINEAR 0	// linear
#define ACW 1		// anti-clockwise
#define CW -1		// clockwise

	const wchar_t* getMessage(const wchar_t* original);							// dummy
	void FAILURE(const wchar_t* str);
	void FAILURE(const std::wstring& str);

	enum MESSAGE_GROUPS {
		GENERAL_MESSAGES,
		GEOMETRY_ERROR_MESSAGES,
		PARAMSPMP
	};

	enum GENERAL_MESSAGES {
		MES_TITLE = 0,
		MES_UNFINISHEDCODING,
		MES_ERRORFILENAME,
		MES_LOGFILE,
		MES_LOGFILE1,
		MES_P4CMENU,
		MES_P4CMENUHINT
	};

	enum GEOMETRY_ERROR_MESSAGES{	// For geometry.lib
		MES_DIFFSCALE = 1000,
		MES_POINTONCENTRE,
		MES_INVALIDARC,
		MES_LOFTUNEQUALSPANCOUNT,
		MES_EQUALSPANCOUNTFAILED,
		MES_CANNOTTRIMSPAN,
		MES_INDEXOUTOFRANGE,
		MES_BAD_VERTEX_NUMBER,
		MES_BAD_REF_OFFSET,
		MES_BAD_SEC_OFFSET,
		MES_ROLLINGBALL4AXIS_ERROR,
		MES_INPUT_EQUALSPANCOUNT,
		MES_INVALIDPLANE
	};

	// homogenous 4 x 4 Matrix class
	class Matrix{
	protected:
	public:
		double e[16];
		bool m_unit;												// true if unit matrix
		int m_mirrored;												// 1 if mirrored, 0 if not and -1 if unknown

	public:
		// constructors etc...
		Matrix();													// create a unit matrix
		Matrix(double m[16]);										// from an array
		Matrix(const Matrix& m);									// copy constructor

		~Matrix(){};

		//operators
		bool operator==(const Matrix &m)const;
		bool operator!=(const Matrix &m)const { return !(*this == m);}

		// methods
		void	Unit();												// unit matrix
		void	Get(double* p) const;								// get the matrix into p
		void	Put(double*p);										// put p[16] into matrix
		void	Translate(double x, double y, double z=0);			// Translation

		void	Rotate(double sinang, double cosang, Vector3d *rotAxis); // Rotation about rotAxis
		void	Rotate(double angle, Vector3d *rotAxis);			// Rotation about rotAxis

		void	Rotate(double sinang, double cosang, int Axis);			// Rotation with cp & dp
		void	Rotate(double angle, int Axis);						// Rotation with angle

		void	Scale(double scale);								// Scale
		void	Scale(double scalex, double scaley, double scalez);

		void	Multiply(Matrix& m);								// Multiply 2 Matrices
		//	void	Transform(Point& p);
		void	Transform(double p0[3]) const;							// Transform p0 thro' this matrix
		void	Transform(double p0[3], double p1[3]) const;				// Transform p0 to p1 thro' this matrix
		void	Transform2d(double p0[2], double p1[2]) const;			// Transform p0 to p1 thro' this matrix

		int		IsMirrored();										// true if matrix has a mirror transformation
		int		IsUnit();											// true if matrix is unit matrix
		void	GetTranslate(double& x, double& y, double& z) const;		// get translation from matrix
		void	GetScale(double& sx, double& sy, double& sz) const;		// get scale from matrix
		bool	GetScale(double& sx) const;								// get scale from matrix (true if uniform scale)
		void	GetRotation(double& ax, double& ay, double& az) const;	// get rotation from matrix

		Matrix	Inverse();											// inverts this matrix
	};

	extern Matrix UnitMatrix;		// a Unit Matrix


	// 2d Point class
	class Point {
		friend wostream& operator << (wostream& op, Point& p);

	public:
		bool	ok;																// true if this point is defined correctly	
		double	x;																// x value
		double	y;																// y value

		// constructors etc...
		inline	Point(){x=0; y=0; ok=false;}																// Point p1
		inline	Point( double xord, double yord, bool okay = true) {					// Point p1(10,30);
			x = xord; y = yord; ok = okay;}
		inline	Point( const Point& p ) {												// copy constructor  Point p1(p2);
			x = p.x; y = p.y; ok = p.ok;}
				Point( const Point3d& p );												// copy constructor  Point p1(p2);
		Point(const Vector2d& v);

		// operators
		bool operator==(const Point &p)const;
		bool operator!=(const Point &p)const { return !(*this == p);}
		inline	Point	operator+(const Point &p)const{return Point(x + p.x, y + p.y);}		// p0 = p1 + p2;
		inline	Point	operator+=(const Point &p){return Point(x += p.x, y += p.y);}		// p0 += p1;
		Point operator+(const Vector2d &v)const;			// p1 = p0 + v0;

		// destructor
		//~Point(){};

		// methods
		Point	Transform(const Matrix& m);											    // transform point
		double	Dist(const Point& p)const;														// distance between 2 points
		double	DistSq(const Point& p)const;													// distance squared
		double	Dist(const CLine& cl)const;													// distance p to cl
		Point	Mid(const Point& p, double factor=.5)const;									// mid point
		void	get(double xyz[2]) {xyz[0] = x; xyz[1] = y;}						// return to array
	};


#define INVALID_POINT	Point(9.9999999e50, 0, false)
#define INVALID_POINT3D	Point3d(9.9999999e50, 0, 0, false)
#define INVALID_CLINE	CLine(INVALID_POINT, 1, 0, false)
#define INVALID_CIRCLE	Circle(INVALID_POINT, 0)

	// 3d point class
	class Point3d {
		friend wostream& operator <<(wostream& op, Point3d& p);
	public:
//		bool	ok;																// true if this point is defined correctly	
		double	x;																// x value
		double	y;																// y value
		double	z;																// z value

		// constructors
		inline	Point3d(){x = 0; y = 0; z = 0;}// {z=0; /*ok=false;*/};												// Point p1
		inline	Point3d(const double* xyz) {x = xyz[0], y = xyz[1]; z = xyz[2];}
		inline	Point3d( double xord, double yord, double zord = 0/*, bool okay = true*/) {	// Point p1(10,30.5);
			x = xord; y = yord; z = zord;/* ok = okay;*/}
		inline	Point3d( const Point3d& p ) {											// copy constructor  Point p1(p2);
			x = p.x; y = p.y;  z = p.z;/* ok = p.ok;*/}
		inline	Point3d( const Point& p ) {												// copy constructor  Point p1(p2);
			x = p.x; y = p.y;  z = 0; /*ok = p.ok;*/}
		inline	Point3d( const Point& p, double zord ) {								// copy constructor  Point p1(p2, z);
			x = p.x; y = p.y;  z = zord;/* ok = p.ok;*/}
		Point3d(const Vector3d& v);

		// destructor
//		~Point3d();

		// operators
		bool operator==(const Point3d &p)const;
		bool operator!=(const Point3d &p)const { return !(*this == p);}
		Point3d operator+(const Vector3d &v)const;			// p1 = p0 + v0;


		// methods
#ifdef PEPSDLL
		void	ToPeps(int id, bool draw = true);									// copy Point to Peps
#endif
		Point3d Transform(const Matrix& m);
		double Dist(const Point3d& p)const;													// distance between 2 points
		double DistSq(const Point3d& p)const;													// distance squared between 2 points
		Point3d	Mid(const Point3d& p, double factor = 0.5)const;									// midpoint
		void get(double xyz[3]) {xyz[0] = x; xyz[1] = y; xyz[2] = z;}
		double* getBuffer(){return &this->x;};																		// returns ptr to data
		const double* getBuffer()const{return &this->x;};																		// returns ptr to data

	};

	// 2d vector class
	class Vector2d{
		friend wostream& operator <<(wostream& op, Vector2d& v);
	private:
		double dx, dy;		
	public:

		// constructors
		inline	Vector2d() {dx = 0; dy = 0;}
		inline	Vector2d(const Vector2d &v) { dx = v.dx; dy = v.dy;}
				Vector2d(const Vector3d &v);		// careful
		inline	Vector2d(double x, double y) {dx = x, dy = y;}
		inline	Vector2d(const Point& p0, const Point& p1) {dx = p1.x - p0.x; dy = p1.y - p0.y;}
		inline	Vector2d(const Point *p0, const Point *p1) {dx = p1->x - p0->x; dy = p1->y - p0->y;}
		inline	Vector2d(const Point& p) { dx = p.x; dy = p.y;} // from 0,0 to p
		inline	Vector2d(double angle) {dx = cos(angle *= DegreesToRadians); dy = sin(angle);}	// constructs a vector from an angle (0° - 360°)


		// operators
		inline	const	Vector2d& operator=(const Vector2d &v){dx = v.dx; dy = v.dy; return *this;}			// v1 = v2;
		inline			Vector2d operator+(const Vector2d &v)const{return Vector2d(dx + v.dx, dy + v.dy);}	// v2 = v0 + v1;
		inline			Point	operator+(const Point &p)const{return Point(this->dx + p.x, this->dy + p.y);}			// p1 = v0 + p0;
		inline			Vector2d operator+(const double d){ return Vector2d(dx + d, dy + d); };

		inline	const	Vector2d& operator+=(const Vector2d &v){dx += v.dx; dy += v.dy; return *this;}		// v1 += v0;
		inline			Vector2d operator-(const Vector2d &v)const{return Vector2d( dx - v.dx, dy - v.dy);}	// v2 = v0 - v1;
		inline	const	Vector2d& operator-=(const Vector2d &v){dx -= v.dx; dy -= v.dy; return *this;}		// v1 -= v0;
		inline			Vector2d operator-(const double d){ return Vector2d(dx - d, dy - d); };

		inline	const	Vector2d operator-(void)const{return Vector2d(-dx, -dy);}							// v1 = -v0;  (unary minus)

		inline		double operator*(const Vector2d &v)const{return (dx * v.dx + dy * v.dy);}			// dot product	m0.m1.cos a = v0 * v1
		inline			Vector2d operator*(double c)const{return Vector2d(dx*c, dy*c);}							// scalar product
		inline	const	Vector2d& operator*=(double c){dx *= c; dy *= c; return *this;}						// scalar product
		inline			Vector2d operator*(int c)const{return Vector2d(dx*(double)c, dy*(double)c);}				// scalar product

		inline		double operator^(const Vector2d &v)const{return (dx * v.dy - dy * v.dx);}			// cross product m0.m1.sin a = v0 ^ v1
		inline			Vector2d operator~(void)const{return Vector2d(-dy, dx);}							// perp to left

						bool operator==(const Vector2d &v)const;													// v1 == v2
		inline			bool operator!=(const Vector2d &v)const { return !(*this == v);}							// v1 != v2



		// methods
		void get(double xyz[2]) {xyz[0] = dx; xyz[1] = dy;}													// return to array
		inline	double	getx()const{return dx;}
		inline	double	gety()const{return dy;}
		inline	void	putx(double x){dx = x;}
		inline	void	puty(double y){dy = y;}
		double	normalise()
		{double m = magnitude(); if(m < TIGHT_TOLERANCE) {dx=dy=0; return 0;} dx/=m; dy/=m; return m;}				// normalise & returns magnitude
		inline	double	magnitudesqd(void)const{return(dx * dx + dy * dy);}									// magnitude squared
		inline	double	magnitude(void)const{return(sqrt(magnitudesqd()));}									// magnitude
		void	Rotate(double cosa, double sina){															// rotate vector by angle
			double temp = -dy * sina + dx * cosa;
			dy = dx * sina + cosa * dy;
			dx = temp;
		}	
		inline	void	Rotate(double angle) { if(FEQZ(angle) == true) return; Rotate(cos(angle), sin(angle));}
		void Transform( const Matrix& m);																			// transform vector

		// destructor
		//~Vector2d(){}

	};


	// 3d vector class
	class Vector3d{
		friend wostream& operator <<(wostream& op, Vector3d& v);
	private:
		double dx, dy, dz;
	public:

		// constructors
		Vector3d() {dx = 0; dy = 0; dz = 0;}
		Vector3d(const Vector3d &v) { dx = v.dx; dy = v.dy; dz = v.dz;}
		Vector3d(double x, double y, double z = 0) {dx = x, dy = y; dz = z;}
		Vector3d(const double* x) {dx = x[0], dy = x[1]; dz = x[2];}
		Vector3d(const double* x0, const double* x1) {dx = x1[0] - x0[0], dy = x1[1] - x0[1]; dz = x1[2] - x0[2];}
		Vector3d(const Point3d& p0, const Point3d& p1) {dx = p1.x - p0.x; dy = p1.y - p0.y; dz = p1.z - p0.z;}
		Vector3d(const Point3d& p) { dx = p.x; dy = p.y; dz = p.z;} // from 0,0,0 to p
		Vector3d(const Vector2d& v) {dx = v.getx(); dy = v.gety(); dz = 0;}

		// operators
		bool operator==(const Vector3d &v)const { return(FEQ(dx, v.dx, UNIT_VECTOR_TOLERANCE) && FEQ(dy, v.dy, UNIT_VECTOR_TOLERANCE) && FEQ(dz, v.dz, UNIT_VECTOR_TOLERANCE)); }		// v1 == v2 (unit only!)
		bool operator!=(const Vector3d &v)const { return (!(*this == v)); }											// v1 != v2
		const	Vector3d& operator=(const Vector3d &v){dx = v.dx; dy = v.dy; dz = v.dz;return *this;}				// v1 = v2;
		//	const	Vector3d& operator=(const Vector2d &v){dx = v.getx(); dy = v.gety(); dz = 0.0;return *this;}	// v1 = v2;
		inline		Point3d	operator+(const Point3d &p)const{return Point3d(dx + p.x, dy + p.y, dz + p.z);}			// p1 = v0 + p0;
		Vector3d operator+(const Vector3d &v)const{return Vector3d(dx + v.dx, dy + v.dy, dz + v.dz);}				// v2 = v0 + v1;
		const	Vector3d& operator+=(const Vector3d &v){dx += v.dx; dy += v.dy; dz += v.dz; return *this;}			// v1 += v0;
		Vector3d operator-(const Vector3d &v)const{return Vector3d( dx - v.dx, dy - v.dy, dz - v.dz);}				// v2 = v0 - v1;
		const	Vector3d& operator-=(const Vector3d &v){
			dx -= v.dx; dy -= v.dy; dz -= v.dz; return *this;}			// v1 -= v0;

		const	Vector3d operator-(void)const{return Vector3d(-dx, -dy, -dz);}										// v1 = -v0;  (unary minus)

			double operator*(const Vector3d &v)const{return (dx * v.dx + dy * v.dy + dz * v.dz);}				// dot product	m0 m1 cos a = v0 * v1

		const Vector3d& operator*=(double c){dx *= c; dy *= c; dz *= c; return *this;}								// scalar products
		friend const Vector3d operator*(const Vector3d &v, double c){return Vector3d(v.dx*c, v.dy*c, v.dz*c);}
		friend const Vector3d operator*(double c, const Vector3d &v){return Vector3d(v.dx*c, v.dy*c, v.dz*c);}
		friend const Vector3d operator/(const Vector3d &v, double c){return Vector3d(v.dx/c, v.dy/c, v.dz/c);}

		const	Vector3d operator^(const Vector3d &v)const{
			return Vector3d(dy * v.dz - dz * v.dy, dz * v.dx - dx * v.dz, dx * v.dy - dy * v.dx);}					// cross product vector

		// = the vector perp to the plane of the 2 vectors
		// the z component magnitude is m0.m1.sin a 	
		// methods
		inline	void get(double xyz[3])const {xyz[0] = dx; xyz[1] = dy; xyz[2] = dz;}									// return to array
		inline	double getx()const{return dx;}
		inline	double gety()const{return dy;}
		inline	double getz()const{return dz;}
		inline	void putx(double x){dx = x;}
		inline	void puty(double y){dy = y;}
		inline	void putz(double z){dz = z;}
		double normalise(){double m = magnitude(); if(m < 1.0e-09) {dx=dy=dz=0; return 0;} dx/=m; dy/=m; dz/=m;		// normalise & returns magnitude
		return m;}
		inline	double magnitude(void)const{return(sqrt(dx * dx + dy * dy + dz * dz));}								// magnitude
		inline	double magnitudeSq(void)const{return(dx * dx + dy * dy + dz * dz);}								    // magnitude squared
		void Transform( const Matrix& m);																					// transform vector
		void arbitrary_axes(Vector3d& x, Vector3d& y);
		int setCartesianAxes(Vector3d& b, Vector3d& c);
		double* getBuffer(){return &this->dx;};																		// returns ptr to data
		const double* getBuffer()const{return &this->dx;};																		// returns ptr to data

		// destructor
		//~Vector3d(){}

	};

#define ORIGIN Point3d(0,0,0)
#define NULL_VECTOR Vector3d(0,0,0)
#define Z_VECTOR Vector3d(0,0,1)
#define Y_VECTOR Vector3d(0,1,0)
#define X_VECTOR Vector3d(1,0,0)

	// 2D cline x = x0 + t * dx;    y = y0 + t * dy
	class CLine{
		friend wostream& operator <<(wostream& op, CLine& cl);
	public:
		bool ok;
		Point p;
		Vector2d v;

		// constructors
		inline	CLine()	{ok = false;};
		inline	CLine(const Point& p0, double dx, double dy, bool normalise = true){ p = p0; v = Vector2d(dx, dy); if(normalise) Normalise();};
		inline	CLine(const Point& p0, const Vector2d& v0, bool normalise = true) {p = p0; v = v0; if(normalise) Normalise();};
		inline	CLine(const CLine& s) {p = s.p; v = s.v; ok = s.ok;}				// copy constructor  CLine s1(s2);
		inline	CLine(const Point& p0, const Point& p1) {p = p0; v = Vector2d(p0, p1); Normalise();};
		CLine(const Span& sp);	

		// operators
		const	CLine operator~(void);// perp to left
		const	CLine operator=(const Point& p0){p.x=p0.x; p.y=p0.y; return *this;};				// s = p;

		// methods
		double c();																// returns c
		void Normalise();														// normalise dx,dy
#ifdef PEPSDLL
		void ToPeps(int id, bool draw = true);									// to Peps
		void DelPeps(int id);													// delete Peps CLine
#endif
		CLine Transform(Matrix& m);												// transform a CLine
		Point Intof(const CLine& s);													// intersection of 2 clines
		Point Intof(int NF, const Circle& c);											// intersection of cline & circle 
		Point Intof(int NF, const Circle& c, Point& otherInters);	double Dist(const Point& p1)const;	//  ditto & other intersection												
		CLine Bisector(const CLine& s);												// Bisector of 2 Clines

		// destructor
//		~CLine();
	};

#define HORIZ_CLINE CLine(geoff_geometry::Point(0,0), 1.0, 0.0, true)


	// 2D circle 
	class Circle{
		friend wostream& operator <<(wostream& op, Circle& c);
	public:
		bool ok;
		Point pc;
		double	radius;

		// constructors etc...
		inline	Circle() {ok = false; radius = 0;}
		Circle( const Point& p, double r);										// Circle  c1(Point(10,30), 20);
		Circle( const Point& p, const Point& pc);								// Circle  c1(p[222], p[223]);
		Circle( const Circle& c ){*this = c;}									// copy constructor  Circle c1(c2);
		Circle( const Span& sp);														// constructor

		// methods
#ifdef PEPSDLL
		void ToPeps(int id, bool draw = true);									// to Peps
		void DelPeps(int id);													// delete Peps Circle
#endif
		bool operator==(const Circle &c)const;									// c == cc
		bool operator!=(const Circle &c)const { return !(*this == c);}
		Circle Transform(Matrix& m);											// transform a Circle
		Point	Intof(int LR, const Circle& c1);										// intof 2 circles
		Point	Intof(int LR, const Circle& c1, Point& otherInters);					// intof 2 circles, (returns the other intersection)
		int		Intof(const Circle& c1, Point& leftInters, Point& rightInters);		// intof 2 circles (returns number of intersections & left/right inters)
		CLine	Tanto(int AT,  double angle, const CLine& s0)const;			// a cline tanto this circle at angle
	//	~Circle();																// destructor
	};

	// 2d box class
	class Box{
	public:
		Point min;
		Point max;
		bool ok;

		Box() { min.x = min.y = 1.0e61; max.x = max.y = -1.0e61; ok = false;};
		Box(Point& pmin, Point& pmax) { min = pmin; max = pmax; ok = true;};

		bool outside(const Box& b)const;		// returns true if box is outside box
		void combine(const Box& b);		// combines this with b
	};

	// 3d box class
	class Box3d{
	public:
		Point3d min;
		Point3d max;
		bool ok;

		Box3d() { min.x = min.y = min.z = 1.0e61; max.x = max.y = max.z = -1.0e61; ok = false;};
		Box3d(const Point3d& pmin, const Point3d& pmax) { min = pmin; max = pmax; ok = true;};

		bool outside(const Box3d& b)const;		// returns true if box is outside box
		void combine(const Box3d& b);		// combines this with b
	};

	inline void MinMax(const Point& p, Point& pmin, Point& pmax) {
		if(p.x > pmax.x) pmax.x = p.x;
		if(p.y > pmax.y) pmax.y = p.y;
		if(p.x < pmin.x) pmin.x = p.x;
		if(p.y < pmin.y) pmin.y = p.y;
	};

	inline void MinMax(const Point3d& p, Point3d& pmin, Point3d& pmax) {
		if(p.x > pmax.x) pmax.x = p.x;
		if(p.y > pmax.y) pmax.y = p.y;
		if(p.z > pmax.z) pmax.z = p.z;
		if(p.x < pmin.x) pmin.x = p.x;
		if(p.y < pmin.y) pmin.y = p.y;
		if(p.z < pmin.z) pmin.z = p.z;
	};




	// 2D line arc span
	class Span{
		friend wostream& operator <<(wostream& op, Span& span);
	public:
		Point p0;			// start
		Point p1;			// end
		Point pc;			// centre
		int	  dir;			// arc direction (CW or ACW or 0 for straight)
		int	ID;				// ID (for offset in wire - stores spanID etc. from original kurve)
		bool ok;

		bool	returnSpanProperties;	// set if properties below are set
		Vector2d vs;			// direction at start or for straight
		Vector2d ve;			// direction at span end

		double length;		// span length
		double radius;		// arc radius
		double angle;		// included arc angle  ( now arc is parameterised start -> start + angle

		Box box;			// span box

		bool	NullSpan;	// true if small span

		// methods
		void SetProperties(bool returnProperties);									// set span properties
		Span Offset(double offset);													// offset span method
		int Split(double tolerance);												// returns number of splits
		void SplitMatrix(int num_vectors, Matrix* matrix);							// returns incremental matrix from split
		void minmax(Box& box, bool start = true);									// minmax of span
		void minmax(Point& pmin, Point& pmax, bool start = true);					// minmax of span
		int Intof(const Span& sp, Point& pInt1, Point& pInt2, double t[4])const;
		void Transform(const Matrix& m, bool setprops = true);
		Point Near(const Point& p)const;														// returns the near point to span from p (on or off)
		Point NearOn(const Point& p)const;														// returns the near point to span from p (on span)
		Point	Mid()const;																// midpoint of a span
		Point MidPerim(double d)const;													// interior point of Span (param 0 - d)
		Point MidParam(double param)const;												// interior point of Span (param 0 - 1)
		bool OnSpan(const Point& p)const;														//  tests if p is on sp *** FAST TEST p MUST LIE on unbounded span
		bool OnSpan(const Point& p, double* t)const;											//  tests if p is on sp *** FAST TEST p MUST LIE on unbounded span
		bool	JoinSeparateSpans(Span& sp);
		Span BlendTwoSpans(Span& sp2, double radius, double maxt);					// Blends 2 Spans
		bool isJoinable(const Span& sp)const;													// is this & sp joinable to 1 span?
		Vector2d GetVector(double fraction)const; // the direction along the span, 0.0 for start, 1.0 for end

		// constructor
		Span() {dir = 0; ID = 0; ok = false; returnSpanProperties = false; length = 0; radius = 0; angle = 0; NullSpan = false;}
		Span(int spandir, const Point& pn, const Point& pf, const Point& c) { dir = spandir; p0 = pn, p1 = pf, pc = c; ID = 0; SetProperties(true); ok = p0.ok;};

		// operators
		//	bool operator==(const Span &sp)const;
		//	bool operator!=(const Span &sp)const { return !(*this == sp);}	
	};

	// general
	double	atn360(double dx, double dy);									// angle 0 to 2pi

	// distance functions
	//double Dist(double px, double py, double p1x, double p1y);				// diatance between 2 points (2d)
	//double Dist(Point& p0, Point& p1);										// distance between 2 points (3d)
	//double Dist(CLine& s, Point& p1);											// distance between cline & point

	double Dist(const Point3d *p, const Vector3d *vl, const Point3d *pf);							// distance from line (p, vl) and pf
	double DistSq(const Point3d *p, const Vector3d *vl, const Point3d *pf);						// distance squared from line (p, vl) and pf
	double Dist(const Circle& c, const Point& p);											// distance between c & p
	double Dist(const Point& p0, const Circle& c, const Point& p1);								// clockwise distance around c from p0 to p1
	double Dist(const CLine& s, const Circle& c);											// distance between line and circle
	double Dist(const Circle& c0, const Circle& c1);										// distance between 2 circles
	double IncludedAngle(const Vector2d& v0, const Vector2d& v1, int dir = 1);				// angle between 2 vectors
	double IncludedAngle(const Vector3d& v0, const Vector3d& v1, const Vector3d& normal, int dir = 1);
	inline	double IncludedAngle(const CLine& s0, const CLine& s1, int dir = 1) {			// angle between 2 Clines
		return IncludedAngle(s0.v, s1.v, dir);
	}


	// point definitions
	Point	Mid(const Point& p0, const Point& p1, double factor = 0.5);					//// midpoint
	Point	Mid(const Span& sp);													//// midpoint of a span
	Point	Rel(const Point& p, double x, double y);								// relative point
	Point	Polar(const Point& p, double angle, double r);						// polar from this point
	Point	AtAngle(const Circle& c, double angle);								// Point at angle on a circle
	Point	XonCLine(const CLine& s, double xval);								// returns point that has X on this line
	Point	YonCLine(const CLine& s, double yval);								// returns point that has Y on this line
	Point	Intof(const CLine& s0, const CLine& s1);									//// intof 2 clines
	Point	Intof(int NF, const CLine& s, const Circle& c);								//// intof of circle & a cline
	Point	Intof(int NF, const CLine& s, const Circle& c, Point& otherInters);			//// intof of circle & a cline (returns the other intersection)
	Point	Intof(int LR, const Circle& c0, const Circle& c1);							//// intof 2 circles
	Point	Intof(int LR, const Circle& c0, const Circle& c1, Point& otherInters);		//// intof 2 circles, (returns the other intersection)
	int		Intof(const Circle& c0, const Circle& c1, Point& pLeft, Point& pRight);		////    ditto
	Point	Along(const CLine& s, double d);										// distance along Cline
	Point	Along(const CLine& s, double d, const Point& p);								// distance along Cline from point
	Point	Around(const Circle& c, double d, const Point& p);								// distance around a circle from point
	Point	On(const CLine& s,  const Point& p);											// returns a point on s nearest to p
	Point	On(const Circle& c, const Point& p);											// returns a point on c nearest to p

	// cline definitions

	CLine	AtAngle(double angle, const Point& p, const CLine& s = HORIZ_CLINE);		// cline at angle to line thro' point
	CLine	Tanto(int AT, const Circle& c,  double angle, const CLine& s0 = HORIZ_CLINE);//// cline tanto circle at angle to optional cline
	CLine	Tanto(int AT, const Circle& c, const Point& p);								// cline tanto circle thro' a point
	CLine	Tanto(int AT0, const Circle& c0, int AT1, const Circle& c1);					// cline tanto 2 circles
	CLine	Normal(const CLine& s);													// noirmal to cline
	CLine	Normal(const CLine& s, const Point& p);										// normal to cline thro' p
	CLine	Parallel(int LR, const CLine& s, double distance);						// parallel to cline by distance
	CLine	Parallel(const CLine& cl, const Point& p);										// parallel to cline thro' a point


	// circle definitions
	Circle	Thro(const Point& p0, const Point& p1);										// circle thro 2 points (diametric)
	Circle	Thro(const Point& p0, const Point& p1, const Point& p2);							// circle thro 3 points
	Circle	Tanto(int NF, const CLine& s0, const Point& p, double rad);					// circle tanto a CLine thro' a point with radius
	Circle	Thro(int LR, const Point& p0, const Point& p1, double rad);					// circle thro' 2 points with radius
	Circle	Tanto(int AT1, const CLine& s1, int AT2, const CLine& s2, double rad);		// circle tanto 2 clines with radius
	Circle	Tanto(int AT1, const CLine& s1, int AT2, const CLine& s2, int AT3, const CLine& s3);	// circle tanto 3 clines
	Circle	Tanto(int LR, int AT, const Circle& c, const Point& p, double rad);			// circle tanto circle & thro' a point
	Circle	Tanto(int NF, int AT0, const CLine& s0, int AT1, const Circle& c1, double rad);// circle tanto cline & circle with radius
	Circle	Tanto(int LR, int AT0, const Circle& c0, int AT1, const Circle& c1, double rad);// circle tanto 2 circles with radius
	Circle	Tanto(int LR, int AT1 , const Circle& c1 , int AT2 , const Circle& c2, int AT3 , const Circle c3); // tanto 3 circles
	int		apolloniusProblem(int AT1 , const Circle& c1 , int AT2 , const Circle& c2, int AT3 , const Circle& c3, Circle& Solution1, Circle& Solution2);
	int		apolloniusProblem(int AT1 , const Circle& c1 , int AT2 , const Circle& c2, int AT3 , const CLine& cl3, Circle& Solution1, Circle& Solution2);
	int		apolloniusProblem(int AT1 , const Circle& c1 , int AT2 , const CLine& cl2, int AT3 , const CLine& cl3, Circle& Solution1, Circle& Solution2);

	//		Circle	Tanto(int AT0, int NF, int AT1, CLine s1, int AT2, CLine s2);	// circle tanto circle, and 2 clines
	Circle	Parallel(int LR, const Circle& c, double distance);					// parallel to circle by a distance


	// misc
	inline double Radians(double degrees) {return degrees * PI / 180;}
	inline double Degrees(double radians) { return radians * 180 / PI;}
	int quadratic(double a, double b, double c, double& x0, double& x1);	// solve quadratic

	int corner(const Vector2d& v0, const Vector2d& v1, double cpTol = CPTANGENTTOL);	// corner (TANGENT, LEFT, RIGHT)
	inline	int corner(const Span& span, const Span& next, double cpTol = CPTANGENTTOL) {
		return corner((Vector2d)span.ve, (Vector2d)next.vs, cpTol);}

	Line IsPtsLine(const double* a, int n, double tolerance, double* deviation);
//	Span3d IsPtsSpan3d(const double* a, int n, double tolerance, double* deviation);

	class Plane {
		friend wostream& operator <<(wostream& op, Plane& pl);

	public:						// ax + by + cz + d = 0
		bool ok;
		double d;				// distance of plane to origin
		Vector3d normal;				// normal to plane a = n.dx, b = n.dy, c = n.dz
		// constructors
		Plane(){ok = false; d = 0;}
		Plane(double dist, const Vector3d& n);
		Plane(const Point3d& p0, const Point3d& p1, const Point3d& p2);
		Plane(const Point3d& p0, const Vector3d& n, bool normalise = true);

		// methods
		double Dist(const Point3d& p)const;							// signed distance of point to plane
		bool Intof(const Line& l, Point3d& intof, double& t)const;		// intersection of plane & line (0 >= t <= 1 if intersect within line) 
		bool Intof(const Plane& pl, Line& intof)const;					// intersection of 2 planes
		bool Intof(const Plane& pl0, const Plane& pl1, Point3d& intof)const;	// intersection of 3 planes
		Point3d Near(const Point3d& p)const;							// returns near point to p on the plane
		void Mirrored(Matrix* m);										// returns a matrix for a mirror about this	
	};




#define SPANSTORAGE 32			// lessens number of object pointers

	class spVertex {
		friend wostream& operator <<(wostream& op, spVertex& sp);

	public:
		int type;
		int spanid;
		Point p;
		Point pc;
		spVertex(){type = 0; spanid = 0;}
		spVertex(int t, const Point& point, const Point& centre): type(t), spanid(0), p(point), pc(centre){};

		bool operator==(spVertex &spv){
			// vertex == spvertex (vertex check - doesn't check spannid!)
			if(this->type != spv.type) return false;
			if(this->p != spv.p) return false;
			if(this->type != LINEAR) {
				if(this->pc != spv.pc) return false;
			}
			return true;
		}

		bool operator!=(spVertex &spv){ return !(*this == spv);}

	};


	class SpanDataObject {
		// holds everything needed for Post-Processing/Simulation
	public:
		int method;	// holds method type

		SpanDataObject(int meth){method = meth;};
		SpanDataObject(const SpanDataObject* obj){method = obj->method;};
	};

	class SpanVertex{
	public:
		int type[SPANSTORAGE];							// LINEAR CW or ACW																// 0 straight (cw = -1 (T)   acw = 1 (A) )
		int spanid[SPANSTORAGE];						// identification (eg wire offset span info)
		const SpanDataObject* index[SPANSTORAGE];					// other - pointer to 
		double x[SPANSTORAGE], y[SPANSTORAGE];			// vertex
		double xc[SPANSTORAGE], yc[SPANSTORAGE];		// centre of arc
	public:
		// methods
		void	Add(int offset, int type, const Point& p0, const Point& pc, int ID = UNMARKED);
		const SpanDataObject* GetIndex(int offset)const;
		void	AddSpanID(int offset, int ID);
		SpanVertex();
		~SpanVertex();
		const SpanVertex& operator= (const SpanVertex& spv );

		void	Add(int offset, const SpanDataObject* Index );
		const SpanDataObject*	Get(int offset);
		int		Get(int offset, Point& pe, Point& pc);
		int GetSpanID(int offset);
	};





#ifdef WIN32
#pragma warning(disable:4522)
#endif

	class Kurve : public Matrix{
	friend wofstream& operator << (wofstream& op, Kurve& k);
	friend wifstream& operator >> (wifstream& op, Kurve& k);
		
	protected:
		vector<SpanVertex*> m_spans;
		bool		m_started;
		int			m_nVertices;					// number of vertices in Kurve
		bool		m_isReversed;					// true if get spans reversed

	public:	
		// for comparing kurves
		struct spanCompare {
			int dir;			// LINEAR, CW or ACW
			double length;		// length of the span
			double cp;			// cross-product to next span (sina)
			double dp;
		};
		// constructors etc...
		Kurve()	{
			m_started = false;
			m_nVertices = 0;
			m_isReversed = false;
		};
		Kurve(const Kurve& k0);
		const Kurve& operator= (const Kurve& k );
		const Kurve& operator=(const Matrix &m);

		bool operator==(const Kurve &k)const;									// k == kk (vertex check)
		bool operator!=(const Kurve &k)const { return !(*this == k);}


		// destructor
		~Kurve();

		// methods
		inline int		nSpans(	)const {return (m_nVertices)? m_nVertices - 1 : 0;}				// returns the number of spans
		bool	Closed()const;									// returns true if kurve is closed
		inline bool	Started()const {return m_started;};
		void	FullCircle(int dir, const Point& c, double radius);								// make a full circle
		void	Start();												// start a new kurve
		void	Start(const Point& p);											// start a new kurve with start point
		bool	Add(const spVertex& spv, bool AddNullSpans = true);								// add a vertex
		void	Get(int vertex, spVertex& spv) const;												// get a vertex
		bool	Add(const Span& sp, bool AddNullSpans = true);									// add a span
		bool	Add(int type, const Point& p0, const Point& pc, bool AddNullSpans = true);				// a span
		void	AddSpanID(int ID);
		bool	Add(const Point& p0, bool AddNullSpans = true);									// linear 
		void	Add();					// add a null span
		void	Add(const Kurve* k, bool AddNullSpans = true);									// a kurve
		void	StoreAllSpans(std::vector<Span>& kSpans)const;			// store all kurve spans in array, normally when fast access is reqd
		void	Clear(); // remove all the spans

		void	Replace(int vertexnumber, const spVertex& spv);
		void	Replace(int vertexnumber, int type, const Point& p, const Point& pc, int ID = UNMARKED);
		int		GetSpanID(int spanVertexNumber) const;								// for spanID (wire offset)
		int		Get(int spanVertexNumber, Point& p, Point& pc) const;
		void	Get(std::vector<Span> *all, bool ignoreNullSpans) const;												// get all spans to vector
		int		Get(int spanVertexNumber, Point3d& p, Point3d& pc) const 
		{ Point p2d, pc2d; int d = Get(spanVertexNumber, p2d, pc2d); p = p2d; pc = pc2d; return d;}
		int		Get(int spannumber, Span& sp, bool returnSpanProperties = false, bool transform = false) const;
//		int		Get(int spannumber, Span3d& sp, bool returnSpanProperties = false, bool transform = false) const;
		void	Get(Point &ps,Point &pe) const; // returns the start- and endpoint of the kurve
		const SpanDataObject* GetIndex(int vertexNumber)const;
		inline double GetLength()const{ return Perim();};  // returns the length of a kurve

		void	minmax(Point& pmin, Point& pmax);			// minmax of span
		void	minmax(Box& b);

		Point	NearToVertex(const Point& p, int& nearSpanNumber)const;
		Point	NearToVertex(const Point& p)const { int nearSpanNumber; return NearToVertex(p, nearSpanNumber);};
		Point	Near(const Point& p, int& nearSpanNumber)const;
		Point	Near(const Point& p) const{ int nearSpanNumber; return Near(p, nearSpanNumber);};
		double	Perim()const;									// perimeter of kurve
		double	Area()const;										// area of closed kurve
		void	Reverse();									// reverse kurve direction - obsolete
		bool	Reverse(bool isReversed) {					// reverse kurve direction - later better method
			bool tmp = m_isReversed;
			m_isReversed = isReversed;
			return tmp;
		};
		int		Reduce(double tolerance);					// reduce spans which are in tolerance

		int		Offset(vector <Kurve*> &OffsetKurves, double offset, int direction, int method, int& ret)const;	// offset methods
		int		OffsetMethod1(Kurve& kOffset, double off, int direction,  int method, int& ret)const;
		int		OffsetISOMethod(Kurve& kOffset, double off, int direction, bool BlendAll)const; // special offset (ISO radius - no span elimination)
		int		Intof(const Span& sp, vector<Point>& p)const;			// intof span
		int		Intof(const Kurve&k, vector<Point>& p)const;			// intof kurve
		bool	Compare(const Kurve* k, Matrix* m, bool bAllowMirror = true)const;				// compare 2 Kurves
		void	ChangeStart(const Point *pNewStart, int startSpanno); // change the Kurve's startpoint
		void	ChangeEnd(const Point *pNewEnd, int endSpanno); // change the Kurve's endpoint

	private:
		bool compareKurves(const std::vector<struct spanCompare> &first, const std::vector<struct spanCompare> &second, int &nOffset/*, Kurve *k, Matrix *m*/)const;
		bool calculateMatrix(const Kurve *k, Matrix *m, int nOffset, bool bMirror = false)const;
	public:


		void	AddIndex(int vertexNumber, const SpanDataObject* data);
		bool	Split(double MaximumRadius, double reslution);	// split arcs larger than MaximumRadius to resoultion
		int	IntExtWire( Kurve& kSec, double Ref, double Sec, double height, Kurve* kOut);	// interpolate / extrapolate a mid height kurve (wire)
		void	SetZ(double z) { e[11] = z; if(fabs(z) > 1.0e-6) m_unit = false;}				// assigns kurve to fixed height (wire)

		void	Part(int startVertex, int EndVertex, Kurve *part);
		Kurve	Part(int fromSpanno, const Point& fromPt, int toSpanno, const Point& toPt);					// make a Part Kurve
		int	Break(double atParam, const Kurve *secInput, Kurve *refOut, Kurve *secOut);// break kurve perimeter parameterisation with synchronised Kurve (wire)
		void	Part(double fromParam, double toParam, const Kurve *secInput, Kurve *refOut, Kurve *secOut);// part kurve perimeter parameterisation with synchronised Kurve (wire)
		Kurve	Part(double fromParam, double toParam);											// part kurve perimeter parameterisation
		void AddSections(const Kurve* k, bool endOfSection);		// special add kurves for rollingball
		void	AddEllipse(int dir, const Point& pStart, const Point& pEnd, const Point& pCentre, const Vector2d& majorAxis, double majorRadius, double minorRadius, double tolerance);
//		void Kurve::AddEllipse(int dir, Plane *plEllipse, Vector3d *cylAxis, Point3d *cylCentre, double cylradius, Point3d *pStart, Point3d *pEnd, double tolerance);		/// elliptical curve - biarc in tolerance

		void	Spiral(const Point& centre, double startAngle, double startRadius, double radiusRisePerRevolution, double endRadius);
#ifdef PARASOLID
		int ToPKcurve(PK_CURVE_t *curves, PK_INTERVAL_t *ranges, int start_spanno, int n_spans); // Convert to PK Curve

		PK_BODY_t ToPKwire();												// Convert to PK Wire Body
		PK_BODY_t ToPKwire(int start_spanno, int n_spans);

		PK_BODY_t ToPKsheet(			);									// Convert to PK Sheet Body
		PK_BODY_t ToPKextrudedBody(PK_VECTOR1_t path, bool solidbody = true);
		// Convert to PK Body (open kurve >> sheet)
		PK_BODY_t ToPKlofted_sheet_body(Kurve &sec);						// Convert 2 kurves to lofted sheet body
		PK_BODY_t ToPKlofted_thickened_body(Kurve &sec, double thickness);
#endif
	};
#ifdef WIN32
#pragma warning(default:4522)
#endif

	void tangential_arc(const Point &p0, const Point &p1, const Vector2d &v0, Point &c, int &dir);

	int		EqualiseSpanCount(Kurve& k1, Kurve& k2, Kurve& k1equal, Kurve& k2equal, bool equalise_same_span_count);		// span count equalisation
	void	EqualiseSpanCountAfterOffset(Kurve& k1, Kurve&k2, Kurve& k1Out, Kurve& k2Out);// span equalisation after offset
	void	EqualiseSpanCountAfterOffsetFromRollAround(Kurve& k1, Kurve&k2, Kurve& k1Out, Kurve& k2Out/*, double offset, int arc_direction*/);// span equalisation after offset

	Point IntofIso(Span& one, Span& two, Span& three);				// for iso blend radiuses - calc intersection

	inline double CPTOL(double offset, double maxOffset) {
		// this returns a suitable tolerance for a cross product
		// the cp for normalised vectors is the sin of the included angle between the vectors
		//
		// this function takes the machine resolution from RESOLUTION

		offset = fabs(offset);

		if(offset <= RESOLUTION) offset = maxOffset;	// no known offset so guess one from the application

		return RESOLUTION / offset;
	}



	// finite Span routines
	int Intof(const Span& sp0 , const Span& sp1, Point& p0, Point& p1, double t[4]);
	int	LineLineIntof(const Span& L0 , const Span& L1, Point& p, double t[2]);
	int LineArcIntof(const Span& line, const Span& arc, Point& p0, Point& p1, double t[4]);
	int ArcArcIntof(const Span& arc0, const Span& arc1, Point& pLeft, Point& pRight);

	bool OnSpan(const Span& sp, const Point& p);
	bool OnSpan(const Span& sp, const Point& p, bool nearPoints, Point& pNear, Point& pOnSpan);	// function returns true if pNear == pOnSpan
	//			pNear (nearest on unbound span)
	//			pOnSpan (nearest on finite span)


	int Intof(const Line& v0, const Line& v1, Point3d& intof);							// intof 2 lines
	double Dist(const Line& l, const Point3d& p, Point3d& pnear, double& t);			// distance from a point to a line
	Point3d Near(const Line& l, const Point3d& p, double& t );							// near point to a line & t in 0-length range
	double Dist(const Span& sp, const Point& p , Point& pnear );						// distance from p to sp, nearpoint returned as pnear

//	Kurve splineUsingBiarc(CLine& cl0, CLine& cl1, std::vector<pts>);

	int biarc(CLine& cl0, CLine& cl1, Span* sp0, Span* sp1 );

	// 3d line segment
	class Line{
	public:
		Point3d p0;				// start
		Vector3d v;				// vector (not normalised)
		double length;			// line length
		Box3d box;
		bool ok;

		// constructors
		Line() {ok = false; length = 0;}
		Line(const Point3d& p0, const Vector3d& v0, bool boxed = true);
		Line(const Point3d& p0, const Point3d& p1);
		Line(const Span& sp);

		// methods
		void minmax();
		Point3d Near(const Point3d& p, double& t)const;				// near point to line from point (0 >= t <= 1) in range
		int Intof(const Line& l, Point3d& intof)const {return geoff_geometry::Intof(*this, l, intof);};	// intof 2 lines
		bool atZ(double z, Point3d& p)const;						// returns p at z on line
		bool Shortest(const Line& l2, Line& lshort, double& t1, double& t2)const;	// calculate shortest line between this & l2
	};


class Triangle3d {
		Point3d vert1;    // first vertex
		Point3d vert2;    // second vertex
		Point3d vert3;    // third vertex
		Vector3d v0;      // vector from vert1 to vert2
		Vector3d v1;      // vector from vert1 to vert3
		bool ok;

		Box3d box;        // box around triangle

public:
		// constructor
		Triangle3d(){ ok = false;};
		Triangle3d(const Point3d& vert1, const Point3d& vert2, const Point3d& vert3);

		// methods
		bool Intof(const Line& l, Point3d& intof)const; // returns intersection triangle to line
};



} // End namespace geoff_geometry




