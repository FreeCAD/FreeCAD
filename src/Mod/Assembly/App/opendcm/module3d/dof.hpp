/*
    openDCM, dimensional constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef GCM_DOF_H
#define GCM_DOF_H

#include <utility>
#include <vector>

namespace dcm {

enum remaining {
    nothing = 0,
    line,
    plane,
    volume
};

template<typename K, typename C>
class Dof {

    typedef typename K::Vector3 Vec;
    typedef std::pair<Vec, C> VecID;

public:
    typedef std::vector<C> ConstraintVector;
    typedef std::pair<bool, ConstraintVector> Result;

    Dof() : m_translation(volume), m_rotation(volume) {};

    int dofTranslational() {
        return m_translation;
    };
    int dofRotational() {
        return m_rotation;
    };
    int dof() {
        return dofTranslational() + dofRotational();
    };


    Result removeTranslationDirection(Vec& v, C constraint) {

        if(m_translation == nothing) {
            ConstraintVector cv;
            cv.push_back(tp1.second);
            cv.push_back(tp2.second);
            cv.push_back(tp3.second);
            return std::make_pair(false,cv);
        } else if(m_translation == volume) {

            m_translation = plane;
            tp1 = std::make_pair(v, constraint);
        } else if(m_translation == plane) {

            if(K::isSame(tp1.first, v, 1e-6) || K::isOpposite(tp1.first, v, 1e-6)) {
                ConstraintVector cv;
                cv.push_back(tp1.second);
                return std::make_pair(false,cv);
            }
            m_translation = line;
            tp2 = std::make_pair(v, constraint);
        } else if(m_translation == line) {

            if(tp1.first.cross(tp2.first).dot(v) < 0.001) {
                ConstraintVector cv;
                cv.push_back(tp1.second);
                cv.push_back(tp2.second);
                return std::make_pair(false,cv);
            }
            m_translation = nothing;
            tp3 = std::make_pair(v, constraint);
        }

        return std::make_pair(true, ConstraintVector());
    };

    Result allowOnlyRotationDirection(Vec& v, C constraint) {

        if(m_rotation == nothing) {
            ConstraintVector cv;
            cv.push_back(rp1.second);
            cv.push_back(rp2.second);
            return std::make_pair(false, cv);
        } else if(m_rotation == volume) {

            m_rotation = line;
            rp1 = std::make_pair(v, constraint);
        } else if(m_rotation == plane) {

            return std::make_pair(false, ConstraintVector()); //error as every function call removes 2 dof's
        } else if(m_rotation == line) {

            if(K::isSame(rp1.first, v, 1e-6) || K::isOpposite(rp1.first, v, 1e-6)) {
                ConstraintVector cv;
                cv.push_back(rp1.second);
                return std::make_pair(false, cv);
            }
            m_rotation = nothing;
            rp2 = std::make_pair(v, constraint);
        }

        return std::make_pair(true, ConstraintVector());
    };


private:
    int m_translation, m_rotation;
    VecID tp1,tp2,tp3;	//translation pairs
    VecID rp1, rp2; //rotation pairs


};

}

#endif //GCM_DOF_H
