/***************************************************************************
 *   Copyright (c) 2025 Walter Steffè <walter.steffe@hierarchical-electromagnetics.com> *
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

#pragma once
#include <boost/signals2/connection.hpp>
#include <string>
#include <map>
#include <unordered_set>

namespace App { class Document; }

namespace PartDesignGui {

class UniPartCSMigrator {
public:
    static void init();
    static UniPartCSMigrator* instance();
    static void destruct();

private:
    UniPartCSMigrator();
    ~UniPartCSMigrator();

    // App::Application post-open hook (signalFinishOpenDocument is void())
    void slotFinishOpenDocument();
    void slotFinishRestoreDocument (const App::Document& doc);

    // True if any PartDesign::Body has non-identity placement
    static bool needsMigration(App::Document* doc);

    boost::signals2::connection connectFinishOpenDocument;
    boost::signals2::connection connectFinishRestoreDocument;
    std::map<std::string, boost::signals2::scoped_connection> onRecomputedConn_;

    static UniPartCSMigrator* _instance;

    // one-shot guard per process session
    std::unordered_set<std::string> migrated_;
    std::unordered_set<std::string> pending_;
};

} // namespace PartDesign

