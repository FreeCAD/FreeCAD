// Box2D.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#pragma once

#include <string.h>	// for memcpy() prototype
#include <math.h>	// for sqrt() prototype

class CBox2D{
public:
	Point m_minxy;
	Point m_maxxy;
	bool m_valid;

	CBox2D():m_valid(false){}
	CBox2D(const Point& minxy, const Point& maxxy):m_minxy(minxy), m_maxxy(maxxy), m_valid(true){}

	bool operator==( const CBox2D & rhs ) const
	{
		if(m_minxy != rhs.m_minxy)return false;
		if(m_maxxy != rhs.m_maxxy)return false;
		if (m_valid != rhs.m_valid) return(false);

		return(true);
	}

	bool operator!=( const CBox2D & rhs ) const { return(! (*this == rhs)); }


	void Insert(const Point &p){ // insert a point
		if(m_valid){
			if(p.x < m_minxy.x)m_minxy.x = p.x;
			if(p.y < m_minxy.y)m_minxy.y = p.y;
			if(p.x > m_maxxy.x)m_maxxy.x = p.x;
			if(p.y > m_maxxy.y)m_maxxy.y = p.y;
		}
		else
		{
			m_valid = true;
			m_minxy = p;
			m_maxxy = p;
		}
	}

	void Insert(const CBox2D& b){
		if(b.m_valid){
			if(m_valid){
				if(b.m_minxy.x < m_minxy.x)m_minxy.x = b.m_minxy.x;
				if(b.m_minxy.y < m_minxy.y)m_minxy.y = b.m_minxy.y;
				if(b.m_maxxy.x > m_maxxy.x)m_maxxy.x = b.m_maxxy.x;
				if(b.m_maxxy.y > m_maxxy.y)m_maxxy.y = b.m_maxxy.y;
			}
			else{
				m_valid = b.m_valid;
				m_minxy = b.m_minxy;
				m_maxxy = b.m_maxxy;
			}
		}
	}
	Point Centre() const {return (m_minxy + m_maxxy) * 0.5;}
	double Width() const {if(m_valid)return m_maxxy.x - m_minxy.x; else return 0.0;}
	double Height() const {if(m_valid)return m_maxxy.y - m_minxy.y; else return 0.0;}
	double Radius() const {return sqrt(Width() * Width() + Height() * Height()) /2;}
	double MinX() const { return(m_minxy.x); }
	double MaxX() const { return(m_maxxy.x); }
	double MinY() const { return(m_minxy.y); }
	double MaxY() const { return(m_maxxy.y); }
};

