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


// src/Mod/PartDesign/App/PartDesignRestoreHook.cpp
#include <App/Application.h>
#include <App/Document.h>
#include "PartDesignMigration.h"

namespace {
struct RestoreHook {
    RestoreHook() {
        // Fire after each document finishes restore
        App::GetApplication().signalFinishRestoreDocument.connect(&onDocRestored);
    }
    static void onDocRestored(const App::Document& adoc) {
        // migrate in-place (safe here; restore is finished)
        PartDesign::migrateLegacyBodyPlacements(const_cast<App::Document*>(&adoc));
    }
};

// Static instance: connects the signal at module load
static RestoreHook _pdRestoreHook;
} // namespace

