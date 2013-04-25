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

#ifndef GCM_SHEDULER_H
#define GCM_SHEDULER_H

#include <set>
#include <algorithm>

namespace dcm {

template<typename Sys>
struct Job {
  
    virtual ~Job() {};

    virtual void execute(Sys& sys) = 0;

    bool operator<(const Job<Sys>& j) const {
        return priority < j.priority;
    };
    int priority;
};

template<typename functor, typename System, int prior>
struct FunctorJob : Job<System> {
    FunctorJob(functor f) : m_functor(f) {
        Job<System>::priority = prior;
    };
    virtual void execute(System& sys) {
        m_functor(sys);
    };

    functor m_functor;
};

template<typename Sys>
class Sheduler {

public:
    Sheduler() {};
    ~Sheduler() {
      
      std::for_each(m_preprocess.begin(), m_preprocess.end(), Deleter());
      std::for_each(m_process.begin(), m_process.end(), Deleter());
      std::for_each(m_postprocess.begin(), m_postprocess.end(), Deleter());      
    };

    void addPreprocessJob(Job<Sys>* j) {
        m_preprocess.insert(j);
    };
    void addPostprocessJob(Job<Sys>* j) {
        m_postprocess.insert(j);
    };
    void addProcessJob(Job<Sys>* j) {
        m_process.insert(j);
    };

    void execute(Sys& system) {
        std::for_each(m_preprocess.begin(), m_preprocess.end(), Executer(system));
        std::for_each(m_process.begin(), m_process.end(), Executer(system));
        std::for_each(m_postprocess.begin(), m_postprocess.end(), Executer(system));
    };

protected:
    struct Executer {
        Executer(Sys& s) : m_system(s) {};
        void operator()(Job<Sys>* j) {
            j->execute(m_system);
        };
        Sys& m_system;
    };
    struct Deleter {
        void operator()(Job<Sys>* j) {
            delete j;
        };
    };

    std::set< Job<Sys>* > m_preprocess, m_process, m_postprocess;
};

}

#endif //GCM_SHEDULER_H
